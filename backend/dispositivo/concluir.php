<?php
/**
 * POST /dispositivo/concluir.php
 *
 * O ESP32 informa que o descarte fisico terminou. Esta e a UNICA rota que
 * registra descarte e credita pontos.
 *
 * Regras que esta rota garante (secao 8 da spec):
 *   1. dispositivo autenticado e ativo;
 *   2. a sessao pertence a este dispositivo;
 *   3. a chave_conclusao confere (hash_equals);
 *   4. a sessao esta 'autorizada' ou 'em_andamento' - nunca ja encerrada;
 *   5. a sessao nao expirou (reconferido sob lock);
 *   6. o peso esta dentro da faixa configurada;
 *   7. material nao-metalico nao gera ponto;
 *   8. os pontos sao calculados AQUI, a partir de pilhas.pontos_por_grama.
 *      O que o dispositivo diga sobre pontos e ignorado;
 *   9. descarte.fk_sessao_descarte_id e UNIQUE: reenvio nunca credita 2x.
 *
 * Autenticacao: headers X-Dispositivo e X-Chave.
 *
 * Corpo (JSON ou form):
 *   sessao           codigo da sessao (32 hex)
 *   chave_conclusao  segredo entregue em autorizacao.php (32 hex)
 *   peso_g           peso medido pela balanca, em gramas
 *   metal            bool: o sensor indutivo detectou metal
 *   resultado        opcional, texto curto para o log
 *
 * Respostas
 *   200 {"ok":true,"creditado":true,"descarte":12,"pontos":34}
 *   200 {"ok":true,"creditado":false,"motivo":"..."}   nao-metal: sem pontos
 *   400 entrada invalida      403 chave/dispositivo errado
 *   404 sessao inexistente    409 sessao ja encerrada ou expirada
 *   422 peso fora da faixa    503 banco indisponivel
 */

declare(strict_types=1);

require_once __DIR__ . '/../bootstrap.php';

/**
 * Desfaz a transacao aberta antes de responder. Usada nos caminhos em que a
 * sessao foi travada com FOR UPDATE mas nada deve ser gravado.
 */
function desfazer_e_responder(int $codigo, array $corpo): void
{
    try {
        bd()->rollback();
    } catch (Throwable $erro) {
        error_log('[IOT][WARN] rollback: ' . $erro->getMessage());
    }

    responder_json($codigo, $corpo);
}

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
    responder_erro(405, 'Use POST nesta rota.');
}

try {
    expirar_sessoes();

    $dispositivo = autenticar_dispositivo();
    $dispositivoId = (int) $dispositivo['id'];

    $dados = ler_corpo();

    $codigoSessao = (string) ($dados['sessao'] ?? '');
    $chaveEnviada = (string) ($dados['chave_conclusao'] ?? '');
    $metal = filter_var($dados['metal'] ?? false, FILTER_VALIDATE_BOOLEAN);
    $resultado = mb_substr((string) ($dados['resultado'] ?? ''), 0, 60);

    if (!preg_match('/^[0-9a-f]{32}$/', $codigoSessao)) {
        responder_erro(400, 'Codigo de sessao invalido.');
    }

    if (!preg_match('/^[0-9a-f]{32}$/', $chaveEnviada)) {
        responder_erro(400, 'Chave de conclusao invalida.');
    }

    if (!isset($dados['peso_g']) || !is_numeric($dados['peso_g'])) {
        responder_erro(400, 'Peso ausente ou nao numerico.');
    }

    $peso = round((float) $dados['peso_g'], 3);

    // -------------------------------------------------------------------------
    // Transacao: tudo ou nada.
    // -------------------------------------------------------------------------

    bd()->begin_transaction();

    // FOR UPDATE serializa duas conclusoes simultaneas da mesma sessao: a
    // segunda so entra depois do COMMIT da primeira e ja encontra 'concluida'.
    $sessao = consultar_linha(
        "SELECT s.id, s.status, s.chave_conclusao, s.fk_dispositivo_id,
                s.fk_usuario_id, s.fk_pilhas_id, s.fk_descarte_id,
                s.pontos_creditados, s.expira_em,
                (s.expira_em <= NOW()) AS vencida,
                p.nome AS pilha_nome, p.pontos_por_grama
           FROM sessao_descarte s
           JOIN pilhas p ON p.id = s.fk_pilhas_id
          WHERE s.codigo = ?
          LIMIT 1
          FOR UPDATE",
        's',
        [$codigoSessao]
    );

    if ($sessao === null) {
        desfazer_e_responder(404, ['ok' => false, 'erro' => 'Sessao nao encontrada.']);
    }

    $sessaoId = (int) $sessao['id'];

    if ((int) $sessao['fk_dispositivo_id'] !== $dispositivoId) {
        registrar_evento(
            'WARN',
            'conclusao_sessao_alheia',
            'Dispositivo tentou concluir sessao de outro dispositivo.',
            $dispositivoId,
            $sessaoId
        );
        desfazer_e_responder(403, ['ok' => false, 'erro' => 'Sessao nao pertence a este dispositivo.']);
    }

    // Idempotencia: reenvio depois de um timeout de rede nao repete nada,
    // devolve o resultado original.
    if ($sessao['status'] === 'concluida') {
        desfazer_e_responder(200, [
            'ok'        => true,
            'creditado' => true,
            'repetido'  => true,
            'descarte'  => (int) $sessao['fk_descarte_id'],
            'pontos'    => (int) $sessao['pontos_creditados'],
        ]);
    }

    if (!hash_equals((string) $sessao['chave_conclusao'], $chaveEnviada)) {
        registrar_evento('WARN', 'conclusao_chave_invalida', 'Chave de conclusao recusada.', $dispositivoId, $sessaoId);
        desfazer_e_responder(403, ['ok' => false, 'erro' => 'Chave de conclusao invalida.']);
    }

    if (!in_array($sessao['status'], ['autorizada', 'em_andamento'], true)) {
        desfazer_e_responder(409, [
            'ok'     => false,
            'erro'   => 'Sessao nao esta ativa.',
            'status' => $sessao['status'],
        ]);
    }

    if ((int) $sessao['vencida'] === 1) {
        consultar(
            "UPDATE sessao_descarte
                SET status = 'expirada', motivo = 'Conclusao chegou depois do prazo.'
              WHERE id = ?",
            'i',
            [$sessaoId]
        );
        bd()->commit();

        registrar_evento('WARN', 'conclusao_expirada', 'Conclusao recebida apos o prazo.', $dispositivoId, $sessaoId);
        responder_json(409, ['ok' => false, 'erro' => 'A autorizacao expirou.', 'status' => 'expirada']);
    }

    // ---- Material: nao-metal nao gera ponto --------------------------------

    if (!$metal) {
        consultar(
            "UPDATE sessao_descarte
                SET status = 'recusada', peso_gramas = ?, pontos_creditados = 0,
                    motivo = 'O material descartado nao e pilha.', concluida_em = NOW()
              WHERE id = ?",
            'di',
            [$peso, $sessaoId]
        );
        bd()->commit();

        registrar_evento('WARN', 'descarte_recusado', 'Material nao-metalico: sem pontos.', $dispositivoId, $sessaoId);
        responder_json(200, [
            'ok'        => true,
            'creditado' => false,
            'motivo'    => 'O material descartado nao e pilha.',
        ]);
    }

    // ---- Peso dentro da faixa ----------------------------------------------

    $pesoMinimo = (float) cfg('peso_minimo_g', 0.5);
    $pesoMaximo = (float) cfg('peso_maximo_g', 2000.0);

    if ($peso < $pesoMinimo || $peso > $pesoMaximo) {
        consultar(
            "UPDATE sessao_descarte
                SET status = 'recusada', peso_gramas = ?, pontos_creditados = 0,
                    motivo = 'Peso fora da faixa aceita.', concluida_em = NOW()
              WHERE id = ?",
            'di',
            [$peso, $sessaoId]
        );
        bd()->commit();

        registrar_evento(
            'WARN',
            'peso_invalido',
            sprintf('Peso %.3f g fora da faixa [%.1f, %.1f].', $peso, $pesoMinimo, $pesoMaximo),
            $dispositivoId,
            $sessaoId
        );
        responder_json(422, [
            'ok'   => false,
            'erro' => 'Peso fora da faixa aceita. Confira a calibracao da balanca.',
        ]);
    }

    // ---- Pontuacao: calculada AQUI, nunca enviada pelo dispositivo ---------

    $pontosPorGrama = (float) $sessao['pontos_por_grama'];
    $pontos = (int) round($peso * $pontosPorGrama);

    if ($pontos < 0) {
        $pontos = 0;
    }

    // ---- Registro do descarte ----------------------------------------------

    consultar(
        "INSERT INTO descarte (data, status, peso_total_gramas, pontos_gerados,
                               fk_usuario_id, fk_ponto_coleta_id,
                               fk_dispositivo_id, fk_sessao_descarte_id)
         VALUES (CURDATE(), 'aprovado', ?, ?, ?, ?, ?, ?)",
        'ddiiii',
        [
            $peso,
            (float) $pontos,
            (int) $sessao['fk_usuario_id'],
            $dispositivo['fk_ponto_coleta_id'] !== null ? (int) $dispositivo['fk_ponto_coleta_id'] : null,
            $dispositivoId,
            $sessaoId,
        ]
    );

    $descarteId = (int) bd()->insert_id;

    consultar(
        'INSERT INTO descarte_pilha (fk_descarte_id, fk_pilhas_id, peso_gramas) VALUES (?, ?, ?)',
        'iid',
        [$descarteId, (int) $sessao['fk_pilhas_id'], $peso]
    );

    // usuario.pontos e a coluna de saldo que php/minha-area.php ja exibe.
    consultar(
        'UPDATE usuario SET pontos = pontos + ?, updated_at = NOW() WHERE id = ?',
        'ii',
        [$pontos, (int) $sessao['fk_usuario_id']]
    );

    consultar(
        "UPDATE sessao_descarte
            SET status = 'concluida', peso_gramas = ?, pontos_creditados = ?,
                fk_descarte_id = ?, concluida_em = NOW(), motivo = NULL
          WHERE id = ?",
        'ddii',
        [$peso, (float) $pontos, $descarteId, $sessaoId]
    );

    bd()->commit();

    registrar_evento(
        'INFO',
        'descarte_concluido',
        sprintf(
            'Descarte %d: %.3f g de %s, %d pontos. %s',
            $descarteId,
            $peso,
            (string) $sessao['pilha_nome'],
            $pontos,
            $resultado
        ),
        $dispositivoId,
        $sessaoId
    );

    responder_json(200, [
        'ok'        => true,
        'creditado' => true,
        'descarte'  => $descarteId,
        'pontos'    => $pontos,
        'peso_g'    => $peso,
    ]);
} catch (Throwable $erro) {
    try {
        bd()->rollback();
    } catch (Throwable $ignorado) {
        // Conexao ja perdida; nada a desfazer.
    }

    error_log('[IOT][ERROR] concluir.php: ' . $erro->getMessage());
    responder_erro(503, 'Nao foi possivel registrar o descarte agora.');
}

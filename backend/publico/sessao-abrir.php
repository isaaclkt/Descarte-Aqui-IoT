<?php
/**
 * POST /publico/sessao-abrir.php
 *
 * O usuario autenticado abre uma sessao temporaria de descarte num
 * dispositivo. Isto NAO gera pontos: so autoriza o dispositivo a comecar.
 *
 * Exige: cidadao logado (sessao do site) + token CSRF.
 * O usuario vem SEMPRE de $_SESSION, nunca do POST.
 *
 * Corpo: csrf, dispositivo (codigo), pilha (id)
 *
 * Respostas
 *   200 {"ok":true,"sessao":"...","expira_em":"...","segundos":180}
 *   400 entrada invalida      401 nao logado      403 CSRF
 *   404 dispositivo inexistente
 *   409 dispositivo ocupado ou usuario ja tem sessao aberta
 *   503 banco indisponivel
 */

declare(strict_types=1);

require_once __DIR__ . '/../bootstrap.php';

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
    responder_erro(405, 'Use POST nesta rota.');
}

try {
    expirar_sessoes();

    $usuarioId = exigir_cidadao_json();
    $dados = ler_corpo();

    if (!csrf_valido($dados)) {
        responder_erro(403, 'Solicitacao invalida. Recarregue a pagina e tente novamente.');
    }

    $codigoDispositivo = trim((string) ($dados['dispositivo'] ?? ''));
    $pilhaId = (int) ($dados['pilha'] ?? 0);

    if (!preg_match('/^[A-Za-z0-9_-]{4,32}$/', $codigoDispositivo)) {
        responder_erro(400, 'Dispositivo invalido.');
    }

    if ($pilhaId <= 0) {
        responder_erro(400, 'Escolha o tipo de pilha.');
    }

    $dispositivo = consultar_linha(
        'SELECT id, nome, ativo FROM dispositivo WHERE codigo = ? LIMIT 1',
        's',
        [$codigoDispositivo]
    );

    if ($dispositivo === null) {
        responder_erro(404, 'Dispositivo nao encontrado.');
    }

    if ((int) $dispositivo['ativo'] !== 1) {
        responder_erro(409, 'Este dispositivo esta fora de operacao.');
    }

    $pilha = consultar_linha('SELECT id, nome FROM pilhas WHERE id = ? LIMIT 1', 'i', [$pilhaId]);

    if ($pilha === null) {
        responder_erro(400, 'Tipo de pilha invalido.');
    }

    // Um usuario de cada vez: evita que a mesma pessoa deixe autorizacoes
    // vivas em varios dispositivos.
    $jaTem = consultar_linha(
        "SELECT id FROM sessao_descarte
          WHERE fk_usuario_id = ?
            AND status IN ('aguardando', 'autorizada', 'em_andamento')
          LIMIT 1",
        'i',
        [$usuarioId]
    );

    if ($jaTem !== null) {
        responder_erro(409, 'Voce ja tem um descarte em andamento. Conclua ou cancele antes de iniciar outro.');
    }

    $minutos = max(1, (int) cfg('sessao_minutos', 3));
    $codigo = token_aleatorio(16);
    $chaveConclusao = token_aleatorio(16);

    try {
        consultar(
            'INSERT INTO sessao_descarte
                 (codigo, chave_conclusao, fk_usuario_id, fk_dispositivo_id, fk_pilhas_id, expira_em)
             VALUES (?, ?, ?, ?, ?, DATE_ADD(NOW(), INTERVAL ? MINUTE))',
            'ssiiii',
            [$codigo, $chaveConclusao, $usuarioId, (int) $dispositivo['id'], $pilhaId, $minutos]
        );
    } catch (mysqli_sql_exception $erro) {
        // 1062 = violacao de UNIQUE. Aqui so pode ser
        // uq_sessao_dispositivo_ocupado: outra pessoa pegou o dispositivo
        // entre a checagem e o INSERT. Quem garante e o banco, nao o PHP.
        if ((int) $erro->getCode() === 1062) {
            responder_erro(409, 'Este dispositivo ja esta sendo usado por outra pessoa. Tente em instantes.');
        }
        throw $erro;
    }

    $sessaoId = (int) bd()->insert_id;

    $criada = consultar_linha(
        'SELECT expira_em, TIMESTAMPDIFF(SECOND, NOW(), expira_em) AS segundos
           FROM sessao_descarte WHERE id = ?',
        'i',
        [$sessaoId]
    );

    registrar_evento(
        'INFO',
        'sessao_iniciada',
        sprintf('Sessao aberta para o tipo "%s".', (string) $pilha['nome']),
        (int) $dispositivo['id'],
        $sessaoId
    );

    // A chave_conclusao NAO vai na resposta: ela e so do dispositivo.
    responder_json(200, [
        'ok'         => true,
        'sessao'     => $codigo,
        'dispositivo' => $dispositivo['nome'],
        'pilha'      => $pilha['nome'],
        'expira_em'  => $criada['expira_em'] ?? null,
        'segundos'   => max(0, (int) ($criada['segundos'] ?? 0)),
    ]);
} catch (Throwable $erro) {
    error_log('[IOT][ERROR] sessao-abrir.php: ' . $erro->getMessage());
    responder_erro(503, 'Nao foi possivel iniciar o descarte agora.');
}

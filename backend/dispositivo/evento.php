<?php
/**
 * POST /dispositivo/evento.php
 *
 * Telemetria do ciclo. O ESP32 avisa que o descarte comecou e manda leituras
 * intermediarias, para a pagina do usuario mostrar o andamento em tempo real.
 *
 * Esta rota NUNCA credita pontos nem registra descarte. Quem faz isso e
 * concluir.php, e so ela.
 *
 * Autenticacao: headers X-Dispositivo e X-Chave.
 *
 * Corpo (JSON ou form):
 *   sessao        codigo da sessao (32 hex)
 *   etapa         objeto_detectado | separando | pesando | porta_aberta | finalizando
 *   distancia_cm  opcional
 *   metal         opcional (bool)
 *   peso_g        opcional
 *
 * Respostas
 *   200 {"ok":true,"status":"em_andamento"}
 *   400 entrada invalida
 *   401/403 dispositivo
 *   404 sessao inexistente
 *   409 sessao ja encerrada
 *   503 banco indisponivel
 */

declare(strict_types=1);

require_once __DIR__ . '/../bootstrap.php';

const ETAPAS_VALIDAS = ['objeto_detectado', 'separando', 'pesando', 'porta_aberta', 'finalizando'];

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
    responder_erro(405, 'Use POST nesta rota.');
}

try {
    expirar_sessoes();

    $dispositivo = autenticar_dispositivo();
    $dispositivoId = (int) $dispositivo['id'];

    $dados = ler_corpo();

    $codigoSessao = (string) ($dados['sessao'] ?? '');
    $etapa = (string) ($dados['etapa'] ?? '');

    if (!preg_match('/^[0-9a-f]{32}$/', $codigoSessao)) {
        responder_erro(400, 'Codigo de sessao invalido.');
    }

    if (!in_array($etapa, ETAPAS_VALIDAS, true)) {
        responder_erro(400, 'Etapa desconhecida.');
    }

    $sessao = consultar_linha(
        'SELECT id, fk_dispositivo_id, status FROM sessao_descarte WHERE codigo = ? LIMIT 1',
        's',
        [$codigoSessao]
    );

    if ($sessao === null) {
        responder_erro(404, 'Sessao nao encontrada.');
    }

    // A sessao tem de ser deste dispositivo. Impede que um dispositivo
    // autenticado mexa na sessao de outro ponto de coleta.
    if ((int) $sessao['fk_dispositivo_id'] !== $dispositivoId) {
        registrar_evento(
            'WARN',
            'evento_sessao_alheia',
            'Dispositivo tentou reportar sessao de outro dispositivo.',
            $dispositivoId,
            (int) $sessao['id']
        );
        responder_erro(403, 'Sessao nao pertence a este dispositivo.');
    }

    if (!in_array($sessao['status'], ['autorizada', 'em_andamento'], true)) {
        responder_erro(409, 'Sessao nao esta ativa.', ['status' => $sessao['status']]);
    }

    if ($etapa === 'objeto_detectado' && $sessao['status'] === 'autorizada') {
        consultar(
            "UPDATE sessao_descarte SET status = 'em_andamento'
              WHERE id = ? AND status = 'autorizada'",
            'i',
            [(int) $sessao['id']]
        );
        $sessao['status'] = 'em_andamento';
    }

    // Peso e distancia entram so no log: nenhuma leitura intermediaria vira
    // ponto. O valor que conta e o enviado em concluir.php.
    $detalhe = sprintf(
        'etapa=%s distancia=%s metal=%s peso=%s',
        $etapa,
        isset($dados['distancia_cm']) ? (string) (float) $dados['distancia_cm'] : '-',
        isset($dados['metal']) ? (filter_var($dados['metal'], FILTER_VALIDATE_BOOLEAN) ? 'sim' : 'nao') : '-',
        isset($dados['peso_g']) ? (string) (float) $dados['peso_g'] : '-'
    );

    registrar_evento('INFO', 'ciclo_' . $etapa, $detalhe, $dispositivoId, (int) $sessao['id']);

    responder_json(200, ['ok' => true, 'status' => $sessao['status']]);
} catch (Throwable $erro) {
    error_log('[IOT][ERROR] evento.php: ' . $erro->getMessage());
    responder_erro(503, 'Servico indisponivel.');
}

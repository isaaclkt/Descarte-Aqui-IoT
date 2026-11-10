<?php
/**
 * GET /dispositivo/autorizacao.php
 *
 * Polling do ESP32: "ha alguma autorizacao de descarte esperando por mim?"
 *
 * Esta e a rota que inverte o sentido da conexao. O ESP32 fica numa rede
 * local e nunca e exposto a internet: e ele que sai perguntando ao backend,
 * de tempos em tempos. Nenhuma porta precisa ser aberta no roteador.
 *
 * Autenticacao: headers X-Dispositivo e X-Chave.
 * Opcional, por querystring: ?mac=...&firmware=...&porta=...
 * (telemetria de presenca; a porta e onde o equipamento serve a pagina de
 * monitoramento, que o site nao tem como deduzir sozinho)
 *
 * Respostas
 *   200 {"ok":true,"autorizacao":null}                  nada pendente
 *   200 {"ok":true,"autorizacao":{...}}                 sessao entregue
 *   401 dispositivo nao identificado / nao autorizado
 *   403 dispositivo inativo
 *   503 banco indisponivel
 */

declare(strict_types=1);

require_once __DIR__ . '/../bootstrap.php';

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
    responder_erro(405, 'Use GET nesta rota.');
}

try {
    expirar_sessoes();

    $dispositivo = autenticar_dispositivo();
    $dispositivoId = (int) $dispositivo['id'];

    $mac = $_GET['mac'] ?? null;
    $mac = is_string($mac) && preg_match('/^[0-9A-Fa-f:]{17}$/', $mac) ? strtoupper($mac) : null;

    $firmware = $_GET['firmware'] ?? null;
    $firmware = is_string($firmware) && $firmware !== '' ? mb_substr($firmware, 0, 20) : null;

    // Porta em que o equipamento serve a pagina de monitoramento. Nao vale
    // para nada alem de montar um link: nao abre acesso nenhum, e a propria
    // placa que informa. Fora da faixa valida, ignora e mantem o que havia.
    $porta = $_GET['porta'] ?? null;
    $porta = is_string($porta) && preg_match('/^\d{1,5}$/', $porta) ? (int) $porta : null;
    $porta = ($porta !== null && $porta >= 1 && $porta <= 65535) ? $porta : null;

    marcar_contato_dispositivo($dispositivoId, $mac, $firmware, $porta);

    // Sessao viva deste dispositivo. O UNIQUE em dispositivo_ocupado garante
    // que existe no maximo uma, entao nao ha ambiguidade de qual entregar.
    $sessao = consultar_linha(
        "SELECT s.id, s.codigo, s.chave_conclusao, s.status, s.expira_em,
                s.fk_pilhas_id, p.nome AS pilha_nome, p.tipo AS pilha_tipo,
                p.pontos_por_grama,
                TIMESTAMPDIFF(SECOND, NOW(), s.expira_em) AS segundos_restantes
           FROM sessao_descarte s
           JOIN pilhas p ON p.id = s.fk_pilhas_id
          WHERE s.fk_dispositivo_id = ?
            AND s.status IN ('aguardando', 'autorizada', 'em_andamento')
          ORDER BY s.id DESC
          LIMIT 1",
        'i',
        [$dispositivoId]
    );

    if ($sessao === null) {
        responder_json(200, ['ok' => true, 'autorizacao' => null]);
    }

    // Primeira entrega: marca que o dispositivo ja sabe da autorizacao.
    if ($sessao['status'] === 'aguardando') {
        consultar(
            "UPDATE sessao_descarte
                SET status = 'autorizada', autorizada_em = NOW()
              WHERE id = ? AND status = 'aguardando'",
            'i',
            [(int) $sessao['id']]
        );

        registrar_evento(
            'INFO',
            'autorizacao_entregue',
            'Autorizacao entregue ao dispositivo.',
            $dispositivoId,
            (int) $sessao['id']
        );

        $sessao['status'] = 'autorizada';
    }

    responder_json(200, [
        'ok' => true,
        'autorizacao' => [
            'sessao'             => $sessao['codigo'],
            'chave_conclusao'    => $sessao['chave_conclusao'],
            'status'             => $sessao['status'],
            'pilha_id'           => (int) $sessao['fk_pilhas_id'],
            'pilha_nome'         => $sessao['pilha_nome'],
            'pilha_tipo'         => $sessao['pilha_tipo'],
            'pontos_por_grama'   => (float) $sessao['pontos_por_grama'],
            'expira_em'          => $sessao['expira_em'],
            'segundos_restantes' => max(0, (int) $sessao['segundos_restantes']),
        ],
    ]);
} catch (Throwable $erro) {
    error_log('[IOT][ERROR] autorizacao.php: ' . $erro->getMessage());
    responder_erro(503, 'Servico indisponivel.');
}

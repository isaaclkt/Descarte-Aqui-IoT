<?php
/**
 * POST /publico/sessao-cancelar.php
 *
 * O usuario desiste do descarte. Libera o dispositivo na hora: como
 * dispositivo_ocupado e coluna gerada a partir do status, cancelar solta o
 * UNIQUE e outra pessoa ja pode usar o equipamento.
 *
 * So cancela sessao que ainda nao foi concluida, e so a do proprio usuario.
 *
 * Corpo: csrf, sessao
 *
 * Respostas
 *   200 {"ok":true}        400 codigo invalido   401 nao logado
 *   403 CSRF               404 nao encontrada    409 ja encerrada
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

    $codigo = (string) ($dados['sessao'] ?? '');

    if (!preg_match('/^[0-9a-f]{32}$/', $codigo)) {
        responder_erro(400, 'Codigo de sessao invalido.');
    }

    $sessao = consultar_linha(
        'SELECT id, status, fk_dispositivo_id FROM sessao_descarte
          WHERE codigo = ? AND fk_usuario_id = ? LIMIT 1',
        'si',
        [$codigo, $usuarioId]
    );

    if ($sessao === null) {
        responder_erro(404, 'Sessao nao encontrada.');
    }

    if (!in_array($sessao['status'], ['aguardando', 'autorizada', 'em_andamento'], true)) {
        responder_erro(409, 'Esta sessao ja foi encerrada.', ['status' => $sessao['status']]);
    }

    consultar(
        "UPDATE sessao_descarte
            SET status = 'cancelada', motivo = 'Cancelado pelo usuario.', concluida_em = NOW()
          WHERE id = ? AND status IN ('aguardando', 'autorizada', 'em_andamento')",
        'i',
        [(int) $sessao['id']]
    );

    registrar_evento(
        'WARN',
        'sessao_cancelada',
        'Descarte cancelado pelo usuario.',
        (int) $sessao['fk_dispositivo_id'],
        (int) $sessao['id']
    );

    responder_json(200, ['ok' => true, 'status' => 'cancelada']);
} catch (Throwable $erro) {
    error_log('[IOT][ERROR] sessao-cancelar.php: ' . $erro->getMessage());
    responder_erro(503, 'Nao foi possivel cancelar agora.');
}

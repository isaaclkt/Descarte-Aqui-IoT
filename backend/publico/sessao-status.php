<?php
/**
 * GET /publico/sessao-status.php?sessao=<codigo>
 *
 * A pagina do usuario consulta esta rota de tempos em tempos para mostrar o
 * andamento: esperando o dispositivo, descarte em andamento, concluido com
 * X pontos, recusado, cancelado ou expirado.
 *
 * A sessao tem de pertencer ao usuario logado. Pedir o codigo de outra pessoa
 * devolve 404, nao 403: nao vazamos nem a existencia da sessao alheia.
 *
 * Respostas
 *   200 {"ok":true,"status":"...","pontos":34,"peso_g":12.5,...}
 *   400 codigo invalido   401 nao logado   404 nao encontrada   503 banco
 */

declare(strict_types=1);

require_once __DIR__ . '/../bootstrap.php';

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
    responder_erro(405, 'Use GET nesta rota.');
}

try {
    expirar_sessoes();

    $usuarioId = exigir_cidadao_json();

    $codigo = (string) ($_GET['sessao'] ?? '');

    if (!preg_match('/^[0-9a-f]{32}$/', $codigo)) {
        responder_erro(400, 'Codigo de sessao invalido.');
    }

    // fk_usuario_id no WHERE: a sessao so e visivel para o dono.
    $sessao = consultar_linha(
        "SELECT s.status, s.peso_gramas, s.pontos_creditados, s.motivo,
                s.fk_descarte_id, s.expira_em,
                GREATEST(0, TIMESTAMPDIFF(SECOND, NOW(), s.expira_em)) AS segundos,
                d.nome AS dispositivo_nome, p.nome AS pilha_nome,
                u.pontos AS saldo_atual
           FROM sessao_descarte s
           JOIN dispositivo d ON d.id = s.fk_dispositivo_id
           JOIN pilhas p      ON p.id = s.fk_pilhas_id
           JOIN usuario u     ON u.id = s.fk_usuario_id
          WHERE s.codigo = ? AND s.fk_usuario_id = ?
          LIMIT 1",
        'si',
        [$codigo, $usuarioId]
    );

    if ($sessao === null) {
        responder_erro(404, 'Sessao nao encontrada.');
    }

    $mensagens = [
        'aguardando'   => 'Esperando o dispositivo receber a autorizacao.',
        'autorizada'   => 'Pode descartar. Coloque a pilha no compartimento.',
        'em_andamento' => 'Descarte em andamento.',
        'concluida'    => 'Descarte registrado e pontos creditados.',
        'recusada'     => (string) ($sessao['motivo'] ?? 'Descarte recusado.'),
        'cancelada'    => 'Descarte cancelado.',
        'expirada'     => 'A autorizacao expirou. Inicie um novo descarte.',
    ];

    $status = (string) $sessao['status'];

    responder_json(200, [
        'ok'          => true,
        'status'      => $status,
        'mensagem'    => $mensagens[$status] ?? '',
        'encerrada'   => in_array($status, ['concluida', 'recusada', 'cancelada', 'expirada'], true),
        'dispositivo' => $sessao['dispositivo_nome'],
        'pilha'       => $sessao['pilha_nome'],
        'peso_g'      => $sessao['peso_gramas'] !== null ? (float) $sessao['peso_gramas'] : null,
        'pontos'      => $sessao['pontos_creditados'] !== null ? (int) $sessao['pontos_creditados'] : null,
        'descarte'    => $sessao['fk_descarte_id'] !== null ? (int) $sessao['fk_descarte_id'] : null,
        'saldo'       => (int) $sessao['saldo_atual'],
        'segundos'    => (int) $sessao['segundos'],
    ]);
} catch (Throwable $erro) {
    error_log('[IOT][ERROR] sessao-status.php: ' . $erro->getMessage());
    responder_erro(503, 'Servico indisponivel.');
}

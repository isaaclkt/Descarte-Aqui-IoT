<?php
/**
 * Teste automatizado do lado DISPOSITIVO da integracao.
 *
 * Exercita, contra um backend de verdade e um banco de verdade, o caminho
 * que gera pontos: autorizacao -> telemetria -> conclusao -> credito, mais
 * os casos que NAO devem gerar pontos (chave errada, reenvio, sessao
 * expirada, material nao-metalico).
 *
 * Nao substitui o teste com a placa: aqui o ESP32 e simulado por requisicoes
 * HTTP identicas as que o firmware faz. O que este script prova e que o
 * backend e o banco se comportam como projetado.
 *
 * Preparacao - use uma base DESCARTAVEL, nunca a de producao. O script do
 * PjBL 3 e reexecutavel e recria os dados de teste, entao rodar de novo
 * devolve a base ao estado inicial:
 *
 *   mysql -u root < "../../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql"
 *   mysql -u root descarteaqui < sql/01_integracao_iot.sql
 *
 *   Depois suba o servidor:
 *   php -S 127.0.0.1:8099 -t backend
 *
 * Uso:
 *   php ferramentas/testar-fluxo.php http://127.0.0.1:8099
 *
 * O numero de pontos esperado NAO e calculado aqui: e perguntado ao proprio
 * banco, com fn_calcular_pontos(). O teste verifica que o backend concorda
 * com a regra da equipe, nao com uma formula redigitada neste arquivo.
 */

declare(strict_types=1);

if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    exit('Esta ferramenta so roda pela linha de comando.');
}

$base = rtrim($argv[1] ?? 'http://127.0.0.1:8099', '/');

require_once __DIR__ . '/../bootstrap.php';

$totais = ['ok' => 0, 'falhas' => 0];

function verificar(string $titulo, bool $condicao, string $detalhe = ''): void
{
    global $totais;

    if ($condicao) {
        $totais['ok']++;
        printf("  [OK]    %s\n", $titulo);
        return;
    }

    $totais['falhas']++;
    printf("  [FALHA] %s%s\n", $titulo, $detalhe !== '' ? " -- {$detalhe}" : '');
}

/**
 * Requisicao igual a do firmware: mesmos headers, mesmo corpo JSON.
 */
function chamar(string $url, string $metodo, array $cabecalhos, ?array $corpo): array
{
    $ch = curl_init($url);
    $opcoes = [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => 10,
        CURLOPT_HTTPHEADER     => $cabecalhos,
    ];

    if ($metodo === 'POST') {
        $opcoes[CURLOPT_POST] = true;
        $opcoes[CURLOPT_POSTFIELDS] = json_encode($corpo ?? []);
        $opcoes[CURLOPT_HTTPHEADER][] = 'Content-Type: application/json';
    }

    curl_setopt_array($ch, $opcoes);
    $resposta = curl_exec($ch);
    $codigo = (int) curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $erro = curl_error($ch);
    curl_close($ch);

    if ($resposta === false) {
        fwrite(STDERR, "Nao foi possivel falar com {$url}: {$erro}\n");
        fwrite(STDERR, "O servidor PHP esta no ar?\n");
        exit(2);
    }

    return [$codigo, json_decode((string) $resposta, true) ?: []];
}

// -----------------------------------------------------------------------------
// Preparacao: dispositivo de teste e atalhos
// -----------------------------------------------------------------------------

$CODIGO = 'esp32-teste';
$CHAVE = bin2hex(random_bytes(16));

try {
    bd();
} catch (Throwable $erro) {
    fwrite(STDERR, "Banco indisponivel: " . $erro->getMessage() . "\n");
    fwrite(STDERR, "Confira backend/config.php e se o MySQL esta no ar.\n");
    exit(2);
}

// Recria o dispositivo de teste com uma chave nova a cada execucao.
consultar('DELETE FROM evento_iot WHERE fk_dispositivo_id IN (SELECT id FROM dispositivo WHERE codigo = ?)', 's', [$CODIGO]);
consultar("UPDATE sessao_descarte SET status = 'cancelada'
            WHERE fk_dispositivo_id IN (SELECT id FROM dispositivo WHERE codigo = ?)
              AND status IN ('aguardando','autorizada','em_andamento')", 's', [$CODIGO]);

$existente = consultar_linha('SELECT id FROM dispositivo WHERE codigo = ? LIMIT 1', 's', [$CODIGO]);
$hash = password_hash($CHAVE, PASSWORD_DEFAULT);

if ($existente === null) {
    $ponto = consultar_linha('SELECT id FROM ponto_coleta ORDER BY id LIMIT 1');
    consultar(
        'INSERT INTO dispositivo (codigo, nome, chave_hash, fk_ponto_coleta_id) VALUES (?, ?, ?, ?)',
        'sssi',
        [$CODIGO, 'Dispositivo de teste', $hash, $ponto['id'] ?? null]
    );
    $dispositivoId = (int) bd()->insert_id;
} else {
    $dispositivoId = (int) $existente['id'];
    consultar('UPDATE dispositivo SET chave_hash = ?, ativo = 1 WHERE id = ?', 'si', [$hash, $dispositivoId]);
}

$usuario = consultar_linha('SELECT id, pontos FROM usuario ORDER BY id LIMIT 1');
$pilha = consultar_linha('SELECT id, nome, pontos_por_grama FROM pilhas ORDER BY id LIMIT 1');

if ($usuario === null || $pilha === null) {
    fwrite(STDERR, "Faltam dados base (usuario / pilhas).\n");
    fwrite(STDERR, "Rode o script do PjBL 3 nesta base:\n");
    fwrite(STDERR, "  mysql -u root < \"../../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql\"\n");
    exit(2);
}

/**
 * Quantos pontos o BANCO diz que um peso vale, com a regra da equipe
 * (fn_calcular_pontos + o FLOOR que sp_registrar_descarte usa ao creditar).
 */
function pontosEsperados(float $peso, float $pontosPorGrama): int
{
    $linha = consultar_linha(
        'SELECT FLOOR(fn_calcular_pontos(?, ?)) AS p',
        'dd',
        [$peso, $pontosPorGrama]
    );

    return (int) $linha['p'];
}

$usuarioId = (int) $usuario['id'];
$pilhaId = (int) $pilha['id'];
$pontosPorGrama = (float) $pilha['pontos_por_grama'];

$cabecalhosOk = ["X-Dispositivo: {$CODIGO}", "X-Chave: {$CHAVE}"];

/**
 * Cria a sessao direto no banco: e exatamente o que sessao-abrir.php faz
 * depois de validar login e CSRF. Aqui nao ha navegador para fazer login,
 * entao o lado do usuario e testado a mao (ver o roteiro no README).
 */
function abrirSessao(int $usuarioId, int $dispositivoId, int $pilhaId, int $minutos = 3): array
{
    $codigo = bin2hex(random_bytes(16));
    $chave = bin2hex(random_bytes(16));

    consultar(
        'INSERT INTO sessao_descarte
             (codigo, chave_conclusao, fk_usuario_id, fk_dispositivo_id, fk_pilhas_id, expira_em)
         VALUES (?, ?, ?, ?, ?, DATE_ADD(NOW(), INTERVAL ? MINUTE))',
        'ssiiii',
        [$codigo, $chave, $usuarioId, $dispositivoId, $pilhaId, $minutos]
    );

    return [$codigo, $chave];
}

function saldo(int $usuarioId): int
{
    return (int) consultar_linha('SELECT pontos FROM usuario WHERE id = ?', 'i', [$usuarioId])['pontos'];
}

function encerrar(string $codigo): void
{
    consultar("UPDATE sessao_descarte SET status = 'cancelada' WHERE codigo = ? AND status NOT IN ('concluida')", 's', [$codigo]);
}

printf("\nTestando %s\nbanco: %s | pilha: %s (%.3f pontos/g)\n\n",
    $base, (string) cfg('banco.nome'), (string) $pilha['nome'], $pontosPorGrama);

// -----------------------------------------------------------------------------
echo "1) Autenticacao do dispositivo\n";
// -----------------------------------------------------------------------------

[$c, $r] = chamar("{$base}/dispositivo/autorizacao.php", 'GET', [], null);
verificar('sem credencial -> 401', $c === 401, "veio {$c}");

[$c, $r] = chamar("{$base}/dispositivo/autorizacao.php", 'GET',
    ["X-Dispositivo: {$CODIGO}", 'X-Chave: chave-errada'], null);
verificar('chave errada -> 401', $c === 401, "veio {$c}");

[$c, $r] = chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);
verificar('credencial correta -> 200', $c === 200, "veio {$c}");
// Cuidado: ?? trata null como ausente, entao nao serve para verificar um
// campo que DEVE ser null. Aqui o que importa e a chave existir valendo null.
verificar('sem sessao aberta -> autorizacao nula',
    array_key_exists('autorizacao', $r) && $r['autorizacao'] === null);

// -----------------------------------------------------------------------------
echo "\n2) Fluxo completo com credito de pontos\n";
// -----------------------------------------------------------------------------

[$sessao, $chaveConclusao] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
$saldoAntes = saldo($usuarioId);

[$c, $r] = chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);
verificar('autorizacao entregue ao dispositivo', ($r['autorizacao']['sessao'] ?? '') === $sessao);
verificar('status virou "autorizada"', ($r['autorizacao']['status'] ?? '') === 'autorizada');
verificar('chave_conclusao entregue ao dispositivo',
    ($r['autorizacao']['chave_conclusao'] ?? '') === $chaveConclusao);
verificar('pontos_por_grama entregue', abs((float) ($r['autorizacao']['pontos_por_grama'] ?? 0) - $pontosPorGrama) < 0.001);

[$c, $r] = chamar("{$base}/dispositivo/evento.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao, 'etapa' => 'objeto_detectado', 'distancia_cm' => 3.2, 'metal' => true]);
verificar('telemetria aceita -> 200', $c === 200, "veio {$c}");
verificar('status virou "em_andamento"', ($r['status'] ?? '') === 'em_andamento');

$peso = 24.0;
$pontosEsperados = pontosEsperados($peso, $pontosPorGrama);

[$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao, 'chave_conclusao' => $chaveConclusao, 'peso_g' => $peso, 'metal' => true,
     'resultado' => 'pilha separada']);
verificar('conclusao aceita -> 200', $c === 200, "veio {$c}");
verificar('creditado = true', ($r['creditado'] ?? false) === true);
verificar(
    sprintf('pontos iguais aos de fn_calcular_pontos = %d', $pontosEsperados),
    (int) ($r['pontos'] ?? -1) === $pontosEsperados,
    'veio ' . (string) ($r['pontos'] ?? 'nada')
);

$descarteId = (int) ($r['descarte'] ?? 0);
verificar('descarte criado', $descarteId > 0);

$descarte = consultar_linha('SELECT status, peso_total_gramas, pontos_gerados, fk_sessao_descarte_id FROM descarte WHERE id = ?', 'i', [$descarteId]);
verificar('descarte gravado como "aprovado"', ($descarte['status'] ?? '') === 'aprovado');
verificar('peso gravado no descarte', abs((float) ($descarte['peso_total_gramas'] ?? 0) - $peso) < 0.001);
verificar('descarte ligado a sessao', (int) ($descarte['fk_sessao_descarte_id'] ?? 0) > 0);

$item = consultar_linha('SELECT peso_gramas FROM descarte_pilha WHERE fk_descarte_id = ?', 'i', [$descarteId]);
verificar('item de descarte_pilha gravado', $item !== null);

verificar('saldo do usuario subiu exatamente os pontos do descarte',
    saldo($usuarioId) === $saldoAntes + $pontosEsperados,
    'antes ' . $saldoAntes . ', agora ' . saldo($usuarioId));

// -----------------------------------------------------------------------------
echo "\n3) Idempotencia: reenvio nao credita de novo\n";
// -----------------------------------------------------------------------------

$saldoDepois = saldo($usuarioId);

[$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao, 'chave_conclusao' => $chaveConclusao, 'peso_g' => $peso, 'metal' => true]);
verificar('reenvio -> 200 (nao erro)', $c === 200, "veio {$c}");
verificar('marcado como repetido', ($r['repetido'] ?? false) === true);
verificar('devolve o MESMO descarte', (int) ($r['descarte'] ?? 0) === $descarteId);
verificar('saldo NAO mudou', saldo($usuarioId) === $saldoDepois,
    'antes ' . $saldoDepois . ', agora ' . saldo($usuarioId));

$quantos = (int) consultar_linha('SELECT COUNT(*) AS n FROM descarte WHERE fk_sessao_descarte_id = (SELECT id FROM sessao_descarte WHERE codigo = ?)', 's', [$sessao])['n'];
verificar('exatamente 1 descarte para a sessao', $quantos === 1, "contei {$quantos}");

// -----------------------------------------------------------------------------
echo "\n3.1) O bonus por volume da equipe e respeitado\n";
// -----------------------------------------------------------------------------

// fn_calcular_pontos aplica fator 1,10 a partir de 500 g e 1,20 a partir de
// 1 kg. Um backend que fizesse apenas peso x pontos_por_grama passaria em
// todos os testes acima e erraria aqui - foi exatamente o caso antes de a
// integracao passar a usar a function do banco.
$pesoBonus = 600.0;

if ($pesoBonus > (float) cfg('peso_maximo_g', 2000.0)) {
    echo "  (pulado: peso_maximo_g da configuracao e menor que {$pesoBonus} g)\n";
} else {
    [$sessaoB, $chaveB] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
    chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);

    $esperadoComBonus = pontosEsperados($pesoBonus, $pontosPorGrama);
    $semBonus = (int) floor($pesoBonus * $pontosPorGrama);
    $saldoAntesB = saldo($usuarioId);

    [$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
        ['sessao' => $sessaoB, 'chave_conclusao' => $chaveB, 'peso_g' => $pesoBonus, 'metal' => true]);

    verificar('conclusao de 600 g aceita', $c === 200, "veio {$c}");
    verificar(
        sprintf('pontos com bonus = %d (sem bonus seriam %d)', $esperadoComBonus, $semBonus),
        (int) ($r['pontos'] ?? -1) === $esperadoComBonus,
        'veio ' . (string) ($r['pontos'] ?? 'nada')
    );
    verificar('o bonus realmente muda o resultado (teste valido)', $esperadoComBonus > $semBonus);
    verificar('saldo subiu o valor com bonus',
        saldo($usuarioId) === $saldoAntesB + $esperadoComBonus);
}

// -----------------------------------------------------------------------------
echo "\n4) Chave de conclusao errada nao gera pontos\n";
// -----------------------------------------------------------------------------

[$sessao2, $chave2] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);
$saldoAntes2 = saldo($usuarioId);

[$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao2, 'chave_conclusao' => bin2hex(random_bytes(16)), 'peso_g' => 50.0, 'metal' => true]);
verificar('chave errada -> 403', $c === 403, "veio {$c}");
verificar('saldo intacto', saldo($usuarioId) === $saldoAntes2);
encerrar($sessao2);

// -----------------------------------------------------------------------------
echo "\n5) Material nao-metalico nao gera pontos\n";
// -----------------------------------------------------------------------------

[$sessao3, $chave3] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);
$saldoAntes3 = saldo($usuarioId);

[$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao3, 'chave_conclusao' => $chave3, 'peso_g' => 0, 'metal' => false]);
verificar('nao-metal -> 200', $c === 200, "veio {$c}");
verificar('creditado = false', ($r['creditado'] ?? true) === false);
verificar('saldo intacto', saldo($usuarioId) === $saldoAntes3);
verificar('sessao marcada como "recusada"',
    (consultar_linha('SELECT status FROM sessao_descarte WHERE codigo = ?', 's', [$sessao3])['status'] ?? '') === 'recusada');

// -----------------------------------------------------------------------------
echo "\n6) Peso fora da faixa nao gera pontos\n";
// -----------------------------------------------------------------------------

[$sessao4, $chave4] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);
$saldoAntes4 = saldo($usuarioId);
$pesoAbsurdo = (float) cfg('peso_maximo_g', 2000.0) * 10;

[$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao4, 'chave_conclusao' => $chave4, 'peso_g' => $pesoAbsurdo, 'metal' => true]);
verificar('peso absurdo -> 422', $c === 422, "veio {$c}");
verificar('saldo intacto', saldo($usuarioId) === $saldoAntes4);

// -----------------------------------------------------------------------------
echo "\n7) Sessao expirada nao gera pontos\n";
// -----------------------------------------------------------------------------

[$sessao5, $chave5] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
chamar("{$base}/dispositivo/autorizacao.php", 'GET', $cabecalhosOk, null);
consultar("UPDATE sessao_descarte SET expira_em = DATE_SUB(NOW(), INTERVAL 1 MINUTE) WHERE codigo = ?", 's', [$sessao5]);
$saldoAntes5 = saldo($usuarioId);

[$c, $r] = chamar("{$base}/dispositivo/concluir.php", 'POST', $cabecalhosOk,
    ['sessao' => $sessao5, 'chave_conclusao' => $chave5, 'peso_g' => 30.0, 'metal' => true]);
verificar('sessao expirada -> 409', $c === 409, "veio {$c}");
verificar('saldo intacto', saldo($usuarioId) === $saldoAntes5);

// -----------------------------------------------------------------------------
echo "\n8) Um dispositivo atende uma pessoa por vez\n";
// -----------------------------------------------------------------------------

[$sessao6, ] = abrirSessao($usuarioId, $dispositivoId, $pilhaId);
$duplicou = false;

try {
    abrirSessao($usuarioId, $dispositivoId, $pilhaId);
    $duplicou = true;
} catch (mysqli_sql_exception $erro) {
    $duplicou = ((int) $erro->getCode() !== 1062);
}

verificar('o banco recusa duas sessoes vivas no mesmo dispositivo', !$duplicou);
encerrar($sessao6);

// -----------------------------------------------------------------------------
echo "\n9) Eventos registrados\n";
// -----------------------------------------------------------------------------

$eventos = consultar_linha('SELECT COUNT(*) AS n FROM evento_iot WHERE fk_dispositivo_id = ?', 'i', [$dispositivoId]);
verificar('evento_iot recebeu registros', (int) $eventos['n'] > 0, 'contei ' . $eventos['n']);

$vazou = consultar_linha("SELECT COUNT(*) AS n FROM evento_iot
                           WHERE mensagem LIKE ? OR mensagem LIKE ?", 'ss',
    ['%' . substr($CHAVE, 0, 12) . '%', '%' . substr($chaveConclusao, 0, 12) . '%']);
verificar('nenhuma chave vazou para o log', (int) $vazou['n'] === 0);

// -----------------------------------------------------------------------------

printf("\n---------------------------------------------\n");
printf("%d verificacoes ok, %d falha(s)\n\n", $totais['ok'], $totais['falhas']);

exit($totais['falhas'] > 0 ? 1 : 0);

<?php
/**
 * Diz, item por item, o que falta no banco desta maquina.
 *
 * Existe por causa de um sintoma que engana: a pagina do equipamento abre,
 * mostra o nome certo e, no lugar da lista de pilhas, avisa "Nenhum tipo de
 * pilha cadastrado". Nada ali indica a causa real, que e o banco ter so a
 * estrutura, sem os dados.
 *
 * Isso acontece quando a importacao do script do site para no meio. O script
 * cria as tabelas, depois define funcoes dentro de um bloco DELIMITER $$ e so
 * entao insere os dados. Ferramenta que nao entende DELIMITER aborta na
 * funcao: as tabelas ficam de pe e tudo que vem depois - funcoes, dados,
 * views, procedures, triggers - nao entra.
 *
 * So le. Nao cria, nao altera e nao apaga nada.
 *
 * Uso:
 *     php ferramentas/conferir-banco.php [nome-do-banco]
 *
 * Sem argumento, usa o banco do backend/config.php. Devolve 0 quando esta
 * tudo no lugar e 1 quando falta alguma coisa.
 */

declare(strict_types=1);

if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    exit('Esta ferramenta so roda pela linha de comando.');
}

$caminhoConfig = __DIR__ . '/../config.php';
$config = is_file($caminhoConfig)
    ? require $caminhoConfig
    : require __DIR__ . '/../config.exemplo.php';

$banco = $config['banco'] ?? [];
$nomeBanco = $argv[1] ?? ($banco['nome'] ?? 'descarteaqui');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

try {
    $conexao = new mysqli(
        $banco['host'] ?? 'localhost',
        $banco['usuario'] ?? 'root',
        $banco['senha'] ?? '',
        $nomeBanco
    );
    $conexao->set_charset('utf8mb4');
} catch (mysqli_sql_exception $falha) {
    fwrite(STDERR, "\nNao foi possivel conectar no banco '{$nomeBanco}'.\n");
    fwrite(STDERR, '  ' . $falha->getMessage() . "\n\n");
    fwrite(STDERR, "Confira se o MySQL esta ligado no painel do XAMPP e se o\n");
    fwrite(STDERR, "backend/config.php aponta para o banco certo.\n\n");
    exit(1);
}

/** Conta linhas de uma consulta que devolve um unico numero. */
$contar = static function (string $sql, array $valores = []) use ($conexao): int {
    $stmt = $conexao->prepare($sql);
    if ($valores !== []) {
        $stmt->bind_param(str_repeat('s', count($valores)), ...$valores);
    }
    $stmt->execute();
    $linha = $stmt->get_result()->fetch_row();
    $stmt->close();

    return (int) ($linha[0] ?? 0);
};

$existeTabela = static function (string $tabela) use ($contar, $nomeBanco): bool {
    return $contar(
        'SELECT COUNT(*) FROM information_schema.TABLES
          WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ?',
        [$nomeBanco, $tabela]
    ) > 0;
};

$existeRotina = static function (string $rotina) use ($contar, $nomeBanco): bool {
    return $contar(
        'SELECT COUNT(*) FROM information_schema.ROUTINES
          WHERE ROUTINE_SCHEMA = ? AND ROUTINE_NAME = ?',
        [$nomeBanco, $rotina]
    ) > 0;
};

$existeGatilho = static function (string $gatilho) use ($contar, $nomeBanco): bool {
    return $contar(
        'SELECT COUNT(*) FROM information_schema.TRIGGERS
          WHERE TRIGGER_SCHEMA = ? AND TRIGGER_NAME = ?',
        [$nomeBanco, $gatilho]
    ) > 0;
};

$existeColuna = static function (string $tabela, string $coluna) use ($contar, $nomeBanco): bool {
    return $contar(
        'SELECT COUNT(*) FROM information_schema.COLUMNS
          WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? AND COLUMN_NAME = ?',
        [$nomeBanco, $tabela, $coluna]
    ) > 0;
};

// -----------------------------------------------------------------------------
// O que conferir.
//
// 'origem' e o script que cria o item: e isso que a pessoa precisa rodar de
// novo quando o item falta.
// -----------------------------------------------------------------------------

const SCRIPT_SITE = 'banco/01_descarteaqui_entrega_final.sql (repositorio Web)';
const SCRIPT_IOT = 'backend/sql/01_integracao_iot.sql (este repositorio)';
const CADASTRO_EQUIPAMENTO = 'php ferramentas/gerar-chave.php "PUCPR" esp32-pucpr';

$itens = [];

foreach (['usuario', 'administrador', 'pilhas', 'coletor', 'recompensa',
          'ponto_coleta', 'descarte', 'troca', 'descarte_pilha',
          'log_pontos_usuario'] as $tabela) {
    $itens[] = ['tabela ' . $tabela, $existeTabela($tabela), SCRIPT_SITE];
}

$itens[] = ['funcao fn_calcular_pontos', $existeRotina('fn_calcular_pontos'), SCRIPT_SITE];
$itens[] = ['funcao fn_total_reciclado_gramas', $existeRotina('fn_total_reciclado_gramas'), SCRIPT_SITE];
$itens[] = ['funcao fn_nivel_reciclador', $existeRotina('fn_nivel_reciclador'), SCRIPT_SITE];
$itens[] = ['funcao fn_formatar_protocolo', $existeRotina('fn_formatar_protocolo'), SCRIPT_SITE];
$itens[] = ['gatilho trg_descarte_pilha_valida_bi', $existeGatilho('trg_descarte_pilha_valida_bi'), SCRIPT_SITE];
$itens[] = ['gatilho trg_usuario_log_pontos_au', $existeGatilho('trg_usuario_log_pontos_au'), SCRIPT_SITE];

$tiposDePilha = $existeTabela('pilhas') ? $contar('SELECT COUNT(*) FROM pilhas') : 0;
$itens[] = ["tipos de pilha cadastrados ({$tiposDePilha})", $tiposDePilha > 0, SCRIPT_SITE];

foreach (['dispositivo', 'sessao_descarte', 'evento_iot'] as $tabela) {
    $itens[] = ['tabela ' . $tabela, $existeTabela($tabela), SCRIPT_IOT];
}

$itens[] = [
    'coluna descarte.fk_sessao_descarte_id',
    $existeTabela('descarte') && $existeColuna('descarte', 'fk_sessao_descarte_id'),
    SCRIPT_IOT,
];

$equipamentos = $existeTabela('dispositivo') ? $contar('SELECT COUNT(*) FROM dispositivo') : 0;
$itens[] = ["equipamentos cadastrados ({$equipamentos})", $equipamentos > 0, CADASTRO_EQUIPAMENTO];

// -----------------------------------------------------------------------------
// Relatorio.
// -----------------------------------------------------------------------------

$larguraNome = 0;
foreach ($itens as [$nome]) {
    $larguraNome = max($larguraNome, strlen($nome));
}

echo "\n";
echo "Banco conferido: {$nomeBanco}\n";
echo str_repeat('=', $larguraNome + 10) . "\n\n";

$faltando = [];

foreach ($itens as [$nome, $presente, $origem]) {
    printf("  %-{$larguraNome}s  %s\n", $nome, $presente ? 'ok' : 'FALTA');
    if (!$presente) {
        $faltando[$origem][] = $nome;
    }
}

echo "\n";

if ($faltando === []) {
    echo "Tudo no lugar. O banco desta maquina tem o que o fluxo do QR Code precisa.\n\n";
    exit(0);
}

$totalFaltando = array_sum(array_map('count', $faltando));
echo "Faltam {$totalFaltando} de " . count($itens) . " itens.\n\n";

foreach ($faltando as $origem => $nomes) {
    echo "  Rode: {$origem}\n";
    foreach ($nomes as $nome) {
        echo "    - {$nome}\n";
    }
    echo "\n";
}

echo "Como rodar os scripts (pela LINHA DE COMANDO, nao pelo phpMyAdmin:\n";
echo "ele costuma abortar no bloco DELIMITER e deixar o banco pela metade):\n\n";
echo "  mysql -u root < \"../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql\"\n";
echo "  mysql -u root descarteaqui < backend/sql/01_integracao_iot.sql\n\n";
echo "O script do site recria os dados de teste, entao rodar de novo devolve\n";
echo "o banco ao estado inicial. As contas criadas no site se perdem; os\n";
echo "equipamentos cadastrados com gerar-chave.php tambem.\n\n";

exit(1);

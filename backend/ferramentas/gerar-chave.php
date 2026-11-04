<?php
/**
 * Gera a credencial de um dispositivo ESP32.
 *
 * Imprime a chave em texto puro UMA UNICA VEZ (para voce colar em
 * src/config_backend.h) e o comando SQL com o hash (para cadastrar o
 * dispositivo no banco). A chave em texto puro nunca e gravada em lugar
 * nenhum: o banco guarda so o hash.
 *
 * Uso:
 *     php ferramentas/gerar-chave.php "Ponto de coleta - Bloco 7" [codigo]
 *
 * Se o codigo nao for informado, um e sorteado. O codigo e publico: e ele
 * que vai na URL do QR Code.
 */

declare(strict_types=1);

if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    exit('Esta ferramenta so roda pela linha de comando.');
}

$nome = $argv[1] ?? '';
$codigo = $argv[2] ?? ('esp32-' . bin2hex(random_bytes(4)));

if (trim($nome) === '') {
    fwrite(STDERR, "Informe o nome do dispositivo.\n");
    fwrite(STDERR, "  php ferramentas/gerar-chave.php \"Ponto de coleta - Bloco 7\" [codigo]\n");
    exit(1);
}

if (!preg_match('/^[A-Za-z0-9_-]{4,32}$/', $codigo)) {
    fwrite(STDERR, "Codigo invalido: use de 4 a 32 caracteres entre A-Z a-z 0-9 _ -\n");
    exit(1);
}

// 32 bytes => 64 caracteres hexadecimais.
$chave = bin2hex(random_bytes(32));
$hash = password_hash($chave, PASSWORD_DEFAULT);

$escaparSql = static function (string $valor): string {
    return "'" . str_replace(["\\", "'"], ["\\\\", "\\'"], $valor) . "'";
};

echo "\n";
echo "=====================================================================\n";
echo " Credencial do dispositivo\n";
echo "=====================================================================\n\n";
echo "1) Em src/config_backend.h do firmware:\n\n";
echo "   #define DISPOSITIVO_CODIGO \"{$codigo}\"\n";
echo "   #define DISPOSITIVO_CHAVE  \"{$chave}\"\n\n";
echo "   Guarde agora: esta chave NAO pode ser recuperada depois.\n\n";
echo "2) No banco descarteaqui:\n\n";
echo "   INSERT INTO dispositivo (codigo, nome, chave_hash, fk_ponto_coleta_id)\n";
echo "   VALUES (" . $escaparSql($codigo) . ", " . $escaparSql($nome) . ", " . $escaparSql($hash) . ", NULL);\n\n";
echo "   Troque o NULL pelo id do ponto de coleta, se ja houver um.\n\n";
echo "3) No QR Code colado no dispositivo:\n\n";
echo "   https://SEU-DOMINIO/php/iot/publico/descartar.php?d={$codigo}\n\n";
echo "   So o codigo do dispositivo vai no QR Code: sem senha, sem token,\n";
echo "   sem dado pessoal.\n\n";

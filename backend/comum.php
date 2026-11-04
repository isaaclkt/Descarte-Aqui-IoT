<?php
/**
 * Utilitarios compartilhados pelas rotas da integracao IoT.
 *
 * Segue as convencoes do PHP que o site ja usa (repo "Descarte Aqui - Web",
 * pasta php/): mysqli com prepared statements, mensagens genericas para o
 * usuario e detalhe tecnico so no error_log.
 *
 * Nada aqui grava senha, hash, chave de dispositivo ou chave de conclusao em
 * log - nem no error_log, nem na tabela evento_iot.
 */

declare(strict_types=1);

// -----------------------------------------------------------------------------
// Configuracao
// -----------------------------------------------------------------------------

/**
 * Le a configuracao aceitando caminho com ponto: cfg('banco.host').
 */
function cfg(string $caminho, $padrao = null)
{
    $atual = $GLOBALS['CONFIG_IOT'] ?? [];

    foreach (explode('.', $caminho) as $parte) {
        if (!is_array($atual) || !array_key_exists($parte, $atual)) {
            return $padrao;
        }
        $atual = $atual[$parte];
    }

    return $atual;
}

// -----------------------------------------------------------------------------
// Banco de dados
// -----------------------------------------------------------------------------

/**
 * Conexao mysqli, criada na primeira chamada e reaproveitada depois.
 *
 * MYSQLI_REPORT_STRICT faz o mysqli lancar mysqli_sql_exception em vez de
 * emitir aviso: assim um erro de banco nunca passa silencioso e sempre cai no
 * tratamento das rotas, que responde 503.
 */
function bd(): mysqli
{
    static $conn = null;

    if ($conn instanceof mysqli) {
        return $conn;
    }

    mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

    $conn = new mysqli(
        (string) cfg('banco.host', 'localhost'),
        (string) cfg('banco.usuario', 'root'),
        (string) cfg('banco.senha', ''),
        (string) cfg('banco.nome', 'descarteaqui')
    );
    $conn->set_charset('utf8mb4');

    return $conn;
}

/**
 * Prepared statement com bind, no mesmo formato que php/cidadao.php usa.
 */
function consultar(string $sql, string $tipos = '', array $valores = []): mysqli_stmt
{
    $stmt = bd()->prepare($sql);

    if ($valores !== []) {
        $stmt->bind_param($tipos, ...$valores);
    }

    $stmt->execute();

    return $stmt;
}

/**
 * Primeira linha da consulta, ou null.
 */
function consultar_linha(string $sql, string $tipos = '', array $valores = []): ?array
{
    $linha = consultar($sql, $tipos, $valores)->get_result()->fetch_assoc();

    return $linha ?: null;
}

// -----------------------------------------------------------------------------
// Respostas HTTP
// -----------------------------------------------------------------------------

/**
 * Encerra a requisicao com JSON e o codigo HTTP adequado.
 */
function responder_json(int $codigo, array $dados): void
{
    http_response_code($codigo);
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    echo json_encode($dados, JSON_UNESCAPED_UNICODE | JSON_INVALID_UTF8_SUBSTITUTE);
    exit;
}

function responder_erro(int $codigo, string $erro, array $extra = []): void
{
    responder_json($codigo, ['ok' => false, 'erro' => $erro] + $extra);
}

/**
 * Corpo JSON da requisicao. Aceita tambem form-urlencoded, para facilitar
 * teste com curl -d.
 */
function ler_corpo(): array
{
    if ($_POST !== []) {
        return $_POST;
    }

    $bruto = file_get_contents('php://input');

    if ($bruto === false || trim($bruto) === '') {
        return [];
    }

    $dados = json_decode($bruto, true);

    return is_array($dados) ? $dados : [];
}

function escapar($valor): string
{
    return htmlspecialchars((string) ($valor ?? ''), ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

function ip_cliente(): string
{
    return (string) ($_SERVER['REMOTE_ADDR'] ?? '');
}

/**
 * Identificador aleatorio em hexadecimal. 16 bytes => 32 caracteres.
 */
function token_aleatorio(int $bytes = 16): string
{
    return bin2hex(random_bytes($bytes));
}

// -----------------------------------------------------------------------------
// Registro de eventos (INFO / WARN / ERROR)
// -----------------------------------------------------------------------------

/**
 * Grava um evento em evento_iot e no error_log do PHP.
 *
 * Nunca lanca excecao: falhar ao registrar log nao pode derrubar a rota que
 * esta tratando o descarte. E nunca recebe segredo - quem chama passa so
 * descricao de evento.
 */
function registrar_evento(
    string $nivel,
    string $tipo,
    string $mensagem,
    ?int $dispositivoId = null,
    ?int $sessaoId = null
): void {
    $nivel = in_array($nivel, ['INFO', 'WARN', 'ERROR'], true) ? $nivel : 'INFO';
    $mensagem = mb_substr($mensagem, 0, 255);

    error_log(sprintf('[IOT][%s] %s: %s', $nivel, $tipo, $mensagem));

    $tipo = mb_substr($tipo, 0, 40);
    $ip = ip_cliente();

    try {
        consultar(
            'INSERT INTO evento_iot (fk_dispositivo_id, fk_sessao_id, nivel, tipo, mensagem, ip)
             VALUES (?, ?, ?, ?, ?, ?)',
            'iissss',
            [$dispositivoId, $sessaoId, $nivel, $tipo, $mensagem, $ip]
        );
    } catch (Throwable $erro) {
        // Falhar ao gravar log nao pode derrubar a rota que trata o descarte.
        // O evento ja foi para o error_log acima, entao nada se perde.
        error_log('[IOT][WARN] Nao foi possivel gravar em evento_iot: ' . $erro->getMessage());
    }
}

// -----------------------------------------------------------------------------
// Sessoes de descarte
// -----------------------------------------------------------------------------

/**
 * Marca como expirada toda sessao que passou de expira_em sem conclusao.
 *
 * Roda no inicio de cada rota: a expiracao nao depende de cron nem de o
 * usuario voltar a pagina. Como dispositivo_ocupado e uma coluna gerada a
 * partir do status, expirar tambem libera o dispositivo.
 */
function expirar_sessoes(): void
{
    try {
        consultar(
            "UPDATE sessao_descarte
                SET status = 'expirada',
                    motivo = COALESCE(motivo, 'Tempo da autorizacao esgotado.')
              WHERE status IN ('aguardando', 'autorizada', 'em_andamento')
                AND expira_em <= NOW()"
        );
    } catch (Throwable $erro) {
        error_log('[IOT][ERROR] Falha ao expirar sessoes: ' . $erro->getMessage());
    }
}

// -----------------------------------------------------------------------------
// Autenticacao do dispositivo (rotas de dispositivo/)
// -----------------------------------------------------------------------------

/**
 * Identifica o ESP32 pelos headers X-Dispositivo e X-Chave.
 *
 * A chave e comparada com password_verify contra dispositivo.chave_hash: o
 * banco nunca guarda a chave em texto puro. Em caso de falha, responde 401 e
 * encerra - a rota que chamou nao continua.
 *
 * Atencao: em HTTP puro a chave trafega em claro. Em producao use HTTPS.
 */
function autenticar_dispositivo(): array
{
    $codigo = trim((string) ($_SERVER['HTTP_X_DISPOSITIVO'] ?? ''));
    $chave = (string) ($_SERVER['HTTP_X_CHAVE'] ?? '');

    if ($codigo === '' || $chave === '') {
        registrar_evento('WARN', 'auth_dispositivo', 'Requisicao sem credencial de dispositivo.');
        responder_erro(401, 'Dispositivo nao identificado.');
    }

    try {
        $dispositivo = consultar_linha(
            'SELECT id, codigo, nome, chave_hash, ativo, fk_ponto_coleta_id
               FROM dispositivo WHERE codigo = ? LIMIT 1',
            's',
            [$codigo]
        );
    } catch (Throwable $erro) {
        error_log('[IOT][ERROR] Banco indisponivel na autenticacao: ' . $erro->getMessage());
        responder_erro(503, 'Servico indisponivel.');
    }

    // password_verify com hash invalido devolve false; o tempo de resposta nao
    // revela se o codigo existe porque a mensagem e o codigo HTTP sao iguais.
    if ($dispositivo === null || !password_verify($chave, (string) $dispositivo['chave_hash'])) {
        registrar_evento(
            'WARN',
            'auth_dispositivo',
            'Credencial recusada para o codigo informado.',
            $dispositivo['id'] ?? null
        );
        responder_erro(401, 'Dispositivo nao autorizado.');
    }

    if ((int) $dispositivo['ativo'] !== 1) {
        registrar_evento('WARN', 'auth_dispositivo', 'Dispositivo inativo tentou conectar.', (int) $dispositivo['id']);
        responder_erro(403, 'Dispositivo inativo.');
    }

    unset($dispositivo['chave_hash']);

    return $dispositivo;
}

/**
 * Atualiza a telemetria de presenca do dispositivo.
 */
function marcar_contato_dispositivo(int $dispositivoId, ?string $mac = null, ?string $firmware = null): void
{
    try {
        consultar(
            'UPDATE dispositivo
                SET ultimo_contato_em = NOW(),
                    ultimo_ip = ?,
                    mac = COALESCE(?, mac),
                    firmware = COALESCE(?, firmware)
              WHERE id = ?',
            'sssi',
            [ip_cliente(), $mac, $firmware, $dispositivoId]
        );
    } catch (Throwable $erro) {
        error_log('[IOT][WARN] Falha ao marcar contato do dispositivo: ' . $erro->getMessage());
    }
}

// -----------------------------------------------------------------------------
// Autenticacao do usuario (rotas de publico/)
// -----------------------------------------------------------------------------

/**
 * Id do cidadao logado, ou null.
 *
 * Mesma regra de php/cidadao.php: exige user_id na sessao e user_role
 * 'cidadao'. Nao e um login novo - le a sessao criada por php/login.php.
 */
function usuario_logado(): ?int
{
    if (empty($_SESSION['user_id']) || ($_SESSION['user_role'] ?? '') !== 'cidadao') {
        return null;
    }

    return (int) $_SESSION['user_id'];
}

/**
 * Para rotas JSON: exige cidadao logado ou responde 401.
 */
function exigir_cidadao_json(): int
{
    $id = usuario_logado();

    if ($id === null) {
        responder_erro(401, 'Faca login para continuar.');
    }

    return $id;
}

// -----------------------------------------------------------------------------
// CSRF (mesmo formato de php/cidadao.php)
// -----------------------------------------------------------------------------

function csrf_token(): string
{
    if (empty($_SESSION['csrf'])) {
        $_SESSION['csrf'] = bin2hex(random_bytes(32));
    }

    return (string) $_SESSION['csrf'];
}

function csrf_valido(array $dados): bool
{
    $enviado = $dados['csrf'] ?? null;

    return is_string($enviado)
        && !empty($_SESSION['csrf'])
        && hash_equals((string) $_SESSION['csrf'], $enviado);
}

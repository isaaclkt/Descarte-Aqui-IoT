<?php
/**
 * Ponto de entrada comum de todas as rotas da integracao IoT.
 *
 * Faz tres coisas, nesta ordem:
 *   1) carrega a configuracao (backend/config.php, ou o modelo como padrao);
 *   2) inicia a sessao PHP reaproveitando o sessao.php DO SITE quando existe;
 *   3) carrega os utilitarios de comum.php.
 *
 * Por que o item 2 importa: o cookie de sessao do PHP so vale dentro do mesmo
 * dominio. Para que o login feito em php/login.php valha nas rotas daqui, esta
 * pasta precisa ser publicada DENTRO do site, como php/iot/. Nesse caso
 * __DIR__ . '/../sessao.php' existe e e ele que configura o cookie - a sessao
 * e literalmente a mesma. Rodando isolada (php -S, para teste), cai na copia
 * de apoio em local/sessao.php.
 *
 * Em nenhum dos casos existe um segundo sistema de autenticacao: o login
 * continua sendo o do site.
 */

declare(strict_types=1);

// -----------------------------------------------------------------------------
// 1) Configuracao
// -----------------------------------------------------------------------------

$caminhoConfig = __DIR__ . '/config.php';

if (is_file($caminhoConfig)) {
    $GLOBALS['CONFIG_IOT'] = require $caminhoConfig;
} else {
    // Sem config.php a integracao ainda sobe com os padroes do XAMPP, que sao
    // os mesmos de php/conn.php. Fica registrado para a equipe nao esquecer.
    error_log('[IOT][WARN] backend/config.php ausente; usando config.exemplo.php.');
    $GLOBALS['CONFIG_IOT'] = require __DIR__ . '/config.exemplo.php';
}

if (!is_array($GLOBALS['CONFIG_IOT'])) {
    http_response_code(500);
    exit('Configuracao invalida.');
}

// -----------------------------------------------------------------------------
// 2) Onde o site esta, no disco e na URL
//
// Nao da para descobrir isso so com __DIR__. Quando a pasta e publicada por
// link simbolico ou juncao do Windows - jeito comum de desenvolver, porque
// evita manter uma copia desatualizada -, __DIR__ aponta para o repositorio de
// origem, nao para dentro do site. A deteccao por caminho fisico entao falha e
// a pagina perde o CSS, as fontes e os botoes de login.
//
// DOCUMENT_ROOT + o trecho da URL antes de /php/iot/ funciona nos dois casos,
// copia ou link, e ainda devolve um prefixo de URL absoluto, melhor que
// '../../../' para montar os links dos assets.
// -----------------------------------------------------------------------------

$GLOBALS['IOT_RAIZ_SITE'] = null;   // caminho no disco, ou null se rodando solto
$GLOBALS['IOT_URL_SITE'] = null;    // prefixo de URL terminado em '/', ou null

(static function (): void {
    $script = (string) ($_SERVER['SCRIPT_NAME'] ?? '');
    $posicao = strpos($script, '/php/iot/');

    if ($posicao !== false) {
        $prefixo = substr($script, 0, $posicao);           // ex.: /descarte-aqui
        $docRoot = realpath((string) ($_SERVER['DOCUMENT_ROOT'] ?? ''));

        if ($docRoot !== false) {
            $candidato = $docRoot . str_replace('/', DIRECTORY_SEPARATOR, $prefixo);

            if (is_dir($candidato . DIRECTORY_SEPARATOR . 'assets' . DIRECTORY_SEPARATOR . 'css')) {
                $GLOBALS['IOT_RAIZ_SITE'] = $candidato;
                $GLOBALS['IOT_URL_SITE'] = $prefixo . '/';
                return;
            }
        }
    }

    // Sem servidor web no meio (php -S apontando direto para backend/), ou
    // layout diferente: tenta o caminho fisico.
    $fisico = realpath(__DIR__ . '/..');

    if ($fisico !== false && is_dir($fisico . DIRECTORY_SEPARATOR . 'assets' . DIRECTORY_SEPARATOR . 'css')) {
        $GLOBALS['IOT_RAIZ_SITE'] = $fisico;
        $GLOBALS['IOT_URL_SITE'] = '../../';
    }
})();

// -----------------------------------------------------------------------------
// 3) Sessao: a do site quando ele existe, a copia local quando nao
//
// E daqui que vem o compartilhamento do login: o sessao.php do site configura o
// cookie, e a sessao passa a ser a mesma criada por php/login.php.
// -----------------------------------------------------------------------------

$sessaoDoSite = $GLOBALS['IOT_RAIZ_SITE'] !== null
    ? $GLOBALS['IOT_RAIZ_SITE'] . DIRECTORY_SEPARATOR . 'php' . DIRECTORY_SEPARATOR . 'sessao.php'
    : __DIR__ . '/../sessao.php';

require_once is_file($sessaoDoSite) ? $sessaoDoSite : __DIR__ . '/local/sessao.php';

// -----------------------------------------------------------------------------
// 4) Utilitarios
// -----------------------------------------------------------------------------

require_once __DIR__ . '/comum.php';

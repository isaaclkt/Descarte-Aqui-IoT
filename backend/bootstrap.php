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
// 2) Sessao: a do site quando publicado em php/iot/, a copia local quando nao
// -----------------------------------------------------------------------------

$sessaoDoSite = __DIR__ . '/../sessao.php';
require_once is_file($sessaoDoSite) ? $sessaoDoSite : __DIR__ . '/local/sessao.php';

// -----------------------------------------------------------------------------
// 3) Utilitarios
// -----------------------------------------------------------------------------

require_once __DIR__ . '/comum.php';

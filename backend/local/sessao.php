<?php
/**
 * Copia de apoio de php/sessao.php do repositorio "Descarte Aqui - Web".
 *
 * O bootstrap.php SO usa este arquivo quando a pasta backend/ roda isolada
 * (por exemplo em php -S, para teste). Quando a pasta e publicada como
 * php/iot/ dentro do site, o bootstrap carrega o php/sessao.php DO SITE,
 * que e o unico jeito de compartilhar o cookie de sessao e, com ele, o login.
 *
 * Mantenha este arquivo igual ao do site. Ele nao e um segundo sistema de
 * autenticacao: so configura o cookie da mesma forma.
 */

if (session_status() !== PHP_SESSION_ACTIVE) {
    ini_set('session.use_strict_mode', '1');
    session_set_cookie_params([
        'httponly' => true,
        'secure'   => !empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off',
        'samesite' => 'Lax',
        'path'     => '/',
    ]);
    session_start();
}

if (!function_exists('encerrarSessao')) {
    function encerrarSessao(): void
    {
        $_SESSION = [];
        if (ini_get('session.use_cookies')) {
            $p = session_get_cookie_params();
            setcookie(session_name(), '', [
                'expires'  => time() - 42000,
                'path'     => $p['path'],
                'domain'   => $p['domain'],
                'secure'   => $p['secure'],
                'httponly' => true,
                'samesite' => 'Lax',
            ]);
        }
        session_destroy();
    }
}

<?php
/**
 * MODELO de configuracao do backend da integracao IoT.
 *
 * Copie este arquivo para  backend/config.php  e ajuste.
 * O config.php esta no .gitignore de proposito: credenciais nao vao para o Git.
 *
 *   Windows : copy backend\config.exemplo.php backend\config.php
 *   Linux   : cp backend/config.exemplo.php backend/config.php
 *
 * Os valores de banco abaixo sao os mesmos que o site usa em php/conn.php
 * (XAMPP padrao). Se a equipe mudar a senha do MySQL, mude nos dois lugares.
 */

return [
    // ---------------------------------------------------------------------
    // Banco de dados - o MESMO banco do site.
    // ---------------------------------------------------------------------
    'banco' => [
        'host'    => 'localhost',
        'usuario' => 'root',
        'senha'   => '',
        'nome'    => 'descarteaqui',
    ],

    // ---------------------------------------------------------------------
    // Sessao de descarte
    // ---------------------------------------------------------------------

    // Quanto tempo o usuario tem, depois de iniciar, para fazer o descarte
    // fisico. Curto de proposito: a autorizacao nao deve ficar viva sozinha.
    'sessao_minutos' => 3,

    // ---------------------------------------------------------------------
    // Faixa de peso aceita na conclusao, em gramas.
    //
    // O backend recusa qualquer peso fora desta faixa. Serve de rede de
    // seguranca contra balanca descalibrada ou leitura absurda: sem isso,
    // uma leitura errada da HX711 viraria pontos.
    //
    // O banco tambem tem o seu proprio limite: o trigger
    // trg_descarte_pilha_valida_bi recusa mais de 5000 g por tipo de pilha
    // numa entrega. O limite daqui e mais apertado de proposito - este
    // equipamento recebe pilha solta, nao entrega de caixa - e e o que o
    // usuario ve, com mensagem clara, antes de o banco precisar barrar.
    // ---------------------------------------------------------------------
    'peso_minimo_g' => 0.5,
    'peso_maximo_g' => 2000.0,

    // ---------------------------------------------------------------------
    // Endereco publico do site, usado para montar a URL do QR Code mostrada
    // na pagina de administracao. Sem barra no fim.
    // Deixe vazio para o backend deduzir do proprio request.
    // ---------------------------------------------------------------------
    'url_base' => '',
];

<?php
/**
 * GET /publico/descartar.php?d=<codigo_do_dispositivo>
 *
 * Pagina para onde o QR Code colado no equipamento aponta.
 *
 * O QR Code carrega SO o codigo do dispositivo. Nada de senha, token
 * permanente, credencial ou dado pessoal: quem identifica a pessoa e a sessao
 * do site, criada por php/login.php.
 *
 * Esta pagina e nova. A pagina inicial aprovada do site nao e tocada.
 */

declare(strict_types=1);

require_once __DIR__ . '/../bootstrap.php';

// -----------------------------------------------------------------------------
// Assets do site quando publicada em php/iot/publico/; estilo minimo quando
// a pasta roda isolada (teste com php -S).
// -----------------------------------------------------------------------------

$raizSite = realpath(__DIR__ . '/../../..');
$dentroDoSite = $raizSite !== false && is_dir($raizSite . '/assets/css');
$prefixo = $dentroDoSite ? '../../../' : '';

$codigoDispositivo = trim((string) ($_GET['d'] ?? ''));
$usuarioId = usuario_logado();

$erro = null;
$dispositivo = null;
$pilhas = [];
$sessaoAberta = null;

try {
    expirar_sessoes();

    if (!preg_match('/^[A-Za-z0-9_-]{4,32}$/', $codigoDispositivo)) {
        $erro = 'Endereco invalido. Escaneie novamente o QR Code do equipamento.';
    } else {
        $dispositivo = consultar_linha(
            'SELECT id, codigo, nome, ativo FROM dispositivo WHERE codigo = ? LIMIT 1',
            's',
            [$codigoDispositivo]
        );

        if ($dispositivo === null) {
            $erro = 'Equipamento nao encontrado.';
        } elseif ((int) $dispositivo['ativo'] !== 1) {
            $erro = 'Este equipamento esta fora de operacao no momento.';
        }
    }

    if ($erro === null && $usuarioId !== null) {
        $pilhas = consultar(
            'SELECT id, nome, tipo, pontos_por_grama FROM pilhas ORDER BY nome'
        )->get_result()->fetch_all(MYSQLI_ASSOC);

        // Retomada: se a pessoa recarregou a pagina no meio do descarte,
        // continuamos a sessao que ja existe em vez de pedir outra.
        $sessaoAberta = consultar_linha(
            "SELECT codigo FROM sessao_descarte
              WHERE fk_usuario_id = ? AND fk_dispositivo_id = ?
                AND status IN ('aguardando', 'autorizada', 'em_andamento')
              LIMIT 1",
            'ii',
            [$usuarioId, (int) $dispositivo['id']]
        );
    }
} catch (Throwable $falha) {
    error_log('[IOT][ERROR] descartar.php: ' . $falha->getMessage());
    $erro = 'Servico indisponivel no momento. Tente novamente em instantes.';
}

// Guarda o equipamento para depois do login.
if ($usuarioId === null && $dispositivo !== null) {
    $_SESSION['iot_dispositivo'] = $dispositivo['codigo'];
}

header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
?>
<!doctype html>
<html lang="pt-BR">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Fazer um descarte | Descarte Aqui</title>
    <meta name="description" content="Inicie um descarte no equipamento e receba seus pontos.">
    <meta name="theme-color" content="#04301A">
    <?php if ($dentroDoSite): ?>
    <link rel="icon" href="<?= $prefixo ?>assets/images/logo_descarteaqui.jpg" type="image/jpeg">
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Lexend:wght@500;600;700&family=Public+Sans:ital,wght@0,400;0,600;0,700;1,400&display=swap">
    <link rel="stylesheet" href="<?= $prefixo ?>assets/css/app.css">
    <?php endif; ?>
    <style>
        /* Complemento desta pagina. Quando o app.css do site esta carregado,
           so acrescenta o que ele nao tem; isolada, serve de estilo minimo. */
        :root { --iot-verde: #04301A; --iot-claro: #3C9A1C; --iot-laranja: #E8630A; }
        .iot-wrap { max-width: 42rem; margin: 0 auto; padding: 1.5rem 1rem 3rem; }
        .iot-card { border: 1px solid rgba(4, 48, 26, .15); border-radius: 12px;
                    padding: 1.25rem; margin-bottom: 1rem; background: #fff; }
        .iot-eq { font-size: .95rem; color: #3f5a4a; }
        .iot-eq strong { display: block; font-size: 1.15rem; color: var(--iot-verde); }
        .iot-campo { display: block; margin: 1rem 0 .35rem; font-weight: 600; }
        .iot-select { width: 100%; padding: .65rem; font: inherit;
                      border: 1px solid rgba(4, 48, 26, .3); border-radius: 8px; background: #fff; }
        .iot-botao { display: inline-block; width: 100%; padding: .8rem 1rem; font: inherit;
                     font-weight: 600; color: #fff; background: var(--iot-claro);
                     border: 0; border-radius: 8px; cursor: pointer; }
        .iot-botao[disabled] { opacity: .55; cursor: not-allowed; }
        .iot-botao--sec { background: transparent; color: var(--iot-verde);
                          border: 1px solid rgba(4, 48, 26, .35); margin-top: .6rem; }
        .iot-passos { list-style: none; padding: 0; margin: 1rem 0 0; }
        .iot-passos li { padding: .5rem 0 .5rem 1.75rem; position: relative; color: #6b7d72; }
        .iot-passos li::before { content: "○"; position: absolute; left: 0; }
        .iot-passos li[data-feito="1"] { color: var(--iot-verde); font-weight: 600; }
        .iot-passos li[data-feito="1"]::before { content: "●"; color: var(--iot-claro); }
        .iot-aviso { padding: .85rem 1rem; border-radius: 8px; margin: 0 0 1rem;
                     background: #fdf1e7; border: 1px solid rgba(232, 99, 10, .35); color: #7a3a06; }
        .iot-ok { background: #eef7ea; border-color: rgba(60, 154, 28, .4); color: #1d4d12; }
        .iot-pontos { font-size: 2rem; font-weight: 700; color: var(--iot-claro); margin: .25rem 0; }
        .iot-relogio { font-variant-numeric: tabular-nums; color: #6b7d72; font-size: .9rem; }
        body { background: #f6f8f6; }
    </style>
</head>
<body>
<main class="iot-wrap">

    <h1>Fazer um descarte</h1>

    <?php if ($erro !== null): ?>

        <p class="iot-aviso"><?= escapar($erro) ?></p>
        <?php if ($dentroDoSite): ?>
            <p><a href="<?= $prefixo ?>index.html">Voltar ao inicio</a></p>
        <?php endif; ?>

    <?php elseif ($usuarioId === null): ?>

        <div class="iot-card">
            <p class="iot-eq">Equipamento <strong><?= escapar($dispositivo['nome']) ?></strong></p>
            <p>Entre na sua conta para que os pontos deste descarte sejam creditados a voce.</p>
            <?php if ($dentroDoSite): ?>
                <p><a class="iot-botao" href="<?= $prefixo ?>paginas/entrar.html">Entrar na minha conta</a></p>
                <p><a class="iot-botao iot-botao--sec" href="<?= $prefixo ?>paginas/cadastro.html">Criar uma conta</a></p>
            <?php endif; ?>
            <p class="iot-eq">Depois de entrar, escaneie o QR Code de novo para voltar a esta tela.</p>
        </div>

    <?php else: ?>

        <div class="iot-card">
            <p class="iot-eq">Equipamento <strong><?= escapar($dispositivo['nome']) ?></strong></p>

            <div id="iot-inicio" <?= $sessaoAberta !== null ? 'hidden' : '' ?>>
                <?php if ($pilhas === []): ?>
                    <p class="iot-aviso">Nenhum tipo de pilha cadastrado. Avise o administrador.</p>
                <?php else: ?>
                    <label class="iot-campo" for="iot-pilha">Que tipo de pilha voce vai descartar?</label>
                    <select class="iot-select" id="iot-pilha">
                        <?php foreach ($pilhas as $pilha): ?>
                            <option value="<?= (int) $pilha['id'] ?>">
                                <?= escapar($pilha['nome']) ?><?= $pilha['tipo'] ? ' (' . escapar($pilha['tipo']) . ')' : '' ?>
                            </option>
                        <?php endforeach; ?>
                    </select>
                    <p class="iot-eq" style="margin-top:.75rem">
                        Os pontos sao calculados pelo peso medido na balanca do equipamento.
                        Voce nao escolhe a quantidade.
                    </p>
                    <p style="margin-top:1rem">
                        <button class="iot-botao" id="iot-iniciar" type="button">Iniciar descarte</button>
                    </p>
                <?php endif; ?>
            </div>

            <div id="iot-andamento" <?= $sessaoAberta === null ? 'hidden' : '' ?>>
                <p id="iot-mensagem" class="iot-aviso">Preparando...</p>
                <p class="iot-relogio">Tempo restante: <span id="iot-tempo">--</span></p>

                <ol class="iot-passos" id="iot-passos">
                    <li data-passo="autorizada">Equipamento recebeu a autorizacao</li>
                    <li data-passo="em_andamento">Objeto detectado, processando</li>
                    <li data-passo="concluida">Descarte registrado</li>
                </ol>

                <div id="iot-resultado" hidden>
                    <p class="iot-pontos"><span id="iot-pontos">0</span> pontos</p>
                    <p class="iot-eq">Peso medido: <span id="iot-peso">--</span> g ·
                       Saldo agora: <strong id="iot-saldo">--</strong></p>
                    <?php if ($dentroDoSite): ?>
                        <p><a class="iot-botao" href="<?= $prefixo ?>php/minha-area.php">Ver minha area</a></p>
                    <?php endif; ?>
                </div>

                <p><button class="iot-botao iot-botao--sec" id="iot-cancelar" type="button">Cancelar descarte</button></p>
            </div>
        </div>

        <script>
        (function () {
            "use strict";

            var CSRF = <?= json_encode(csrf_token()) ?>;
            var DISPOSITIVO = <?= json_encode($dispositivo['codigo']) ?>;
            var sessao = <?= json_encode($sessaoAberta['codigo'] ?? null) ?>;
            var timer = null;

            function el(id) { return document.getElementById(id); }

            function mostrarErro(texto) {
                var m = el("iot-mensagem");
                m.textContent = texto;
                m.className = "iot-aviso";
            }

            function formatarTempo(s) {
                if (s <= 0) { return "esgotado"; }
                var min = Math.floor(s / 60);
                var seg = s % 60;
                return min + ":" + (seg < 10 ? "0" : "") + seg;
            }

            function marcarPassos(status) {
                var ordem = ["autorizada", "em_andamento", "concluida"];
                var atingido = ordem.indexOf(status);
                var itens = el("iot-passos").querySelectorAll("li");

                for (var i = 0; i < itens.length; i++) {
                    itens[i].setAttribute("data-feito", i <= atingido ? "1" : "0");
                }
            }

            async function iniciar() {
                var botao = el("iot-iniciar");
                botao.disabled = true;

                try {
                    var resposta = await fetch("sessao-abrir.php", {
                        method: "POST",
                        headers: { "Content-Type": "application/json" },
                        body: JSON.stringify({
                            csrf: CSRF,
                            dispositivo: DISPOSITIVO,
                            pilha: Number(el("iot-pilha").value)
                        })
                    });
                    var dados = await resposta.json();

                    if (!resposta.ok || !dados.ok) {
                        botao.disabled = false;
                        alert(dados.erro || "Nao foi possivel iniciar o descarte.");
                        return;
                    }

                    sessao = dados.sessao;
                    el("iot-inicio").hidden = true;
                    el("iot-andamento").hidden = false;
                    acompanhar();
                } catch (falha) {
                    botao.disabled = false;
                    alert("Sem conexao com o servidor. Tente novamente.");
                }
            }

            async function consultar() {
                if (!sessao) { return; }

                try {
                    var resposta = await fetch("sessao-status.php?sessao=" + encodeURIComponent(sessao), {
                        headers: { "Accept": "application/json" }
                    });
                    var dados = await resposta.json();

                    if (!resposta.ok || !dados.ok) {
                        mostrarErro(dados.erro || "Nao foi possivel consultar o andamento.");
                        return;
                    }

                    var mensagem = el("iot-mensagem");
                    mensagem.textContent = dados.mensagem;
                    mensagem.className = dados.status === "concluida" ? "iot-aviso iot-ok" : "iot-aviso";

                    el("iot-tempo").textContent = formatarTempo(dados.segundos);
                    marcarPassos(dados.status);

                    if (dados.status === "concluida") {
                        el("iot-pontos").textContent = dados.pontos;
                        el("iot-peso").textContent = dados.peso_g;
                        el("iot-saldo").textContent = dados.saldo;
                        el("iot-resultado").hidden = false;
                    }

                    if (dados.encerrada) {
                        clearInterval(timer);
                        el("iot-cancelar").hidden = true;
                        el("iot-tempo").parentNode.hidden = true;
                    }
                } catch (falha) {
                    // Perda momentanea de rede nao derruba a tela: a proxima
                    // consulta tenta de novo.
                    mostrarErro("Sem conexao. Tentando novamente...");
                }
            }

            function acompanhar() {
                consultar();
                clearInterval(timer);
                timer = setInterval(consultar, 2000);
            }

            async function cancelar() {
                if (!sessao) { return; }

                try {
                    await fetch("sessao-cancelar.php", {
                        method: "POST",
                        headers: { "Content-Type": "application/json" },
                        body: JSON.stringify({ csrf: CSRF, sessao: sessao })
                    });
                } catch (falha) {
                    // Mesmo sem resposta, a sessao expira sozinha.
                }

                consultar();
            }

            if (el("iot-iniciar")) { el("iot-iniciar").addEventListener("click", iniciar); }
            el("iot-cancelar").addEventListener("click", cancelar);

            if (sessao) { acompanhar(); }
        })();
        </script>

    <?php endif; ?>

</main>
</body>
</html>

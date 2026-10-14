// ARQUIVO GERADO AUTOMATICAMENTE - NAO EDITE A MAO.
// Edite os arquivos em src/client/ e rode: python tools/gerar_pagina_web.py

#ifndef PAGINA_WEB_H
#define PAGINA_WEB_H

#include <Arduino.h>

// ---------- index.html ----------
static const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Descarte Aqui - Painel</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>

  <header class="cabecalho">
    <h1>Descarte Aqui</h1>
    <p class="subtitulo">Sistema inteligente para descarte de resíduos</p>
    <div class="linha-conexao">
      <span id="badge-conexao" class="badge badge-off">Desconectado</span>
      <span class="ip">IP do ESP32: <strong id="ip-esp32">--</strong></span>
      <span class="ip">Rede: <strong id="rede-esp32">--</strong></span>
    </div>
  </header>

  <main>

    <section class="painel">
      <h2>Estado do sistema</h2>
      <p id="estado-sistema" class="estado-grande">--</p>
    </section>

    <section class="grade" aria-label="Sensores e atuadores">

      <article class="cartao">
        <h3>Objeto presente</h3>
        <p class="valor" id="objeto-presente">--</p>
        <p class="detalhe">Distância: <span id="distancia">--</span> cm</p>
      </article>

      <article class="cartao">
        <h3>Material</h3>
        <p class="valor" id="metal">--</p>
        <p class="detalhe">Sensor indutivo (GPIO 32)</p>
      </article>

      <article class="cartao">
        <h3>Peso</h3>
        <p class="valor"><span id="peso">--</span> g</p>
        <p class="detalhe">Leitura bruta: <span id="peso-bruto">--</span></p>
      </article>

      <article class="cartao">
        <h3>Porta (atuador)</h3>
        <p class="valor" id="estado-porta">--</p>
        <p class="detalhe">Servo no GPIO 26</p>
      </article>

      <article class="cartao">
        <h3>Servo de separação</h3>
        <p class="valor"><span id="servo-separacao">--</span>&deg;</p>
        <p class="detalhe">Servo no GPIO 25</p>
      </article>

      <article class="cartao">
        <h3>Tempo ligado</h3>
        <p class="valor" id="uptime">--</p>
        <p class="detalhe">Memória livre: <span id="heap">--</span> bytes</p>
      </article>

    </section>

    <section class="painel">
      <h2>Comandos</h2>
      <p class="aviso">Os comandos abaixo acionam o servo da portinha diretamente pela rede.</p>
      <div class="botoes">
        <button id="btn-abrir" class="botao botao-on">Abrir porta</button>
        <button id="btn-fechar" class="botao botao-off">Fechar porta</button>
      </div>
      <p id="retorno-comando" class="retorno" role="status" aria-live="polite"></p>
    </section>

  </main>

  <footer class="rodape">
    <span>Última atualização: <strong id="ultima-atualizacao">--</strong></span>
  </footer>

  <script src="app.js"></script>
</body>
</html>
)rawliteral";

// ---------- style.css ----------
static const char PAGINA_CSS[] PROGMEM = R"rawliteral(
/* Descarte Aqui - estilos do painel web servido pelo ESP32 */

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  padding: 0 16px 32px;
  font-family: "Segoe UI", Roboto, Arial, sans-serif;
  background: #11161d;
  color: #e8edf3;
  line-height: 1.5;
}

/* ---------- Cabeçalho ---------- */

.cabecalho {
  max-width: 960px;
  margin: 0 auto;
  padding: 28px 0 20px;
  border-bottom: 1px solid #263141;
}

.cabecalho h1 {
  margin: 0;
  font-size: 1.9rem;
  color: #46d39a;
}

.subtitulo {
  margin: 4px 0 16px;
  color: #93a1b3;
  font-size: 0.92rem;
}

.linha-conexao {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  align-items: center;
}

.badge {
  display: inline-block;
  padding: 4px 12px;
  border-radius: 999px;
  font-size: 0.82rem;
  font-weight: 600;
}

.badge-on {
  background: #10492f;
  color: #46d39a;
}

.badge-off {
  background: #4a1c1c;
  color: #f07a7a;
}

.ip {
  font-size: 0.88rem;
  color: #93a1b3;
}

.ip strong {
  color: #e8edf3;
}

/* ---------- Blocos ---------- */

main {
  max-width: 960px;
  margin: 0 auto;
}

.painel {
  margin: 24px 0;
  padding: 18px 20px;
  background: #182028;
  border: 1px solid #263141;
  border-radius: 10px;
}

.painel h2 {
  margin: 0 0 10px;
  font-size: 1.05rem;
  color: #93a1b3;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}

.estado-grande {
  margin: 0;
  font-size: 1.5rem;
  font-weight: 600;
  color: #46d39a;
}

/* ---------- Grade de cartões ---------- */

.grade {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
  gap: 14px;
  margin: 24px 0;
}

.cartao {
  padding: 16px 18px;
  background: #182028;
  border: 1px solid #263141;
  border-radius: 10px;
}

.cartao h3 {
  margin: 0 0 8px;
  font-size: 0.78rem;
  color: #93a1b3;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}

.valor {
  margin: 0 0 4px;
  font-size: 1.45rem;
  font-weight: 600;
  color: #e8edf3;
}

.detalhe {
  margin: 0;
  font-size: 0.8rem;
  color: #6f7f93;
}

/* ---------- Comandos ---------- */

.aviso {
  margin: 0 0 14px;
  font-size: 0.85rem;
  color: #93a1b3;
}

.botoes {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.botao {
  padding: 11px 24px;
  font-size: 0.95rem;
  font-weight: 600;
  color: #11161d;
  border: none;
  border-radius: 8px;
  cursor: pointer;
}

.botao:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.botao-on {
  background: #46d39a;
}

.botao-off {
  background: #f0a67a;
}

.retorno {
  margin: 12px 0 0;
  font-size: 0.85rem;
  min-height: 1.2em;
  color: #93a1b3;
}

.retorno.erro {
  color: #f07a7a;
}

/* ---------- Rodapé ---------- */

.rodape {
  max-width: 960px;
  margin: 0 auto;
  padding-top: 16px;
  border-top: 1px solid #263141;
  font-size: 0.82rem;
  color: #6f7f93;
}

.cabecalho,
main,
.rodape {
  width: 100%;
  max-width: 1280px;
}

main > *,
.cartao {
  min-width: 0;
}

.ip strong,
.valor {
  overflow-wrap: anywhere;
}

.botao {
  min-height: 48px;
  transition: filter 150ms ease;
}

.botao:hover:not(:disabled) {
  filter: brightness(1.12);
}

.botao:focus-visible {
  outline: 3px solid #e8edf3;
  outline-offset: 4px;
}

@media (min-width: 1024px) {
  body {
    padding: 0 40px 32px;
  }

  .cabecalho {
    padding: 40px 0 28px;
  }

  .cabecalho h1 {
    font-size: 2.4rem;
  }

  main {
    display: grid;
    grid-template-columns: minmax(0, 1fr) 320px;
    gap: 24px;
    padding: 28px 0;
    align-items: start;
  }

  main > .painel:first-child {
    grid-column: 1;
    grid-row: 1;
    margin: 0;
    padding: 24px;
  }

  .estado-grande {
    font-size: 1.8rem;
  }

  .grade {
    grid-column: 1;
    grid-row: 2;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    margin: 0;
    gap: 18px;
  }

  .cartao {
    min-height: 156px;
    padding: 24px 20px;
  }

  main > .painel:last-child {
    grid-column: 2;
    grid-row: 1 / span 2;
    margin: 0;
    padding: 24px;
    position: sticky;
    top: 24px;
  }

  .botoes {
    flex-direction: column;
  }

  .botao {
    width: 100%;
  }
}

@media (max-width: 600px) {
  .grade {
    grid-template-columns: 1fr;
  }

  .linha-conexao {
    align-items: flex-start;
    flex-direction: column;
    gap: 8px;
  }

  .botoes {
    flex-direction: column;
  }

  .botao {
    width: 100%;
  }
}
)rawliteral";

// ---------- app.js ----------
static const char PAGINA_JS[] PROGMEM = R"rawliteral(
/* Descarte Aqui - cliente web que conversa com o servidor HTTP do ESP32 */

// Quando a pagina e servida pelo proprio ESP32, o endereco base e vazio
// (as requisicoes vao para o mesmo IP que serviu a pagina).
// Se voce abrir este arquivo direto do disco para testar, coloque aqui
// o IP do ESP32, por exemplo: "http://192.168.1.100"
const ESP32_BASE_URL = "";

const INTERVALO_MS = 2000;   // de quanto em quanto tempo consultamos o ESP32
const TIMEOUT_MS = 3000;     // tempo maximo de espera por resposta

// ---------------------------------------------------------------
// Atalhos de acesso ao DOM
// ---------------------------------------------------------------

function el(id) {
  return document.getElementById(id);
}

// ---------------------------------------------------------------
// Requisicao HTTP com timeout (trata queda de conexao do ESP32)
// ---------------------------------------------------------------

async function buscarJson(rota) {
  const controle = new AbortController();
  const timer = setTimeout(() => controle.abort(), TIMEOUT_MS);

  try {
    const resposta = await fetch(ESP32_BASE_URL + rota, { signal: controle.signal });
    if (!resposta.ok) {
      throw new Error("HTTP " + resposta.status);
    }
    return await resposta.json();
  } finally {
    clearTimeout(timer);
  }
}

// ---------------------------------------------------------------
// Atualizacao da tela
// ---------------------------------------------------------------

function marcarConectado(conectado) {
  const badge = el("badge-conexao");
  badge.textContent = conectado ? "Conectado" : "Desconectado";
  badge.className = "badge " + (conectado ? "badge-on" : "badge-off");

  el("btn-abrir").disabled = !conectado;
  el("btn-fechar").disabled = !conectado;
}

function formatarUptime(segundos) {
  const h = Math.floor(segundos / 3600);
  const m = Math.floor((segundos % 3600) / 60);
  const s = segundos % 60;
  return h + "h " + m + "m " + s + "s";
}

function aplicarStatus(status) {
  el("ip-esp32").textContent = status.ip;

  // modo_rede: "STA" = entrou numa rede existente
  //            "AP"  = o proprio ESP32 criou a rede
  el("rede-esp32").textContent =
    status.ssid + (status.modo_rede === "AP" ? " (rede do ESP32)" : "");
  el("estado-sistema").textContent = status.estado;
  el("estado-porta").textContent = status.porta;
  el("servo-separacao").textContent = status.servo_separacao;
  el("uptime").textContent = formatarUptime(status.uptime_s);
  el("heap").textContent = status.heap_livre;
}

function aplicarSensor(sensor) {
  el("distancia").textContent =
    sensor.distancia_cm < 0 ? "sem eco" : sensor.distancia_cm.toFixed(1);

  el("objeto-presente").textContent = sensor.objeto_presente ? "Sim" : "Nao";
  el("metal").textContent = sensor.metal_detectado ? "Metal (pilha)" : "Nao metal";

  if (sensor.balanca_ok) {
    el("peso").textContent = sensor.peso_g.toFixed(1);
    el("peso-bruto").textContent = sensor.peso_bruto;
  } else {
    el("peso").textContent = "--";
    el("peso-bruto").textContent = "balanca indisponivel";
  }
}

function marcarHorario() {
  el("ultima-atualizacao").textContent = new Date().toLocaleTimeString("pt-BR");
}

// ---------------------------------------------------------------
// Ciclo de consulta: /status + /sensor
// ---------------------------------------------------------------

async function atualizar() {
  try {
    const [status, sensor] = await Promise.all([
      buscarJson("/status"),
      buscarJson("/sensor")
    ]);

    aplicarStatus(status);
    aplicarSensor(sensor);
    marcarConectado(true);
    marcarHorario();
  } catch (erro) {
    console.log("Falha ao consultar o ESP32:", erro.message);
    marcarConectado(false);
  }
}

// ---------------------------------------------------------------
// Envio de comandos para o atuador
// ---------------------------------------------------------------

async function enviarComando(estado) {
  const retorno = el("retorno-comando");

  try {
    const resposta = await buscarJson("/atuador?estado=" + estado);
    retorno.className = "retorno";
    retorno.textContent = "Comando aceito. Porta agora: " + resposta.estado;
    atualizar();
  } catch (erro) {
    retorno.className = "retorno erro";
    retorno.textContent = "Falha ao enviar o comando: " + erro.message;
  }
}

el("btn-abrir").addEventListener("click", () => enviarComando("on"));
el("btn-fechar").addEventListener("click", () => enviarComando("off"));

// ---------------------------------------------------------------
// Inicio
// ---------------------------------------------------------------

atualizar();
setInterval(atualizar, INTERVALO_MS);
)rawliteral";

#endif  // PAGINA_WEB_H

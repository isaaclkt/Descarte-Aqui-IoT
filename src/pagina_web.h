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
      <a class="link-3d" href="/3d">Ver o modelo 3D</a>
    </div>
  </header>

  <main>

    <section class="painel">
      <h2>Estado do sistema</h2>
      <p id="estado-sistema" class="estado-grande">--</p>
    </section>

    <section class="painel" aria-label="Descarte autorizado pelo site">
      <h2>Descarte pelo site</h2>
      <p class="aviso" id="iot-aviso">Consultando a integração...</p>

      <div class="grade grade-iot">

        <article class="cartao">
          <h3>Sessão</h3>
          <p class="valor" id="iot-sessao">--</p>
          <p class="detalhe">Código: <span id="iot-codigo">--</span></p>
        </article>

        <article class="cartao">
          <h3>Tipo declarado</h3>
          <p class="valor" id="iot-pilha">--</p>
          <p class="detalhe"><span id="iot-ppg">--</span> pontos por grama</p>
        </article>

        <article class="cartao">
          <h3>Backend</h3>
          <p class="valor" id="iot-backend">--</p>
          <p class="detalhe">Último contato: <span id="iot-contato">--</span></p>
        </article>

        <article class="cartao">
          <h3>Último descarte</h3>
          <p class="valor"><span id="iot-pontos">--</span> pts</p>
          <p class="detalhe" id="iot-motivo">--</p>
        </article>

      </div>
    </section>

    <div class="conteudo">

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

    <section class="painel painel-3d" aria-label="Modelo 3D do equipamento">
      <h2>Modelo 3D</h2>
      <p class="aviso">Espelho visual do equipamento: mostra apenas o que o ESP32 confirmou.</p>
      <div class="moldura-3d" id="moldura-3d">
        <button id="btn-carregar-3d" class="botao botao-on">Mostrar modelo 3D</button>
        <p class="detalhe">São 113 KB servidos pela placa, o carregamento leva alguns segundos.</p>
      </div>
      <p class="detalhe"><a class="link-3d" href="/3d" target="_blank" rel="noopener">Abrir em tela cheia</a></p>
    </section>

    </div>

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
.link-3d {
  font-size: 0.88rem;
  font-weight: 600;
  color: #46d39a;
  text-decoration: none;
  border-bottom: 1px solid rgba(70, 211, 154, 0.4);
}

.link-3d:hover {
  border-bottom-color: #46d39a;
}

/* ---------- Painel do modelo 3D ---------- */

/* No celular os dois ficam empilhados; a partir de 960 px o 3D vai para o
   lado dos cartões. */
.conteudo {
  display: grid;
  grid-template-columns: 1fr;
  gap: 14px;
  margin: 24px 0;
}

@media (min-width: 960px) {
  .conteudo {
    grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
    align-items: start;
  }
}

.conteudo > .grade,
.conteudo > .painel-3d {
  margin: 0;
}

.moldura-3d {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 10px;
  min-height: 300px;
  padding: 16px;
  background: #11161d;
  border: 1px solid #263141;
  border-radius: 8px;
  text-align: center;
}

.moldura-3d.is-carregado {
  padding: 0;
  display: block;
}

.moldura-3d iframe {
  display: block;
  width: 100%;
  height: 420px;
  border: 0;
  border-radius: 8px;
}

/* Grade dentro do painel "Descarte pelo site": a .grade solta usa 24px de
   margem vertical, que aqui abriria um vao grande logo abaixo do aviso. */
.grade-iot {
  margin: 14px 0 0;
}

/* Faixa de simulacao: so aparece quando /status traz "simulado": true, ou
   seja, quando quem responde e tools/simular_esp32.py e nao a placa. */
.faixa-simulacao {
  margin: 0;
  padding: 10px 16px;
  background: #7a1d1d;
  color: #ffe9e9;
  font-weight: 600;
  font-size: 0.9rem;
  text-align: center;
  letter-spacing: 0.3px;
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

let ultimoStatus = null;
let ultimoSensor = null;

// Faixa de aviso quando quem responde e o simulador, nao a placa.
//
// O firmware real nunca envia "simulado", entao a faixa so aparece em
// tools/simular_esp32.py. Existe para ninguem apresentar uma demonstracao
// como se fosse medicao de hardware - inclusive sem querer, com a tela
// projetada e a placa desligada em cima da mesa.
function marcarSimulacao(simulado) {
  const existente = document.getElementById("faixa-simulacao");

  if (!simulado) {
    if (existente) existente.remove();
    return;
  }

  if (existente) return;

  const faixa = document.createElement("p");
  faixa.id = "faixa-simulacao";
  faixa.className = "faixa-simulacao";
  faixa.textContent =
    "SIMULACAO - nao ha placa conectada. Sensores, servos e balanca sao " +
    "valores gerados por tools/simular_esp32.py.";
  document.body.insertBefore(faixa, document.body.firstChild);
}

function aplicarStatus(status) {
  ultimoStatus = status;
  marcarSimulacao(status.simulado === true);
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
  ultimoSensor = sensor;
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
// Painel "Descarte pelo site" (rota /descarte)
//
// Esta parte e so informativa: mostra o que o ESP32 combinou com o backend.
// Se a integracao estiver desligada ou o backend estiver fora do ar, o
// painel diz isso e o resto da pagina continua funcionando igual.
// ---------------------------------------------------------------

const TEXTO_SESSAO = {
  ocioso: "Sem autorização",
  autorizada: "Autorizada",
  em_andamento: "Em andamento",
  enviando: "Enviando resultado",
  concluida: "Concluída"
};

const TEXTO_BACKEND = {
  ok: "Respondendo",
  sem_rede: "Sem Wi-Fi",
  erro_http: "Fora do ar",
  nao_configurado: "Não configurado"
};

function aplicarDescarte(d) {
  el("iot-sessao").textContent = TEXTO_SESSAO[d.sessao_estado] || d.sessao_estado;
  el("iot-codigo").textContent = d.sessao_parcial ? d.sessao_parcial + "..." : "--";
  el("iot-pilha").textContent = d.pilha || "--";
  el("iot-ppg").textContent = d.pontos_por_grama > 0 ? d.pontos_por_grama : "--";
  el("iot-backend").textContent = TEXTO_BACKEND[d.backend_estado] || d.backend_estado;

  el("iot-contato").textContent =
    d.backend_ultimo_contato_s < 0 ? "nunca" : d.backend_ultimo_contato_s + " s atrás";

  el("iot-pontos").textContent = d.ultimo_descarte > 0 ? d.ultimos_pontos : "--";

  const aviso = el("iot-aviso");

  if (!d.backend_configurado) {
    aviso.className = "aviso";
    aviso.textContent =
      "Integração desligada (src/config_backend.h não preenchido). " +
      "O servidor local, os sensores e o servo funcionam normalmente.";
    el("iot-motivo").textContent = "--";
    return;
  }

  if (d.sessao_estado === "autorizada") {
    aviso.className = "aviso";
    aviso.textContent =
      "Autorizado: " + d.pilha + ". Restam " + d.segundos_restantes + " s para o descarte.";
  } else if (d.sessao_estado === "em_andamento") {
    aviso.className = "aviso";
    aviso.textContent = "Descarte em andamento.";
  } else if (d.conclusao_pendente) {
    aviso.className = "aviso";
    aviso.textContent = "Enviando o resultado ao backend...";
  } else if (d.sessao_estado === "concluida") {
    aviso.className = "aviso";
    aviso.textContent = d.ultimo_creditado
      ? "Descarte " + d.ultimo_descarte + " registrado: " + d.ultimos_pontos + " pontos."
      : "Último descarte não pontuou.";
  } else {
    aviso.className = "aviso";
    aviso.textContent = "Nenhuma autorização ativa. Escaneie o QR Code no site para iniciar.";
  }

  el("iot-motivo").textContent = d.ultimo_motivo || (d.ultimo_creditado ? "Creditado" : "--");
}

async function atualizarDescarte() {
  try {
    aplicarDescarte(await buscarJson("/descarte"));
  } catch (erro) {
    // Falha aqui NAO marca a pagina como desconectada: /descarte e um extra.
    el("iot-aviso").className = "aviso";
    el("iot-aviso").textContent = "Não foi possível ler o estado do descarte.";
  }
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
    avisarModelo3d(status, sensor, "estado do ESP32");

    // Depois do essencial e fora do Promise.all de proposito: se /descarte
    // falhar, a pagina nao passa a se dizer desconectada.
    atualizarDescarte();
  } catch (erro) {
    console.log("Falha ao consultar o ESP32:", erro.message);
    marcarConectado(false);
    avisarFalha3d();

    // Sem a placa nao ha o que dizer sobre a sessao de descarte. Sem isto o
    // painel ficaria preso em "Consultando a integracao..." para sempre.
    el("iot-aviso").className = "aviso";
    el("iot-aviso").textContent = "Sem conexão com a placa.";
  }
}

// ---------------------------------------------------------------
// Sincronia com o modelo 3D
//
// O 3D embutido nao consulta o ESP32 por conta propria: quem consulta e
// esta pagina, que repassa o estado. Assim o WebServer da placa, que atende
// uma conexao por vez, continua com um unico cliente consultando.
// O BroadcastChannel alcanca tambem o 3D aberto em outra aba.
// ---------------------------------------------------------------

const CANAL_SINCRONIA = "descarte-aqui";

const canal3d = "BroadcastChannel" in window ? new BroadcastChannel(CANAL_SINCRONIA) : null;

function enviar3d(mensagem) {
  const quadro = el("iframe-3d");
  if (quadro && quadro.contentWindow) {
    quadro.contentWindow.postMessage(mensagem, window.location.origin);
  }
  if (canal3d) canal3d.postMessage(mensagem);
}

function avisarModelo3d(status, sensor, origem) {
  enviar3d({ tipo: CANAL_SINCRONIA, status, sensor, origem });
}

function avisarFalha3d() {
  enviar3d({ tipo: CANAL_SINCRONIA, erro: true });
}

// ---------------------------------------------------------------
// Carga sob demanda do 3D
// ---------------------------------------------------------------

function carregar3d() {
  const moldura = el("moldura-3d");
  if (moldura.classList.contains("is-carregado")) return;

  const quadro = document.createElement("iframe");
  quadro.id = "iframe-3d";
  quadro.src = "/3d?embed=1";
  quadro.title = "Modelo 3D do ponto de coleta";
  quadro.loading = "lazy";

  moldura.innerHTML = "";
  moldura.classList.add("is-carregado");
  moldura.appendChild(quadro);

  // Assim que o 3D terminar de carregar, mandamos o estado que ja temos
  // para ele nao ficar esperando o proximo ciclo de consulta.
  quadro.addEventListener("load", () => {
    if (ultimoStatus) avisarModelo3d(ultimoStatus, ultimoSensor, "estado do ESP32");
  });
}

el("btn-carregar-3d").addEventListener("click", carregar3d);

// ---------------------------------------------------------------
// Envio de comandos para o atuador
// ---------------------------------------------------------------

async function enviarComando(estado) {
  const retorno = el("retorno-comando");
  const botoes = [el("btn-abrir"), el("btn-fechar")];

  // Trava os dois botoes enquanto o comando esta em andamento, para nao
  // acumular cliques enquanto a placa ainda nao respondeu.
  botoes.forEach((botao) => { botao.disabled = true; });

  try {
    const resposta = await buscarJson("/atuador?estado=" + estado);

    // So avisamos o 3D depois do ok do ESP32. Se a placa recusar ou nao
    // responder, o modelo nao se mexe.
    if (!resposta.ok) throw new Error(resposta.erro || "comando recusado");

    retorno.className = "retorno";
    retorno.textContent = "Comando aceito. Porta agora: " + resposta.estado;

    if (ultimoStatus) {
      avisarModelo3d({ ...ultimoStatus, porta: resposta.estado }, ultimoSensor, "comando da página");
    }

    await atualizar();
  } catch (erro) {
    retorno.className = "retorno erro";
    retorno.textContent = "Falha ao enviar o comando: " + erro.message;
  } finally {
    botoes.forEach((botao) => { botao.disabled = false; });
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

// ---------- ponto_coleta3d.html ----------
static const char PAGINA_3D[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Descarte Aqui 3D</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Archivo:wdth,wght@62..125,400..900&family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500;600&display=swap">
<style>
/* Layout: palco 3D em tela cheia; marca no topo à esquerda, fluxo à direita, controles em dock inferior */
:root {
  color-scheme: dark;
  --bg: #0a0e0c;
  --panel: rgba(13, 19, 16, 0.74);
  --panel-solid: #111814;
  --line: rgba(150, 210, 170, 0.14);
  --line-strong: rgba(150, 210, 170, 0.3);
  --text: #e7efe9;
  --muted: #8d9e95;
  --green: #3fae3a;
  --green-deep: #1f6b35;
  --glow: #5ee27a;
  --oled: #7fd6ff;
  --warn: #f2b84b;
  --font-display: "Archivo", "Arial Black", system-ui, sans-serif;
  --font-body: "IBM Plex Sans", system-ui, -apple-system, "Segoe UI", sans-serif;
  --font-mono: "IBM Plex Mono", ui-monospace, "SFMono-Regular", Menlo, monospace;
  --radius: 14px;
}

* { box-sizing: border-box; }
html, body { height: 100%; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 14px/1.45 var(--font-body);
  overflow: hidden;
  -webkit-font-smoothing: antialiased;
}

.app { position: fixed; inset: 0; }
#viewport { position: absolute; inset: 0; }
#viewport canvas { display: block; outline: none; touch-action: none; }
.app::after {
  content: "";
  position: absolute;
  inset: 0;
  pointer-events: none;
  background: radial-gradient(ellipse 80% 70% at 50% 45%, transparent 55%, rgba(0, 0, 0, 0.45) 100%);
}

.label-layer { position: absolute; inset: 0; pointer-events: none; opacity: 0; transition: opacity 0.5s ease; z-index: 1; }
.label-layer.is-visible { opacity: 1; }
.label-layer.is-visible .hotspot { pointer-events: auto; }

.hotspot {
  display: flex;
  align-items: center;
  padding: 0;
  border: 0;
  background: none;
  color: var(--text);
  font: 500 10.5px/1 var(--font-mono);
  letter-spacing: 0.06em;
  text-transform: uppercase;
  cursor: pointer;
  pointer-events: none;
  white-space: nowrap;
}
.hotspot--left { flex-direction: row-reverse; }
.hotspot__dot {
  width: 9px;
  height: 9px;
  border-radius: 50%;
  background: var(--glow);
  box-shadow: 0 0 0 3px rgba(94, 226, 122, 0.18), 0 0 10px rgba(94, 226, 122, 0.7);
  flex: none;
}
.hotspot--right .hotspot__dot { margin-left: -4.5px; }
.hotspot--left .hotspot__dot { margin-right: -4.5px; }
.hotspot__line { width: 26px; height: 1px; background: linear-gradient(90deg, rgba(94, 226, 122, 0.8), rgba(94, 226, 122, 0.25)); }
.hotspot--left .hotspot__line { transform: scaleX(-1); }
.hotspot__text {
  padding: 5px 8px;
  border: 1px solid var(--line-strong);
  border-radius: 6px;
  background: rgba(8, 12, 10, 0.82);
  backdrop-filter: blur(6px);
  transition: color 0.2s, border-color 0.2s, background 0.2s;
}
.hotspot:hover .hotspot__text,
.hotspot:focus-visible .hotspot__text,
.hotspot.is-active .hotspot__text { color: var(--glow); border-color: rgba(94, 226, 122, 0.7); background: rgba(14, 32, 20, 0.9); }
.hotspot:focus-visible { outline: none; }
.hotspot__short { display: none; }

.brand {
  position: absolute;
  top: calc(20px + env(safe-area-inset-top, 0px));
  left: 24px;
  display: flex;
  align-items: center;
  gap: 12px;
  z-index: 2;
  pointer-events: none;
}
.brand canvas { width: 44px; height: 44px; flex: none; }
.brand h1 {
  margin: 0;
  font: 800 19px/1 var(--font-display);
  font-stretch: 125%;
  letter-spacing: 0.02em;
  text-transform: uppercase;
}
.brand h1 span { color: var(--green); }
.brand p { margin: 5px 0 0; color: var(--muted); font-size: 12.5px; letter-spacing: 0.02em; }

.status {
  position: absolute;
  top: calc(26px + env(safe-area-inset-top, 0px));
  right: 24px;
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 7px 12px;
  border: 1px solid var(--line);
  border-radius: 999px;
  background: var(--panel);
  backdrop-filter: blur(10px);
  color: var(--muted);
  font: 500 11px/1 var(--font-mono);
  letter-spacing: 0.04em;
  z-index: 2;
}
.status__dot { width: 7px; height: 7px; border-radius: 50%; background: var(--warn); box-shadow: 0 0 8px rgba(242, 184, 75, 0.6); }
.status.is-running .status__dot { background: var(--glow); box-shadow: 0 0 8px rgba(94, 226, 122, 0.8); }
.status.is-live .status__dot { background: var(--glow); box-shadow: 0 0 8px rgba(94, 226, 122, 0.8); }
.status.is-error .status__dot { background: #ff4a3a; box-shadow: 0 0 8px rgba(255, 74, 58, 0.8); }

.flow {
  position: absolute;
  top: calc(84px + env(safe-area-inset-top, 0px));
  right: 24px;
  width: 272px;
  padding: 16px 14px 14px;
  border: 1px solid var(--line);
  border-radius: var(--radius);
  background: var(--panel);
  backdrop-filter: blur(12px);
  z-index: 2;
}
.flow h2, .card__eyebrow, .dock__label {
  margin: 0;
  color: var(--muted);
  font: 500 10.5px/1 var(--font-mono);
  letter-spacing: 0.12em;
  text-transform: uppercase;
}
.flow h2 { padding: 0 4px 12px; }
.flow ol { list-style: none; margin: 0; padding: 0; display: grid; gap: 2px; }
.flow__step {
  position: relative;
  display: grid;
  grid-template-columns: 26px 1fr;
  gap: 10px;
  align-items: center;
  width: 100%;
  padding: 8px 6px;
  border: 1px solid transparent;
  border-radius: 10px;
  background: none;
  color: var(--text);
  font: inherit;
  text-align: left;
  cursor: pointer;
  transition: background 0.2s, border-color 0.2s;
}
.flow__step:hover { background: rgba(94, 226, 122, 0.06); }
.flow__step:focus-visible { outline: 2px solid var(--glow); outline-offset: 1px; }
.flow__num {
  display: grid;
  place-items: center;
  width: 26px;
  height: 26px;
  border: 1px solid var(--line-strong);
  border-radius: 50%;
  color: var(--muted);
  font: 600 11px/1 var(--font-mono);
  font-variant-numeric: tabular-nums;
  transition: all 0.25s;
}
.flow__title { display: block; font-weight: 500; font-size: 13.5px; line-height: 1.2; }
.flow__sub { display: block; margin-top: 2px; color: var(--muted); font-size: 12px; line-height: 1.3; }
.flow li:not(:last-child) .flow__step::after {
  content: "";
  position: absolute;
  left: 18.5px;
  bottom: -5px;
  width: 1px;
  height: 9px;
  background: var(--line-strong);
}
.flow__split { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; padding: 6px 0 0 42px; }
.flow__split .flow__step { grid-template-columns: 1fr; padding: 8px 10px; border-color: var(--line); }
.flow__split .flow__step::after { display: none; }
.flow__tag { display: inline-flex; align-items: center; gap: 6px; font-weight: 500; font-size: 13px; }
.flow__tag::before { content: ""; width: 8px; height: 8px; border-radius: 2px; background: var(--green); }
.flow__tag--reject::before { background: #6c7772; }

.flow__step.is-done .flow__num { border-color: rgba(94, 226, 122, 0.45); color: var(--glow); }
.flow__step.is-active { border-color: rgba(94, 226, 122, 0.55); background: rgba(94, 226, 122, 0.09); }
.flow__step.is-active .flow__num { background: var(--glow); border-color: var(--glow); color: #06120a; box-shadow: 0 0 14px rgba(94, 226, 122, 0.6); }
.flow__split .flow__step.is-active.is-reject { border-color: rgba(242, 184, 75, 0.6); background: rgba(242, 184, 75, 0.08); }

.card {
  position: absolute;
  left: 24px;
  bottom: calc(96px + env(safe-area-inset-bottom, 0px));
  width: min(320px, calc(100% - 48px));
  padding: 16px 44px 16px 16px;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  background: var(--panel);
  backdrop-filter: blur(12px);
  z-index: 3;
}
.card h2 { margin: 8px 0 6px; font: 700 17px/1.2 var(--font-display); font-stretch: 112%; text-wrap: balance; }
.card p:last-of-type { margin: 0; color: #c3d0c8; font-size: 13.5px; line-height: 1.5; }
.card__close {
  position: absolute;
  top: 10px;
  right: 10px;
  width: 28px;
  height: 28px;
  border: 1px solid var(--line);
  border-radius: 8px;
  background: none;
  color: var(--muted);
  font-size: 16px;
  line-height: 1;
  cursor: pointer;
}
.card__close:hover { color: var(--text); border-color: var(--line-strong); }

.dock {
  position: absolute;
  left: 50%;
  bottom: calc(22px + env(safe-area-inset-bottom, 0px));
  transform: translateX(-50%);
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 6px;
  border: 1px solid var(--line);
  border-radius: 16px;
  background: var(--panel);
  backdrop-filter: blur(14px);
  z-index: 3;
  max-width: calc(100% - 32px);
}
.dock__group { display: flex; align-items: center; gap: 4px; }
.dock__divider { width: 1px; height: 26px; background: var(--line); flex: none; }
.dock__label { padding: 0 6px 0 8px; }
.segmented { display: flex; padding: 3px; border-radius: 11px; background: rgba(255, 255, 255, 0.04); }
.btn {
  display: inline-flex;
  align-items: center;
  gap: 7px;
  height: 36px;
  padding: 0 14px;
  border: 1px solid transparent;
  border-radius: 9px;
  background: none;
  color: var(--text);
  font: 500 13px/1 var(--font-body);
  white-space: nowrap;
  cursor: pointer;
  transition: background 0.2s, color 0.2s, border-color 0.2s;
}
.btn svg { width: 15px; height: 15px; flex: none; }
.btn:hover { background: rgba(255, 255, 255, 0.06); }
.btn:focus-visible { outline: 2px solid var(--glow); outline-offset: 1px; }
.btn[aria-pressed="true"] { background: var(--green); color: #06120a; font-weight: 600; }
.btn--ghost { border-color: var(--line); }
.btn--sim { border-color: rgba(94, 226, 122, 0.35); color: var(--glow); }
.btn--sim:hover { background: rgba(94, 226, 122, 0.1); }
.btn--sim.btn--reject { border-color: rgba(242, 184, 75, 0.35); color: var(--warn); }
.btn--sim.btn--reject:hover { background: rgba(242, 184, 75, 0.08); }
.btn:disabled { opacity: 0.4; cursor: default; }

.hint {
  position: absolute;
  left: 24px;
  top: calc(80px + env(safe-area-inset-top, 0px));
  margin: 0;
  color: var(--muted);
  font: 400 11px/1.6 var(--font-mono);
  letter-spacing: 0.03em;
  z-index: 2;
  pointer-events: none;
}

.loader {
  position: absolute;
  inset: 0;
  display: grid;
  place-items: center;
  background: var(--bg);
  z-index: 10;
  transition: opacity 0.7s ease, visibility 0.7s;
}
.loader.is-hidden { opacity: 0; visibility: hidden; }
.loader__inner { display: grid; justify-items: center; gap: 14px; padding: 0 24px; text-align: center; }
.loader__title { font: 800 15px/1 var(--font-display); font-stretch: 125%; letter-spacing: 0.08em; text-transform: uppercase; }
.loader__bar { width: 160px; height: 2px; border-radius: 2px; background: var(--line); overflow: hidden; }
.loader__bar::after { content: ""; display: block; width: 40%; height: 100%; background: var(--glow); animation: loading 1.1s ease-in-out infinite; }
.loader p { margin: 0; max-width: 34ch; color: var(--muted); font: 400 12px/1.5 var(--font-mono); }
.loader.is-error .loader__bar { display: none; }
.loader.is-error p { color: var(--warn); }
@keyframes loading { from { transform: translateX(-100%); } to { transform: translateX(250%); } }

@media (max-width: 1100px) {
  .hint { display: none; }
  .dock { flex-wrap: wrap; justify-content: center; }
}
@media (max-width: 860px) {
  .flow { display: none; }
  .status { display: none; }
  .brand { left: 16px; }
  .brand h1 { font-size: 16px; }
  .dock { flex-wrap: wrap; justify-content: center; bottom: calc(14px + env(safe-area-inset-bottom, 0px)); width: calc(100% - 32px); }
  .dock__divider { display: none; }
  .dock__label { display: none; }
  .card { left: 16px; right: 16px; width: auto; bottom: calc(176px + env(safe-area-inset-bottom, 0px)); }
  .hotspot { font-size: 9.5px; }
  .hotspot__full { display: none; }
  .hotspot__short { display: inline; }
  .btn__text { display: none; }
  #resetCamera { padding: 0 11px; }
  .hotspot__line { width: 16px; }
}
@media (max-width: 420px) {
  .btn { padding: 0 10px; font-size: 12.5px; }
}
@media (prefers-reduced-motion: reduce) {
  .loader__bar::after { animation: none; }
}
</style>
</head>
<body>
<div class="app">
  <div id="viewport" aria-label="Modelo 3D interativo do ponto de coleta Descarte Aqui"></div>

  <header class="brand">
    <canvas id="brandMark" width="88" height="88" aria-hidden="true"></canvas>
    <div>
      <h1>Descarte <span>Aqui</span></h1>
      <p>Ponto de Coleta Inteligente</p>
    </div>
  </header>

  <div class="status" id="statusChip" role="status">
    <span class="status__dot"></span>
    <span id="statusText">Visualização · sem conexão com o ESP32</span>
  </div>

  <aside class="flow" aria-label="Fluxo de funcionamento">
    <h2>Fluxo de funcionamento</h2>
    <ol>
      <li><button class="flow__step" type="button" data-stage="entrada"><span class="flow__num">1</span><span><span class="flow__title">Entrada</span><span class="flow__sub">Pilha inserida na abertura</span></span></button></li>
      <li><button class="flow__step" type="button" data-stage="sensor1"><span class="flow__num">2</span><span><span class="flow__title">Sensor 1</span><span class="flow__sub">Detecta a presença</span></span></button></li>
      <li><button class="flow__step" type="button" data-stage="sensor2"><span class="flow__num">3</span><span><span class="flow__title">Sensor 2</span><span class="flow__sub">Lê o tipo de objeto</span></span></button></li>
      <li><button class="flow__step" type="button" data-stage="esp32"><span class="flow__num">4</span><span><span class="flow__title">Identificação</span><span class="flow__sub">ESP32 decide o destino</span></span></button></li>
      <li><button class="flow__step" type="button" data-stage="servo"><span class="flow__num">5</span><span><span class="flow__title">Servo</span><span class="flow__sub">Inclina a bandeja defletora</span></span></button></li>
      <li class="flow__split">
        <button class="flow__step" type="button" data-stage="pilhas"><span class="flow__tag">Pilha</span></button>
        <button class="flow__step is-reject" type="button" data-stage="rejeicao"><span class="flow__tag flow__tag--reject">Rejeição</span></button>
      </li>
    </ol>
  </aside>

  <section class="card" id="componentCard" hidden aria-live="polite">
    <p class="card__eyebrow" id="cardEyebrow">Componente</p>
    <h2 id="cardTitle"></h2>
    <p id="cardText"></p>
    <button class="card__close" id="cardClose" type="button" aria-label="Fechar detalhes">×</button>
  </section>

  <nav class="dock" aria-label="Controles da visualização">
    <div class="segmented" role="group" aria-label="Modo de visualização">
      <button class="btn" type="button" id="viewExterior" data-view="exterior" aria-pressed="true">Visão Externa</button>
      <button class="btn" type="button" id="viewInterior" data-view="interior" aria-pressed="false">Visão Interna</button>
    </div>
    <button class="btn btn--ghost" type="button" id="resetCamera" aria-label="Resetar Câmera">
      <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M2.5 8a5.5 5.5 0 1 0 1.7-4"/><path d="M2.5 2.5v3h3"/></svg>
      <span class="btn__text">Resetar Câmera</span>
    </button>
    <span class="dock__divider" aria-hidden="true"></span>
    <div class="dock__group" role="group" aria-label="Simular descarte">
      <span class="dock__label">Simular</span>
      <button class="btn btn--sim" type="button" id="simulateBattery" data-simulate="pilha">
        <svg viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M4.5 2.8v10.4L13 8z"/></svg>
        Pilha
      </button>
      <button class="btn btn--sim btn--reject" type="button" id="simulateReject" data-simulate="rejeicao">
        <svg viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M4.5 2.8v10.4L13 8z"/></svg>
        Objeto inválido
      </button>
    </div>
    <span class="dock__divider" aria-hidden="true"></span>
    <button class="btn btn--sim btn--reject" type="button" id="simulateRejectDoor" data-simulate="portinha">
      <svg viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M4.5 2.8v10.4L13 8z"/></svg>
      Simular objeto rejeitado
    </button>
    <span class="dock__divider" aria-hidden="true"></span>
    <div class="dock__group" role="group" aria-label="Comandar a portinha no ESP32">
      <span class="dock__label">Portinha</span>
      <button class="btn" type="button" id="abrirPortinha" data-atuador="on" disabled>Abrir</button>
      <button class="btn btn--ghost" type="button" id="fecharPortinha" data-atuador="off" disabled>Fechar</button>
    </div>
  </nav>

  <p class="hint">Arraste para girar · Role para aproximar · Botão direito para mover<br>Clique em um componente para ver detalhes</p>

  <div class="loader" id="loader">
    <div class="loader__inner">
      <span class="loader__title">Descarte Aqui</span>
      <span class="loader__bar"></span>
      <p id="loaderText">Carregando modelo 3D</p>
    </div>
  </div>
</div>

<script type="importmap">
{
  "imports": {
    "three": "https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.js",
    "three/addons/": "https://cdn.jsdelivr.net/npm/three@0.160.0/examples/jsm/"
  }
}
</script>
<script>
window.setTimeout(function () {
  if (window.descarteAqui) return;
  document.getElementById('loader').classList.add('is-error');
  document.getElementById('loaderText').textContent = 'Não foi possível carregar o Three.js. Verifique a conexão com a internet e recarregue a página.';
}, 15000);
</script>
<script type="module">
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { CSS2DRenderer, CSS2DObject } from 'three/addons/renderers/CSS2DRenderer.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';

const BODY = { width: 0.9, depth: 0.72, feetTop: 0.05, plinthTop: 0.26, top: 1.9, wall: 0.022, corner: 0.03 };
const TOP_SURFACE = BODY.top + 0.046;
const HOLE_RADIUS = 0.145;
const FLOW_Z = 0.04;
const PEDESTAL_HEIGHT = 0.035;
const SENSOR1_Y = 1.64;
const SENSOR2_Y = 1.435;
const TRAY = { pivotY: 1.4, halfLength: 0.085, tilt: 0.75 };
const CHUTE = { startX: 0.07, startY: 1.322, endX: 0.255, endY: 1.128 };
const BIN_FLOOR = { pilhas: 0.844 };
const REJECT = { x0: 0.07, x1: 0.34, back: -0.16, front: 0.336, bottom: 0.776, top: 1.072, floorBackZ: -0.15, floorBackY: 0.9, floorFrontY: 0.795, pocketZ: 0.285 };
const DOOR = { x0: 0.1, x1: 0.31, y0: 0.8, y1: 0.97, hingeZ: 0.364, openAngle: 1.1, speed: 1.4, closeDelay: 0.7 };
const DOOR_LINK = { x: 0.257, shaftY: 1.03, shaftZ: 0.289 };
const DOOR_HINGE = new THREE.Vector3(DOOR_LINK.x, DOOR.y1, DOOR.hingeZ);
const DOOR_SHAFT = new THREE.Vector3(DOOR_LINK.x, DOOR_LINK.shaftY, DOOR_LINK.shaftZ);
const DOOR_ARM = new THREE.Vector3(0, -0.012, -0.045);
const AXIS_X = new THREE.Vector3(1, 0, 0);
const COLORS = { background: 0x0a0e0c, glow: 0x5ee27a, warn: 0xf2b84b, oled: '#7fd6ff' };
const FONT_DISPLAY = '"Archivo", "Arial Black", sans-serif';
const FONT_MONO = '"IBM Plex Mono", ui-monospace, monospace';
const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

const COMPONENTS = {
  entrada: {
    label: 'Entrada', side: 'right', outside: true, distance: 1.25, stage: 1,
    title: 'Abertura de entrada',
    text: 'A pilha é inserida pela abertura superior. O anel luminoso indica que o ponto está pronto e muda de cor quando um objeto é recusado.'
  },
  sensor1: {
    label: 'Sensor 1', side: 'right', distance: 0.75, stage: 2,
    title: 'Sensor 1 · presença (HC-SR04)',
    text: 'Sensor ultrassônico montado na entrada, ligado aos pinos TRIG 19 e ECHO 18. Mede a distância até o objeto e, abaixo de 5 cm, dispara o ciclo de separação.'
  },
  sensor2: {
    label: 'Sensor 2', side: 'right', distance: 0.8, stage: 3,
    title: 'Sensor 2 · identificação',
    text: 'Sensor indutivo cilíndrico apontado para o objeto parado sobre a bandeja. Confirma se ele tem o corpo metálico de uma pilha.'
  },
  esp32: {
    label: 'ESP32', side: 'right', distance: 0.85, stage: 4,
    title: 'ESP32 · identificação',
    text: 'Lê os dois sensores e a célula de carga, decide entre pilha e rejeição, comanda o servo e atualiza o OLED. Futuramente envia os dados ao site.'
  },
  servo: {
    label: 'Servo', side: 'left', distance: 0.75, stage: 5,
    title: 'Servo motor · direcionamento',
    text: 'Gira a bandeja defletora: inclina para a esquerda para aceitar a pilha ou para a direita para mandar o objeto à rejeição.'
  },
  pilhas: {
    label: 'Compartimento de pilhas', short: 'Pilhas', side: 'left', distance: 1.2, stage: 6,
    title: 'Compartimento de pilhas',
    text: 'Recipiente removível que armazena as pilhas aceitas. Fica apoiado sobre a célula de carga, que mede o peso de cada descarte.'
  },
  rejeicao: {
    label: 'Compartimento de rejeição', short: 'Rejeição', side: 'right', distance: 1.2, stage: 6,
    title: 'Compartimento de rejeição',
    text: 'Recebe tudo o que não foi identificado como pilha. O piso inclinado leva o objeto até a portinha de devolução, na frente do equipamento.'
  },
  hcsr04: {
    label: 'Sensor previsto', side: 'right', distance: 0.75,
    title: 'Posição prevista · ainda não instalada',
    text: 'O protótipo tem um único HC-SR04, e ele fica na entrada (Sensor 1). Este corpo marca onde entraria um segundo sensor para confirmar a retirada do objeto — ele ainda não existe no firmware.'
  },
  portinha: {
    label: 'Portinha + servo', side: 'right', outside: true, distance: 0.8,
    title: 'Portinha de devolução',
    text: 'Portinha articulada por uma dobradiça superior, movida por um servo no GPIO 26 entre 0° (fechada) e 90° (aberta). No ciclo automático ela abre por 5 segundos fixos quando o objeto não é metálico, e também aceita comando pela rota /atuador.'
  },
  celula: {
    label: 'Célula de carga + HX711', short: 'Célula de carga', side: 'left', distance: 0.85,
    title: 'Célula de carga + HX711',
    text: 'Barra extensométrica sob o compartimento de pilhas. O módulo HX711 amplifica o sinal e entrega a leitura de peso ao ESP32.'
  },
  oled: {
    label: 'OLED', side: 'left', outside: true, distance: 0.9,
    title: 'Display OLED',
    text: 'Mostra o estado do equipamento: aguardando, objeto detectado, pilha identificada, objeto rejeitado e pesagem.'
  },
  fonte: {
    label: 'Fonte 5 V', side: 'left', distance: 0.9,
    title: 'Fonte de alimentação',
    text: 'Fornece 5 V ao sistema. Um regulador entrega 3,3 V à lógica do ESP32 e dos sensores.'
  }
};

const FLOW_ORDER = ['entrada', 'sensor1', 'sensor2', 'esp32', 'servo'];

const OLED_SCREENS = {
  aguardando: { title: 'DESCARTE AQUI', icon: 'battery', lines: () => ['Aguardando', 'insira uma pilha'] },
  detectado: { title: 'OBJETO DETECTADO', icon: 'cube', lines: () => ['Analisando...'] },
  pilha: { title: 'PILHA IDENTIFICADA', icon: 'batteryFull', lines: () => ['Descarte aceito'] },
  rejeitado: { title: 'OBJETO REJEITADO', icon: 'trash', lines: () => ['Não é pilha', 'Devolvendo...'] },
  pesagem: { title: 'PILHA', icon: 'batteryFull', lines: (data) => [`Peso: ${(data.peso ?? 0).toFixed(1).replace('.', ',')} g`, 'Processando...'] }
};

const ui = {
  viewport: document.getElementById('viewport'),
  loader: document.getElementById('loader'),
  loaderText: document.getElementById('loaderText'),
  viewButtons: [...document.querySelectorAll('[data-view]')],
  resetButton: document.getElementById('resetCamera'),
  simulateButtons: [...document.querySelectorAll('[data-simulate]')],
  flowSteps: [...document.querySelectorAll('[data-stage]')],
  card: document.getElementById('componentCard'),
  cardEyebrow: document.getElementById('cardEyebrow'),
  cardTitle: document.getElementById('cardTitle'),
  cardText: document.getElementById('cardText'),
  cardClose: document.getElementById('cardClose'),
  statusChip: document.getElementById('statusChip'),
  statusText: document.getElementById('statusText'),
  doorButtons: [...document.querySelectorAll('[data-atuador]')],
  brandMark: document.getElementById('brandMark')
};

const state = {
  viewMode: 'exterior',
  xray: 0,
  activeComponent: null,
  simulationRunning: false,
  lastInteraction: performance.now(),
  ringAlert: 0,
  ringBoost: 0,
  sensor1Flash: 0,
  sensor2Scan: 0,
  sensor2Result: 0,
  espActivity: 0,
  loadCellPulse: 0
};

const tweens = new Set();
const xrayParts = [];
const ghostEdges = [];
const pickables = [];
const hotspots = {};
const componentGroups = {};
const anchors = {};
const deposited = { pilhas: [], rejeicao: [] };
const waiters = new Set();
const rejectDoor = { angle: 0, target: 0, detecting: false, clearTime: 0, items: new Set(), statusVisible: false, lastStatus: '' };
let cameraMoveId = 0;
let xrayMoveId = 0;

let renderer, scene, camera, controls, composer, labelRenderer, deviceRoot;
let parts = {};

const easeInOutCubic = (t) => (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2);
const easeOutCubic = (t) => 1 - Math.pow(1 - t, 3);
const easeInQuad = (t) => t * t;
const easeOutBack = (t) => 1 + 2.2 * Math.pow(t - 1, 3) + 1.2 * Math.pow(t - 1, 2);
const linear = (t) => t;

function tween(duration, update, ease = easeInOutCubic) {
  const scaled = prefersReducedMotion ? Math.min(duration, 160) : duration;
  return new Promise((resolve) => {
    tweens.add({ start: performance.now(), duration: Math.max(1, scaled), update, ease, resolve });
  });
}

function wait(ms) {
  return tween(ms, () => {}, linear);
}

function updateTweens(now) {
  for (const item of tweens) {
    const progress = Math.min(1, (now - item.start) / item.duration);
    item.update(item.ease(progress));
    if (progress >= 1) {
      tweens.delete(item);
      item.resolve();
    }
  }
}

function seededRandom(seed) {
  let value = seed;
  return () => {
    value = (value * 16807) % 2147483647;
    return (value - 1) / 2147483646;
  };
}

function canvasTexture(width, height, draw) {
  const canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext('2d');
  draw(ctx, width, height);
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.anisotropy = 8;
  return texture;
}

function setFont(ctx, family, weight, size, { italic = false, expanded = false } = {}) {
  ctx.font = `${italic ? 'italic ' : ''}${weight} ${size}px ${family}`;
  if ('fontStretch' in ctx) ctx.fontStretch = expanded ? 'expanded' : 'normal';
}

function setLetterSpacing(ctx, value) {
  if ('letterSpacing' in ctx) ctx.letterSpacing = value;
}

function roundedRectPath(ctx, x, y, w, h, r) {
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  ctx.closePath();
}

function paintGraphite(ctx, w, h, seed = 7) {
  const gradient = ctx.createLinearGradient(0, 0, w * 0.45, h);
  gradient.addColorStop(0, '#262d2a');
  gradient.addColorStop(0.5, '#1b211e');
  gradient.addColorStop(1, '#121614');
  ctx.fillStyle = gradient;
  ctx.fillRect(0, 0, w, h);
  const random = seededRandom(seed);
  for (let x = 0; x < w; x += 2) {
    ctx.fillStyle = random() > 0.5 ? 'rgba(255,255,255,0.025)' : 'rgba(0,0,0,0.05)';
    ctx.fillRect(x, 0, 1, h);
  }
}

function drawRecycleMark(ctx, cx, cy, r, color, options = {}) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.fillStyle = color;
  ctx.lineWidth = r * 0.27;
  for (let i = 0; i < 3; i += 1) {
    const start = -Math.PI / 2 + (i * 2 * Math.PI) / 3 + 0.3;
    const end = start + (2 * Math.PI) / 3 - 0.95;
    ctx.beginPath();
    ctx.arc(cx, cy, r, start, end);
    ctx.stroke();
    const px = cx + r * Math.cos(end);
    const py = cy + r * Math.sin(end);
    const tx = -Math.sin(end);
    const ty = Math.cos(end);
    const nx = Math.cos(end);
    const ny = Math.sin(end);
    const head = r * 0.3;
    ctx.beginPath();
    ctx.moveTo(px + tx * head * 1.3, py + ty * head * 1.3);
    ctx.lineTo(px + nx * head, py + ny * head);
    ctx.lineTo(px - nx * head, py - ny * head);
    ctx.closePath();
    ctx.fill();
  }
  if (options.battery) {
    drawBatteryIcon(ctx, cx, cy + r * 0.04, r * 0.42, r * 0.74, options.batteryColor ?? color, { lineWidth: r * 0.08, bolt: true });
  }
  if (options.leaf) {
    drawLeaf(ctx, cx + r * 0.92, cy + r * 0.72, r * 0.75, 0.75, options.leaf);
  }
  ctx.restore();
}

function drawBatteryIcon(ctx, cx, cy, w, h, color, { lineWidth = w * 0.12, level = 0, bolt = false } = {}) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.fillStyle = color;
  ctx.lineWidth = lineWidth;
  const capWidth = w * 0.42;
  const capHeight = h * 0.08;
  roundedRectPath(ctx, cx - capWidth / 2, cy - h / 2 - capHeight, capWidth, capHeight + lineWidth, Math.min(capHeight / 2, 6));
  ctx.fill();
  roundedRectPath(ctx, cx - w / 2, cy - h / 2, w, h, w * 0.16);
  ctx.stroke();
  if (level > 0) {
    const pad = lineWidth * 1.5;
    const innerHeight = (h - pad * 2) * level;
    roundedRectPath(ctx, cx - w / 2 + pad, cy + h / 2 - pad - innerHeight, w - pad * 2, innerHeight, w * 0.07);
    ctx.fill();
  }
  if (bolt) {
    const s = h * 0.3;
    ctx.beginPath();
    ctx.moveTo(cx + s * 0.18, cy - s);
    ctx.lineTo(cx - s * 0.38, cy + s * 0.12);
    ctx.lineTo(cx - s * 0.02, cy + s * 0.12);
    ctx.lineTo(cx - s * 0.18, cy + s);
    ctx.lineTo(cx + s * 0.38, cy - s * 0.12);
    ctx.lineTo(cx + s * 0.02, cy - s * 0.12);
    ctx.closePath();
    ctx.fill();
  }
  ctx.restore();
}

function drawLeaf(ctx, x, y, size, angle, color) {
  ctx.save();
  ctx.translate(x, y);
  ctx.rotate(angle);
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(0, size * 0.5);
  ctx.bezierCurveTo(size * 0.52, size * 0.22, size * 0.42, -size * 0.32, 0, -size * 0.5);
  ctx.bezierCurveTo(-size * 0.42, -size * 0.32, -size * 0.52, size * 0.22, 0, size * 0.5);
  ctx.fill();
  ctx.strokeStyle = 'rgba(8, 20, 12, 0.5)';
  ctx.lineWidth = size * 0.035;
  ctx.beginPath();
  ctx.moveTo(0, size * 0.46);
  ctx.lineTo(0, -size * 0.36);
  ctx.stroke();
  ctx.restore();
}

function drawTrashIcon(ctx, cx, cy, size, color, lineWidth) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  const w = size * 0.7;
  ctx.beginPath();
  ctx.moveTo(cx - w * 0.62, cy - size * 0.36);
  ctx.lineTo(cx + w * 0.62, cy - size * 0.36);
  ctx.moveTo(cx - w * 0.2, cy - size * 0.36);
  ctx.lineTo(cx - w * 0.2, cy - size * 0.48);
  ctx.lineTo(cx + w * 0.2, cy - size * 0.48);
  ctx.lineTo(cx + w * 0.2, cy - size * 0.36);
  ctx.moveTo(cx - w * 0.48, cy - size * 0.26);
  ctx.lineTo(cx - w * 0.4, cy + size * 0.48);
  ctx.lineTo(cx + w * 0.4, cy + size * 0.48);
  ctx.lineTo(cx + w * 0.48, cy - size * 0.26);
  for (const offset of [-0.18, 0, 0.18]) {
    ctx.moveTo(cx + w * offset, cy - size * 0.14);
    ctx.lineTo(cx + w * offset, cy + size * 0.34);
  }
  ctx.stroke();
  ctx.restore();
}

function drawCubeIcon(ctx, cx, cy, size, color, lineWidth) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth;
  ctx.lineJoin = 'round';
  const s = size / 2;
  ctx.beginPath();
  ctx.moveTo(cx, cy - s);
  ctx.lineTo(cx + s * 0.88, cy - s * 0.5);
  ctx.lineTo(cx + s * 0.88, cy + s * 0.5);
  ctx.lineTo(cx, cy + s);
  ctx.lineTo(cx - s * 0.88, cy + s * 0.5);
  ctx.lineTo(cx - s * 0.88, cy - s * 0.5);
  ctx.closePath();
  ctx.moveTo(cx - s * 0.88, cy - s * 0.5);
  ctx.lineTo(cx, cy);
  ctx.lineTo(cx + s * 0.88, cy - s * 0.5);
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx, cy + s);
  ctx.stroke();
  ctx.restore();
}

function createFrontTexture() {
  return canvasTexture(640, 1250, (ctx, w, h) => {
    paintGraphite(ctx, w, h, 11);
    const oledY = h * 0.195;
    roundedRectPath(ctx, w / 2 - 150, oledY - 96, 300, 192, 20);
    ctx.fillStyle = '#0c100e';
    ctx.fill();
    ctx.strokeStyle = 'rgba(94, 226, 122, 0.4)';
    ctx.lineWidth = 2;
    ctx.stroke();
    ctx.fillStyle = 'rgba(214, 232, 221, 0.6)';
    ctx.textAlign = 'center';
    setFont(ctx, FONT_MONO, 500, 19);
    setLetterSpacing(ctx, '3px');
    ctx.fillText('INSIRA A PILHA NA ABERTURA', w / 2, oledY + 142);
    setLetterSpacing(ctx, '0px');
    drawBatteryIcon(ctx, w / 2, h * 0.45, 118, 206, '#3fae3a', { lineWidth: 13, level: 0.3 });
    drawRecycleMark(ctx, w / 2, h * 0.665, 70, '#3fae3a');
    ctx.beginPath();
    ctx.moveTo(0, h * 0.8);
    ctx.bezierCurveTo(w * 0.35, h * 0.835, w * 0.7, h * 0.895, w, h * 0.92);
    ctx.lineTo(w, h);
    ctx.lineTo(0, h);
    ctx.closePath();
    const swoosh = ctx.createLinearGradient(0, h * 0.8, w, h);
    swoosh.addColorStop(0, '#45b53f');
    swoosh.addColorStop(1, '#22803b');
    ctx.fillStyle = swoosh;
    ctx.fill();
    ctx.beginPath();
    ctx.moveTo(0, h * 0.8);
    ctx.bezierCurveTo(w * 0.35, h * 0.835, w * 0.7, h * 0.895, w, h * 0.92);
    ctx.strokeStyle = 'rgba(160, 255, 180, 0.65)';
    ctx.lineWidth = 3;
    ctx.stroke();
    const sheen = ctx.createLinearGradient(0, 0, w, 0);
    sheen.addColorStop(0, 'rgba(255,255,255,0.06)');
    sheen.addColorStop(0.25, 'rgba(255,255,255,0)');
    ctx.fillStyle = sheen;
    ctx.fillRect(0, 0, w, h);
  });
}

function createSideTexture(variant) {
  return canvasTexture(512, 1272, (ctx, w, h) => {
    paintGraphite(ctx, w, h, variant === 'right' ? 23 : 37);
    ctx.beginPath();
    if (variant === 'right') {
      ctx.moveTo(0, h * 0.925);
      ctx.bezierCurveTo(w * 0.4, h * 0.9, w * 0.72, h * 0.83, w, h * 0.78);
    } else {
      ctx.moveTo(w, h * 0.805);
      ctx.bezierCurveTo(w * 0.62, h * 0.83, w * 0.3, h * 0.88, 0, h * 0.9);
    }
    ctx.lineTo(variant === 'right' ? w : 0, h);
    ctx.lineTo(variant === 'right' ? 0 : w, h);
    ctx.closePath();
    const wedge = ctx.createLinearGradient(0, h * 0.78, 0, h);
    wedge.addColorStop(0, '#3fae3a');
    wedge.addColorStop(1, '#22803b');
    ctx.fillStyle = wedge;
    ctx.fill();

    if (variant === 'right') {
      drawLeaf(ctx, w * 0.66, h * 0.12, 120, 0.5, '#3fae3a');
      drawLeaf(ctx, w * 0.36, h * 0.66, 92, -0.4, '#2f9e44');
      ctx.save();
      ctx.translate(w * 0.5, h * 0.4);
      ctx.rotate(-0.13);
      ctx.fillStyle = '#4cbf45';
      ctx.textAlign = 'center';
      setFont(ctx, FONT_DISPLAY, 700, 54, { italic: true });
      ['Pequenas', 'ações fazem', 'grande', 'diferença!'].forEach((line, index) => {
        ctx.fillText(line, 0, (index - 1.5) * 66);
      });
      ctx.restore();
    } else {
      drawRecycleMark(ctx, w * 0.5, h * 0.34, 96, '#3fae3a', { battery: true, batteryColor: '#d9e8de' });
      ctx.save();
      ctx.translate(w * 0.5, h * 0.62);
      ctx.fillStyle = 'rgba(214, 232, 221, 0.75)';
      ctx.textAlign = 'center';
      setFont(ctx, FONT_DISPLAY, 800, 40, { expanded: true });
      ctx.fillText('DESCARTE', 0, 0);
      ctx.fillStyle = '#3fae3a';
      ctx.fillText('AQUI', 0, 48);
      setFont(ctx, FONT_MONO, 500, 18);
      setLetterSpacing(ctx, '4px');
      ctx.fillStyle = 'rgba(214, 232, 221, 0.5)';
      ctx.fillText('COLETA DE PILHAS', 0, 100);
      setLetterSpacing(ctx, '0px');
      ctx.restore();
    }
  });
}

function createBackTexture() {
  return canvasTexture(640, 1250, (ctx, w, h) => {
    paintGraphite(ctx, w, h, 51);
    ctx.fillStyle = '#0b0e0d';
    for (let row = 0; row < 6; row += 1) {
      roundedRectPath(ctx, w * 0.22, h * 0.08 + row * 34, w * 0.56, 12, 6);
      ctx.fill();
    }
    roundedRectPath(ctx, w * 0.2, h * 0.68, w * 0.6, 150, 10);
    ctx.fillStyle = '#c9d3ce';
    ctx.fill();
    ctx.fillStyle = '#18201c';
    ctx.textAlign = 'left';
    setFont(ctx, FONT_DISPLAY, 800, 26, { expanded: true });
    ctx.fillText('DESCARTE AQUI', w * 0.24, h * 0.68 + 42);
    setFont(ctx, FONT_MONO, 500, 17);
    ['Ponto de coleta inteligente', 'Protótipo v0.1', 'Entrada 5 V ⎓ 3 A'].forEach((line, index) => {
      ctx.fillText(line, w * 0.24, h * 0.68 + 76 + index * 24);
    });
  });
}

function createSignTexture() {
  return canvasTexture(1024, 512, (ctx, w, h) => {
    const gradient = ctx.createLinearGradient(0, 0, 0, h);
    gradient.addColorStop(0, '#1f2724');
    gradient.addColorStop(1, '#121715');
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, w, h);
    drawRecycleMark(ctx, 205, 262, 112, '#3fae3a', { battery: true, batteryColor: '#e4efe8', leaf: '#7bd147' });
    ctx.textAlign = 'left';
    ctx.fillStyle = '#eef5f0';
    setFont(ctx, FONT_DISPLAY, 800, 92, { expanded: true });
    ctx.fillText('DESCARTE', 380, 215);
    ctx.fillStyle = '#4cbf45';
    ctx.fillText('AQUI', 380, 318);
    ctx.fillStyle = 'rgba(226, 238, 230, 0.62)';
    setFont(ctx, FONT_DISPLAY, 700, 44, { expanded: true });
    setLetterSpacing(ctx, '10px');
    ctx.fillText('PILHAS', 384, 404);
    setLetterSpacing(ctx, '0px');
    ctx.fillStyle = '#3fae3a';
    ctx.fillRect(0, h - 10, w, 10);
  });
}

function createMountPlateTexture() {
  const texture = canvasTexture(512, 1024, (ctx, w, h) => {
    ctx.fillStyle = '#20262a';
    ctx.fillRect(0, 0, w, h);
    for (let y = 16; y < h; y += 26) {
      for (let x = 16; x < w; x += 26) {
        ctx.fillStyle = '#0c0f10';
        ctx.beginPath();
        ctx.arc(x, y, 5.5, 0, Math.PI * 2);
        ctx.fill();
        ctx.fillStyle = 'rgba(255,255,255,0.06)';
        ctx.beginPath();
        ctx.arc(x + 0.8, y + 1.6, 5.5, 0.2, Math.PI - 0.2);
        ctx.fill();
      }
    }
  });
  return texture;
}

function createEsp32Texture() {
  return canvasTexture(512, 256, (ctx, w, h) => {
    ctx.fillStyle = '#141617';
    ctx.fillRect(0, 0, w, h);
    const random = seededRandom(3);
    ctx.strokeStyle = 'rgba(70, 90, 80, 0.55)';
    ctx.lineWidth = 2;
    for (let i = 0; i < 40; i += 1) {
      const x = random() * w * 0.7 + 30;
      const y = random() * h;
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x + (random() - 0.5) * 120, y);
      ctx.lineTo(x + (random() - 0.5) * 120, y + (random() - 0.5) * 90);
      ctx.stroke();
    }
    ctx.fillStyle = '#d8b45a';
    for (let i = 0; i < 19; i += 1) {
      const x = 40 + i * 21;
      ctx.beginPath(); ctx.arc(x, 14, 5, 0, Math.PI * 2); ctx.fill();
      ctx.beginPath(); ctx.arc(x, h - 14, 5, 0, Math.PI * 2); ctx.fill();
    }
    ctx.strokeStyle = '#d8b45a';
    ctx.lineWidth = 5;
    ctx.beginPath();
    let x = w - 70;
    ctx.moveTo(x, 50);
    for (let i = 0; i < 6; i += 1) {
      ctx.lineTo(x + 44, 50 + i * 26);
      ctx.lineTo(x, 63 + i * 26);
    }
    ctx.stroke();
    ctx.fillStyle = '#e6ebe8';
    setFont(ctx, FONT_MONO, 600, 22);
    ctx.fillText('ESP32-DevKit', 34, h / 2 + 8);
  });
}

function createSmallBoardTexture(color, label) {
  return canvasTexture(256, 160, (ctx, w, h) => {
    ctx.fillStyle = color;
    ctx.fillRect(0, 0, w, h);
    ctx.fillStyle = '#d8b45a';
    for (let i = 0; i < 6; i += 1) {
      ctx.beginPath(); ctx.arc(30 + i * 38, h - 18, 7, 0, Math.PI * 2); ctx.fill();
    }
    ctx.fillStyle = 'rgba(255,255,255,0.85)';
    setFont(ctx, FONT_MONO, 600, 30);
    ctx.textAlign = 'center';
    ctx.fillText(label, w / 2, 52);
  });
}

function createPsuTexture() {
  return canvasTexture(512, 320, (ctx, w, h) => {
    const gradient = ctx.createLinearGradient(0, 0, w, h);
    gradient.addColorStop(0, '#aeb7b4');
    gradient.addColorStop(1, '#7f8986');
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, w, h);
    ctx.fillStyle = '#2b3130';
    for (let y = 26; y < h * 0.62; y += 22) {
      for (let x = 26; x < w - 20; x += 22) {
        ctx.beginPath(); ctx.arc(x, y, 6, 0, Math.PI * 2); ctx.fill();
      }
    }
    ctx.fillStyle = '#1c2120';
    setFont(ctx, FONT_MONO, 600, 30);
    ctx.fillText('DC 5V  3A', 30, h - 46);
    setFont(ctx, FONT_MONO, 500, 20);
    ctx.fillText('SWITCHING POWER SUPPLY', 30, h - 18);
  });
}

function createBinLabelTexture(kind) {
  const texture = canvasTexture(320, 256, (ctx, w) => {
    ctx.clearRect(0, 0, w, 256);
    ctx.fillStyle = '#f2f6f3';
    ctx.textAlign = 'center';
    setFont(ctx, FONT_DISPLAY, 800, kind === 'pilhas' ? 46 : 38, { expanded: true });
    ctx.fillText(kind === 'pilhas' ? 'PILHAS' : 'REJEIÇÃO', w / 2, 66);
    if (kind === 'pilhas') {
      drawBatteryIcon(ctx, w / 2, 160, 52, 92, '#f2f6f3', { lineWidth: 7, level: 0.4 });
    } else {
      drawTrashIcon(ctx, w / 2, 160, 104, '#f2f6f3', 7);
    }
  });
  return texture;
}

function drawOledScreen(ctx, w, h, screenKey, data = {}) {
  const screen = OLED_SCREENS[screenKey] ?? OLED_SCREENS.aguardando;
  const color = COLORS.oled;
  ctx.fillStyle = '#010406';
  ctx.fillRect(0, 0, w, h);
  ctx.textAlign = 'left';
  setFont(ctx, FONT_MONO, 600, 23);
  const titleWidth = ctx.measureText(screen.title).width;
  const startX = (w - titleWidth - 30) / 2;
  drawRecycleMark(ctx, startX + 10, 38, 10, color);
  ctx.fillStyle = color;
  ctx.fillText(screen.title, startX + 30, 46);
  ctx.fillStyle = 'rgba(127, 214, 255, 0.35)';
  ctx.fillRect(24, 64, w - 48, 2);
  const iconY = 120;
  if (screen.icon === 'battery') drawBatteryIcon(ctx, w / 2, iconY, 30, 52, color, { lineWidth: 4, level: 0.35 });
  if (screen.icon === 'batteryFull') drawBatteryIcon(ctx, w / 2, iconY, 30, 52, color, { lineWidth: 4, level: 1 });
  if (screen.icon === 'cube') drawCubeIcon(ctx, w / 2, iconY, 52, color, 4);
  if (screen.icon === 'trash') drawTrashIcon(ctx, w / 2, iconY, 56, color, 4);
  ctx.fillStyle = color;
  ctx.textAlign = 'center';
  setFont(ctx, FONT_MONO, 500, 25);
  screen.lines(data).forEach((line, index) => ctx.fillText(line, w / 2, 192 + index * 32));
  ctx.fillStyle = 'rgba(0, 0, 0, 0.22)';
  for (let y = 0; y < h; y += 4) ctx.fillRect(0, y, w, 1);
}

function standard(color, roughness = 0.5, metalness = 0.1, extra = {}) {
  return new THREE.MeshStandardMaterial({ color, roughness, metalness, ...extra });
}

const wireMaterials = new Map();
function wireMaterial(color) {
  if (!wireMaterials.has(color)) wireMaterials.set(color, standard(color, 0.55, 0.05));
  return wireMaterials.get(color);
}

function addMesh(parent, geometry, material, position = [0, 0, 0], rotation = [0, 0, 0], { castShadow = true, receiveShadow = true } = {}) {
  const mesh = new THREE.Mesh(geometry, material);
  mesh.position.set(...position);
  mesh.rotation.set(...rotation);
  mesh.castShadow = castShadow;
  mesh.receiveShadow = receiveShadow;
  parent.add(mesh);
  return mesh;
}

function addBox(parent, size, material, position, rotation, options) {
  return addMesh(parent, new THREE.BoxGeometry(...size), material, position, rotation, options);
}

function addCylinder(parent, radiusTop, radiusBottom, height, material, position, rotation, segments = 24, options) {
  return addMesh(parent, new THREE.CylinderGeometry(radiusTop, radiusBottom, height, segments), material, position, rotation, options);
}

function componentGroup(key) {
  const group = new THREE.Group();
  group.name = key;
  group.userData.component = key;
  deviceRoot.add(group);
  componentGroups[key] = group;
  return group;
}

function setAnchor(key, x, y, z) {
  anchors[key] = new THREE.Vector3(x, y, z);
}

function registerXray(mesh, minOpacity, { edges = true } = {}) {
  const materials = [...new Set(Array.isArray(mesh.material) ? mesh.material : [mesh.material])];
  materials.forEach((material) => { material.transparent = true; });
  xrayParts.push({ mesh, materials, minOpacity });
  if (edges) {
    const lines = new THREE.LineSegments(
      new THREE.EdgesGeometry(mesh.geometry, 30),
      new THREE.LineBasicMaterial({ color: COLORS.glow, transparent: true, opacity: 0, depthWrite: false })
    );
    lines.raycast = () => {};
    mesh.add(lines);
    ghostEdges.push(lines);
  }
}

function applyXray(amount) {
  for (const part of xrayParts) {
    const opacity = 1 - amount * (1 - part.minOpacity);
    for (const material of part.materials) {
      material.opacity = opacity;
      material.depthWrite = opacity > 0.92;
    }
    part.mesh.castShadow = opacity > 0.6;
  }
  for (const lines of ghostEdges) lines.material.opacity = amount * 0.45;
  parts.interiorLights.forEach((light) => { light.intensity = light.userData.max * amount; });
  parts.binLabels.forEach((material) => { material.opacity = 0.35 + amount * 0.65; });
}

function texturedPanel(size, texture, faceIndex, baseMaterial) {
  const edge = baseMaterial.clone();
  const face = baseMaterial.clone();
  face.map = texture;
  face.color.set(0xffffff);
  const materials = Array.from({ length: 6 }, (_, index) => (index === faceIndex ? face : edge));
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(...size), materials);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  return mesh;
}

function roundedRectShape(width, height, radius) {
  const shape = new THREE.Shape();
  const x = -width / 2;
  const y = -height / 2;
  shape.moveTo(x + radius, y);
  shape.lineTo(x + width - radius, y);
  shape.quadraticCurveTo(x + width, y, x + width, y + radius);
  shape.lineTo(x + width, y + height - radius);
  shape.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
  shape.lineTo(x + radius, y + height);
  shape.quadraticCurveTo(x, y + height, x, y + height - radius);
  shape.lineTo(x, y + radius);
  shape.quadraticCurveTo(x, y, x + radius, y);
  return shape;
}

function rectPath(path, x0, y0, x1, y1) {
  path.moveTo(x0, y0);
  path.lineTo(x1, y0);
  path.lineTo(x1, y1);
  path.lineTo(x0, y1);
  path.closePath();
  return path;
}

function applyPanelUv(geometry, width, height) {
  const position = geometry.attributes.position;
  const uv = new Float32Array(position.count * 2);
  for (let i = 0; i < position.count; i += 1) {
    uv[i * 2] = (position.getX(i) + width / 2) / width;
    uv[i * 2 + 1] = (position.getY(i) + height / 2) / height;
  }
  geometry.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
}

function addLink(parent, from, to, radius, material) {
  const direction = new THREE.Vector3().subVectors(to, from);
  const mesh = addMesh(parent, new THREE.CylinderGeometry(radius, radius, direction.length(), 12), material);
  mesh.position.addVectors(from, to).multiplyScalar(0.5);
  mesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), direction.normalize());
  return mesh;
}

function buildMaterials() {
  return {
    graphite: standard(0x1c2220, 0.5, 0.35),
    graphiteDeep: standard(0x111513, 0.6, 0.3),
    green: standard(0x268a3b, 0.45, 0.12),
    greenPlastic: standard(0x2e9b45, 0.48, 0.02),
    greyPlastic: standard(0x5d6662, 0.55, 0.02),
    aluminium: standard(0xb4bdba, 0.42, 0.8),
    steel: standard(0x8f9895, 0.38, 0.9),
    rubber: standard(0x0a0b0b, 0.92, 0),
    blackPlastic: standard(0x161918, 0.55, 0.05),
    servoBlue: standard(0x2b66cf, 0.32, 0.05),
    pcbBlue: standard(0x1d4ea3, 0.5, 0.1),
    pcbGreen: standard(0x1d6b3a, 0.5, 0.1),
    brass: standard(0xc9a54a, 0.35, 0.9),
    white: standard(0xe8ece9, 0.5, 0),
    acrylic: standard(0xd9f3ff, 0.06, 0, { transparent: true, opacity: 0.2, side: THREE.DoubleSide, depthWrite: false }),
    glow: standard(0x2f9e44, 0.3, 0, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 2.2 }),
    strip: standard(0x1c2220, 0.3, 0, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 0.9 })
  };
}

function buildShell(mat, textures) {
  const { width: W, depth: D, feetTop, plinthTop, top, wall, corner } = BODY;
  const shell = new THREE.Group();
  shell.name = 'shell';
  deviceRoot.add(shell);

  const panelHeight = top - plinthTop;
  const panelY = plinthTop + panelHeight / 2;
  const plinthHeight = plinthTop - feetTop;
  const plinthY = feetTop + plinthHeight / 2;
  const frontWidth = W - corner * 2;
  const sideWidth = D - corner * 2;

  const frontShape = rectPath(new THREE.Shape(), -frontWidth / 2, -panelHeight / 2, frontWidth / 2, panelHeight / 2);
  frontShape.holes.push(rectPath(new THREE.Path(), DOOR.x0, DOOR.y0 - panelY, DOOR.x1, DOOR.y1 - panelY));
  const frontGeometry = new THREE.ExtrudeGeometry(frontShape, { depth: wall, bevelEnabled: false });
  applyPanelUv(frontGeometry, frontWidth, panelHeight);
  frontGeometry.translate(0, 0, -wall / 2);
  const frontFace = mat.graphite.clone();
  frontFace.map = textures.front;
  frontFace.color.set(0xffffff);
  const front = new THREE.Mesh(frontGeometry, [frontFace, mat.graphite.clone()]);
  front.castShadow = true;
  front.receiveShadow = true;
  front.position.set(0, panelY, D / 2 - wall / 2);
  const back = texturedPanel([frontWidth, panelHeight, wall], textures.back, 5, mat.graphite);
  back.position.set(0, panelY, -D / 2 + wall / 2);
  const right = texturedPanel([wall, panelHeight, sideWidth], textures.right, 0, mat.graphite);
  right.position.set(W / 2 - wall / 2, panelY, 0);
  const left = texturedPanel([wall, panelHeight, sideWidth], textures.left, 1, mat.graphite);
  left.position.set(-W / 2 + wall / 2, panelY, 0);
  shell.add(front, back, right, left);
  registerXray(front, 0.05);
  registerXray(right, 0.09);
  registerXray(left, 0.45);

  const plinthPanels = [
    { size: [frontWidth, plinthHeight, wall], position: [0, plinthY, D / 2 - wall / 2], min: 0.08 },
    { size: [frontWidth, plinthHeight, wall], position: [0, plinthY, -D / 2 + wall / 2], min: 1 },
    { size: [wall, plinthHeight, sideWidth], position: [W / 2 - wall / 2, plinthY, 0], min: 0.14 },
    { size: [wall, plinthHeight, sideWidth], position: [-W / 2 + wall / 2, plinthY, 0], min: 0.5 }
  ];
  for (const panel of plinthPanels) {
    const material = mat.green.clone();
    const mesh = addBox(shell, panel.size, material, panel.position);
    if (panel.min < 1) registerXray(mesh, panel.min);
  }

  for (const sx of [-1, 1]) {
    for (const sz of [-1, 1]) {
      const x = sx * (W / 2 - corner);
      const z = sz * (D / 2 - corner);
      const isNearCorner = sz > 0 && sx > 0;
      const post = addCylinder(shell, corner, corner, panelHeight, mat.graphite.clone(), [x, panelY, z], [0, 0, 0], 28);
      const base = addCylinder(shell, corner, corner, plinthHeight, mat.green.clone(), [x, plinthY, z], [0, 0, 0], 28);
      if (isNearCorner || sz > 0) {
        registerXray(post, 0.25, { edges: false });
        registerXray(base, 0.25, { edges: false });
      }
      addCylinder(shell, 0.034, 0.03, feetTop, mat.rubber, [sx * (W / 2 - 0.09), feetTop / 2, sz * (D / 2 - 0.09)], [0, 0, 0], 20);
    }
  }

  addBox(shell, [frontWidth, 0.006, 0.004], mat.strip, [0, plinthTop, D / 2 + 0.001], [0, 0, 0], { castShadow: false });
  addBox(shell, [0.004, 0.006, sideWidth], mat.strip, [W / 2 + 0.001, plinthTop, 0], [0, 0, 0], { castShadow: false });
  addBox(shell, [0.004, 0.006, sideWidth], mat.strip, [-W / 2 - 0.001, plinthTop, 0], [0, 0, 0], { castShadow: false });
  addBox(shell, [W - 0.02, 0.01, D - 0.02], mat.graphiteDeep, [0, feetTop + 0.005, 0]);

  const bevel = 0.008;
  const capShape = roundedRectShape(W - bevel * 2, D - bevel * 2, corner);
  const hole = new THREE.Path();
  hole.absarc(0, -FLOW_Z, HOLE_RADIUS + bevel, 0, Math.PI * 2, true);
  capShape.holes.push(hole);
  const capGeometry = new THREE.ExtrudeGeometry(capShape, { depth: 0.03, bevelEnabled: true, bevelThickness: bevel, bevelSize: bevel, bevelSegments: 3, curveSegments: 48 });
  capGeometry.rotateX(-Math.PI / 2);
  const cap = addMesh(shell, capGeometry, mat.graphite.clone(), [0, top + bevel, 0]);
  registerXray(cap, 0.22);

  const sign = new THREE.Group();
  sign.position.set(0, TOP_SURFACE + 0.185, -D / 2 + 0.06);
  shell.add(sign);
  addMesh(sign, new RoundedBoxGeometry(0.66, 0.35, 0.04, 4, 0.016), mat.graphite);
  addMesh(sign, new THREE.PlaneGeometry(0.62, 0.31), new THREE.MeshStandardMaterial({ map: textures.sign, color: 0xd0d6d2, roughness: 0.6, metalness: 0.1, envMapIntensity: 0.35 }), [0, 0, 0.0205], [0, 0, 0], { castShadow: false });
  addBox(sign, [0.6, 0.01, 0.06], mat.graphiteDeep, [0, -0.18, 0]);

  return { front, frontFace, panelY, frontWidth, panelHeight };
}

function buildEntry(mat) {
  const group = componentGroup('entrada');
  const ringMaterial = mat.glow.clone();
  addMesh(group, new THREE.TorusGeometry(HOLE_RADIUS + 0.004, 0.009, 16, 96), ringMaterial, [0, TOP_SURFACE, FLOW_Z], [Math.PI / 2, 0, 0], { castShadow: false });
  const halo = new THREE.MeshBasicMaterial({ color: COLORS.glow, transparent: true, opacity: 0.18, depthWrite: false, side: THREE.DoubleSide });
  addMesh(group, new THREE.RingGeometry(HOLE_RADIUS + 0.013, HOLE_RADIUS + 0.045, 96), halo, [0, TOP_SURFACE + 0.001, FLOW_Z], [-Math.PI / 2, 0, 0], { castShadow: false, receiveShadow: false });
  const sleeveMaterial = standard(0x123a20, 0.4, 0.1, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 0.35, side: THREE.DoubleSide });
  addMesh(group, new THREE.CylinderGeometry(HOLE_RADIUS, HOLE_RADIUS, 0.05, 64, 1, true), sleeveMaterial, [0, TOP_SURFACE - 0.025, FLOW_Z], [0, 0, 0], { castShadow: false });

  const funnelProfile = [
    new THREE.Vector2(HOLE_RADIUS, 0),
    new THREE.Vector2(HOLE_RADIUS - 0.004, -0.018),
    new THREE.Vector2(0.066, -0.13),
    new THREE.Vector2(0.06, -0.15)
  ];
  addMesh(group, new THREE.LatheGeometry(funnelProfile, 64), standard(0x2a312e, 0.35, 0.7, { side: THREE.DoubleSide }), [0, BODY.top, FLOW_Z]);
  addMesh(group, new THREE.CylinderGeometry(0.055, 0.055, 0.28, 40, 1, true), mat.acrylic, [0, 1.61, FLOW_Z], [0, 0, 0], { castShadow: false });
  for (const y of [1.735, 1.49]) {
    addMesh(group, new THREE.TorusGeometry(0.057, 0.0045, 10, 48), mat.aluminium, [0, y, FLOW_Z], [Math.PI / 2, 0, 0]);
  }
  const glowLight = new THREE.PointLight(COLORS.glow, 0.25, 0.7, 2);
  glowLight.position.set(0, TOP_SURFACE - 0.05, FLOW_Z);
  deviceRoot.add(glowLight);

  setAnchor('entrada', HOLE_RADIUS + 0.03, TOP_SURFACE + 0.005, FLOW_Z);
  return { ringMaterial, halo, sleeveMaterial, glowLight };
}

function buildFrame(mat, textures) {
  const { width: W, depth: D, wall } = BODY;
  const frame = new THREE.Group();
  frame.name = 'frame';
  deviceRoot.add(frame);
  const plate = new THREE.MeshStandardMaterial({ map: textures.mountPlate, roughness: 0.6, metalness: 0.5 });
  addMesh(frame, new THREE.PlaneGeometry(W - 0.1, 1.6), plate, [0, 1.08, -D / 2 + wall + 0.003], [0, 0, 0], { castShadow: false });
  addBox(frame, [W - 2 * wall - 0.004, 0.008, D - 0.05], mat.graphiteDeep, [0, 0.064, 0]);
  for (const y of [0.764, 0.44]) {
    addBox(frame, [W - 2 * wall - 0.004, 0.012, 0.52], standard(0x2a312e, 0.5, 0.55), [0, y, 0.01]);
    for (const sx of [-1, 1]) addBox(frame, [0.014, 0.014, 0.6], mat.aluminium, [sx * (W / 2 - wall - 0.009), y - 0.013, 0.0]);
  }
  addBox(frame, [0.024, 1.1, 0.024], mat.aluminium, [0, 1.32, -0.21]);
  addBox(frame, [0.012, 0.012, 0.17], mat.aluminium, [0.035, SENSOR1_Y, -0.12]);
  addBox(frame, [0.15, 0.012, 0.012], mat.aluminium, [0.075, SENSOR2_Y, -0.21]);
  addBox(frame, [0.004, 0.05, 0.28], mat.aluminium, [0.15, SENSOR2_Y, -0.07]);
  addBox(frame, [0.012, 0.012, 0.14], mat.aluminium, [0, 1.735, -0.135]);
}

function buildSensors(mat) {
  const sensor1 = componentGroup('sensor1');
  addMesh(sensor1, new THREE.CylinderGeometry(0.063, 0.063, 0.022, 40, 1, true), standard(0x111414, 0.5, 0.2, { side: THREE.DoubleSide }), [0, SENSOR1_Y, FLOW_Z]);
  const emitterLed = standard(0x331010, 0.3, 0, { emissive: new THREE.Color(0xff3b3b), emissiveIntensity: 0.3 });
  for (const sx of [-1, 1]) {
    addBox(sensor1, [0.024, 0.032, 0.032], mat.blackPlastic, [sx * 0.077, SENSOR1_Y, FLOW_Z]);
    addMesh(sensor1, new THREE.SphereGeometry(0.008, 16, 12), sx < 0 ? emitterLed : standard(0x0e1a24, 0.15, 0.2), [sx * 0.064, SENSOR1_Y, FLOW_Z]);
  }
  const boardTexture = createSmallBoardTexture('#1d4ea3', 'HC-SR04');
  const board = new THREE.MeshStandardMaterial({ map: boardTexture, roughness: 0.5 });
  addBox(sensor1, [0.085, 0.045, 0.004], [mat.pcbBlue, mat.pcbBlue, mat.pcbBlue, mat.pcbBlue, board, mat.pcbBlue], [0.035, SENSOR1_Y, -0.032]);
  const beamMaterial = new THREE.MeshBasicMaterial({ color: 0xff4a4a, transparent: true, opacity: 0, blending: THREE.AdditiveBlending, depthWrite: false });
  addMesh(sensor1, new THREE.CylinderGeometry(0.0025, 0.0025, 0.13, 8), beamMaterial, [0, SENSOR1_Y, FLOW_Z], [0, 0, Math.PI / 2], { castShadow: false });
  setAnchor('sensor1', 0.095, SENSOR1_Y + 0.01, FLOW_Z);

  const sensor2 = componentGroup('sensor2');
  addMesh(sensor2, new THREE.CylinderGeometry(0.019, 0.019, 0.14, 32), mat.steel, [0.145, SENSOR2_Y, FLOW_Z], [0, 0, Math.PI / 2]);
  for (let x = 0.085; x <= 0.206; x += 0.011) {
    addMesh(sensor2, new THREE.TorusGeometry(0.019, 0.0013, 6, 32), mat.steel, [x, SENSOR2_Y, FLOW_Z], [0, Math.PI / 2, 0], { castShadow: false });
  }
  for (const x of [0.141, 0.159]) {
    addMesh(sensor2, new THREE.CylinderGeometry(0.027, 0.027, 0.008, 6), mat.steel, [x, SENSOR2_Y, FLOW_Z], [0, 0, Math.PI / 2]);
  }
  addMesh(sensor2, new THREE.CylinderGeometry(0.019, 0.019, 0.008, 32), standard(0xd27a1f, 0.5, 0), [0.071, SENSOR2_Y, FLOW_Z], [0, 0, Math.PI / 2]);
  addMesh(sensor2, new THREE.CylinderGeometry(0.015, 0.015, 0.022, 24), mat.blackPlastic, [0.226, SENSOR2_Y, FLOW_Z], [0, 0, Math.PI / 2]);
  const sensor2Led = standard(0x10200f, 0.3, 0, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 0.25 });
  addMesh(sensor2, new THREE.SphereGeometry(0.005, 12, 8), sensor2Led, [0.228, SENSOR2_Y + 0.015, FLOW_Z]);
  const scanMaterial = new THREE.MeshBasicMaterial({ color: 0x6fd3ff, transparent: true, opacity: 0, blending: THREE.AdditiveBlending, depthWrite: false, side: THREE.DoubleSide });
  const scanField = addMesh(sensor2, new THREE.CylinderGeometry(0.03, 0.017, 0.05, 32, 1, true), scanMaterial, [0.042, SENSOR2_Y, FLOW_Z], [0, 0, Math.PI / 2], { castShadow: false });
  setAnchor('sensor2', 0.24, SENSOR2_Y, FLOW_Z);

  return { beamMaterial, emitterLed, sensor2Led, scanMaterial, scanField };
}

function buildServo(mat) {
  const group = componentGroup('servo');
  const { pivotY, halfLength } = TRAY;
  addBox(group, [0.09, 0.045, 0.05], mat.servoBlue, [0.022, pivotY, -0.11]);
  addBox(group, [0.13, 0.045, 0.005], mat.servoBlue, [0.022, pivotY, -0.093]);
  addMesh(group, new THREE.CylinderGeometry(0.012, 0.012, 0.012, 20), mat.servoBlue, [0, pivotY, -0.08], [Math.PI / 2, 0, 0]);
  addBox(group, [0.13, 0.004, 0.08], mat.aluminium, [0.022, pivotY - 0.0245, -0.165]);
  addBox(group, [0.03, 0.05, 0.004], mat.aluminium, [0, pivotY - 0.0, -0.198]);

  const tray = new THREE.Group();
  tray.position.set(0, pivotY, FLOW_Z);
  group.add(tray);
  addBox(tray, [halfLength * 2, 0.008, 0.13], mat.aluminium, [0, 0.002, 0]);
  for (const sz of [-1, 1]) addBox(tray, [halfLength * 2, 0.018, 0.004], mat.aluminium, [0, 0.01, sz * 0.064]);
  addBox(tray, [0.016, 0.012, 0.045], mat.aluminium, [0, -0.008, -0.0875]);
  addBox(tray, [0.07, 0.008, 0.004], mat.white, [0, 0, -0.114]);
  addMesh(tray, new THREE.CylinderGeometry(0.008, 0.008, 0.008, 16), mat.white, [0, 0, -0.112], [Math.PI / 2, 0, 0]);

  setAnchor('servo', -0.03, pivotY, -0.11);
  return { tray };
}

function buildChutes(mat) {
  const chuteParts = {};
  for (const side of [-1, 1]) {
    const key = side < 0 ? 'pilhas' : 'rejeicao';
    const group = componentGroup(key);
    const dx = CHUTE.endX - CHUTE.startX;
    const dy = CHUTE.startY - CHUTE.endY;
    const length = Math.hypot(dx, dy);
    const chute = new THREE.Group();
    chute.position.set(side * (CHUTE.startX + CHUTE.endX) / 2, (CHUTE.startY + CHUTE.endY) / 2, FLOW_Z);
    chute.rotation.z = -side * Math.atan2(dy, dx);
    group.add(chute);
    const chuteMaterial = side < 0 ? mat.greenPlastic : mat.aluminium;
    addBox(chute, [length, 0.008, 0.16], chuteMaterial, [0, 0, 0]);
    for (const sz of [-1, 1]) addBox(chute, [length, 0.03, 0.005], chuteMaterial, [0, 0.015, sz * 0.08]);
    chuteParts[key] = group;
  }
  return chuteParts;
}

function buildBin(mat, kind) {
  const group = componentGroups[kind];
  const isBattery = kind === 'pilhas';
  const height = isBattery ? 0.256 : 0.296;
  const bottom = isBattery ? 0.822 : 0.776;
  const x = isBattery ? -0.205 : 0.205;
  const material = (isBattery ? mat.greenPlastic : mat.greyPlastic).clone();
  material.side = THREE.DoubleSide;
  material.transparent = true;
  material.opacity = 0.82;
  material.depthWrite = false;
  const rTop = 0.2;
  const rBottom = 0.175;
  const wallGeometry = new THREE.CylinderGeometry(rTop, rBottom, height, 4, 1, true);
  wallGeometry.rotateY(Math.PI / 4);
  const bin = addMesh(group, wallGeometry, material, [x, bottom + height / 2, 0.02]);
  bin.scale.z = 1.3;
  const rimGeometry = new THREE.CylinderGeometry(rTop + 0.006, rTop + 0.002, 0.014, 4, 1, true);
  rimGeometry.rotateY(Math.PI / 4);
  const rim = addMesh(group, rimGeometry, material, [x, bottom + height - 0.007, 0.02]);
  rim.scale.z = 1.3;
  addBox(group, [rBottom * Math.SQRT2, 0.006, rBottom * Math.SQRT2 * 1.3], material, [x, bottom + 0.003, 0.02]);

  const frontTop = (rTop / Math.SQRT2) * 1.3;
  const frontBottom = (rBottom / Math.SQRT2) * 1.3;
  const labelMaterial = new THREE.MeshStandardMaterial({ map: createBinLabelTexture(kind), transparent: true, roughness: 0.6, depthWrite: false });
  addMesh(group, new THREE.PlaneGeometry(0.19, 0.152), labelMaterial, [x, bottom + height * 0.55, 0.02 + (frontTop + frontBottom) / 2 + 0.004], [Math.atan((frontTop - frontBottom) / height), 0, 0], { castShadow: false });
  if (!isBattery) {
    for (const sx of [-1, 1]) for (const sz of [-1, 1]) {
      addCylinder(group, 0.008, 0.008, 0.006, mat.rubber, [x + sx * 0.09, 0.773, 0.02 + sz * 0.12], [0, 0, 0], 12);
    }
  }
  setAnchor(kind, x + (isBattery ? -0.13 : 0.13), bottom + height * 0.6, 0.2);
  return labelMaterial;
}

function buildRejectCompartment(mat) {
  const group = componentGroups.rejeicao;
  const material = mat.greyPlastic.clone();
  material.side = THREE.DoubleSide;
  material.transparent = true;
  material.opacity = 0.82;
  material.depthWrite = false;
  const height = REJECT.top - REJECT.bottom;
  const depth = REJECT.front - REJECT.back;
  const centerY = REJECT.bottom + height / 2;
  const centerZ = (REJECT.front + REJECT.back) / 2;
  const centerX = (REJECT.x0 + REJECT.x1) / 2;
  const width = REJECT.x1 - REJECT.x0;
  for (const x of [REJECT.x0, REJECT.x1]) addBox(group, [0.006, height, depth], material, [x, centerY, centerZ]);
  addBox(group, [width, height, 0.006], material, [centerX, centerY, REJECT.back]);
  addBox(group, [width + 0.006, 0.006, depth], material, [centerX, REJECT.bottom + 0.003, centerZ]);
  const run = REJECT.front - REJECT.floorBackZ;
  const drop = REJECT.floorBackY - REJECT.floorFrontY;
  addBox(group, [width - 0.006, 0.006, Math.hypot(run, drop)], material, [centerX, (REJECT.floorBackY + REJECT.floorFrontY) / 2, (REJECT.front + REJECT.floorBackZ) / 2], [Math.atan2(drop, run), 0, 0]);
  for (const z of [-0.05, 0.12]) addBox(group, [width - 0.012, rejectFloorY(z) - REJECT.bottom - 0.006, 0.006], material, [centerX, (rejectFloorY(z) + REJECT.bottom) / 2, z]);

  const labelMaterial = new THREE.MeshStandardMaterial({ map: createBinLabelTexture('rejeicao'), transparent: true, roughness: 0.6, depthWrite: false });
  addMesh(group, new THREE.PlaneGeometry(0.19, 0.152), labelMaterial, [centerX, 0.985, REJECT.back + 0.005], [0, 0, 0], { castShadow: false });
  setAnchor('rejeicao', REJECT.x1 + 0.005, 1.05, 0.05);
  return labelMaterial;
}

function buildRejectDoor(mat, shellInfo) {
  const group = componentGroup('portinha');
  const { panelY, frontWidth, panelHeight, frontFace } = shellInfo;
  const innerFaceZ = BODY.depth / 2 - BODY.wall;
  const doorWidth = DOOR.x1 - DOOR.x0;
  const doorCenterX = (DOOR.x0 + DOOR.x1) / 2;

  const pivot = new THREE.Group();
  pivot.position.set(0, DOOR.y1, DOOR.hingeZ);
  group.add(pivot);
  const doorShape = rectPath(new THREE.Shape(), DOOR.x0 + 0.003, DOOR.y0 + 0.003 - panelY, DOOR.x1 - 0.003, DOOR.y1 - 0.005 - panelY);
  const doorGeometry = new THREE.ExtrudeGeometry(doorShape, { depth: BODY.wall, bevelEnabled: false });
  applyPanelUv(doorGeometry, frontWidth, panelHeight);
  doorGeometry.translate(0, panelY - DOOR.y1, innerFaceZ - DOOR.hingeZ);
  const door = new THREE.Mesh(doorGeometry, [frontFace.clone(), mat.graphite.clone()]);
  door.castShadow = true;
  door.receiveShadow = true;
  pivot.add(door);
  registerXray(door, 0.6);

  addBox(pivot, [doorWidth - 0.012, 0.014, 0.002], mat.steel, [doorCenterX, -0.009, -0.003]);
  addMesh(pivot, new THREE.CylinderGeometry(0.0045, 0.0045, doorWidth - 0.006, 16), mat.steel, [doorCenterX, 0, 0], [0, 0, Math.PI / 2]);
  for (let i = 1; i < 5; i += 1) {
    addMesh(pivot, new THREE.CylinderGeometry(0.0049, 0.0049, 0.002, 16), mat.blackPlastic, [DOOR.x0 + (doorWidth * i) / 5, 0, 0], [0, 0, Math.PI / 2], { castShadow: false });
  }
  addBox(group, [doorWidth - 0.012, 0.014, 0.002], mat.steel, [doorCenterX, DOOR.y1 + 0.009, DOOR.hingeZ - 0.003]);
  const tabTip = DOOR_ARM.clone().add(new THREE.Vector3(DOOR_LINK.x, 0, 0));
  addLink(pivot, new THREE.Vector3(DOOR_LINK.x, -0.012, -0.004), tabTip, 0.004, mat.aluminium);
  addMesh(pivot, new THREE.SphereGeometry(0.005, 12, 10), mat.steel, tabTip.toArray());

  const ledMaterial = standard(0x0b300b, 0.3, 0, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 1.6 });
  addBox(group, [0.03, 0.004, 0.002], ledMaterial, [doorCenterX, DOOR.y1 + 0.026, BODY.depth / 2 + 0.001], [0, 0, 0], { castShadow: false });

  const servoX = DOOR_LINK.x + 0.0335;
  addBox(group, [0.045, 0.09, 0.045], mat.servoBlue, [servoX, DOOR_LINK.shaftY + 0.025, DOOR_LINK.shaftZ]);
  addBox(group, [0.005, 0.125, 0.045], mat.servoBlue, [servoX - 0.012, DOOR_LINK.shaftY + 0.025, DOOR_LINK.shaftZ]);
  addMesh(group, new THREE.CylinderGeometry(0.0065, 0.0065, 0.01, 16), mat.white, [DOOR_LINK.x + 0.006, DOOR_LINK.shaftY, DOOR_LINK.shaftZ], [0, 0, Math.PI / 2]);
  addBox(group, [0.05, 0.03, innerFaceZ - (DOOR_LINK.shaftZ + 0.0225)], mat.aluminium, [servoX, DOOR_LINK.shaftY + 0.055, (innerFaceZ + DOOR_LINK.shaftZ + 0.0225) / 2]);
  addBox(group, [0.06, 0.11, 0.003], mat.aluminium, [servoX, DOOR_LINK.shaftY + 0.025, innerFaceZ - 0.0015]);

  const horn = new THREE.Group();
  horn.position.copy(DOOR_SHAFT);
  group.add(horn);
  addLink(horn, new THREE.Vector3(0, 0, 0), DOOR_ARM, 0.004, mat.white);
  addMesh(horn, new THREE.SphereGeometry(0.005, 12, 10), mat.steel, DOOR_ARM.toArray());

  const rod = addLink(group, DOOR_HINGE.clone().add(DOOR_ARM), DOOR_SHAFT.clone().add(DOOR_ARM), 0.0025, mat.steel);
  setAnchor('portinha', DOOR.x1 + 0.01, DOOR.y0 + 0.01, BODY.depth / 2 + 0.01);
  return { pivot, horn, rod, ledMaterial };
}

function buildUltrasonic(mat) {
  const group = componentGroup('hcsr04');
  const y = 0.865;
  const z = 0.25;
  const wallX = REJECT.x1;
  const boardTexture = createSmallBoardTexture('#1d4ea3', 'HC-SR04');
  const boardFace = new THREE.MeshStandardMaterial({ map: boardTexture, roughness: 0.5 });
  addBox(group, [0.003, 0.036, 0.074], [boardFace, mat.pcbBlue, mat.pcbBlue, mat.pcbBlue, mat.pcbBlue, mat.pcbBlue], [wallX + 0.021, y, z]);
  for (const dz of [-0.019, 0.019]) {
    addMesh(group, new THREE.CylinderGeometry(0.0125, 0.0125, 0.02, 24), mat.steel, [wallX + 0.009, y, z + dz], [0, 0, Math.PI / 2]);
    addMesh(group, new THREE.CylinderGeometry(0.0105, 0.0105, 0.002, 24), standard(0x1a1d1c, 0.8, 0), [wallX - 0.0015, y, z + dz], [0, 0, Math.PI / 2], { castShadow: false });
  }
  addBox(group, [0.004, 0.006, 0.01], mat.steel, [wallX + 0.018, y + 0.012, z]);
  addBox(group, [0.008, 0.008, 0.026], mat.blackPlastic, [wallX + 0.021, y - 0.022, z]);
  addBox(group, [0.026, 0.004, 0.08], mat.aluminium, [wallX + 0.014, y - 0.03, z]);
  addBox(group, [0.003, 0.03, 0.08], mat.aluminium, [wallX + 0.0045, y - 0.017, z]);

  const coneMaterial = new THREE.MeshBasicMaterial({ color: 0x6fd3ff, transparent: true, opacity: 0, blending: THREE.AdditiveBlending, depthWrite: false, side: THREE.DoubleSide });
  const length = REJECT.x1 - REJECT.x0 - 0.02;
  addMesh(group, new THREE.CylinderGeometry(0.055, 0.016, length, 32, 1, true), coneMaterial, [wallX - 0.003 - length / 2, y - 0.01, z + 0.01], [0, 0, Math.PI / 2], { castShadow: false, receiveShadow: false }).raycast = () => {};
  setAnchor('hcsr04', wallX + 0.04, y + 0.03, z);
  return { coneMaterial };
}

function buildLoadCell(mat) {
  const group = componentGroup('celula');
  const cellMaterial = standard(0xc6cdca, 0.3, 0.85, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 0 });
  addBox(group, [0.04, 0.012, 0.03], mat.aluminium, [-0.29, 0.776, 0.02]);
  addBox(group, [0.22, 0.026, 0.028], cellMaterial, [-0.205, 0.795, 0.02]);
  addBox(group, [0.03, 0.002, 0.02], mat.white, [-0.205, 0.809, 0.02], [0, 0, 0], { castShadow: false });
  for (const x of [-0.23, -0.18]) addMesh(group, new THREE.CylinderGeometry(0.008, 0.008, 0.03, 16), standard(0x0c0f0e, 0.7, 0), [x, 0.795, 0.02], [Math.PI / 2, 0, 0], { castShadow: false });
  addBox(group, [0.04, 0.008, 0.03], mat.aluminium, [-0.12, 0.812, 0.02]);
  addBox(group, [0.27, 0.006, 0.36], standard(0x3a423f, 0.5, 0.6), [-0.205, 0.819, 0.02]);

  const hxTexture = createSmallBoardTexture('#1d6b3a', 'HX711');
  const hxTop = new THREE.MeshStandardMaterial({ map: hxTexture, roughness: 0.5 });
  addBox(group, [0.06, 0.005, 0.038], [mat.pcbGreen, mat.pcbGreen, hxTop, mat.pcbGreen, mat.pcbGreen, mat.pcbGreen], [0, 0.776, 0.235]);
  addBox(group, [0.016, 0.004, 0.012], mat.blackPlastic, [0.008, 0.7805, 0.232]);
  setAnchor('celula', -0.33, 0.8, 0.12);
  return { cellMaterial };
}

function buildElectronics(mat, textures) {
  const esp = componentGroup('esp32');
  const top = new THREE.MeshStandardMaterial({ map: textures.esp32, roughness: 0.45, metalness: 0.1 });
  const pcb = standard(0x141617, 0.5, 0.1);
  addBox(esp, [0.2, 0.012, 0.1], [pcb, pcb, top, pcb, pcb, pcb], [0.1, 0.46, 0]);
  addBox(esp, [0.07, 0.012, 0.058], mat.steel, [0.12, 0.472, 0]);
  addBox(esp, [0.02, 0.01, 0.026], mat.steel, [-0.002, 0.468, 0]);
  for (const sz of [-1, 1]) addBox(esp, [0.16, 0.008, 0.008], mat.blackPlastic, [0.09, 0.47, sz * 0.043]);
  for (const sx of [0.01, 0.19]) for (const sz of [-0.04, 0.04]) addCylinder(esp, 0.004, 0.004, 0.008, mat.brass, [sx, 0.45, sz], [0, 0, 0], 10);
  const statusLed = standard(0x0b1530, 0.3, 0, { emissive: new THREE.Color(0x3f8cff), emissiveIntensity: 0.4 });
  addBox(esp, [0.006, 0.003, 0.004], statusLed, [0.03, 0.4675, 0.028], [0, 0, 0], { castShadow: false });
  addBox(esp, [0.006, 0.003, 0.004], standard(0x300b0b, 0.3, 0, { emissive: new THREE.Color(0xff3030), emissiveIntensity: 1.2 }), [0.03, 0.4675, -0.028], [0, 0, 0], { castShadow: false });
  setAnchor('esp32', 0.205, 0.475, 0);

  const buck = new THREE.MeshStandardMaterial({ map: createSmallBoardTexture('#1d4ea3', '3V3'), roughness: 0.5 });
  addBox(esp, [0.06, 0.004, 0.036], [mat.pcbBlue, mat.pcbBlue, buck, mat.pcbBlue, mat.pcbBlue, mat.pcbBlue], [-0.13, 0.452, 0]);
  addCylinder(esp, 0.009, 0.009, 0.008, mat.steel, [-0.14, 0.458, 0.004], [0, 0, 0], 16);

  const psuGroup = componentGroup('fonte');
  const psuTop = new THREE.MeshStandardMaterial({ map: textures.psu, roughness: 0.4, metalness: 0.75 });
  const psuSide = standard(0x9aa3a0, 0.4, 0.8);
  addBox(psuGroup, [0.22, 0.07, 0.13], [psuSide, psuSide, psuTop, psuSide, psuTop, psuSide], [-0.17, 0.103, -0.06]);
  addBox(psuGroup, [0.016, 0.03, 0.11], mat.blackPlastic, [-0.052, 0.09, -0.06]);
  for (let i = 0; i < 5; i += 1) addCylinder(psuGroup, 0.004, 0.004, 0.004, mat.steel, [-0.052, 0.106, -0.1 + i * 0.02], [0, 0, 0], 8);
  addMesh(psuGroup, new THREE.SphereGeometry(0.004, 10, 8), standard(0x0b300b, 0.3, 0, { emissive: new THREE.Color(COLORS.glow), emissiveIntensity: 1.5 }), [-0.09, 0.139, 0.0]);
  setAnchor('fonte', -0.29, 0.11, -0.04);
  return { statusLed };
}

function buildOled(mat, oledTexture) {
  const group = componentGroup('oled');
  const z = BODY.depth / 2;
  addMesh(group, new RoundedBoxGeometry(0.3, 0.18, 0.024, 4, 0.01), standard(0x0b0e0d, 0.35, 0.5), [0, 1.58, z + 0.003]);
  const screenMaterial = new THREE.MeshBasicMaterial({ map: oledTexture, toneMapped: false });
  screenMaterial.color.setScalar(1.25);
  addMesh(group, new THREE.PlaneGeometry(0.25, 0.125), screenMaterial, [0, 1.58, z + 0.0155], [0, 0, 0], { castShadow: false, receiveShadow: false });
  addBox(group, [0.24, 0.15, 0.004], mat.pcbBlue, [0, 1.58, z - BODY.wall - 0.004]);
  setAnchor('oled', -0.16, 1.6, z + 0.02);
}

function wireBundle(points, colors, radius = 0.0032) {
  const base = points.map((point) => new THREE.Vector3(...point));
  colors.forEach((color, index) => {
    const offset = (index - (colors.length - 1) / 2) * radius * 2.3;
    const shift = new THREE.Vector3(offset, 0, offset * 0.6);
    const curve = new THREE.CatmullRomCurve3(base.map((point) => point.clone().add(shift)));
    const mesh = new THREE.Mesh(new THREE.TubeGeometry(curve, 90, radius, 6, false), wireMaterial(color));
    mesh.castShadow = true;
    deviceRoot.add(mesh);
  });
}

function buildWiring() {
  const z = BODY.depth / 2;
  wireBundle([[0.04, 0.474, -0.043], [0.035, 0.52, -0.2], [0.035, 0.6, -0.29], [0.04, 1.0, -0.295], [0.045, 1.5, -0.29], [0.04, 1.6, -0.15], [0.035, 1.635, -0.036]], [0xd33b3b, 0x1b1b1b, 0xe8c547]);
  wireBundle([[0.065, 0.474, -0.043], [0.065, 0.52, -0.2], [0.07, 0.6, -0.29], [0.075, 1.25, -0.295], [0.08, 1.36, -0.2], [0.07, 1.4, -0.136]], [0x7a4a2a, 0xd33b3b, 0xf08c2a]);
  wireBundle([[0.09, 0.474, -0.043], [0.1, 0.52, -0.2], [0.12, 0.6, -0.29], [0.24, 1.2, -0.295], [0.27, 1.42, -0.1], [0.26, 1.435, 0.04], [0.237, 1.435, 0.04]], [0x2d6fd8, 0x1b1b1b, 0x7a4a2a]);
  wireBundle([[0.05, 0.474, 0.043], [0.0, 0.5, -0.15], [-0.3, 0.55, -0.29], [-0.395, 1.0, -0.29], [-0.395, 1.5, 0.1], [-0.25, 1.58, z - 0.06], [-0.08, 1.58, z - BODY.wall - 0.008]], [0xd33b3b, 0x1b1b1b, 0x2d6fd8, 0x3fae3a]);
  wireBundle([[-0.095, 0.797, 0.03], [-0.07, 0.79, 0.13], [-0.03, 0.782, 0.228]], [0xd33b3b, 0x1b1b1b, 0xf2f2f2, 0x3fae3a], 0.0024);
  wireBundle([[0.02, 0.782, 0.24], [0.045, 0.775, 0.29], [0.06, 0.62, 0.3], [0.09, 0.5, 0.2], [0.11, 0.474, 0.043]], [0xd33b3b, 0x1b1b1b, 0xe8c547, 0x3fae3a], 0.0026);
  wireBundle([[-0.09, 0.12, -0.1], [-0.05, 0.2, -0.22], [-0.07, 0.35, -0.29], [-0.1, 0.47, -0.2], [-0.13, 0.458, -0.02]], [0xd33b3b, 0x1b1b1b], 0.0036);
  wireBundle([[-0.1, 0.458, 0.0], [-0.05, 0.47, 0.01], [-0.004, 0.47, 0.0]], [0xd33b3b, 0x1b1b1b], 0.003);
  wireBundle([[-0.27, 0.11, -0.08], [-0.3, 0.1, -0.25], [-0.25, 0.12, -0.37]], [0x111111], 0.006);
  wireBundle([[REJECT.x1 + 0.021, 0.835, 0.25], [0.37, 0.8, 0.3], [0.38, 0.7, 0.31], [0.25, 0.56, 0.26], [0.13, 0.474, 0.043]], [0xd33b3b, 0xe8c547, 0x3fae3a, 0x1b1b1b], 0.0026);
  wireBundle([[0.29, 1.1, 0.3], [0.36, 1.11, 0.31], [0.395, 1.0, 0.32], [0.395, 0.72, 0.318], [0.27, 0.56, 0.28], [0.15, 0.474, 0.043]], [0x7a4a2a, 0xd33b3b, 0xf08c2a]);
}

function buildStage() {
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(40, 40), standard(0x070a09, 0.9, 0.05));
  floor.rotation.x = -Math.PI / 2;
  floor.receiveShadow = true;
  scene.add(floor);
  const pedestal = new THREE.Mesh(new THREE.CylinderGeometry(1.0, 1.03, PEDESTAL_HEIGHT, 96), standard(0x141a17, 0.55, 0.5));
  pedestal.position.y = PEDESTAL_HEIGHT / 2;
  pedestal.receiveShadow = true;
  scene.add(pedestal);
  const edge = new THREE.Mesh(new THREE.TorusGeometry(1.0, 0.003, 8, 160), new THREE.MeshBasicMaterial({ color: COLORS.glow, transparent: true, opacity: 0.55 }));
  edge.rotation.x = Math.PI / 2;
  edge.position.y = PEDESTAL_HEIGHT + 0.001;
  scene.add(edge);
  const shadowTexture = canvasTexture(256, 256, (ctx, w, h) => {
    const gradient = ctx.createRadialGradient(w / 2, h / 2, 0, w / 2, h / 2, w / 2);
    gradient.addColorStop(0, 'rgba(0,0,0,0.7)');
    gradient.addColorStop(0.55, 'rgba(0,0,0,0.35)');
    gradient.addColorStop(1, 'rgba(0,0,0,0)');
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, w, h);
  });
  const contact = new THREE.Mesh(new THREE.PlaneGeometry(1.5, 1.3), new THREE.MeshBasicMaterial({ map: shadowTexture, transparent: true, depthWrite: false }));
  contact.rotation.x = -Math.PI / 2;
  contact.position.y = PEDESTAL_HEIGHT + 0.002;
  scene.add(contact);
}

function buildLights() {
  scene.add(new THREE.HemisphereLight(0xe2efe7, 0x0b0f0d, 0.55));
  const key = new THREE.DirectionalLight(0xfff8f0, 2.4);
  key.position.set(3, 5.2, 4);
  key.castShadow = true;
  key.shadow.mapSize.set(2048, 2048);
  key.shadow.camera.left = -1.6;
  key.shadow.camera.right = 1.6;
  key.shadow.camera.top = 2.6;
  key.shadow.camera.bottom = -0.6;
  key.shadow.camera.near = 1;
  key.shadow.camera.far = 14;
  key.shadow.bias = -0.0004;
  key.shadow.normalBias = 0.02;
  scene.add(key);
  const rim = new THREE.DirectionalLight(0x86ffab, 1.3);
  rim.position.set(-3.5, 2.6, -3);
  scene.add(rim);
  const fill = new THREE.DirectionalLight(0xa9c9ff, 0.55);
  fill.position.set(-3, 1.6, 2.5);
  scene.add(fill);

  const interiorLights = [];
  for (const [x, y, z, max] of [[-0.25, 1.55, 0.25, 0.12], [0.05, 0.62, 0.22, 0.14], [-0.1, 0.3, 0.2, 0.08]]) {
    const light = new THREE.PointLight(0xeafff1, 0, 1.2, 2);
    light.position.set(x, y, z);
    light.userData.max = max;
    deviceRoot.add(light);
    interiorLights.push(light);
  }
  return interiorLights;
}

function createBattery() {
  const group = new THREE.Group();
  const body = standard(0x1f6fd8, 0.3, 0.45);
  const metal = standard(0xd5dbd8, 0.25, 0.95);
  addCylinder(group, 0.016, 0.016, 0.064, body, [0, 0, 0], [0, 0, 0], 28);
  addCylinder(group, 0.0162, 0.0162, 0.012, standard(0x111a2a, 0.35, 0.4), [0, 0.022, 0], [0, 0, 0], 28);
  addCylinder(group, 0.0155, 0.0155, 0.006, metal, [0, 0.035, 0], [0, 0, 0], 28);
  addCylinder(group, 0.0155, 0.0155, 0.006, metal, [0, -0.035, 0], [0, 0, 0], 28);
  addCylinder(group, 0.0055, 0.0055, 0.005, metal, [0, 0.0405, 0], [0, 0, 0], 16);
  group.scale.setScalar(1.25);
  group.userData.radius = 0.02;
  group.userData.half = 0.054;
  return group;
}

function createRejectObject() {
  const group = new THREE.Group();
  const geometry = new THREE.IcosahedronGeometry(0.026, 0);
  geometry.scale(1, 0.85, 1.15);
  addMesh(group, geometry, standard(0xa0703c, 0.75, 0.05, { flatShading: true }));
  group.userData.radius = 0.024;
  group.userData.half = 0.024;
  return group;
}

function setupLabels() {
  for (const [key, info] of Object.entries(COMPONENTS)) {
    const element = document.createElement('button');
    element.type = 'button';
    element.className = `hotspot hotspot--${info.side}`;
    element.innerHTML = `<span class="hotspot__dot"></span><span class="hotspot__line"></span><span class="hotspot__text"><span class="hotspot__full"></span><span class="hotspot__short"></span></span>`;
    element.querySelector('.hotspot__full').textContent = info.label;
    element.querySelector('.hotspot__short').textContent = info.short ?? info.label;
    element.setAttribute('aria-label', `Ver detalhes: ${info.title}`);
    element.addEventListener('click', (event) => {
      event.stopPropagation();
      focusComponent(key);
    });
    const label = new CSS2DObject(element);
    label.position.copy(anchors[key]);
    label.center.set(info.side === 'right' ? 0 : 1, 0.5);
    deviceRoot.add(label);
    hotspots[key] = element;
  }
}

function collectPickables() {
  for (const [key, group] of Object.entries(componentGroups)) {
    group.traverse((child) => {
      if (child.isMesh) {
        child.userData.component = key;
        pickables.push(child);
      }
    });
  }
}

function aspectFactor() {
  const aspect = ui.viewport.clientWidth / Math.max(1, ui.viewport.clientHeight);
  return aspect < 1 ? Math.min(2.1, Math.pow(1 / aspect, 0.9)) : 1;
}

function homeView(mode) {
  const factor = aspectFactor();
  if (mode === 'interior') {
    const target = new THREE.Vector3(0, 1.08 + PEDESTAL_HEIGHT, 0.02);
    return { target, position: target.clone().add(new THREE.Vector3(2.3, 1.15, 3.5).multiplyScalar(factor)) };
  }
  const target = new THREE.Vector3(0, 1.08 + PEDESTAL_HEIGHT, 0);
  return { target, position: target.clone().add(new THREE.Vector3(2.35, 1.45, 3.5).multiplyScalar(factor)) };
}

function moveCamera(position, target, duration = 1100, ease = easeInOutCubic) {
  const id = ++cameraMoveId;
  const fromPosition = camera.position.clone();
  const fromTarget = controls.target.clone();
  return tween(duration, (k) => {
    if (id !== cameraMoveId) return;
    camera.position.lerpVectors(fromPosition, position, k);
    controls.target.lerpVectors(fromTarget, target, k);
  }, ease);
}

function setViewMode(mode, { animateCamera = true } = {}) {
  state.viewMode = mode;
  ui.viewButtons.forEach((button) => button.setAttribute('aria-pressed', String(button.dataset.view === mode)));
  labelRenderer.domElement.classList.toggle('is-visible', mode === 'interior');
  if (mode === 'exterior') clearFocus();
  const id = ++xrayMoveId;
  const from = state.xray;
  const to = mode === 'interior' ? 1 : 0;
  const fade = tween(950, (k) => {
    if (id !== xrayMoveId) return;
    state.xray = from + (to - from) * k;
    applyXray(state.xray);
  });
  if (!animateCamera) return fade;
  const home = homeView(mode);
  return Promise.all([fade, moveCamera(home.position, home.target, 1300)]);
}

function resetCamera() {
  clearFocus();
  const home = homeView(state.viewMode);
  return moveCamera(home.position, home.target, 1100);
}

function worldAnchor(key) {
  return anchors[key].clone().add(deviceRoot.position);
}

function focusComponent(key) {
  const info = COMPONENTS[key];
  if (!info) return;
  if (!info.outside && state.viewMode !== 'interior') setViewMode('interior', { animateCamera: false });
  rejectDoor.statusVisible = false;
  state.activeComponent = key;
  const target = worldAnchor(key);
  if (key === 'pilhas' || key === 'rejeicao') target.x *= 0.5;
  const direction = (info.outside ? new THREE.Vector3(0.72, 0.36, 1.25) : new THREE.Vector3(1.05, 0.42, 0.95)).normalize();
  const position = target.clone().addScaledVector(direction, (info.distance ?? 0.9) * 1.35 * Math.min(1.5, aspectFactor()));
  moveCamera(position, target, 1100);

  parts.highlightBox.box.setFromObject(componentGroups[key]).expandByScalar(0.012);
  parts.highlightBox.visible = true;
  ui.cardEyebrow.textContent = info.stage ? `Etapa ${info.stage} do fluxo` : 'Componente';
  ui.cardTitle.textContent = info.title;
  ui.cardText.textContent = textoDoCartao(key);
  ui.card.hidden = false;
  Object.entries(hotspots).forEach(([hotspotKey, element]) => element.classList.toggle('is-active', hotspotKey === key));
  if (!state.simulationRunning) markFlow(key, { focusOnly: true });
}

function clearFocus() {
  rejectDoor.statusVisible = false;
  state.activeComponent = null;
  if (parts.highlightBox) parts.highlightBox.visible = false;
  ui.card.hidden = true;
  Object.values(hotspots).forEach((element) => element.classList.remove('is-active'));
  if (!state.simulationRunning) markFlow(null);
}

function markFlow(stage, { focusOnly = false } = {}) {
  const index = FLOW_ORDER.indexOf(stage);
  const isDestination = stage === 'pilhas' || stage === 'rejeicao';
  ui.flowSteps.forEach((step) => {
    const key = step.dataset.stage;
    const stepIndex = FLOW_ORDER.indexOf(key);
    const active = key === stage;
    const done = !focusOnly && ((index >= 0 && stepIndex >= 0 && stepIndex < index) || (isDestination && stepIndex >= 0));
    step.classList.toggle('is-active', active);
    step.classList.toggle('is-done', done);
  });
}

function setOledState(screenKey, data = {}) {
  const canvas = parts.oledTexture.image;
  drawOledScreen(canvas.getContext('2d'), canvas.width, canvas.height, screenKey, data);
  parts.oledTexture.needsUpdate = true;
}

function setSimulationUi(running) {
  state.simulationRunning = running;
  atualizarSelo();
}

function chuteNormal(side) {
  const dx = CHUTE.endX - CHUTE.startX;
  const dy = CHUTE.startY - CHUTE.endY;
  return new THREE.Vector3(side * dy, dx, 0).normalize();
}

function binSlot() {
  const index = deposited.pilhas.length;
  const layer = Math.floor(index / 6);
  const cell = index % 6;
  return new THREE.Vector3(-0.205 + ((cell % 2) - 0.5) * 0.11, BIN_FLOOR.pilhas + layer * 0.031, 0.02 + (Math.floor(cell / 2) - 1) * 0.07);
}

function rejectFloorY(z) {
  const t = (z - REJECT.floorBackZ) / (REJECT.front - REJECT.floorBackZ);
  return REJECT.floorBackY + (REJECT.floorFrontY - REJECT.floorBackY) * t + 0.003;
}

function waitUntil(predicate) {
  return new Promise((resolve) => {
    if (predicate()) resolve();
    else waiters.add({ predicate, resolve });
  });
}

function updateWaiters() {
  for (const waiter of waiters) {
    if (waiter.predicate()) {
      waiters.delete(waiter);
      waiter.resolve();
    }
  }
}

function isInDetectionZone(item) {
  const p = item.position;
  return item.scale.x > 0.2 && p.x > REJECT.x0 && p.x < REJECT.x1 && p.y > 0.78 && p.y < 0.95 && p.z > 0.17 && p.z < 0.42;
}

function applyDoorAngle(angle) {
  parts.door.pivot.rotation.x = -angle;
  parts.door.horn.rotation.x = -angle;
  const arm = DOOR_ARM.clone().applyAxisAngle(AXIS_X, -angle);
  const tabTip = DOOR_HINGE.clone().add(arm);
  const hornTip = DOOR_SHAFT.clone().add(arm);
  parts.door.rod.position.addVectors(tabTip, hornTip).multiplyScalar(0.5);
}

function updateRejectDoor(delta) {
  if (esp32.aoVivo) {
    // Ao vivo a portinha obedece ao campo "porta" do /status.
    // A deteccao propria do 3D fica desligada para nao disputar o controle.
    rejectDoor.detecting = false;
    rejectDoor.target = esp32.portaAberta ? DOOR.openAngle : 0;
  } else {
    rejectDoor.detecting = [...rejectDoor.items].some(isInDetectionZone);
    if (rejectDoor.detecting) {
      rejectDoor.clearTime = 0;
      rejectDoor.target = DOOR.openAngle;
    } else if (rejectDoor.target > 0) {
      rejectDoor.clearTime += delta;
      if (rejectDoor.clearTime >= DOOR.closeDelay) rejectDoor.target = 0;
    }
  }
  const step = DOOR.speed * delta;
  rejectDoor.angle += THREE.MathUtils.clamp(rejectDoor.target - rejectDoor.angle, -step, step);
  applyDoorAngle(rejectDoor.angle);
  updateDoorStatus();
}

function doorStatus() {
  const isOpen = rejectDoor.angle >= DOOR.openAngle - 0.001;
  const isClosed = rejectDoor.angle <= 0.001;

  if (esp32.aoVivo) {
    if (esp32.portaAberta) {
      const vindoDeComando = esp32.origem === 'comando da página';
      return isOpen
        ? ['Portinha aberta', vindoDeComando
            ? 'Aberta por comando da página de controle. O ESP32 confirmou o movimento do servo.'
            : 'O firmware mantém a portinha aberta por 5 segundos fixos para a retirada do objeto.']
        : ['Abrindo a portinha', 'O ESP32 comandou a abertura do servo no GPIO 26.'];
    }
    return isClosed
      ? ['Portinha fechada', 'Aguardando o próximo objeto não metálico.']
      : ['Fechando a portinha', 'O tempo de abertura terminou e o servo está fechando a portinha.'];
  }

  if (rejectDoor.detecting) {
    return isOpen
      ? ['Portinha aberta', 'Simulação: o objeto está na região da portinha.']
      : ['Abrindo a portinha', 'Simulação: o servo está abrindo a portinha.'];
  }
  if (rejectDoor.target > 0) return ['Confirmando área livre', 'Simulação: a portinha segue aberta por um instante.'];
  if (!isClosed) return ['Fechando a portinha', 'Simulação: área livre, o servo está fechando a portinha.'];
  return ['Portinha fechada', 'Aguardando um novo objeto na região da portinha.'];
}

function updateDoorStatus() {
  if (!rejectDoor.statusVisible) return;
  const [title, text] = doorStatus();
  const key = title + text;
  if (key === rejectDoor.lastStatus) return;
  rejectDoor.lastStatus = key;
  ui.cardEyebrow.textContent = 'Servo · portinha de rejeição';
  ui.cardTitle.textContent = title;
  ui.cardText.textContent = text;
  ui.card.hidden = false;
}

async function dropIntoRejectCompartment(item) {
  const radius = item.userData.radius;
  const start = item.position.clone();
  const spin = item.rotation.x;
  const landing = new THREE.Vector3(0.2 + (Math.random() - 0.5) * 0.04, 0, 0.04);
  landing.y = rejectFloorY(landing.z) + radius;
  await tween(420, (k) => {
    item.position.x = start.x + (landing.x - start.x) * easeOutCubic(k);
    item.position.z = start.z + (landing.z - start.z) * k;
    item.position.y = start.y + (landing.y - start.y) * easeInQuad(k);
    item.rotation.x = spin + k * 2;
  }, linear);
  const pocket = new THREE.Vector3(landing.x, rejectFloorY(REJECT.pocketZ) + radius, REJECT.pocketZ);
  await tween(800, (k) => {
    item.position.lerpVectors(landing, pocket, k);
    item.rotation.x = spin + 2 + k * 5;
  }, easeOutCubic);
}

async function runRejectExit(item) {
  rejectDoor.statusVisible = true;
  rejectDoor.lastStatus = '';
  rejectDoor.items.add(item);
  await waitUntil(() => rejectDoor.angle >= DOOR.openAngle - 0.001);
  await wait(1800);
  const start = item.position.clone();
  await tween(900, (k) => {
    item.position.z = start.z + 0.2 * k;
    item.position.y = start.y + 0.035 * Math.sin(k * Math.PI * 0.5);
    item.scale.setScalar(Math.max(0.001, 1 - easeInQuad(k)));
  }, easeInOutCubic);
  rejectDoor.items.delete(item);
  deviceRoot.remove(item);
  await waitUntil(() => rejectDoor.target === 0 && rejectDoor.angle <= 0.001);
}

function focusRejectDoorArea() {
  const target = new THREE.Vector3(0.2, 0.9 + PEDESTAL_HEIGHT, 0.28);
  const direction = new THREE.Vector3(0.85, 0.42, 1.0).normalize();
  return moveCamera(target.clone().addScaledVector(direction, 1.2 * Math.min(1.5, aspectFactor())), target, 1200);
}

async function simulateRejectedObject() {
  if (state.simulationRunning || esp32.aoVivo) return;
  setSimulationUi(true);
  clearFocus();
  if (state.viewMode !== 'interior') setViewMode('interior', { animateCamera: false });
  await focusRejectDoorArea();
  markFlow('rejeicao');
  setOledState('rejeitado');
  state.ringAlert = 1;
  const item = createRejectObject();
  item.position.set(0.215, 1.12, 0.04);
  deviceRoot.add(item);
  await dropIntoRejectCompartment(item);
  await runRejectExit(item);
  setOledState('aguardando');
  await wait(400);
  markFlow(null);
  setSimulationUi(false);
}

async function clearBin(kind) {
  const items = deposited[kind].splice(0);
  await tween(400, (k) => items.forEach((item) => item.scale.setScalar(1 - k)));
  items.forEach((item) => deviceRoot.remove(item));
}

async function simulateDisposal(kind) {
  if (state.simulationRunning || esp32.aoVivo) return;
  setSimulationUi(true);
  clearFocus();
  const isBattery = kind === 'pilha';
  const binKey = isBattery ? 'pilhas' : 'rejeicao';
  const side = isBattery ? -1 : 1;
  if (state.viewMode !== 'interior') await setViewMode('interior');
  if (isBattery && deposited.pilhas.length >= 12) await clearBin('pilhas');

  const item = isBattery ? createBattery() : createRejectObject();
  const { radius, half } = item.userData;
  item.position.set(0, TOP_SURFACE + 0.26, FLOW_Z);
  deviceRoot.add(item);

  markFlow('entrada');
  state.ringBoost = 1;
  await tween(750, (k) => { item.position.y = TOP_SURFACE + 0.26 - 0.28 * k; }, easeInOutCubic);
  await tween(360, (k) => { item.position.y = TOP_SURFACE - 0.02 + (SENSOR1_Y - TOP_SURFACE + 0.02) * k; }, easeInQuad);

  markFlow('sensor1');
  state.sensor1Flash = 1;
  setOledState('detectado');
  const restY = TRAY.pivotY + 0.006 + half;
  await tween(260, (k) => { item.position.y = SENSOR1_Y + (restY - SENSOR1_Y) * k; }, easeInQuad);
  await tween(160, (k) => { item.position.y = restY + Math.sin(k * Math.PI) * 0.008; }, linear);
  await wait(250);

  markFlow('sensor2');
  state.sensor2Scan = 1;
  await wait(900);

  markFlow('esp32');
  state.espActivity = 1;
  state.sensor2Result = isBattery ? 1 : -1;
  if (!isBattery) state.ringAlert = 1;
  setOledState(isBattery ? 'pilha' : 'rejeitado');
  await wait(900);

  markFlow('servo');
  const tilt = -side * TRAY.tilt;
  const normal = chuteNormal(side);
  const surfaceOffset = 0.004 + radius;
  const chuteStart = new THREE.Vector3(side * CHUTE.startX, CHUTE.startY, FLOW_Z).addScaledVector(normal, surfaceOffset + 0.004);
  const chuteEnd = new THREE.Vector3(side * CHUTE.endX, CHUTE.endY, FLOW_Z).addScaledVector(normal, surfaceOffset);
  const restPosition = item.position.clone();
  const lieAngle = isBattery ? side * 0.76 : 0;
  await Promise.all([
    tween(480, (k) => { parts.tray.rotation.z = tilt * k; }, easeInOutCubic),
    wait(200).then(() => tween(520, (k) => {
      item.position.lerpVectors(restPosition, chuteStart, k);
      if (isBattery) item.rotation.z = lieAngle * k;
      else item.rotation.x = -side * k * 2;
    }, easeInQuad))
  ]);

  markFlow(binKey);
  await tween(480, (k) => {
    item.position.lerpVectors(chuteStart, chuteEnd, k);
    if (!isBattery) item.rotation.z = -side * k * 5;
  }, easeInQuad);

  if (isBattery) {
    const slot = binSlot();
    const dropStart = item.position.clone();
    const startRotation = item.rotation.clone();
    const finalRotation = new THREE.Euler(0, (Math.random() - 0.5) * 0.5, Math.PI / 2);
    await tween(440, (k) => {
      item.position.x = dropStart.x + (slot.x - dropStart.x) * easeOutCubic(k);
      item.position.z = dropStart.z + (slot.z - dropStart.z) * k;
      item.position.y = dropStart.y + (slot.y - dropStart.y) * easeInQuad(k);
      item.rotation.set(
        startRotation.x + (finalRotation.x - startRotation.x) * k,
        startRotation.y + (finalRotation.y - startRotation.y) * k,
        startRotation.z + (finalRotation.z - startRotation.z) * k
      );
    }, linear);
    await tween(180, (k) => { item.position.y = slot.y + Math.sin(k * Math.PI) * 0.012; }, linear);
    deposited.pilhas.push(item);
    state.loadCellPulse = 1;
    setOledState('pesagem', { peso: 22.5 + Math.random() * 1.6 });
    await wait(1700);
    await tween(420, (k) => { parts.tray.rotation.z = tilt * (1 - k); }, easeInOutCubic);
  } else {
    await dropIntoRejectCompartment(item);
    await tween(420, (k) => { parts.tray.rotation.z = tilt * (1 - k); }, easeInOutCubic);
    await runRejectExit(item);
  }
  setOledState('aguardando');
  state.sensor2Result = 0;
  state.ringBoost = 0;
  await wait(500);
  markFlow(null);
  setSimulationUi(false);
}

function updateEffects(time, delta) {
  const decay = (value, rate) => Math.max(0, value - delta * rate);
  state.sensor1Flash = decay(state.sensor1Flash, 1.4);
  state.sensor2Scan = decay(state.sensor2Scan, 0.9);
  state.espActivity = decay(state.espActivity, 0.8);
  state.ringAlert = decay(state.ringAlert, 0.35);
  state.loadCellPulse = decay(state.loadCellPulse, 0.6);

  const breathing = 0.5 + 0.5 * Math.sin(time * 1.8);
  const ringColor = new THREE.Color(COLORS.glow).lerp(new THREE.Color(COLORS.warn), Math.min(1, state.ringAlert * 1.5));
  parts.entry.ringMaterial.emissive.copy(ringColor);
  parts.entry.ringMaterial.emissiveIntensity = 1.7 + breathing * 0.7 + state.ringBoost * 0.8;
  parts.entry.halo.color.copy(ringColor);
  parts.entry.halo.opacity = 0.12 + breathing * 0.1 + state.ringBoost * 0.08;
  parts.entry.sleeveMaterial.emissive.copy(ringColor);
  parts.entry.glowLight.color.copy(ringColor);
  parts.entry.glowLight.intensity = 0.18 + breathing * 0.12 + state.ringBoost * 0.2;

  parts.sensors.beamMaterial.opacity = 0.15 * state.xray + state.sensor1Flash * 0.9;
  parts.sensors.emitterLed.emissiveIntensity = 0.4 + state.sensor1Flash * 3;
  const scanWave = state.sensor2Scan > 0 ? 0.5 + 0.5 * Math.sin(time * 18) : 0;
  parts.sensors.scanMaterial.opacity = state.sensor2Scan * (0.25 + scanWave * 0.35);
  parts.sensors.scanField.scale.y = 1 + scanWave * 0.15;
  const ledColor = state.sensor2Result < 0 ? new THREE.Color(0xff4a3a) : new THREE.Color(COLORS.glow);
  parts.sensors.sensor2Led.emissive.copy(ledColor);
  parts.sensors.sensor2Led.emissiveIntensity = state.sensor2Result !== 0 ? 3 : 0.3 + state.sensor2Scan * 2;

  const blink = Math.sin(time * (state.espActivity > 0 ? 30 : 3)) > 0 ? 1 : 0;
  parts.electronics.statusLed.emissiveIntensity = 0.4 + blink * (0.8 + state.espActivity * 2.5);
  parts.loadCell.cellMaterial.emissiveIntensity = state.loadCellPulse * (0.6 + 0.4 * Math.sin(time * 12));

  const ping = 0.5 + 0.5 * Math.sin(time * 9);
  parts.ultrasonic.coneMaterial.opacity = rejectDoor.detecting ? 0.2 + ping * 0.16 : state.xray * (0.025 + ping * 0.03);
  const doorMoving = rejectDoor.angle > 0.001 || rejectDoor.target > 0;
  parts.door.ledMaterial.emissive.set(doorMoving ? COLORS.warn : COLORS.glow);
  parts.door.ledMaterial.emissiveIntensity = doorMoving ? 1.4 + ping * 1.2 : 1.4;

  if (parts.highlightBox.visible) {
    parts.highlightBox.material.opacity = 0.55 + 0.35 * Math.sin(time * 3);
  }
}

function setupInteraction() {
  const raycaster = new THREE.Raycaster();
  const pointer = new THREE.Vector2();
  let downPosition = null;
  const canvas = renderer.domElement;

  const pick = (event) => {
    const rect = canvas.getBoundingClientRect();
    pointer.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
    pointer.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;
    raycaster.setFromCamera(pointer, camera);
    const hits = raycaster.intersectObjects(pickables, false);
    for (const hit of hits) {
      const key = hit.object.userData.component;
      if (state.xray > 0.5 || COMPONENTS[key]?.outside) return key;
      return null;
    }
    return null;
  };

  canvas.addEventListener('pointerdown', (event) => { downPosition = { x: event.clientX, y: event.clientY }; });
  canvas.addEventListener('pointerup', (event) => {
    if (!downPosition) return;
    const moved = Math.hypot(event.clientX - downPosition.x, event.clientY - downPosition.y);
    downPosition = null;
    if (moved > 5) return;
    const key = pick(event);
    if (key) focusComponent(key);
  });
  canvas.addEventListener('pointermove', (event) => {
    if (event.buttons) return;
    canvas.style.cursor = pick(event) ? 'pointer' : 'grab';
  });

  controls.addEventListener('start', () => {
    cameraMoveId += 1;
    state.lastInteraction = performance.now();
  });

  ui.viewButtons.forEach((button) => button.addEventListener('click', () => {
    state.lastInteraction = performance.now();
    setViewMode(button.dataset.view);
  }));
  ui.resetButton.addEventListener('click', () => {
    state.lastInteraction = performance.now();
    resetCamera();
  });
  ui.simulateButtons.forEach((button) => button.addEventListener('click', () => {
    state.lastInteraction = performance.now();
    if (button.dataset.simulate === 'portinha') simulateRejectedObject();
    else simulateDisposal(button.dataset.simulate);
  }));
  ui.flowSteps.forEach((step) => step.addEventListener('click', () => {
    state.lastInteraction = performance.now();
    focusComponent(step.dataset.stage);
  }));
  ui.cardClose.addEventListener('click', () => {
    clearFocus();
    resetCamera();
  });
  window.addEventListener('keydown', (event) => {
    if (event.key === 'Escape' && state.activeComponent) {
      clearFocus();
      resetCamera();
    }
  });
  window.addEventListener('resize', onResize);
}

function onResize() {
  const width = ui.viewport.clientWidth;
  const height = ui.viewport.clientHeight;
  camera.aspect = width / height;
  camera.updateProjectionMatrix();
  renderer.setSize(width, height);
  composer.setSize(width, height);
  labelRenderer.setSize(width, height);
}

async function loadFonts() {
  if (!document.fonts) return;
  const requests = Promise.all([
    document.fonts.load('800 40px "Archivo"'),
    document.fonts.load('italic 700 40px "Archivo"'),
    document.fonts.load('500 20px "IBM Plex Mono"'),
    document.fonts.load('600 20px "IBM Plex Mono"')
  ]).catch(() => {});
  await Promise.race([requests, new Promise((resolve) => setTimeout(resolve, 2500))]);
}

function drawBrandMark() {
  const ctx = ui.brandMark.getContext('2d');
  ctx.clearRect(0, 0, ui.brandMark.width, ui.brandMark.height);
  drawRecycleMark(ctx, 40, 46, 27, '#3fae3a', { battery: true, batteryColor: '#e7efe9', leaf: '#7bd147' });
}

function createRenderer() {
  const width = ui.viewport.clientWidth;
  const height = ui.viewport.clientHeight;
  renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: 'high-performance' });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.setSize(width, height);
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  ui.viewport.appendChild(renderer.domElement);

  scene = new THREE.Scene();
  scene.background = new THREE.Color(COLORS.background);
  scene.fog = new THREE.Fog(COLORS.background, 7, 17);
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(renderer), 0.04).texture;

  camera = new THREE.PerspectiveCamera(35, width / height, 0.05, 60);
  camera.position.set(5.2, 3.1, 7.2);

  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.08;
  controls.minDistance = 0.45;
  controls.maxDistance = 9;
  controls.maxPolarAngle = Math.PI * 0.49;
  controls.screenSpacePanning = true;
  controls.autoRotateSpeed = 0.5;
  controls.target.set(0, 1.1, 0);

  labelRenderer = new CSS2DRenderer();
  labelRenderer.setSize(width, height);
  labelRenderer.domElement.className = 'label-layer';
  ui.viewport.appendChild(labelRenderer.domElement);

  const renderTarget = new THREE.WebGLRenderTarget(width, height, { type: THREE.HalfFloatType, samples: 4 });
  composer = new EffectComposer(renderer, renderTarget);
  composer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  composer.setSize(width, height);
  composer.addPass(new RenderPass(scene, camera));
  composer.addPass(new UnrealBloomPass(new THREE.Vector2(width, height), 0.5, 0.45, 0.92));
  composer.addPass(new OutputPass());
}

function buildDevice() {
  deviceRoot = new THREE.Group();
  deviceRoot.name = 'descarte-aqui';
  deviceRoot.position.y = PEDESTAL_HEIGHT;
  scene.add(deviceRoot);

  const mat = buildMaterials();
  const oledTexture = canvasTexture(512, 256, () => {});
  const textures = {
    front: createFrontTexture(),
    back: createBackTexture(),
    right: createSideTexture('right'),
    left: createSideTexture('left'),
    sign: createSignTexture(),
    mountPlate: createMountPlateTexture(),
    esp32: createEsp32Texture(),
    psu: createPsuTexture()
  };

  parts.interiorLights = buildLights();
  buildStage();
  const shellInfo = buildShell(mat, textures);
  parts.entry = buildEntry(mat);
  buildFrame(mat, textures);
  parts.sensors = buildSensors(mat);
  const servo = buildServo(mat);
  parts.tray = servo.tray;
  buildChutes(mat);
  parts.binLabels = [buildBin(mat, 'pilhas'), buildRejectCompartment(mat)];
  parts.door = buildRejectDoor(mat, shellInfo);
  parts.ultrasonic = buildUltrasonic(mat);
  parts.loadCell = buildLoadCell(mat);
  parts.electronics = buildElectronics(mat, textures);
  buildOled(mat, oledTexture);
  buildWiring();
  parts.oledTexture = oledTexture;

  const highlightBox = new THREE.Box3Helper(new THREE.Box3(), COLORS.glow);
  highlightBox.material.transparent = true;
  highlightBox.material.depthTest = false;
  highlightBox.renderOrder = 10;
  highlightBox.visible = false;
  scene.add(highlightBox);
  parts.highlightBox = highlightBox;

  setOledState('aguardando');
  collectPickables();
  setupLabels();
  applyXray(0);
}

/* =====================================================================
   CAMADA DE DADOS - consome /status e /sensor do ESP32.
   Separada do codigo 3D: so le o JSON e empurra o resultado para a cena.
   ===================================================================== */

// Vazio quando a pagina e servida pela propria placa (mesma origem).
// Para abrir o arquivo localmente, coloque aqui o IP: "http://192.168.1.100"
const ESP32_BASE_URL = '';

const INTERVALO_CONSULTA_MS = 500;
const TIMEOUT_CONSULTA_MS = 1500;
const FALHAS_ATE_SIMULACAO = 3;

const SERVO_REPOUSO = 90;
const SERVO_CURSO = 70;

// Estes mapas precisam cobrir TODOS os valores de estadoSistema em
// src/main.cpp. Estado que nao estiver aqui cai no 'aguardando' e o modelo
// fica parado enquanto a maquina trabalha.
//
// O segundo ultrassonico trocou o antigo 'Porta aberta', de tempo fixo, por
// quatro estados: a porta so abre quando o objeto chega ao compartimento e so
// fecha quando a retirada e confirmada (ou quando o prazo estoura).
const TELA_POR_ESTADO = {
  'Iniciando': 'aguardando',
  'Aguardando objeto': 'aguardando',
  'Separando metal': 'pilha',
  'Pesando': 'pesagem',
  'Separando nao-metal': 'rejeitado',
  'Aguardando nao-metal': 'rejeitado',
  'Retire o objeto': 'rejeitado',
  'Objeto retirado': 'rejeitado',
  'Retirada nao confirmada': 'rejeitado'
};

const ETAPA_POR_ESTADO = {
  'Iniciando': null,
  'Aguardando objeto': null,
  'Separando metal': 'servo',
  'Pesando': 'pilhas',
  'Separando nao-metal': 'servo',
  'Aguardando nao-metal': 'rejeicao',
  'Retire o objeto': 'rejeicao',
  'Objeto retirado': null,
  'Retirada nao confirmada': null
};

// Estados do caminho nao-metal: acendem o alerta do anel.
const ESTADOS_NAO_METAL = [
  'Separando nao-metal',
  'Aguardando nao-metal',
  'Retire o objeto',
  'Objeto retirado',
  'Retirada nao confirmada'
];

// Embutido = dentro do iframe da pagina de controle. Nesse caso nao fazemos
// polling proprio: o pai ja consulta o ESP32 e repassa o estado por
// postMessage. Uma unica fonte de consulta evita dobrar a carga no WebServer,
// que atende uma conexao por vez.
const EMBUTIDO = new URLSearchParams(location.search).has('embed');

const CANAL_SINCRONIA = 'descarte-aqui';

const esp32 = {
  aoVivo: false,
  falhas: 0,
  consultando: false,
  status: null,
  sensor: null,
  portaAberta: false,
  ultimaTela: '',
  mensagemErro: '',
  erroAte: 0,
  origem: ''
};

async function buscarJson(rota) {
  const controle = new AbortController();
  const limite = setTimeout(() => controle.abort(), TIMEOUT_CONSULTA_MS);
  try {
    const resposta = await fetch(ESP32_BASE_URL + rota, { signal: controle.signal, cache: 'no-store' });
    if (!resposta.ok) throw new Error('HTTP ' + resposta.status);
    return await resposta.json();
  } finally {
    clearTimeout(limite);
  }
}

// O servo real vai de 20 a 160 graus com repouso em 90; a bandeja do 3D
// inclina de -TRAY.tilt a +TRAY.tilt. A conversao e proporcional.
function inclinacaoReal(servoGraus) {
  const proporcao = (servoGraus - SERVO_REPOUSO) / SERVO_CURSO;
  return THREE.MathUtils.clamp(proporcao, -1, 1) * TRAY.tilt;
}

function detalheAoVivo(key) {
  if (!esp32.aoVivo || !esp32.status || !esp32.sensor) return '';
  const s = esp32.status;
  const n = esp32.sensor;
  if (key === 'sensor1') {
    return n.distancia_cm < 0
      ? 'Agora: sem eco de retorno.'
      : `Agora: ${n.distancia_cm.toFixed(1)} cm · objeto ${n.objeto_presente ? 'presente' : 'ausente'}.`;
  }
  if (key === 'sensor2') {
    if (!n.objeto_presente) return 'Agora: sem objeto sobre a bandeja.';
    return `Agora: ${n.metal_detectado ? 'metal detectado' : 'não metálico'}.`;
  }
  if (key === 'celula') {
    return s.balanca_ok
      ? `Agora: ${n.peso_g.toFixed(1)} g (bruto ${n.peso_bruto}).`
      : 'Agora: a balança não respondeu na inicialização.';
  }
  if (key === 'oled') {
    return s.oled_ok ? `Agora: exibindo "${s.estado}".` : 'Agora: display não encontrado.';
  }
  if (key === 'servo') return `Agora: servo de separação em ${s.servo_separacao}°.`;
  if (key === 'portinha') {
    const origem = esp32.origem ? ` (${esp32.origem})` : '';
    return `Agora: portinha ${s.porta}${origem}.`;
  }
  if (key === 'esp32') {
    return `Agora: ${s.estado} · ${s.ip} · RSSI ${s.rssi} dBm · heap ${s.heap_livre} B.`;
  }
  return '';
}

function textoDoCartao(key) {
  const info = COMPONENTS[key];
  if (!info) return '';
  const extra = detalheAoVivo(key);
  return extra ? `${info.text} ${extra}` : info.text;
}

function atualizarCartaoAoVivo() {
  const key = state.activeComponent;
  if (!key || rejectDoor.statusVisible) return;
  ui.cardText.textContent = textoDoCartao(key);
}

function atualizarSelo() {
  if (esp32.mensagemErro && performance.now() < esp32.erroAte) {
    ui.statusChip.classList.remove('is-running', 'is-live');
    ui.statusChip.classList.add('is-error');
    ui.statusText.textContent = esp32.mensagemErro;
    return;
  }

  esp32.mensagemErro = '';
  ui.statusChip.classList.remove('is-error');

  if (state.simulationRunning) {
    ui.statusChip.classList.add('is-running');
    ui.statusChip.classList.remove('is-live');
    ui.statusText.textContent = 'Simulação visual em andamento';
  } else if (esp32.aoVivo) {
    ui.statusChip.classList.add('is-live');
    ui.statusChip.classList.remove('is-running');
    const ip = esp32.status && esp32.status.ip ? ` · ${esp32.status.ip}` : '';
    ui.statusText.textContent = `ESP32 ao vivo${ip}`;
  } else {
    ui.statusChip.classList.remove('is-running', 'is-live');
    ui.statusText.textContent = 'Sem conexão com o ESP32 · modo simulação';
  }

  ui.doorButtons.forEach((botao) => { botao.disabled = !esp32.aoVivo; });
  ui.simulateButtons.forEach((botao) => { botao.disabled = state.simulationRunning || esp32.aoVivo; });
}

function mostrarErro(mensagem) {
  esp32.mensagemErro = mensagem;
  esp32.erroAte = performance.now() + 4000;
  atualizarSelo();
  setTimeout(atualizarSelo, 4100);
}

function entrarModoAoVivo() {
  esp32.aoVivo = true;
  esp32.ultimaTela = '';
  rejectDoor.items.clear();
  rejectDoor.detecting = false;
}

function sairModoAoVivo() {
  esp32.aoVivo = false;
  esp32.status = null;
  esp32.sensor = null;
  esp32.portaAberta = false;
  esp32.ultimaTela = '';
  esp32.origem = '';
  state.sensor2Result = 0;
  state.ringBoost = 0;
  parts.tray.rotation.z = 0;
  setOledState('aguardando');
  markFlow(null);
  atualizarSelo();
}

function aplicarEstadoReal(status, sensor) {
  esp32.portaAberta = status.porta === 'aberta';
  parts.tray.rotation.z = inclinacaoReal(status.servo_separacao);

  const presente = sensor.objeto_presente === true;

  // O indutivo so diz algo util quando ha objeto: em repouso o pino fica em
  // HIGH e "nao e metal" seria enganoso, entao deixamos o LED neutro.
  state.sensor2Result = presente ? (sensor.metal_detectado ? 1 : -1) : 0;

  if (presente) {
    state.sensor1Flash = 1;
    state.sensor2Scan = 1;
    state.ringBoost = 1;
  } else {
    state.ringBoost = 0;
  }

  if (ESTADOS_NAO_METAL.indexOf(status.estado) !== -1) {
    state.ringAlert = 1;
  }

  const tela = TELA_POR_ESTADO[status.estado] || 'aguardando';
  const assinatura = tela === 'pesagem' ? `pesagem:${Number(sensor.peso_g).toFixed(1)}` : tela;
  if (assinatura !== esp32.ultimaTela) {
    esp32.ultimaTela = assinatura;
    setOledState(tela, { peso: Number(sensor.peso_g) });
  }

  const etapa = status.estado === 'Aguardando objeto' && presente
    ? 'sensor1'
    : (ETAPA_POR_ESTADO[status.estado] || null);
  markFlow(etapa);

  atualizarCartaoAoVivo();
  atualizarSelo();
}

// Caminho unico de sucesso, venha o estado do polling proprio ou do pai.
function registrarEstado(status, sensor, origem) {
  esp32.falhas = 0;

  // Se uma simulacao local estiver rodando, ela e dona da cena.
  // Entramos ao vivo na proxima atualizacao, ja com a cena livre.
  if (state.simulationRunning) return;

  esp32.status = status;
  esp32.sensor = sensor || esp32.sensor;
  esp32.origem = origem;
  if (!esp32.aoVivo) entrarModoAoVivo();
  aplicarEstadoReal(status, esp32.sensor);
}

// Caminho unico de falha. So caimos para simulacao depois de FALHAS_ATE_SIMULACAO
// seguidas: uma falha isolada acontece quando a pesagem bloqueia o loop.
function registrarFalha() {
  esp32.falhas += 1;
  if (esp32.aoVivo && esp32.falhas >= FALHAS_ATE_SIMULACAO) sairModoAoVivo();
}

async function consultarEsp32() {
  if (esp32.consultando) return;
  esp32.consultando = true;
  try {
    // Em sequencia, nao em paralelo: o WebServer do ESP32 atende uma
    // conexao por vez, e dois sockets simultaneos so criam disputa.
    const status = await buscarJson('/status');
    const sensor = await buscarJson('/sensor');
    registrarEstado(status, sensor, 'estado do ESP32');
  } catch (erro) {
    registrarFalha();
  } finally {
    esp32.consultando = false;
  }
}

function iniciarConsultas() {
  const proxima = async () => {
    await consultarEsp32();
    setTimeout(proxima, INTERVALO_CONSULTA_MS);
  };
  proxima();
}

// Mensagens vindas da pagina de controle: pelo iframe (postMessage) e pelo
// BroadcastChannel, que alcanca o 3D aberto em outra aba.
function tratarMensagem(dados) {
  if (!dados || dados.tipo !== CANAL_SINCRONIA) return;
  if (dados.erro) {
    registrarFalha();
    return;
  }
  if (!dados.status) return;
  registrarEstado(dados.status, dados.sensor, dados.origem || 'estado do ESP32');
}

function ouvirSincronia() {
  window.addEventListener('message', (evento) => {
    if (evento.source !== window.parent) return;
    tratarMensagem(evento.data);
  });

  if ('BroadcastChannel' in window) {
    const canal = new BroadcastChannel(CANAL_SINCRONIA);
    canal.addEventListener('message', (evento) => tratarMensagem(evento.data));
  }
}

async function comandarPortinha(estado) {
  if (!esp32.aoVivo) return;
  try {
    const resposta = await buscarJson(`/atuador?estado=${estado}`);
    if (!resposta.ok) throw new Error(resposta.erro || 'comando recusado');
    esp32.portaAberta = resposta.estado === 'aberta';
    await consultarEsp32();
  } catch (erro) {
    mostrarErro('Falha ao comandar a portinha');
  }
}

function startLoop() {
  const clock = new THREE.Clock();
  renderer.setAnimationLoop(() => {
    const delta = Math.min(clock.getDelta(), 0.1);
    const time = clock.elapsedTime;
    updateTweens(performance.now());
    updateRejectDoor(delta);
    updateWaiters();
    const idle = performance.now() - state.lastInteraction > 14000;
    controls.autoRotate = !prefersReducedMotion && idle && state.viewMode === 'exterior' && !state.activeComponent && !state.simulationRunning;
    controls.update();
    updateEffects(time, delta);
    composer.render();
    labelRenderer.render(scene, camera);
  });
}

async function init() {
  drawBrandMark();
  await loadFonts();
  try {
    createRenderer();
  } catch (error) {
    ui.loader.classList.add('is-error');
    ui.loaderText.textContent = 'Este navegador não conseguiu iniciar o WebGL. Tente abrir a página no Chrome, Edge ou Firefox atualizados.';
    return;
  }
  buildDevice();
  setupInteraction();
  startLoop();
  ui.doorButtons.forEach((botao) => {
    botao.addEventListener('click', () => comandarPortinha(botao.dataset.atuador));
  });
  atualizarSelo();
  ouvirSincronia();
  if (!EMBUTIDO) iniciarConsultas();
  window.descarteAqui = {
    setViewMode, focusComponent, setOledState, simulateDisposal, simulateRejectedObject, resetCamera,
    comandarPortinha,
    aplicarEstadoReal: (status, sensor) => registrarEstado(status, sensor, 'estado do ESP32'),
    getRejectDoorState: () => ({ detecting: rejectDoor.detecting, angle: rejectDoor.angle, target: rejectDoor.target, open: rejectDoor.angle >= DOOR.openAngle - 0.001 }),
    getEsp32State: () => ({ aoVivo: esp32.aoVivo, falhas: esp32.falhas, status: esp32.status, sensor: esp32.sensor, origem: esp32.origem, embutido: EMBUTIDO })
  };
  ui.loader.classList.add('is-hidden');
  drawBrandMark();
  const home = homeView('exterior');
  moveCamera(home.position, home.target, 2000, easeOutCubic);
}

init();
</script>
</body>
</html>
)rawliteral";

#endif  // PAGINA_WEB_H

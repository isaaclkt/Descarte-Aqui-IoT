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

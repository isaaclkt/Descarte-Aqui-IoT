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

#include "nuvem.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "registro.h"

/* ---------------------------------------------------------------------
   Configuracao

   Ao contrario do config_wifi.h, a ausencia do config_backend.h NAO e erro
   de compilacao: a integracao com o backend e opcional. Sem o arquivo, o
   firmware compila, o servidor local funciona e /status informa
   backend_estado = "nao_configurado".
   --------------------------------------------------------------------- */

#if __has_include("config_backend.h")
  #include "config_backend.h"
#else
  #define BACKEND_URL ""
  #define DISPOSITIVO_CODIGO ""
  #define DISPOSITIVO_CHAVE ""
  #define BACKEND_INTERVALO_MS 3000
  #define BACKEND_TIMEOUT_MS 5000
#endif

#define BACKEND_INTERVALO_MAXIMO_MS 30000
#define CONCLUSAO_MAX_TENTATIVAS 3

namespace {

/* ---------------------------------------------------------------------
   Estado compartilhado entre a task da nuvem (core 0) e o loop (core 1).
   Todo acesso passa por este mutex.
   --------------------------------------------------------------------- */

struct Estado {
  EstadoBackend backend = BACKEND_NAO_CONFIGURADO;
  EstadoSessao sessao = SESSAO_OCIOSA;

  char codigoSessao[33] = "";
  char chaveConclusao[33] = "";
  char pilhaNome[41] = "";
  float pontosPorGrama = 0;
  int segundosRestantes = 0;

  // Conclusao aguardando envio.
  bool conclusaoPendente = false;
  float conclusaoPeso = 0;
  bool conclusaoMetal = false;
  char conclusaoResultado[41] = "";
  int conclusaoTentativas = 0;

  // Etapa de telemetria aguardando envio (a mais recente vence).
  bool etapaPendente = false;
  char etapaNome[24] = "";
  float etapaDistancia = 0;
  float etapaPeso = 0;
  bool etapaMetal = false;

  // Ultimo resultado confirmado pelo backend.
  int ultimoDescarte = 0;
  int ultimosPontos = 0;
  bool ultimoCreditado = false;
  char ultimoMotivo[81] = "";

  unsigned long ultimoContatoMs = 0;
  int ultimoHttp = 0;
};

Estado estado;
SemaphoreHandle_t trava = nullptr;

bool travar() {
  if (trava == nullptr) {
    return false;
  }
  return xSemaphoreTake(trava, pdMS_TO_TICKS(100)) == pdTRUE;
}

void destravar() {
  if (trava != nullptr) {
    xSemaphoreGive(trava);
  }
}

bool integracaoConfigurada() {
  return strlen(BACKEND_URL) > 0 && strlen(DISPOSITIVO_CODIGO) > 0 && strlen(DISPOSITIVO_CHAVE) > 0;
}

/* ---------------------------------------------------------------------
   Leitura do JSON de resposta

   O backend desta integracao e nosso e responde um JSON plano, de chaves
   conhecidas e unicas. Para nao acrescentar uma biblioteca (e um download
   a mais para cada integrante antes da apresentacao), extraimos os campos
   por busca direta da chave. Se o formato da resposta mudar, estas funcoes
   mudam junto.
   --------------------------------------------------------------------- */

// Procura "chave":"valor" e copia o valor.
bool extrairTexto(const String &corpo, const char *chave, char *destino, size_t tamanho) {
  String alvo = "\"";
  alvo += chave;
  alvo += "\":\"";

  int inicio = corpo.indexOf(alvo);
  if (inicio < 0) {
    return false;
  }

  inicio += alvo.length();
  int fim = corpo.indexOf('"', inicio);
  if (fim < 0) {
    return false;
  }

  String valor = corpo.substring(inicio, fim);
  snprintf(destino, tamanho, "%s", valor.c_str());
  return true;
}

// Procura "chave":<numero> e devolve o numero.
bool extrairNumero(const String &corpo, const char *chave, float &valor) {
  String alvo = "\"";
  alvo += chave;
  alvo += "\":";

  int inicio = corpo.indexOf(alvo);
  if (inicio < 0) {
    return false;
  }

  inicio += alvo.length();

  // Rejeita texto e null: aqui esperamos numero.
  if (corpo[inicio] == '"' || corpo.startsWith("null", inicio)) {
    return false;
  }

  int fim = inicio;
  while (fim < (int) corpo.length() &&
         (isDigit(corpo[fim]) || corpo[fim] == '.' || corpo[fim] == '-' || corpo[fim] == '+')) {
    fim++;
  }

  if (fim == inicio) {
    return false;
  }

  valor = corpo.substring(inicio, fim).toFloat();
  return true;
}

bool extrairBooleano(const String &corpo, const char *chave) {
  String alvo = "\"";
  alvo += chave;
  alvo += "\":true";

  return corpo.indexOf(alvo) >= 0;
}

/* ---------------------------------------------------------------------
   Requisicao HTTP

   Roda SEMPRE dentro da task do core 0. Nunca chame do loop().
   --------------------------------------------------------------------- */

int requisicao(const String &url, bool post, const String &corpo, String &resposta) {
  const bool https = url.startsWith("https://");

  WiFiClient clienteSimples;
  WiFiClientSecure clienteSeguro;

  if (https) {
    // O ESP32 nao valida a cadeia de certificados aqui: sem isto, cada
    // renovacao do certificado do site derrubaria a integracao no meio da
    // apresentacao. Esta limitacao esta registrada na documentacao.
    clienteSeguro.setInsecure();
  }

  HTTPClient http;
  http.setConnectTimeout(BACKEND_TIMEOUT_MS);
  http.setTimeout(BACKEND_TIMEOUT_MS);
  http.setReuse(false);

  const bool aberto = https ? http.begin(clienteSeguro, url) : http.begin(clienteSimples, url);
  if (!aberto) {
    return -1000;
  }

  http.addHeader("X-Dispositivo", DISPOSITIVO_CODIGO);
  http.addHeader("X-Chave", DISPOSITIVO_CHAVE);

  int codigo;
  if (post) {
    http.addHeader("Content-Type", "application/json");
    codigo = http.POST(corpo);
  } else {
    codigo = http.GET();
  }

  if (codigo > 0) {
    resposta = http.getString();
  }

  http.end();
  return codigo;
}

void anotarResultadoHttp(int codigo) {
  if (!travar()) {
    return;
  }

  estado.ultimoHttp = codigo;

  if (codigo >= 200 && codigo < 300) {
    estado.backend = BACKEND_OK;
    estado.ultimoContatoMs = millis();
  } else {
    estado.backend = BACKEND_ERRO_HTTP;
  }

  destravar();
}

/* ---------------------------------------------------------------------
   GET /dispositivo/autorizacao.php
   --------------------------------------------------------------------- */

bool consultarAutorizacao() {
  // Telemetria de presenca junto com a consulta: MAC, versao e a porta em
  // que este equipamento serve a pagina de monitoramento. A porta e o unico
  // dado que o site nao conseguiria obter de outro jeito - o IP ele aprende
  // da origem desta propria requisicao, mas a porta de origem e efemera.
  String url = String(BACKEND_URL) + "/dispositivo/autorizacao.php?mac=" + WiFi.macAddress()
             + "&firmware=" + FIRMWARE
             + "&porta=" + String(DISPOSITIVO_PORTA_HTTP);

  String resposta;
  const int codigo = requisicao(url, false, "", resposta);
  anotarResultadoHttp(codigo);

  static bool falhando = false;
  static unsigned long ultimoAviso = 0;
  if (codigo != 200) {
    if (!falhando || millis() - ultimoAviso >= 60000) {
      registrar(LOG_WARN, "NUVEM", "Consulta de autorizacao falhou (HTTP %d).", codigo);
      ultimoAviso = millis();
    }
    falhando = true;
    return false;
  }

  if (falhando) registrar(LOG_INFO, "NUVEM", "Consulta de autorizacao restabelecida.");
  falhando = false;

  char sessao[33] = "";
  const bool temSessao = extrairTexto(resposta, "sessao", sessao, sizeof(sessao));

  if (!travar()) {
    return true;
  }

  if (!temSessao) {
    // Nenhuma autorizacao pendente. Solta a sessao antiga, mas guarda o
    // ultimo resultado para a pagina local ainda poder mostra-lo.
    if (estado.sessao == SESSAO_AUTORIZADA || estado.sessao == SESSAO_CONCLUIDA) {
      estado.sessao = SESSAO_OCIOSA;
      estado.codigoSessao[0] = '\0';
      estado.chaveConclusao[0] = '\0';
    }
    destravar();
    return true;
  }

  const bool novaSessao = strcmp(estado.codigoSessao, sessao) != 0;

  snprintf(estado.codigoSessao, sizeof(estado.codigoSessao), "%s", sessao);
  extrairTexto(resposta, "chave_conclusao", estado.chaveConclusao, sizeof(estado.chaveConclusao));
  extrairTexto(resposta, "pilha_nome", estado.pilhaNome, sizeof(estado.pilhaNome));

  float numero = 0;
  if (extrairNumero(resposta, "pontos_por_grama", numero)) {
    estado.pontosPorGrama = numero;
  }
  if (extrairNumero(resposta, "segundos_restantes", numero)) {
    estado.segundosRestantes = (int) numero;
  }

  if (estado.sessao != SESSAO_EM_ANDAMENTO && estado.sessao != SESSAO_ENVIANDO) {
    estado.sessao = SESSAO_AUTORIZADA;
  }

  const int segundos = estado.segundosRestantes;
  destravar();

  if (novaSessao) {
    registrar(LOG_INFO, "NUVEM", "Autorizacao recebida; %d s para descartar.", segundos);
  }

  return true;
}

/* ---------------------------------------------------------------------
   POST /dispositivo/evento.php
   --------------------------------------------------------------------- */

bool enviarEtapa() {
  char sessao[33];
  char etapa[24];
  float distancia;
  float peso;
  bool metal;

  if (!travar()) {
    return false;
  }

  snprintf(sessao, sizeof(sessao), "%s", estado.codigoSessao);
  snprintf(etapa, sizeof(etapa), "%s", estado.etapaNome);
  distancia = estado.etapaDistancia;
  peso = estado.etapaPeso;
  metal = estado.etapaMetal;
  estado.etapaPendente = false;
  destravar();

  if (strlen(sessao) == 0) {
    return false;
  }

  String corpo = "{\"sessao\":\"";
  corpo += sessao;
  corpo += "\",\"etapa\":\"";
  corpo += etapa;
  corpo += "\",\"distancia_cm\":";
  corpo += String(distancia, 1);
  corpo += ",\"metal\":";
  corpo += metal ? "true" : "false";
  corpo += ",\"peso_g\":";
  corpo += String(peso, 1);
  corpo += "}";

  String resposta;
  const int codigo = requisicao(String(BACKEND_URL) + "/dispositivo/evento.php", true, corpo, resposta);
  anotarResultadoHttp(codigo);

  if (codigo != 200) {
    // Telemetria perdida nao e grave: a conclusao e que vale pontos.
    registrar(LOG_WARN, "NUVEM", "Etapa \"%s\" nao registrada (HTTP %d).", etapa, codigo);
    return false;
  }

  return true;
}

/* ---------------------------------------------------------------------
   POST /dispositivo/concluir.php
   --------------------------------------------------------------------- */

bool enviarConclusao() {
  char sessao[33];
  char chave[33];
  char resultado[41];
  float peso;
  bool metal;
  int tentativas;

  if (!travar()) {
    return false;
  }

  snprintf(sessao, sizeof(sessao), "%s", estado.codigoSessao);
  snprintf(chave, sizeof(chave), "%s", estado.chaveConclusao);
  snprintf(resultado, sizeof(resultado), "%s", estado.conclusaoResultado);
  peso = estado.conclusaoPeso;
  metal = estado.conclusaoMetal;
  estado.conclusaoTentativas++;
  tentativas = estado.conclusaoTentativas;
  destravar();

  if (strlen(sessao) == 0 || strlen(chave) == 0) {
    if (travar()) {
      estado.conclusaoPendente = false;
      estado.sessao = SESSAO_OCIOSA;
      destravar();
    }
    registrar(LOG_ERRO, "NUVEM", "Conclusao sem sessao valida; descartada.");
    return false;
  }

  String corpo = "{\"sessao\":\"";
  corpo += sessao;
  corpo += "\",\"chave_conclusao\":\"";
  corpo += chave;
  corpo += "\",\"peso_g\":";
  corpo += String(peso, 3);
  corpo += ",\"metal\":";
  corpo += metal ? "true" : "false";
  corpo += ",\"resultado\":\"";
  corpo += resultado;
  corpo += "\"}";

  String resposta;
  const int codigo = requisicao(String(BACKEND_URL) + "/dispositivo/concluir.php", true, corpo, resposta);
  anotarResultadoHttp(codigo);

  // 200 = o backend decidiu (creditou ou recusou). Em ambos os casos a
  // conclusao foi entregue e nao deve ser reenviada.
  if (codigo == 200) {
    const bool creditado = extrairBooleano(resposta, "creditado");

    float numero = 0;
    const int pontos = extrairNumero(resposta, "pontos", numero) ? (int) numero : 0;
    const int descarte = extrairNumero(resposta, "descarte", numero) ? (int) numero : 0;

    char motivo[81] = "";
    extrairTexto(resposta, "motivo", motivo, sizeof(motivo));

    if (travar()) {
      estado.conclusaoPendente = false;
      estado.conclusaoTentativas = 0;
      estado.sessao = SESSAO_CONCLUIDA;
      estado.ultimoCreditado = creditado;
      estado.ultimosPontos = pontos;
      estado.ultimoDescarte = descarte;
      snprintf(estado.ultimoMotivo, sizeof(estado.ultimoMotivo), "%s", motivo);
      destravar();
    }

    if (creditado) {
      registrar(LOG_INFO, "NUVEM", "Descarte %d registrado: %d pontos creditados.", descarte, pontos);
    } else {
      registrar(LOG_WARN, "NUVEM", "Descarte recusado pelo backend; sem pontos.");
    }

    return true;
  }

  // 4xx = o backend recusou de forma definitiva (sessao expirada, chave
  // errada, peso fora da faixa). Reenviar nao muda nada.
  const bool definitivo = codigo >= 400 && codigo < 500;

  if (definitivo || tentativas >= CONCLUSAO_MAX_TENTATIVAS) {
    if (travar()) {
      estado.conclusaoPendente = false;
      estado.conclusaoTentativas = 0;
      estado.sessao = SESSAO_OCIOSA;
      estado.ultimoCreditado = false;
      snprintf(estado.ultimoMotivo, sizeof(estado.ultimoMotivo),
               definitivo ? "Backend recusou (HTTP %d)." : "Sem resposta do backend (HTTP %d).", codigo);
      destravar();
    }

    registrar(LOG_ERRO, "NUVEM", "Conclusao abandonada depois de %d tentativa(s) (HTTP %d).", tentativas, codigo);
    return false;
  }

  registrar(LOG_WARN, "NUVEM", "Conclusao falhou (HTTP %d); tentativa %d de %d.",
            codigo, tentativas, CONCLUSAO_MAX_TENTATIVAS);
  return false;
}

/* ---------------------------------------------------------------------
   Task do core 0

   Enquanto esta task espera o backend, o core 1 segue atendendo o
   servidor HTTP local. E isto que garante que uma queda de internet nao
   torna o ESP32 inutilizavel.
   --------------------------------------------------------------------- */

void tarefaNuvem(void *) {
  unsigned long intervalo = BACKEND_INTERVALO_MS;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      if (travar()) {
        estado.backend = BACKEND_SEM_REDE;
        destravar();
      }
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    bool conclusao = false;
    bool etapa = false;

    if (travar()) {
      conclusao = estado.conclusaoPendente;
      etapa = estado.etapaPendente;
      destravar();
    }

    bool sucesso;

    if (conclusao) {
      sucesso = enviarConclusao();
    } else if (etapa) {
      sucesso = enviarEtapa();
    } else {
      sucesso = consultarAutorizacao();
    }

    // Backoff: backend fora do ar nao vira tempestade de requisicoes.
    if (sucesso) {
      intervalo = BACKEND_INTERVALO_MS;
    } else {
      intervalo *= 2;
      if (intervalo > BACKEND_INTERVALO_MAXIMO_MS) {
        intervalo = BACKEND_INTERVALO_MAXIMO_MS;
      }
    }

    // Conclusao pendente merece pressa: nao espera o backoff cheio.
    if (conclusao && intervalo > 2000) {
      intervalo = 2000;
    }

    vTaskDelay(pdMS_TO_TICKS(intervalo));
  }
}

}  // namespace

/* =====================================================================
   API publica
   ===================================================================== */

void nuvemIniciar() {
  if (trava == nullptr) {
    trava = xSemaphoreCreateMutex();
  }

  if (!integracaoConfigurada()) {
    estado.backend = BACKEND_NAO_CONFIGURADO;
    registrar(LOG_WARN, "NUVEM",
              "Integracao desligada: preencha src/config_backend.h. O servidor local segue normal.");
    return;
  }

  estado.backend = BACKEND_SEM_REDE;

  // Core 0: o core 1 fica livre para o loop() e o servidor HTTP.
  // 8 KB de pilha porque HTTPS consome bem mais que HTTP.
  const BaseType_t criada = xTaskCreatePinnedToCore(
      tarefaNuvem, "nuvem", 8192, nullptr, 1, nullptr, 0);

  if (criada != pdPASS) {
    estado.backend = BACKEND_ERRO_HTTP;
    registrar(LOG_ERRO, "NUVEM", "Nao foi possivel criar a task da nuvem.");
    return;
  }

  registrar(LOG_INFO, "NUVEM", "Task da nuvem ativa no core 0, polling a cada %d ms.", BACKEND_INTERVALO_MS);
}

bool nuvemConfigurada() {
  return integracaoConfigurada();
}

bool nuvemTemAutorizacao() {
  if (!travar()) {
    return false;
  }

  const bool autorizada = (estado.sessao == SESSAO_AUTORIZADA || estado.sessao == SESSAO_EM_ANDAMENTO);
  destravar();

  return autorizada;
}

void nuvemAvisarEtapa(const char *etapa, float distanciaCm, bool metal, float pesoG) {
  if (!integracaoConfigurada() || !travar()) {
    return;
  }

  if (estado.sessao == SESSAO_AUTORIZADA || estado.sessao == SESSAO_EM_ANDAMENTO) {
    if (strcmp(etapa, "objeto_detectado") == 0) {
      estado.sessao = SESSAO_EM_ANDAMENTO;
    }

    snprintf(estado.etapaNome, sizeof(estado.etapaNome), "%s", etapa);
    estado.etapaDistancia = distanciaCm;
    estado.etapaMetal = metal;
    estado.etapaPeso = pesoG;
    estado.etapaPendente = true;
  }

  destravar();
}

void nuvemConcluir(float pesoG, bool metal, const char *resultado) {
  if (!integracaoConfigurada()) {
    return;
  }

  if (!travar()) {
    return;
  }

  const bool temSessao = (estado.sessao == SESSAO_AUTORIZADA || estado.sessao == SESSAO_EM_ANDAMENTO);

  if (temSessao) {
    estado.conclusaoPeso = pesoG;
    estado.conclusaoMetal = metal;
    snprintf(estado.conclusaoResultado, sizeof(estado.conclusaoResultado), "%s", resultado);
    estado.conclusaoTentativas = 0;
    estado.conclusaoPendente = true;
    estado.etapaPendente = false;  // a conclusao substitui a telemetria
    estado.sessao = SESSAO_ENVIANDO;
  }

  destravar();

  if (temSessao) {
    registrar(LOG_INFO, "NUVEM", "Conclusao enfileirada: %.1f g, %s.", pesoG, metal ? "metal" : "nao-metal");
  } else {
    // Sem sessao a maquina trabalhou normalmente, so nao houve a quem
    // creditar. Isso e esperado, nao e erro.
    registrar(LOG_INFO, "NUVEM", "Ciclo sem sessao autorizada: nada a pontuar.");
  }
}

const char *nuvemEstadoBackendTexto() {
  if (!travar()) {
    return "indefinido";
  }

  const EstadoBackend valor = estado.backend;
  destravar();

  switch (valor) {
    case BACKEND_OK:         return "ok";
    case BACKEND_SEM_REDE:   return "sem_rede";
    case BACKEND_ERRO_HTTP:  return "erro_http";
    default:                 return "nao_configurado";
  }
}

const char *nuvemEstadoSessaoTexto() {
  if (!travar()) {
    return "indefinido";
  }

  const EstadoSessao valor = estado.sessao;
  destravar();

  switch (valor) {
    case SESSAO_AUTORIZADA:   return "autorizada";
    case SESSAO_EM_ANDAMENTO: return "em_andamento";
    case SESSAO_ENVIANDO:     return "enviando";
    case SESSAO_CONCLUIDA:    return "concluida";
    default:                  return "ocioso";
  }
}

long nuvemSegundosDesdeContato() {
  if (!travar()) {
    return -1;
  }

  const unsigned long contato = estado.ultimoContatoMs;
  destravar();

  if (contato == 0) {
    return -1;
  }

  return (long) ((millis() - contato) / 1000);
}

String nuvemJson() {
  char sessaoParcial[13] = "";
  int pontos = 0;
  int descarte = 0;
  int segundos = 0;
  float pontosPorGrama = 0;
  bool creditado = false;
  bool pendente = false;
  char pilha[41] = "";
  char motivo[81] = "";

  if (travar()) {
    // So os 12 primeiros caracteres do codigo da sessao: o suficiente para
    // conferir na tela, sem publicar o identificador inteiro na rede local.
    snprintf(sessaoParcial, sizeof(sessaoParcial), "%s", estado.codigoSessao);
    snprintf(pilha, sizeof(pilha), "%s", estado.pilhaNome);
    snprintf(motivo, sizeof(motivo), "%s", estado.ultimoMotivo);
    pontos = estado.ultimosPontos;
    descarte = estado.ultimoDescarte;
    segundos = estado.segundosRestantes;
    pontosPorGrama = estado.pontosPorGrama;
    creditado = estado.ultimoCreditado;
    pendente = estado.conclusaoPendente;
    destravar();
  }

  String j = "{";

  j += "\"sessao_estado\":\"";
  j += nuvemEstadoSessaoTexto();

  j += "\",\"sessao_parcial\":\"";
  j += sessaoParcial;

  j += "\",\"pilha\":\"";
  j += pilha;

  j += "\",\"pontos_por_grama\":";
  j += String(pontosPorGrama, 3);

  j += ",\"segundos_restantes\":";
  j += segundos;

  j += ",\"conclusao_pendente\":";
  j += pendente ? "true" : "false";

  j += ",\"ultimo_descarte\":";
  j += descarte;

  j += ",\"ultimos_pontos\":";
  j += pontos;

  j += ",\"ultimo_creditado\":";
  j += creditado ? "true" : "false";

  j += ",\"ultimo_motivo\":\"";
  j += motivo;

  j += "\",\"backend_estado\":\"";
  j += nuvemEstadoBackendTexto();

  j += "\",\"backend_configurado\":";
  j += integracaoConfigurada() ? "true" : "false";

  j += ",\"backend_ultimo_contato_s\":";
  j += nuvemSegundosDesdeContato();

  j += "}";

  return j;
}

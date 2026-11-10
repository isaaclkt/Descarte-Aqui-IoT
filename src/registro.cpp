#include "registro.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>
#include <esp_timer.h>
#include <memory>
#include <new>

namespace {

struct Linha {
  uint64_t ms;
  time_t epoch;
  NivelLog nivel;
  char origem[12];
  char texto[REGISTRO_TAMANHO_LINHA];
};

Linha buffer[REGISTRO_CAPACIDADE];
int proximo = 0;        // onde a proxima linha sera escrita
int gravadas = 0;       // quantas linhas validas existem (satura na capacidade)

SemaphoreHandle_t trava = nullptr;
QueueHandle_t fila = nullptr;
SemaphoreHandle_t historicoTrava = nullptr;
portMUX_TYPE contadoresTrava = portMUX_INITIALIZER_UNLOCKED;
uint32_t perdidos = 0;
enum EstadoFS { INICIANDO, ATIVO, FALHOU };
EstadoFS estadoFS = INICIANDO; // protegido por historicoTrava
bool pedido = false, pronto = false, processando = false;
unsigned paginaOffset = 0, paginaLimite = 20;
unsigned long paginaMs = 0;
String paginaJson;
constexpr size_t LIMITE_ARQUIVO = 16384;
constexpr int ARQUIVOS = 4;

void perder() {
  portENTER_CRITICAL(&contadoresTrava);
  ++perdidos;
  portEXIT_CRITICAL(&contadoresTrava);
}
uint32_t perdas() {
  portENTER_CRITICAL(&contadoresTrava);
  uint32_t n = perdidos;
  portEXIT_CRITICAL(&contadoresTrava);
  return n;
}

const char *nomeNivel(NivelLog nivel) {
  switch (nivel) {
    case LOG_WARN: return "WARN";
    case LOG_ERRO: return "ERROR";
    default:       return "INFO";
  }
}

// Escapa o que quebraria o JSON de /logs. Mantemos simples de proposito:
// as mensagens sao escritas por nos, nao vem de fora.
void anexarEscapado(String &destino, const char *texto) {
  for (const char *p = texto; *p; ++p) {
    switch (*p) {
      case '"':  destino += "\\\""; break;
      case '\\': destino += "\\\\"; break;
      case '\n': destino += "\\n";  break;
      case '\r': break;
      default:
        if ((unsigned char) *p < 0x20) {
          destino += ' ';
        } else {
          destino += *p;
        }
    }
  }
}

String linhaJson(const Linha &linha) {
  String j;
  j.reserve(400);
  char uptime[24];
  snprintf(uptime, sizeof(uptime), "%llu", (unsigned long long) linha.ms);
  j += "{\"ms\":"; j += uptime;
  j += ",\"data_hora\":";
  if (linha.epoch >= 1704067200) {
    struct tm local;
    localtime_r(&linha.epoch, &local);
    char data[40];
    strftime(data, sizeof(data), "%Y-%m-%dT%H:%M:%S%z", &local);
    j += '"'; j += data; j += '"';
  } else j += "null";
  j += ",\"nivel\":\""; j += nomeNivel(linha.nivel);
  j += "\",\"origem\":\""; anexarEscapado(j, linha.origem);
  j += "\",\"texto\":\""; anexarEscapado(j, linha.texto);
  j += "\"}";
  return j;
}

String caminho(int i) { return String("/eventos") + i + ".jsonl"; }

// Somente esta task toca no FS: nenhuma trava de RAM fica tomada durante I/O.
bool gravar(const String &lote) {
  File atual = LittleFS.open(caminho(0), FILE_READ);
  size_t tamanho = atual ? atual.size() : 0;
  atual.close();
  if (tamanho + lote.length() + 1 > LIMITE_ARQUIVO) {
    if (LittleFS.exists(caminho(ARQUIVOS - 1)) &&
        !LittleFS.remove(caminho(ARQUIVOS - 1))) return false;
    for (int i = ARQUIVOS - 2; i >= 0; --i) {
      if (LittleFS.exists(caminho(i)) && !LittleFS.rename(caminho(i), caminho(i + 1))) return false;
    }
  }
  File arquivo = LittleFS.open(caminho(0), FILE_APPEND);
  if (!arquivo) return false;
  bool ok = arquivo.print("\n") == 1;
  ok = (arquivo.print(lote) == lote.length()) && ok;
  arquivo.flush();
  arquivo.close();
  return ok;
}

String lerPagina(unsigned offset, unsigned limite) {
  String j = "{\"linhas\":[";
  j.reserve(16000);
  unsigned indice = 0, incluidos = 0;
  bool mais = false;
  for (int i = ARQUIVOS - 1; i >= 0 && !mais; --i) {
    File f = LittleFS.open(caminho(i), FILE_READ);
    while (f && f.available()) {
      // Limite fixo; ignora cauda incompleta deixada por queda de energia.
      char linha[1024];
      size_t n = 0;
      bool completa = false, excedeu = false;
      while (f.available()) {
        char c = f.read();
        if (c == '\n') { completa = true; break; }
        if (n < sizeof(linha) - 1) linha[n++] = c; else excedeu = true;
      }
      linha[n] = 0;
      if (!completa || excedeu || n < 2 || linha[0] != '{' || linha[n-1] != '}') continue;
      if (indice++ < offset) continue;
      if (incluidos == limite) { mais = true; break; }
      if (incluidos++) j += ',';
      j += linha;
      taskYIELD();
    }
    f.close();
  }
  j += "],\"offset\":"; j += offset;
  j += ",\"limite\":"; j += limite;
  j += ",\"retornados\":"; j += incluidos;
  j += ",\"proximo_offset\":";
  if (mais) j += offset + incluidos; else j += "null";
  j += ",\"descartados_boot\":"; j += perdas();
  j += '}';
  return j;
}

void gravador(void *) {
  bool ok = LittleFS.begin(false); // Nunca formatar dados existentes.
  xSemaphoreTake(historicoTrava, portMAX_DELAY);
  estadoFS = ok ? ATIVO : FALHOU;
  xSemaphoreGive(historicoTrava);
  registrar(ok ? LOG_INFO : LOG_ERRO, "LITTLEFS", ok ? "Persistencia ativa." : "Montagem falhou; somente RAM e Serial, sem formatacao.");
  bool ntpConfigurado = false, relogioValido = false;
  for (;;) {
    if (!ntpConfigurado && WiFi.status() == WL_CONNECTED) {
      // Regra POSIX de America/Sao_Paulo: UTC-3, sem horario de verao.
      configTzTime("<-03>3", "pool.ntp.org", "time.nist.gov");
      ntpConfigurado = true;
    }
    if (!relogioValido && time(nullptr) >= 1704067200) {
      relogioValido = true;
      registrar(LOG_INFO, "RELOGIO", "Relogio sincronizado; fuso America/Sao_Paulo.");
    }
    String lote;
    lote.reserve(4096);
    Linha linha;
    unsigned recebidos = 0;
    while (recebidos < 8 && xQueueReceive(fila, &linha, 0) == pdTRUE) {
      lote += linhaJson(linha); lote += '\n'; ++recebidos;
    }
    if (recebidos && (!ok || !gravar(lote))) {
      for (unsigned i = 0; i < recebidos; ++i) perder();
      if (ok) {
        ok = false; // Nao repetir escritas em flash com erro.
        xSemaphoreTake(historicoTrava, portMAX_DELAY);
        estadoFS = FALHOU;
        xSemaphoreGive(historicoTrava);
        registrar(LOG_ERRO, "LITTLEFS", "Falha de escrita; persistencia suspensa ate reiniciar.");
      }
    }
    unsigned offset = 0, limite = 0;
    xSemaphoreTake(historicoTrava, portMAX_DELAY);
    bool consultar = pedido;
    if (consultar) { offset = paginaOffset; limite = paginaLimite; pedido = false; processando = true; }
    xSemaphoreGive(historicoTrava);
    if (consultar && ok) {
      String resultado = lerPagina(offset, limite);
      xSemaphoreTake(historicoTrava, portMAX_DELAY);
      paginaJson = resultado; paginaMs = millis(); pronto = true; processando = false;
      xSemaphoreGive(historicoTrava);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

bool pegarTrava() {
  if (trava == nullptr) {
    return false;
  }
  return xSemaphoreTake(trava, 0) == pdTRUE;
}

void soltarTrava() {
  if (trava != nullptr) {
    xSemaphoreGive(trava);
  }
}

}  // namespace

void registroIniciar() {
  if (trava == nullptr) {
    trava = xSemaphoreCreateMutex();
  }
  if (fila != nullptr) return;
  historicoTrava = xSemaphoreCreateMutex();
  fila = xQueueCreate(64, sizeof(Linha));
  if (!trava || !historicoTrava || !fila ||
      xTaskCreate(gravador, "registroFS", 6144, nullptr, 1, nullptr) != pdPASS) {
    Serial.println("[ERROR][LITTLEFS] Recursos insuficientes; persistencia indisponivel.");
    if (historicoTrava) estadoFS = FALHOU;
  }
}

void registrar(NivelLog nivel, const char *origem, const char *formato, ...) {
  char texto[REGISTRO_TAMANHO_LINHA];

  va_list argumentos;
  va_start(argumentos, formato);
  vsnprintf(texto, sizeof(texto), formato, argumentos);
  va_end(argumentos);

  // O Serial continua sendo a saida principal durante a montagem e a defesa.
  Serial.printf("[%s][%s] %s\n", nomeNivel(nivel), origem, texto);

  Linha linha{};
  linha.ms = esp_timer_get_time() / 1000ULL;
  linha.epoch = time(nullptr);
  linha.nivel = nivel;
  snprintf(linha.origem, sizeof(linha.origem), "%s", origem);
  snprintf(linha.texto, sizeof(linha.texto), "%s", texto);
  // Nenhum produtor espera por flash ou por espaco na fila.
  if (!fila || xQueueSend(fila, &linha, 0) != pdTRUE) perder();
  if (!pegarTrava()) return;
  buffer[proximo] = linha;
  proximo = (proximo + 1) % REGISTRO_CAPACIDADE;
  if (gravadas < REGISTRO_CAPACIDADE) ++gravadas;
  soltarTrava();
}

String registroJson() {
  String j = "{\"linhas\":[";

  if (!pegarTrava()) {
    j += "],\"erro\":\"registro ocupado\"}";
    return j;
  }

  std::unique_ptr<Linha[]> copia(new (std::nothrow) Linha[REGISTRO_CAPACIDADE]);
  if (!copia) {
    soltarTrava();
    return "{\"linhas\":[],\"erro\":\"memoria insuficiente\"}";
  }
  int total = gravadas;
  memcpy(copia.get(), buffer, sizeof(buffer));
  int cursor = proximo;
  soltarTrava();

  // Comeca na linha mais antiga ainda viva.
  int inicio = (total < REGISTRO_CAPACIDADE) ? 0 : cursor;

  for (int i = 0; i < total; i++) {
    const Linha &linha = copia[(inicio + i) % REGISTRO_CAPACIDADE];

    if (i > 0) {
      j += ',';
    }

    j += linhaJson(linha);
  }

  j += "],\"total\":";
  j += total;
  j += ",\"capacidade\":";
  j += REGISTRO_CAPACIDADE;
  j += "}";

  return j;
}

int registroHistoricoJson(unsigned offset, unsigned limite, String &resposta) {
  if (!historicoTrava || xSemaphoreTake(historicoTrava, 0) != pdTRUE) {
    resposta = "{\"erro\":\"registro ocupado\"}"; return 503;
  }
  int codigo;
  limite = constrain(limite, 1U, 20U);
  if (estadoFS == FALHOU) {
    resposta = "{\"erro\":\"persistencia indisponivel\"}"; codigo = 503;
  } else if (pronto && paginaOffset == offset && paginaLimite == limite && millis() - paginaMs < 5000) {
    resposta = paginaJson; codigo = 200;
  } else {
    if (!pedido && !processando && (!pronto || millis() - paginaMs >= 5000)) {
      paginaOffset = offset; paginaLimite = limite; pronto = false; pedido = true;
    } else if (!processando && pronto && (paginaOffset != offset || paginaLimite != limite)) {
      paginaOffset = offset; paginaLimite = limite; pronto = false; pedido = true;
    }
    resposta = "{\"pendente\":true,\"repetir_apos_ms\":1000}"; codigo = 202;
  }
  xSemaphoreGive(historicoTrava);
  return codigo;
}

#include "registro.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {

struct Linha {
  unsigned long ms;
  NivelLog nivel;
  char origem[12];
  char texto[REGISTRO_TAMANHO_LINHA];
};

Linha buffer[REGISTRO_CAPACIDADE];
int proximo = 0;        // onde a proxima linha sera escrita
int gravadas = 0;       // quantas linhas validas existem (satura na capacidade)

SemaphoreHandle_t trava = nullptr;

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

bool pegarTrava() {
  if (trava == nullptr) {
    return false;
  }
  return xSemaphoreTake(trava, pdMS_TO_TICKS(50)) == pdTRUE;
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
  proximo = 0;
  gravadas = 0;
}

void registrar(NivelLog nivel, const char *origem, const char *formato, ...) {
  char texto[REGISTRO_TAMANHO_LINHA];

  va_list argumentos;
  va_start(argumentos, formato);
  vsnprintf(texto, sizeof(texto), formato, argumentos);
  va_end(argumentos);

  // O Serial continua sendo a saida principal durante a montagem e a defesa.
  Serial.printf("[%s][%s] %s\n", nomeNivel(nivel), origem, texto);

  // Se o mutex estiver ocupado, preferimos perder a linha do buffer a travar
  // o ciclo fisico ou a task da nuvem esperando por log.
  if (!pegarTrava()) {
    return;
  }

  Linha &linha = buffer[proximo];
  linha.ms = millis();
  linha.nivel = nivel;
  snprintf(linha.origem, sizeof(linha.origem), "%s", origem);
  snprintf(linha.texto, sizeof(linha.texto), "%s", texto);

  proximo = (proximo + 1) % REGISTRO_CAPACIDADE;
  if (gravadas < REGISTRO_CAPACIDADE) {
    gravadas++;
  }

  soltarTrava();
}

String registroJson() {
  String j = "{\"linhas\":[";

  if (!pegarTrava()) {
    j += "],\"erro\":\"registro ocupado\"}";
    return j;
  }

  // Comeca na linha mais antiga ainda viva.
  int inicio = (gravadas < REGISTRO_CAPACIDADE)
                   ? 0
                   : proximo;

  for (int i = 0; i < gravadas; i++) {
    const Linha &linha = buffer[(inicio + i) % REGISTRO_CAPACIDADE];

    if (i > 0) {
      j += ',';
    }

    j += "{\"ms\":";
    j += linha.ms;
    j += ",\"nivel\":\"";
    j += nomeNivel(linha.nivel);
    j += "\",\"origem\":\"";
    anexarEscapado(j, linha.origem);
    j += "\",\"texto\":\"";
    anexarEscapado(j, linha.texto);
    j += "\"}";
  }

  int total = gravadas;
  soltarTrava();

  j += "],\"total\":";
  j += total;
  j += ",\"capacidade\":";
  j += REGISTRO_CAPACIDADE;
  j += "}";

  return j;
}

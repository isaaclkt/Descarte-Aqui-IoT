/* =====================================================================
   DESCARTE AQUI - Sistema inteligente para descarte de residuos
   TDE - Conectividade em Sistemas Ciberfisicos (PUCPR)

   O ESP32 funciona como SERVIDOR HTTP na rede Wi-Fi local.
   O navegador acessa http://<ip-do-esp32>/ e consulta os sensores
   e comanda o atuador sem depender de nenhum servico externo.

   Fluxo:   NAVEGADOR  <--HTTP-->  ESP32  <-->  SENSORES / ATUADORES

   A partir da integracao com o site, existe um SEGUNDO fluxo, opcional e
   independente deste:

            ESP32  --HTTP de saida-->  backend PHP  -->  banco

   Ele roda numa task separada (src/nuvem.cpp, core 0) e serve para creditar
   pontos ao usuario que escaneou o QR Code. O servidor local continua no
   core 1 e NAO depende dele: sem internet, sem backend ou sem
   src/config_backend.h, tudo aqui continua funcionando igual.
   ===================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include <ESP32Servo.h>

#include "pagina_web.h"   // HTML/CSS/JS embutidos (gerado por tools/gerar_pagina_web.py)
#include "registro.h"     // log INFO/WARN/ERROR, servido tambem em GET /logs
#include "nuvem.h"        // cliente do backend PHP (task propria no core 0)

/* =====================================================================
   1) CONFIGURACAO DO WI-FI

   O SSID e a senha ficam em src/config_wifi.h, que NAO vai para o GitHub
   (esta no .gitignore). Antes da primeira compilacao, cada integrante copia
   o modelo e preenche com os dados da rede:

       copy src\config_wifi.exemplo.h src\config_wifi.h
   ===================================================================== */

#if __has_include("config_wifi.h")
  #include "config_wifi.h"
#else
  #error "Falta src/config_wifi.h - copie src/config_wifi.exemplo.h para src/config_wifi.h e preencha o SSID e a senha."
#endif

// Tempo maximo esperando o Wi-Fi conectar antes de seguir mesmo assim.
#define TIMEOUT_WIFI_MS 20000

// De quanto em quanto tempo verificamos se o Wi-Fi caiu.
#define INTERVALO_RECONEXAO_MS 10000

/* =====================================================================
   2) PINAGEM DO HARDWARE
   ===================================================================== */

// Display OLED SSD1306 (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ENDERECO 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Balanca HX711 + celula de carga
#define HX711_DT 16
#define HX711_SCK 4

HX711 balanca;

// Fator de calibracao da balanca.
// Para calibrar: coloque um peso conhecido, veja "peso_bruto" em /sensor e
// divida esse valor pelo peso real em gramas. Ajuste a constante abaixo.
#define FATOR_CALIBRACAO 420.0

// Sensor ultrassonico HC-SR04
#define TRIG_PIN 19
#define ECHO_PIN 18
#define DISTANCIA_DETECCAO 5     // cm - abaixo disso consideramos que ha objeto

// Sensor indutivo de metal (LOW = metal detectado)
#define SENSOR_METAL 32

// Servo que desvia o objeto para o lado certo
#define SERVO_SEPARACAO_PIN 25
#define SERVO_NAO_METAL 20
#define SERVO_METAL 160
#define SERVO_INICIAL 90

Servo servoSeparacao;

// Servo da portinha de retirada do nao-metal
// (este e o atuador comandado pela rota /atuador)
#define SERVO_PORTA_PIN 26
#define PORTA_FECHADA 0
#define PORTA_ABERTA 90

Servo servoPorta;

// Tempos do ciclo de separacao
#define TEMPO_ESPERA 4000
#define TEMPO_PORTA_ABERTA 5000
#define TEMPO_ESTABILIZACAO 3000

/* =====================================================================
   3) ESTADO GLOBAL - e isto que as rotas HTTP publicam
   ===================================================================== */

WebServer server(80);            // servidor HTTP escutando na porta 80

const char *FIRMWARE = "0.2.0";

// Leituras dos sensores
float distanciaCm = -1;
bool  objetoPresente = false;
bool  metalDetectado = false;
long  pesoBruto = 0;
float pesoGramas = 0;

// Saude dos perifericos
bool oledOk = false;
bool balancaOk = false;

// Estado dos atuadores
int  posicaoServoSeparacao = SERVO_INICIAL;
bool portaAberta = false;

// Estado do sistema mostrado na interface
const char *estadoSistema = "Iniciando";

bool wifiEstavaConectado = false;

// Quantas vezes tentamos reconectar desde que a placa ligou. Vai para /rede:
// e a prova, na apresentacao, de que a reconexao automatica esta agindo.
unsigned long tentativasReconexao = 0;

unsigned long ultimaLeituraMs = 0;
unsigned long ultimaVerificacaoWiFi = 0;

/* =====================================================================
   4) AUXILIARES
   ===================================================================== */

// Espera sem travar o servidor.
// Trocamos todos os delay() do ciclo de separacao por esta funcao: assim o
// ESP32 continua respondendo ao navegador mesmo durante os 4-5 segundos em
// que os servos estao trabalhando.
void aguardar(unsigned long ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    server.handleClient();
    delay(1);
  }
}

// Endereco em que o servidor responde, seja qual for o modo de rede.
String enderecoIP() {
  return WiFi.localIP().toString();
}

// Escreve ate 4 linhas no OLED. Se o display nao existir, nao faz nada.
void mostrarOled(const char *l1, const char *l2, const char *l3, const char *l4) {
  if (!oledOk) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (l1) { display.setCursor(0, 0);  display.println(l1); }
  if (l2) { display.setCursor(0, 20); display.println(l2); }
  if (l3) { display.setCursor(0, 35); display.println(l3); }
  if (l4) { display.setCursor(0, 50); display.println(l4); }

  display.display();
}

// Dispara o pulso do HC-SR04 e converte o eco em centimetros.
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracao == 0) {
    return -1;                       // nenhum eco voltou dentro de 30 ms
  }

  return duracao * 0.0343 / 2;       // velocidade do som, ida e volta
}

// Move o servo da portinha e atualiza o estado publicado pelas rotas.
void definirPorta(bool abrir) {
  portaAberta = abrir;
  servoPorta.write(abrir ? PORTA_ABERTA : PORTA_FECHADA);
  registrar(LOG_INFO, "ATUADOR", "Porta -> %s (GPIO %d).", abrir ? "ABERTA" : "FECHADA", SERVO_PORTA_PIN);
}

// Move o servo de separacao e guarda a posicao.
void definirSeparacao(int posicao) {
  posicaoServoSeparacao = posicao;
  servoSeparacao.write(posicao);
}

// Converte a leitura bruta do HX711 em gramas, usando a tara (offset) e o
// fator de calibracao (scale) que definimos no setup.
float converterParaGramas(long bruto) {
  return (bruto - balanca.get_offset()) / balanca.get_scale();
}

// Le a balanca SEM travar o loop.
// is_ready() apenas pergunta se ja existe amostra disponivel; se nao existir,
// saimos na hora. Nao usamos get_units() aqui porque ele chama wait_ready()
// internamente e ficaria bloqueado ate a proxima amostra (~100 ms), o que
// seguraria o loop e o atendimento das requisicoes HTTP.
void atualizarPeso() {
  if (!balancaOk || !balanca.is_ready()) return;

  pesoBruto = balanca.read();
  pesoGramas = converterParaGramas(pesoBruto);
}

/* =====================================================================
   5) MONTAGEM DAS RESPOSTAS JSON
   ===================================================================== */

// Observacao: montamos o JSON sempre com +=, nunca com "texto" + String(...).
// Em C++ nao existe operator+ para (const char* + String), entao aquela forma
// nem compilaria. Usar += tambem evita criar Strings temporarias na memoria.

String jsonStatus() {
  String j = "{";
  j += "\"dispositivo\":\"Descarte Aqui - ESP32\",";

  j += "\"firmware\":\"";
  j += FIRMWARE;

  j += "\",\"estado\":\"";
  j += estadoSistema;

  j += "\",\"wifi_conectado\":";
  j += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

  j += ",\"modo_rede\":\"";
  j += "STA";

  j += "\",\"ssid\":\"";
  j += WIFI_SSID;

  j += "\",\"ip\":\"";
  j += enderecoIP();

  // MAC: exigido pelo TDE ("exibicao do IP e MAC do dispositivo").
  j += "\",\"mac\":\"";
  j += WiFi.macAddress();

  j += "\",\"rssi\":";
  j += WiFi.RSSI();

  j += ",\"uptime_s\":";
  j += millis() / 1000;

  j += ",\"heap_livre\":";
  j += ESP.getFreeHeap();

  j += ",\"oled_ok\":";
  j += oledOk ? "true" : "false";

  j += ",\"balanca_ok\":";
  j += balancaOk ? "true" : "false";

  j += ",\"porta\":\"";
  j += portaAberta ? "aberta" : "fechada";

  j += "\",\"servo_separacao\":";
  j += posicaoServoSeparacao;

  // Integracao com o site. Se o backend estiver fora do ar, estes campos
  // mudam de valor - e so isso: nada aqui deixa de responder por causa deles.
  j += ",\"backend_estado\":\"";
  j += nuvemEstadoBackendTexto();

  j += "\",\"backend_configurado\":";
  j += nuvemConfigurada() ? "true" : "false";

  j += ",\"backend_ultimo_contato_s\":";
  j += nuvemSegundosDesdeContato();

  j += ",\"sessao_estado\":\"";
  j += nuvemEstadoSessaoTexto();

  j += "\"}";
  return j;
}

// Estado dos dois servos. O TDE pede um endpoint para "consultar o estado do
// servo"; /status ja mostra o resumo, aqui vem o detalhe com pinos e limites.
String jsonServo() {
  String j = "{\"servos\":[";

  j += "{\"nome\":\"porta\",\"pino\":";
  j += SERVO_PORTA_PIN;
  j += ",\"posicao\":";
  j += portaAberta ? PORTA_ABERTA : PORTA_FECHADA;
  j += ",\"estado\":\"";
  j += portaAberta ? "aberta" : "fechada";
  j += "\",\"minimo\":";
  j += PORTA_FECHADA;
  j += ",\"maximo\":";
  j += PORTA_ABERTA;
  j += "},";

  j += "{\"nome\":\"separacao\",\"pino\":";
  j += SERVO_SEPARACAO_PIN;
  j += ",\"posicao\":";
  j += posicaoServoSeparacao;
  j += ",\"estado\":\"";
  if (posicaoServoSeparacao == SERVO_METAL) {
    j += "metal";
  } else if (posicaoServoSeparacao == SERVO_NAO_METAL) {
    j += "nao_metal";
  } else {
    j += "centro";
  }
  j += "\",\"minimo\":";
  j += SERVO_NAO_METAL;
  j += ",\"maximo\":";
  j += SERVO_METAL;
  j += "}";

  j += "]}";
  return j;
}

// Informacoes de conexao exigidas pelo TDE, num lugar so.
String jsonRede() {
  const bool conectado = (WiFi.status() == WL_CONNECTED);

  String j = "{";

  j += "\"conectado\":";
  j += conectado ? "true" : "false";

  j += ",\"modo\":\"STA\"";

  j += ",\"ssid\":\"";
  j += WIFI_SSID;

  j += "\",\"ip\":\"";
  j += enderecoIP();

  j += "\",\"mac\":\"";
  j += WiFi.macAddress();

  j += "\",\"gateway\":\"";
  j += WiFi.gatewayIP().toString();

  j += "\",\"mascara\":\"";
  j += WiFi.subnetMask().toString();

  j += "\",\"dns\":\"";
  j += WiFi.dnsIP().toString();

  j += "\",\"rssi\":";
  j += WiFi.RSSI();

  j += ",\"canal\":";
  j += WiFi.channel();

  j += ",\"tentativas_reconexao\":";
  j += tentativasReconexao;

  j += ",\"porta_servidor\":80";

  j += "}";
  return j;
}

String jsonSensor() {
  String j = "{";

  j += "\"distancia_cm\":";
  j += String(distanciaCm, 1);

  j += ",\"objeto_presente\":";
  j += objetoPresente ? "true" : "false";

  j += ",\"metal_detectado\":";
  j += metalDetectado ? "true" : "false";

  j += ",\"peso_bruto\":";
  j += pesoBruto;

  j += ",\"peso_g\":";
  j += String(pesoGramas, 1);

  j += ",\"balanca_ok\":";
  j += balancaOk ? "true" : "false";

  j += ",\"atualizado_ms\":";
  j += ultimaLeituraMs;

  j += "}";
  return j;
}

/* =====================================================================
   6) ROTAS HTTP
   ===================================================================== */

// As requisicoes HTTP ficam SO no Serial, de proposito: a pagina consulta
// /status e /sensor a cada 2 s e, se isso entrasse no buffer de /logs, em
// poucos segundos empurraria para fora os eventos que importam (Wi-Fi,
// sensor, servo, sessao de descarte).
void registrarRequisicao() {
  Serial.printf("[HTTP] %s %s  (cliente %s)\n",
                server.method() == HTTP_GET ? "GET" : "OUTRO",
                server.uri().c_str(),
                server.client().remoteIP().toString().c_str());
}

void enviarJson(int codigo, const String &corpo) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(codigo, "application/json", corpo);
}

// GET /  -> pagina principal
void rotaRaiz() {
  registrarRequisicao();
  server.send_P(200, "text/html", PAGINA_HTML);
}

// GET /3d -> visualizacao 3D do ponto de coleta
void rota3d() {
  registrarRequisicao();
  server.send_P(200, "text/html", PAGINA_3D);
}

// GET /style.css
void rotaCss() {
  registrarRequisicao();
  server.send_P(200, "text/css", PAGINA_CSS);
}

// GET /app.js
void rotaJs() {
  registrarRequisicao();
  server.send_P(200, "application/javascript", PAGINA_JS);
}

// GET /status -> estado geral do sistema
void rotaStatus() {
  registrarRequisicao();
  enviarJson(200, jsonStatus());
}

// GET /sensor -> leituras atuais dos sensores
void rotaSensor() {
  registrarRequisicao();
  enviarJson(200, jsonSensor());
}

// GET /atuador?estado=on | off  -> abre ou fecha a portinha
void rotaAtuador() {
  registrarRequisicao();

  if (!server.hasArg("estado")) {
    enviarJson(400, "{\"ok\":false,\"erro\":\"faltou o parametro estado (on ou off)\"}");
    return;
  }

  String estado = server.arg("estado");

  if (estado == "on") {
    definirPorta(true);
  } else if (estado == "off") {
    definirPorta(false);
  } else {
    enviarJson(400, "{\"ok\":false,\"erro\":\"estado invalido, use on ou off\"}");
    return;
  }

  String r = "{\"ok\":true,\"atuador\":\"porta\",\"estado\":\"";
  r += portaAberta ? "aberta" : "fechada";
  r += "\",\"pino\":";
  r += SERVO_PORTA_PIN;
  r += "}";
  enviarJson(200, r);
}

// GET /servo -> estado detalhado dos dois servos
void rotaServo() {
  registrarRequisicao();
  enviarJson(200, jsonServo());
}

// GET /rede -> informacoes da conexao (IP, MAC, gateway, RSSI...)
void rotaRede() {
  registrarRequisicao();
  enviarJson(200, jsonRede());
}

// GET /descarte -> estado do descarte autorizado pelo site
void rotaDescarte() {
  registrarRequisicao();
  enviarJson(200, nuvemJson());
}

// GET /logs -> ultimos eventos registrados (INFO/WARN/ERROR)
void rotaLogs() {
  registrarRequisicao();
  enviarJson(200, registroJson());
}

void rotaNaoEncontrada() {
  registrarRequisicao();
  enviarJson(404, "{\"ok\":false,\"erro\":\"rota nao encontrada\"}");
}

/* =====================================================================
   7) CONEXAO WI-FI
   ===================================================================== */

bool conectarWiFi() {
  Serial.printf("\n[WIFI] Conectando em \"%s\"", WIFI_SSID);
  mostrarOled("DESCARTE AQUI", "Conectando ao", "Wi-Fi...", nullptr);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < TIMEOUT_WIFI_MS) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    registrar(LOG_ERRO, "WIFI", "Sem conexao. Confira o hotspot 2,4 GHz, nome e senha.");
    Serial.println("[WIFI] Continuaremos tentando conectar ao hotspot.");
    mostrarOled("DESCARTE AQUI", "Sem Wi-Fi", "Ative o hotspot", "2,4 GHz");
    return false;
  }

  wifiEstavaConectado = true;
  String ip = WiFi.localIP().toString();

  // IP e MAC no log: os dois dados de identificacao que o TDE pede.
  registrar(LOG_INFO, "WIFI", "Conectado. IP %s, MAC %s.", ip.c_str(), WiFi.macAddress().c_str());
  Serial.print("[WIFI] Acesse no navegador: http://");
  Serial.println(ip);

  mostrarOled("DESCARTE AQUI", "Wi-Fi conectado", ip.c_str(), nullptr);
  return true;
}

/* =====================================================================
   8) SETUP
   ===================================================================== */

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== DESCARTE AQUI - iniciando ===");

  // Antes de qualquer registrar(): prepara o mutex do buffer de log.
  registroIniciar();
  registrar(LOG_INFO, "SETUP", "Firmware %s iniciando.", FIRMWARE);

  // ---------- OLED ----------
  Wire.begin(OLED_SDA, OLED_SCL);
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ENDERECO);
  if (!oledOk) {
    // Antes o codigo parava aqui com while(true). Agora seguimos sem display:
    // o servidor HTTP continua funcionando e a interface mostra oled_ok: false.
    registrar(LOG_WARN, "OLED", "Nao encontrado - seguindo sem display.");
  }

  // ---------- Balanca HX711 ----------
  balanca.begin(HX711_DT, HX711_SCK);
  if (balanca.wait_ready_timeout(1000)) {
    balanca.set_scale(FATOR_CALIBRACAO);
    balanca.tare();
    balancaOk = true;
    registrar(LOG_INFO, "HX711", "Pronta e tarada (fator %.1f).", (double) FATOR_CALIBRACAO);
  } else {
    // wait_ready_timeout evita que tare() trave o ESP32 para sempre
    // caso o HX711 esteja desconectado.
    registrar(LOG_ERRO, "HX711", "Nao respondeu - seguindo sem balanca.");
  }

  // ---------- Ultrassonico ----------
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // ---------- Sensor indutivo ----------
  pinMode(SENSOR_METAL, INPUT_PULLUP);

  // ---------- Servos ----------
  servoSeparacao.attach(SERVO_SEPARACAO_PIN);
  definirSeparacao(SERVO_INICIAL);

  servoPorta.attach(SERVO_PORTA_PIN);
  definirPorta(false);

  // ---------- Wi-Fi ----------
  conectarWiFi();

  // ---------- Rotas e inicio do servidor ----------
  server.on("/", HTTP_GET, rotaRaiz);
  server.on("/3d", HTTP_GET, rota3d);
  server.on("/style.css", HTTP_GET, rotaCss);
  server.on("/app.js", HTTP_GET, rotaJs);
  server.on("/status", HTTP_GET, rotaStatus);
  server.on("/sensor", HTTP_GET, rotaSensor);
  server.on("/atuador", HTTP_GET, rotaAtuador);
  server.on("/servo", HTTP_GET, rotaServo);
  server.on("/rede", HTTP_GET, rotaRede);
  server.on("/descarte", HTTP_GET, rotaDescarte);
  server.on("/logs", HTTP_GET, rotaLogs);
  server.onNotFound(rotaNaoEncontrada);

  server.begin();
  registrar(LOG_INFO, "HTTP", "Servidor ativo na porta 80.");

  // ---------- Integracao com o backend ----------
  // Depois do servidor local, de proposito: se a integracao falhar, o
  // servidor ja esta no ar.
  nuvemIniciar();

  estadoSistema = "Aguardando objeto";
  aguardar(2000);
}

/* =====================================================================
   9) LOOP - ciclo de separacao + atendimento das requisicoes
   ===================================================================== */

void loop() {
  // Atende o navegador a cada volta do loop.
  server.handleClient();
  bool wifiConectado = WiFi.status() == WL_CONNECTED;
  if (wifiConectado && !wifiEstavaConectado) {
    registrar(LOG_INFO, "WIFI", "Reconectado. IP %s, MAC %s.",
              enderecoIP().c_str(), WiFi.macAddress().c_str());
    mostrarOled("DESCARTE AQUI", "Wi-Fi conectado", enderecoIP().c_str(), nullptr);
  }
  if (!wifiConectado && wifiEstavaConectado) {
    registrar(LOG_WARN, "WIFI", "Conexao perdida; iniciando reconexao automatica.");
  }
  wifiEstavaConectado = wifiConectado;
  if (!wifiConectado && millis() - ultimaVerificacaoWiFi >= INTERVALO_RECONEXAO_MS) {
    ultimaVerificacaoWiFi = millis();
    tentativasReconexao++;
    registrar(LOG_WARN, "WIFI", "Tentativa de reconexao numero %lu.", tentativasReconexao);
    WiFi.reconnect();
  }
  // ---------- Leitura dos sensores ----------
  distanciaCm = medirDistancia();
  metalDetectado = (digitalRead(SENSOR_METAL) == LOW);
  objetoPresente = (distanciaCm > 0 && distanciaCm <= DISTANCIA_DETECCAO);
  atualizarPeso();
  ultimaLeituraMs = millis();

  if (objetoPresente) {

    // Avisa o backend do inicio do ciclo. Nao bloqueia: so enfileira para a
    // task do core 0. Sem sessao autorizada, nao faz nada.
    registrar(LOG_INFO, "SENSOR", "Objeto detectado a %.1f cm (%s).",
              (double) distanciaCm, metalDetectado ? "metal" : "nao-metal");
    nuvemAvisarEtapa("objeto_detectado", distanciaCm, metalDetectado, pesoGramas);

    if (metalDetectado) {
      // ---------- Caminho da pilha (metal) ----------
      estadoSistema = "Separando metal";
      definirSeparacao(SERVO_METAL);
      mostrarOled("DESCARTE AQUI", "Objeto detectado!", "PILHA", "Separando...");
      nuvemAvisarEtapa("separando", distanciaCm, true, pesoGramas);
      aguardar(TEMPO_ESPERA);

      // Pesagem
      estadoSistema = "Pesando";
      mostrarOled("DESCARTE AQUI", "METAL DETECTADO!", "Aguardando peso...", nullptr);
      nuvemAvisarEtapa("pesando", distanciaCm, true, pesoGramas);
      aguardar(TEMPO_ESTABILIZACAO);

      if (balancaOk && balanca.wait_ready_timeout(1000)) {
        // Aqui a precisao importa mais que a velocidade, entao tiramos a media
        // de 5 amostras. Esta chamada bloqueia ~500 ms, mas acontece uma unica
        // vez por objeto, no momento em que a pesagem realmente ocorre.
        pesoBruto = balanca.read_average(5);
        pesoGramas = converterParaGramas(pesoBruto);

        char linhaPeso[24];
        snprintf(linhaPeso, sizeof(linhaPeso), "Peso: %.1f g", pesoGramas);

        registrar(LOG_INFO, "BALANCA", "Bruto %ld, peso %.1f g.", pesoBruto, (double) pesoGramas);
        mostrarOled("DESCARTE AQUI", "PILHA", linhaPeso, nullptr);

        // Fim do ciclo com material valido: e AQUI que o descarte pode virar
        // pontos. Quem decide se vira, e quantos, e o backend.
        nuvemConcluir(pesoGramas, true, "pilha separada");
      } else {
        registrar(LOG_ERRO, "BALANCA", "Erro na leitura; sem peso para reportar.");
        mostrarOled("DESCARTE AQUI", "Erro na balanca!", nullptr, nullptr);

        // Sem peso confiavel nao ha pontuacao. Enviamos assim mesmo para que
        // o usuario receba a recusa na hora, em vez de esperar a expiracao.
        nuvemConcluir(0, true, "erro na balanca");
      }

      aguardar(2000);
      definirSeparacao(SERVO_INICIAL);
      aguardar(1000);

    } else {
      // ---------- Caminho do nao-metal ----------
      estadoSistema = "Separando nao-metal";
      definirSeparacao(SERVO_NAO_METAL);
      mostrarOled("DESCARTE AQUI", "Objeto detectado!", "NAO E PILHA", "Separando...");
      nuvemAvisarEtapa("separando", distanciaCm, false, pesoGramas);
      aguardar(TEMPO_ESPERA);

      // Abre a portinha para o usuario retirar o objeto
      estadoSistema = "Porta aberta";
      mostrarOled("NAO PILHA DETECTADO!", "RETIRE O OBJETO!", "Porta aberta!", nullptr);
      definirPorta(true);
      nuvemAvisarEtapa("porta_aberta", distanciaCm, false, pesoGramas);
      aguardar(TEMPO_PORTA_ABERTA);
      definirPorta(false);
      aguardar(1000);

      definirSeparacao(SERVO_INICIAL);

      // Nao-metal encerra a sessao sem pontos: o backend marca "recusada" e a
      // pagina do usuario explica que o material nao e pilha.
      nuvemConcluir(0, false, "nao e pilha");

      aguardar(1000);
    }

    estadoSistema = "Aguardando objeto";

  } else {
    // ---------- Nenhum objeto ----------
    estadoSistema = "Aguardando objeto";
    mostrarOled("DESCARTE AQUI", "Aguardando", "objeto...", nullptr);
  }

  aguardar(300);
}

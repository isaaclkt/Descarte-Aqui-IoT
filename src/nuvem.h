/* =====================================================================
   CLIENTE DO BACKEND PHP - Descarte Aqui

   Este modulo e a parte do firmware que conversa com o site. Ele NAO
   substitui o servidor HTTP local: e uma funcionalidade a mais.

   Por que polling de saida
     O ESP32 fica numa rede local (hotspot do celular, Wi-Fi da sala) e nao
     deve ser exposto a internet - nada de port forwarding. Entao a conexao
     e sempre no sentido ESP32 -> backend: o dispositivo pergunta "ha
     autorizacao para mim?" de tempos em tempos. Nenhuma porta e aberta no
     roteador.

   Por que numa task propria
     HTTPClient e bloqueante: uma requisicao que demora 5 s dentro do loop()
     deixaria o servidor local 5 s sem responder, a cada ciclo de polling.
     Aqui a comunicacao roda numa task FreeRTOS fixada no core 0, enquanto o
     loop() segue no core 1 chamando server.handleClient(). Resultado: o
     backend pode cair, demorar ou nem existir, e a pagina local do ESP32
     continua respondendo na mesma velocidade.

   O estado compartilhado entre as duas tasks e protegido por mutex.
   ===================================================================== */

#ifndef NUVEM_H
#define NUVEM_H

#include <Arduino.h>

// Porta do servidor HTTP local, declarada aqui porque dois lugares precisam
// dela: o main.cpp, que sobe o WebServer nela, e a consulta de autorizacao,
// que informa ao site onde a pagina de monitoramento atende. O site nao tem
// como descobrir isso sozinho - a porta de ORIGEM das consultas e efemera.
#define DISPOSITIVO_PORTA_HTTP 80

// Versao do firmware, informada ao backend junto com o MAC.
extern const char *FIRMWARE;

// Em que ponto do descarte autorizado estamos.
enum EstadoSessao {
  SESSAO_OCIOSA,        // nenhuma autorizacao: a maquina funciona, mas nao pontua
  SESSAO_AUTORIZADA,    // backend liberou, esperando o objeto
  SESSAO_EM_ANDAMENTO,  // objeto detectado, ciclo rodando
  SESSAO_ENVIANDO,      // ciclo terminou, enviando o resultado
  SESSAO_CONCLUIDA      // backend confirmou e creditou
};

// Saude da comunicacao com o backend.
enum EstadoBackend {
  BACKEND_NAO_CONFIGURADO,  // BACKEND_URL vazio: integracao desligada
  BACKEND_SEM_REDE,         // Wi-Fi caiu
  BACKEND_OK,               // ultima requisicao respondeu
  BACKEND_ERRO_HTTP         // backend fora do ar ou respondeu erro
};

// Cria o mutex e, se a integracao estiver configurada, sobe a task do core 0.
// Chame no setup(), depois do Wi-Fi e do registro.
void nuvemIniciar();

// true quando BACKEND_URL, codigo e chave foram preenchidos.
bool nuvemConfigurada();

// Ha autorizacao valida agora? E isto que o loop() consulta para decidir se
// o ciclo fisico deve ser reportado - nunca para decidir se ele acontece.
bool nuvemTemAutorizacao();

// Avisa o backend de uma etapa do ciclo (telemetria). Nao bloqueia: so
// enfileira. Etapas aceitas: objeto_detectado, separando, pesando,
// porta_aberta, finalizando.
void nuvemAvisarEtapa(const char *etapa, float distanciaCm, bool metal, float pesoG);

// Enfileira a conclusao do descarte. Nao bloqueia. Se nao houver sessao
// autorizada, apenas registra em log: sem sessao nao existe pontuacao.
void nuvemConcluir(float pesoG, bool metal, const char *resultado);

// Estado da sessao e do backend em JSON, para a rota GET /descarte.
String nuvemJson();

// Textos curtos usados pela rota GET /status.
const char *nuvemEstadoBackendTexto();
const char *nuvemEstadoSessaoTexto();

// Segundos desde a ultima resposta do backend; -1 se nunca respondeu.
long nuvemSegundosDesdeContato();

#endif  // NUVEM_H

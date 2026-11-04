/* =====================================================================
   REGISTRO DE EVENTOS - Descarte Aqui

   Log com niveis INFO / WARN / ERROR que vai para dois lugares ao mesmo
   tempo:

     1) o Serial, como ja era feito com Serial.printf;
     2) um buffer circular na memoria, servido pela rota GET /logs.

   O (2) existe por causa do TDE: "registro de eventos e erros" precisa
   poder ser consultado pela rede, nao so por quem esta com o cabo USB na
   mao durante a apresentacao.

   O buffer e escrito pelas duas tasks (o loop no core 1 e a task da nuvem
   no core 0), entao todo acesso passa por um mutex.

   NUNCA passe senha de Wi-Fi, chave de dispositivo ou chave de conclusao
   para estas funcoes.
   ===================================================================== */

#ifndef REGISTRO_H
#define REGISTRO_H

#include <Arduino.h>

enum NivelLog {
  LOG_INFO,
  LOG_WARN,
  LOG_ERRO
};

// Quantas linhas ficam guardadas. 40 x 112 bytes ~= 4,4 KB de RAM.
#define REGISTRO_CAPACIDADE 40
#define REGISTRO_TAMANHO_LINHA 112

// Prepara o mutex. Chame no inicio do setup(), antes de qualquer registrar().
void registroIniciar();

// Grava uma linha. Formato igual ao do printf.
void registrar(NivelLog nivel, const char *origem, const char *formato, ...);

// Buffer inteiro como array JSON, do mais antigo para o mais recente.
String registroJson();

#endif  // REGISTRO_H

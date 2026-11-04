// MODELO de configuracao da integracao com o backend PHP.
//
// Copie este arquivo para  src/config_backend.h  e preencha.
// O config_backend.h esta no .gitignore de proposito: a chave do dispositivo
// NAO vai para o GitHub.
//
//   Windows : copy src\config_backend.exemplo.h src\config_backend.h
//   Linux   : cp src/config_backend.exemplo.h src/config_backend.h
//
// O codigo e a chave sao gerados no backend, uma vez por equipamento:
//
//   php backend/ferramentas/gerar-chave.php "Ponto de coleta - Bloco 7"
//
// Esse comando imprime o que colar aqui e o INSERT para cadastrar o
// dispositivo no banco. A chave em texto puro so aparece naquele momento:
// o banco guarda apenas o hash.
//
// IMPORTANTE
//   * Deixe BACKEND_URL vazio ("") para rodar SEM a integracao. O servidor
//     HTTP local, os sensores e os servos continuam funcionando igual; a rota
//     /status informa backend_estado = "nao_configurado". E assim que se
//     demonstra o TDE numa maquina sem o site publicado.
//   * Nao exponha o ESP32 a internet. Ele so faz requisicoes DE SAIDA.
//   * Em HTTP puro a chave trafega em claro. Em producao use https://.

#ifndef CONFIG_BACKEND_H
#define CONFIG_BACKEND_H

// Raiz das rotas do backend, SEM barra no fim.
// Exemplos:
//   "http://192.168.0.10/descarte-aqui/php/iot"   (XAMPP na mesma rede)
//   "https://seu-dominio.com/php/iot"             (site publicado)
#define BACKEND_URL ""

// Codigo publico do dispositivo (o mesmo que vai no QR Code).
#define DISPOSITIVO_CODIGO ""

// Chave secreta do dispositivo. Nunca compartilhe nem versione.
#define DISPOSITIVO_CHAVE ""

// De quanto em quanto tempo o ESP32 pergunta ao backend se ha autorizacao.
#define BACKEND_INTERVALO_MS 3000

// Tempo maximo esperando o backend responder.
#define BACKEND_TIMEOUT_MS 5000

#endif  // CONFIG_BACKEND_H

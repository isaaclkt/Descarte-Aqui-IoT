// MODELO de configuracao do Wi-Fi.
//
// Copie este arquivo para  src/config_wifi.h  e preencha com os dados da
// hotspot do celular de voces. O config_wifi.h esta no .gitignore de proposito: assim a
// senha do Wi-Fi NAO vai para o GitHub.
//
//   Windows : copy src\config_wifi.exemplo.h src\config_wifi.h
//   Linux   : cp src/config_wifi.exemplo.h src/config_wifi.h
//
// Lembrete: o ESP32 so conecta em redes de 2,4 GHz.

#ifndef CONFIG_WIFI_H
#define CONFIG_WIFI_H
#define WIFI_SSID  "COLOQUE_O_NOME_DA_REDE"
#define WIFI_SENHA "COLOQUE_A_SENHA"

#endif  // CONFIG_WIFI_H

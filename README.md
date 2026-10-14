# Descarte Aqui — Sistema inteligente para descarte de resíduos

Protótipo de sistema ciberfísico que identifica, separa e pesa resíduos
descartados numa lixeira inteligente. O **ESP32 funciona como servidor HTTP**
na rede Wi-Fi local: o navegador acessa a página diretamente pelo IP da placa,
consulta os sensores e envia comandos para o atuador.

TDE da disciplina **Conectividade em Sistemas Ciberfísicos** — Engenharia de
Software, PUCPR.

> **Integrantes da equipe:** _a preencher (nome completo + RA de cada membro)._

---

## 1. Problema

Pilhas e baterias não podem ir para o lixo comum, mas o descarte depende da
boa vontade e do conhecimento de quem descarta. O protótipo detecta se o
objeto depositado é metálico (pilha/bateria), desvia-o para o compartimento
correto e registra o peso descartado. Objetos que não são pilha são
devolvidos ao usuário por uma portinha.

## 2. Arquitetura

```
                  +-----------------------------------+
                  |        APLICACAO CLIENTE          |
                  |   Navegador (HTML + CSS + JS)     |
                  +-----------------+-----------------+
                                    |
                    GET /status     |   Respostas JSON
                    GET /sensor     |
                    GET /atuador    |
                                    v
+-----------------------------------v-----------------------------------+
|                         ESP32 (modo SERVIDOR)                         |
|                   Servidor HTTP escutando na porta 80                 |
|                      biblioteca WiFi.h + WebServer.h                  |
+-----------------------------+--------------+--------------------------+
|           Leitura           |              |      Acionamento        |
|              v              |              v                          |
|  +------------------------+ |   +-------------------------------+     |
|  | HC-SR04  (distancia)   | |   | Servo separacao  (GPIO 25)    |     |
|  | Sensor indutivo (metal)| |   | Servo porta      (GPIO 26)    |     |
|  | HX711 + celula (peso)  | |   | Display OLED SSD1306 (I2C)    |     |
|  +------------------------+ |   +-------------------------------+     |
+-----------------------------------------------------------------------+
```

Toda a comunicação acontece **dentro da rede Wi-Fi local**. Não há servidor
externo, nuvem, banco de dados nem API de terceiros: o ESP32 é o servidor.

A página web não fica num servidor separado — ela viaja **dentro do próprio
firmware**, como texto na memória de programa (`PROGMEM`). Quando o navegador
pede `/`, o ESP32 devolve o HTML; quando pede `/style.css` e `/app.js`, devolve
o CSS e o JavaScript. Assim o requisito "navegador ↔ ESP32" é cumprido sem
precisar configurar sistema de arquivos (SPIFFS/LittleFS) na placa.

## 3. Hardware utilizado

Placa: **ESP32 DevKit v1 (ESP32-WROOM-32)** — ambiente `esp32doit-devkit-v1`.

| Componente | Função | Pinos no ESP32 |
|---|---|---|
| HC-SR04 (ultrassônico) | Detecta objeto a até 5 cm | TRIG = 19, ECHO = 18 |
| Sensor indutivo de metal | Distingue pilha de não-pilha | GPIO 32 (`LOW` = metal) |
| HX711 + célula de carga | Pesa o objeto | DT = 16, SCK = 4 |
| Servo de separação | Desvia o objeto para o lado correto | GPIO 25 |
| Servo da portinha | Devolve o objeto não-metálico | GPIO 26 |
| Display OLED SSD1306 128×64 | Mostra o estado localmente | I2C: SDA = 21, SCL = 22, endereço 0x3C |

Posições dos servos (constantes no firmware): separação em 90° (repouso),
160° (metal) e 20° (não-metal); porta em 0° (fechada) e 90° (aberta).

> O diagrama de ligações e as fotos da montagem devem ser colocados em `docs/`.

## 4. Estrutura do repositório

```
Descarte-Aqui-IoT/
├── docs/                      # diagramas, esquemático e fotos da montagem
├── src/
│   ├── main.cpp               # firmware do ESP32 (servidor HTTP + sensores + atuadores)
│   ├── pagina_web.h           # HTML/CSS/JS embutidos no firmware (GERADO)
│   └── client/                # fonte da aplicação cliente
│       ├── index.html
│       ├── style.css
│       └── app.js
├── tools/
│   └── gerar_pagina_web.py    # gera pagina_web.h a partir de src/client/
├── platformio.ini             # dependências e configuração da placa
├── README.md
└── .gitignore
```

A organização segue a estrutura recomendada no regulamento do TDE
(`docs/`, `src/esp32`, `src/client`), adaptada ao layout que o PlatformIO
exige: o código da placa fica em `src/main.cpp` em vez de `src/esp32/`.

### Como editar a interface

**Nunca edite `src/pagina_web.h` à mão** — ele é gerado. Edite os arquivos em
`src/client/` e rode:

```bash
python tools/gerar_pagina_web.py
```

Isso mantém uma única fonte de verdade para o HTML/CSS/JS e evita que a versão
embutida no firmware fique diferente da versão do repositório.

## 5. Documentação da API

O ESP32 escuta na **porta 80**. Todas as rotas são `GET`, para poderem ser
testadas direto na barra de endereço do navegador.

| Rota | Função | Resposta |
|---|---|---|
| `/` | Página principal | HTML |
| `/style.css` | Estilos da página | CSS |
| `/app.js` | JavaScript da página | JS |
| `/status` | Estado geral do sistema | JSON |
| `/sensor` | Leituras atuais dos sensores | JSON |
| `/atuador?estado=on` | Abre a portinha | JSON |
| `/atuador?estado=off` | Fecha a portinha | JSON |

### `GET /status`

```json
{
  "dispositivo": "Descarte Aqui - ESP32",
  "firmware": "0.1.0",
  "estado": "Aguardando objeto",
  "wifi_conectado": true,
  "ssid": "MinhaRede",
  "ip": "192.168.1.100",
  "rssi": -55,
  "uptime_s": 134,
  "heap_livre": 212345,
  "oled_ok": true,
  "balanca_ok": true,
  "porta": "fechada",
  "servo_separacao": 90
}
```

O campo `estado` assume os valores `Aguardando objeto`, `Separando metal`,
`Pesando`, `Separando nao-metal` e `Porta aberta`.

### `GET /sensor`

```json
{
  "distancia_cm": 12.4,
  "objeto_presente": false,
  "metal_detectado": false,
  "peso_bruto": 83472,
  "peso_g": 15.3,
  "balanca_ok": true,
  "atualizado_ms": 134872
}
```

`distancia_cm` vale `-1.0` quando o HC-SR04 não recebe eco dentro de 30 ms.
`peso_bruto` é a leitura crua do HX711 (usada para calibrar) e `peso_g` é o
valor convertido em gramas.

### `GET /atuador?estado=on|off`

```json
{ "ok": true, "atuador": "porta", "estado": "aberta", "pino": 26 }
```

Erros retornam o código HTTP adequado:

| Situação | HTTP | Corpo |
|---|---|---|
| Parâmetro ausente | 400 | `{"ok":false,"erro":"faltou o parametro estado (on ou off)"}` |
| Valor inválido | 400 | `{"ok":false,"erro":"estado invalido, use on ou off"}` |
| Rota inexistente | 404 | `{"ok":false,"erro":"rota nao encontrada"}` |

## 6. Instalação e execução

### Pré-requisitos

- [PlatformIO](https://platformio.org/) (extensão do VS Code ou CLI)
- Cabo USB de dados e driver USB-serial da placa (CP210x ou CH340)
- Rede Wi-Fi **2,4 GHz** — o ESP32 não conecta em redes 5 GHz

As bibliotecas são baixadas automaticamente pelo PlatformIO a partir do
`platformio.ini`: `ESP32Servo`, `Adafruit SSD1306`, `Adafruit GFX` e `HX711`.

### Passo 1 — configurar o Wi-Fi

Ative o **roteador Wi-Fi / hotspot do celular em 2,4 GHz**.
Abra `src/config_wifi.h` (criado nesta máquina) e preencha:

```cpp
#define WIFI_SSID  "NOME_DO_HOTSPOT_DO_CELULAR"
#define WIFI_SENHA "SENHA_DO_HOTSPOT_DO_CELULAR"
```

Para outro integrante, copie `src/config_wifi.exemplo.h` para `src/config_wifi.h`.
O arquivo com a senha é ignorado pelo Git. Após trocar os dados, compile e
envie novamente o firmware para a placa.

O ESP32 conecta ao hotspot como cliente (STA). Se a conexão falhar, continua
tentando sem criar uma rede própria. O IP recebido aparece no monitor serial,
inclusive após reconectar.
### Passo 2 — compilar e enviar

Pelo VS Code: botões **Build** (✓) e **Upload** (→) na barra do PlatformIO.

Pela linha de comando:

```bash
pio run              # compila
pio run -t upload    # compila e envia para a placa
pio device monitor   # abre o monitor serial em 115200
```

### Passo 3 — descobrir o IP

Abra o monitor serial em **115200 baud**. O ESP32 imprime:

```
=== DESCARTE AQUI - iniciando ===
[WIFI] Conectando em "MinhaRede"....
[WIFI] Conectado!
[WIFI] Acesse no navegador: http://192.168.1.100
[HTTP] Servidor ativo na porta 80.
```

O IP também aparece no display OLED logo após a conexão.

### Passo 4 — acessar a interface

Digite o IP no navegador, por exemplo `http://192.168.1.100`. O computador ou
celular precisa estar **na mesma rede Wi-Fi** do ESP32.

A página mostra o estado da conexão, o IP, as leituras dos sensores, o estado
dos atuadores, o horário da última atualização e dois botões para abrir e
fechar a portinha. O JavaScript consulta `/status` e `/sensor` a cada 2
segundos, sem recarregar a página.

## 7. Como testar

### 7.1 Testar as rotas sem a interface

Com o IP em mãos, dá para testar tudo pelo navegador ou pelo terminal:

```bash
curl http://192.168.1.100/status
curl http://192.168.1.100/sensor
curl "http://192.168.1.100/atuador?estado=on"
curl "http://192.168.1.100/atuador?estado=off"

# casos de erro
curl -i "http://192.168.1.100/atuador"              # espera HTTP 400
curl -i "http://192.168.1.100/atuador?estado=xyz"   # espera HTTP 400
curl -i "http://192.168.1.100/rota-que-nao-existe"  # espera HTTP 404
```

As mesmas URLs funcionam colando na barra de endereço do navegador, e também
no Postman.

### 7.2 Roteiro de teste completo

1. Abrir o projeto no VS Code com a extensão PlatformIO.
2. Ativar o hotspot e preencher `WIFI_SSID` e `WIFI_SENHA` em `src/config_wifi.h`.
3. Compilar (**Build**) e corrigir eventuais erros.
4. Enviar para o ESP32 (**Upload**).
5. Abrir o monitor serial em **115200**.
6. Conferir no serial o IP recebido e a mensagem `[HTTP] Servidor ativo na porta 80.`
7. Digitar o IP no navegador e verificar se a página aparece.
8. Confirmar que o selo de conexão fica verde (**Conectado**) e que o IP aparece no topo.
9. Observar os cartões de sensores mudando sozinhos a cada 2 segundos.
10. Testar `/status` e `/sensor` direto na barra de endereço (deve aparecer JSON).
11. Clicar em **Abrir porta** e confirmar que o servo se move e que o cartão muda para `aberta`.
12. Clicar em **Fechar porta** e confirmar o retorno.
13. Acompanhar no monitor serial as linhas `[HTTP] GET /status`, `[HTTP] GET /atuador` e `[ATUADOR] Porta -> ABERTA`, confirmando que as requisições chegam.
14. Aproximar um objeto metálico a menos de 5 cm e verificar o ciclo de separação com pesagem.
15. Aproximar um objeto não metálico e verificar a abertura automática da portinha.
16. Desligar o Wi-Fi do computador e confirmar que o selo muda para **Desconectado** (tratamento de falha de conexão).

### Teste com várias pessoas no hotspot

1. Conecte o ESP32 e dois ou mais celulares/notebooks ao mesmo hotspot.
2. Abra em todos `http://<IP informado pelo ESP32>/` e confira as leituras.
3. Teste os comandos da porta: todos controlam a mesma porta, e o último
   comando recebido prevalece.
4. Desligue e ligue o hotspot e confira a reconexão no monitor serial.
   Se o IP mudar, abra o novo endereço nos aparelhos.

O limite de aparelhos depende do celular. O servidor atende as requisições
em sequência, permitindo o uso da página por várias pessoas. Caso os clientes
não consigam acessar a placa, verifique o isolamento de clientes do hotspot.
Não é necessário acesso à internet para usar a página local.

### 7.3 Testar a interface sem a placa

Dá para abrir `src/client/index.html` direto no navegador apontando para o
ESP32: basta preencher a constante no início de `src/client/app.js`.

```js
const ESP32_BASE_URL = "http://192.168.1.100";
```

Quando a página é servida pelo próprio ESP32, essa constante fica vazia
(`""`) e as requisições vão para o mesmo IP que serviu a página. O firmware
envia o cabeçalho `Access-Control-Allow-Origin: *` nas respostas JSON, o que
permite esse teste a partir de um arquivo local.

## 8. Calibração da balança

A constante `FATOR_CALIBRACAO` em `src/main.cpp` vem com o valor provisório
`420.0` e **precisa ser ajustada** para a célula de carga de vocês:

1. Com a balança vazia, ligue a placa (o `tare()` roda sozinho no boot).
2. Coloque um objeto de peso conhecido, por exemplo 100 g.
3. Consulte `/sensor` e anote o valor de `peso_bruto`.
4. Calcule `FATOR_CALIBRACAO = peso_bruto / peso_real_em_gramas`.
5. Substitua a constante, recompile e confira se `peso_g` bate com a realidade.

## 9. Decisões técnicas (para a defesa)

Três pontos do firmware valem ser compreendidos por todos antes da defesa:

**Por que existe a função `aguardar()` em vez de `delay()`.**
O ciclo de separação precisa de pausas longas (4 s esperando o servo, 5 s com
a porta aberta, 3 s estabilizando a balança). Um `delay()` comum travaria o
processador inteiro nesse período e o ESP32 deixaria de atender o navegador —
a página daria erro de conexão justamente durante a demonstração. A função
`aguardar()` faz a mesma pausa, mas chamando `server.handleClient()` em
loop: o tempo passa e as requisições continuam sendo respondidas.

**Por que `atualizarPeso()` não usa `get_units()`.**
O `get_units()` da biblioteca HX711 chama `wait_ready()` internamente, que
bloqueia até a próxima amostra chegar. Como a célula de carga entrega cerca de
10 amostras por segundo, isso prenderia o loop por ~100 ms em cada volta.
Em vez disso, verificamos com `is_ready()` se já existe amostra disponível,
lemos o valor cru e convertemos para gramas por conta própria, usando a tara
e o fator de calibração. No momento da pesagem de verdade, onde a precisão
importa mais que a velocidade, usamos `read_average(5)` para tirar a média de
5 amostras.

**Por que falha de periférico não para a placa.**
Se o OLED ou o HX711 não responderem, o firmware registra o problema, marca
`oled_ok`/`balanca_ok` como `false` e continua. O servidor HTTP sobe de
qualquer forma, e a interface mostra qual periférico falhou. Isso evita que um
cabo solto no display derrube o sistema inteiro no dia da apresentação.

## 10. Próximas etapas

- [ ] Preencher integrantes da equipe (nome + RA) neste README
- [ ] Adicionar diagrama de blocos e esquemático em `docs/`
- [ ] Calibrar a balança e fixar o `FATOR_CALIBRACAO` real
- [ ] Adicionar fotos e vídeo da demonstração
- [ ] Testes de estresse (várias requisições simultâneas)
- [ ] Relatório técnico (modelo será divulgado pelo professor)

# Descarte Aqui — Sistema inteligente para descarte de resíduos

Protótipo de sistema ciberfísico que identifica, separa e pesa resíduos
descartados numa lixeira inteligente. O **ESP32 funciona como servidor HTTP**
na rede Wi-Fi local: o navegador acessa a página diretamente pelo IP da placa,
consulta os sensores e envia comandos para o atuador.

TDE da disciplina **Conectividade em Sistemas Ciberfísicos** — Engenharia de
Software, PUCPR.

**Integrantes da equipe — Grupo 6**

| Nome completo | RA | Usuário GitHub |
|---|---|---|
| André Fochesatto | 40130997 | `andrefochesatto0` |
| Arthur Medeiros Diniz Azevedo | 40126823 | `arthurmedeiros0` |
| Isaac de Lima Klimiont | 40132716 | `isaaclkt` |
| Lucas Buzzi Lima | 40126429 | `Llucas-buzzi-lima` |
| Luiz Felipe Rodrigues da Silva Souza | 40141767 | `nunu-bol` |

A coluna do usuário GitHub liga cada integrante ao histórico de commits deste
repositório, que o regulamento do TDE avalia junto com a defesa individual.

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
|  | Indutivo (boca)        | |   | Servo separacao  (GPIO 25)    |     |
|  | HC-SR04 (portinha)     | |   | Servo porta      (GPIO 26)    |     |
|  | IR E18-D80NK (bandeja) | |   | Display OLED SSD1306 (I2C)    |     |
|  | HX711 + celula (peso)  | |   +-------------------------------+     |
|  +------------------------+ |                                         |
+-----------------------------------------------------------------------+
```

Toda a comunicação **do servidor local** acontece dentro da rede Wi-Fi: o ESP32
é o servidor, e o navegador da rede fala direto com ele.

Existe ainda um segundo fluxo, **opcional e independente deste**, que liga o
equipamento ao site Descarte Aqui para creditar pontos ao usuário:

```
   ESP32  ──requisições DE SAÍDA──▶  backend PHP  ──▶  MySQL
```

Ele roda numa task separada, no outro núcleo do ESP32, e o servidor local não
depende dele: sem internet, sem backend ou sem `src/config_backend.h`, tudo
acima continua funcionando igual. O ESP32 nunca é exposto à internet — nenhuma
porta é aberta no roteador. Detalhes em
[`docs/integracao-iot-backend.md`](docs/integracao-iot-backend.md).

A página web não fica num servidor separado — ela viaja **dentro do próprio
firmware**, como texto na memória de programa (`PROGMEM`). Quando o navegador
pede `/`, o ESP32 devolve o HTML; quando pede `/style.css` e `/app.js`, devolve
o CSS e o JavaScript. Assim o requisito "navegador ↔ ESP32" é cumprido sem
precisar configurar sistema de arquivos (SPIFFS/LittleFS) na placa.

## 3. Hardware utilizado

Placa: **ESP32 DevKit v1 (ESP32-WROOM-32)** — ambiente `esp32doit-devkit-v1`.

| Componente | Função | Pinos no ESP32 |
|---|---|---|
| Sensor indutivo de metal | **1º do caminho**, na boca: registra que passou metal | GPIO 32 (`LOW` = metal) |
| E18-D80NK (infravermelho) | **2º do caminho**, sobre a bandeja: confirma a chegada e inicia o ciclo | GPIO 19 (`LOW` = objeto) |
| HC-SR04 (ultrassônico) | Detecta a mão na abertura da portinha, a até 14 cm | TRIG = 27, ECHO = 33 |
| HX711 + célula de carga | Pesa o objeto | DT = 16, SCK = 4 |
| Servo de separação | Desvia o objeto para o lado correto | GPIO 25 |
| Servo da portinha | Devolve o objeto não-metálico | GPIO 26 |
| Display OLED SSD1306 128×64 | Mostra o estado localmente | I2C: SDA = 21, SCL = 22, endereço 0x3C |

Posições dos servos (constantes no firmware): separação em 90° (repouso),
160° (metal) e 20° (não-metal); porta em 170° (fechada) e 10° (aberta).

O E18-D80NK tem saída NPN de coletor aberto: ela só puxa para o terra. Por
isso o GPIO 19 precisa de algo puxando para 3,3 V — há um resistor externo de
10 k, e o firmware ainda liga o `INPUT_PULLUP` interno por garantia. Sem
nenhum dos dois o pino flutua e a placa dispara ciclos sozinha.

> O diagrama de ligações e as fotos da montagem devem ser colocados em `docs/`.

## 4. Estrutura do repositório

```
Descarte-Aqui-IoT/
├── docs/                      # diagramas, esquemático e fotos da montagem
│   └── integracao-iot-backend.md  # arquitetura da integração com o site
├── src/
│   ├── main.cpp               # firmware do ESP32 (servidor HTTP + sensores + atuadores)
│   ├── nuvem.h / nuvem.cpp    # cliente do backend PHP (task própria, core 0)
│   ├── registro.h / .cpp      # log INFO/WARN/ERROR, servido em GET /logs
│   ├── config_wifi.exemplo.h  # modelo das credenciais de Wi-Fi
│   ├── config_backend.exemplo.h   # modelo das credenciais do dispositivo
│   ├── pagina_web.h           # HTML/CSS/JS embutidos no firmware (GERADO)
│   └── client/                # fonte da aplicação cliente
│       ├── index.html
│       ├── style.css
│       ├── app.js
│       └── ponto_coleta3d.html   # visualização 3D (Three.js via CDN)
├── backend/                   # rotas PHP da integração (publicar como php/iot/ no site)
│   ├── publico/               # rotas do usuário autenticado
│   ├── dispositivo/           # rotas que o ESP32 consome
│   ├── sql/                   # migração do banco
│   └── ferramentas/           # gerar chave do dispositivo, testar o fluxo
├── tools/
│   ├── gerar_pagina_web.py    # gera pagina_web.h a partir de src/client/
│   ├── gerar_qrcode.py        # gera o QR Code e a etiqueta de um dispositivo
│   └── simular_esp32.py       # simula a placa: mesmas rotas, mesmo ciclo
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
| `/3d` | Visualização 3D do ponto de coleta | HTML |
| `/style.css` | Estilos da página | CSS |
| `/app.js` | JavaScript da página | JS |
| `/status` | Estado geral do sistema | JSON |
| `/sensor` | Leituras atuais dos sensores | JSON |
| `/atuador?estado=on` | Abre a portinha | JSON |
| `/atuador?estado=off` | Fecha a portinha | JSON |
| `/servo` | Estado dos dois servos (pino, posição, limites) | JSON |
| `/rede` | IP, MAC, gateway, RSSI, tentativas de reconexão | JSON |
| `/descarte` | Estado do descarte autorizado pelo site | JSON |
| `/logs` | Últimos eventos registrados (INFO/WARN/ERROR) | JSON |

As quatro últimas são documentadas em
[`docs/integracao-iot-backend.md`](docs/integracao-iot-backend.md), seção 3.

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
  "distancia_cm": -1.0,
  "sensor_entrada_tipo": "indutivo",
  "sensor_entrada_gpio": 32,
  "sensor_confirmacao_tipo": "infravermelho",
  "sensor_confirmacao_gpio": 19,
  "objeto_presente": false,
  "confirmacao_armada": true,
  "metal_recente": false,
  "distancia_nao_metal_cm": 9.8,
  "objeto_nao_metal_presente": false,
  "metal_detectado": false,
  "peso_bruto": 83472,
  "peso_g": 15.3,
  "balanca_ok": true,
  "atualizado_ms": 134872
}
```

São **dois sensores em sequência**, e é a ordem entre eles que classifica o
objeto:

1. o **indutivo** (GPIO 32) na boca da lixeira registra que passou metal, mas
   não inicia nada sozinho;
2. o **infravermelho E18-D80NK** (GPIO 19), logo abaixo, confirma que o objeto
   chegou e dispara o ciclo.

Se o indutivo acusou metal nos 3 s anteriores à chegada ao infravermelho, é
pilha. Se o infravermelho disparou sem esse registro, é não-pilha. Daí a
ordem física importar: com os dois trocados de lugar, o infravermelho dispara
primeiro, o indutivo ainda não viu nada, e **toda pilha é classificada como
não-pilha**.

Nenhum dos dois mede distância, então `distancia_cm` vale sempre `-1.0`: o
campo continua na resposta só para não quebrar quem já lia a rota.

`objeto_presente` é a leitura do infravermelho agora. `confirmacao_armada` diz
se ele está pronto para iniciar outro ciclo: depois de um objeto, só rearma
quando o feixe fica livre por 100 ms seguidos. Sem esse campo, um objeto que
pareceu ignorado só se explica com a placa na mão.

`metal_recente` é a memória do indutivo dentro da janela de 3 s. Os dois
campos juntos explicam qualquer classificação.

`distancia_nao_metal_cm` e `objeto_nao_metal_presente` vêm do **ultrassônico**
(TRIG 27 / ECHO 33), apontado para a abertura da portinha: ele detecta a mão a
até 14 cm e segura a porta aberta enquanto ela estiver ali. Enquanto esse
controle automático está ativo, `/atuador` responde `409` — a portinha não
aceita comando manual no meio da retirada.

A porta **não fecha por prazo**, de propósito: fechar com a mão dentro seria
pior do que ficar aberta. Ela abre assim que o desvio termina e só fecha
depois que a região da abertura fica livre por 1,5 s seguidos. Leitura sem eco
é inconclusiva e nunca fecha a porta sozinha.

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

## 5.1 Organização da página de controle

A página principal (`/`) continua sendo o painel de comando. Ela ganhou um
painel com o modelo 3D ao lado dos cartões de sensores no computador, e abaixo
deles no celular.

O 3D **não carrega sozinho**: são 117 KB servidos pela placa, e pelo Wi-Fi real
isso leva alguns segundos. Há um botão "Mostrar modelo 3D" que carrega o iframe
sob demanda, e um link "Abrir em tela cheia" que abre `/3d` numa aba separada.

### Como os dois ficam sincronizados

A regra é que **o estado verdadeiro é o do ESP32**: o 3D nunca mostra algo que a
placa não confirmou.

Ao clicar em "Abrir porta" ou "Fechar porta":

1. a página chama `/atuador?estado=on|off`, como sempre fez;
2. os dois botões ficam desabilitados enquanto o comando está em andamento;
3. **só depois** da resposta `{"ok": true}` a página avisa o 3D;
4. se a placa recusar (HTTP 400) ou não responder, o 3D **não se move** e a
   página mostra o erro.

O aviso vai por dois caminhos: `postMessage` para o iframe e `BroadcastChannel`,
que alcança o 3D aberto em outra aba. A mensagem serve só para a resposta
imediata — o `/status` seguinte confirma ou corrige, e é ele que manda quando o
estado muda por outro caminho (ciclo automático do firmware, `curl`, outra aba).

### Quem consulta o ESP32

O `WebServer` do ESP32 atende **uma conexão por vez**, então só um cliente
consulta a placa:

| Situação | Quem consulta | Intervalo |
|---|---|---|
| 3D embutido na página de controle (`/3d?embed=1`) | A página de controle | 2 s |
| 3D aberto sozinho (`/3d`) | O próprio 3D | 500 ms |

O 3D detecta o modo embutido pelo parâmetro `?embed=1` e, nesse caso, desliga o
polling próprio e passa a esperar as mensagens do pai. Os 2 s da página de
controle bastam porque o estado mais curto do ciclo (`Pesando`) dura ~3,5 s.

O 3D expõe `descarteAqui.aplicarEstadoReal(status, sensor)`, usada tanto pelas
mensagens quanto pelo polling, e mostra no cartão da portinha de onde veio a
última mudança: "comando da página" ou "estado do ESP32".

## 5.2 Visualização 3D (`/3d`)

A rota `/3d` serve uma maquete tridimensional do ponto de coleta que espelha o
estado real do equipamento. Ela é um arquivo único, `src/client/ponto_coleta3d.html`,
embutido no firmware junto com as demais páginas.

**Como abrir:** pelo link "Ver o modelo 3D" no topo da página principal, ou
digitando `http://<ip-do-esp32>/3d` direto no navegador.

**Dois modos de operação:**

| Modo | Quando acontece | O que o selo mostra |
|---|---|---|
| ESP32 ao vivo | A placa respondeu à última consulta | `ESP32 ao vivo · <ip>` (verde) |
| Simulação | Três consultas seguidas falharam | `Sem conexão com o ESP32 · modo simulação` (âmbar) |

A troca é automática nos dois sentidos: basta a placa voltar a responder para o
modelo religar no estado real. Em modo ao vivo os botões de simulação ficam
desabilitados, para que a animação local não dispute o controle dos servos.

**O que cada dado controla no modelo:**

| Campo do JSON | Efeito no 3D |
|---|---|
| `estado` | Tela do OLED e etapa destacada no painel de fluxo |
| `servo_separacao` | Inclinação da bandeja, proporcional: `(ângulo − 90) / 70` |
| `porta` | Ângulo da portinha (substitui a lógica própria do modelo) |
| `objeto_presente`, `distancia_cm` | Feixe e brilho do sensor de presença |
| `metal_detectado` | LED do sensor indutivo: verde para metal, vermelho para não metal |
| `peso_g` | Tela de pesagem do OLED |
| `ip`, `rssi`, `oled_ok`, `balanca_ok`, `heap_livre` | Selo de status e cartão de cada componente |

O LED do indutivo só fica verde ou vermelho quando há objeto presente. Em
repouso o pino fica em `HIGH` por causa do `INPUT_PULLUP`, e mostrar "não é
metal" o tempo todo seria enganoso.

**Intervalo de consulta:** 500 ms, com `/status` e `/sensor` buscados em
sequência, nunca em paralelo. O `WebServer` do ESP32 atende uma conexão por vez,
então dois sockets simultâneos só criariam disputa. Cada resposta tem ~300 bytes.

### Limitações conhecidas

- **O Three.js vem de CDN** (`cdn.jsdelivr.net`). Se o celular estiver conectado
  a uma rede sem saída para a internet, a biblioteca não baixa e o 3D não
  carrega — a página mostra um aviso. A página principal (`/`) não depende de
  CDN e continua funcionando normalmente.
- **Os corpos dos dois sensores do caminho estão trocados no 3D.** O Sensor 1
  ainda é desenhado como o HC-SR04 que ficava na entrada, e o Sensor 2 como o
  cilindro do indutivo. As fichas ao lado descrevem o que está lá de verdade
  — indutivo na boca, infravermelho sobre a bandeja —, mas o modelo não foi
  refeito.
- O valor da pesagem **não fica retido**: `atualizarPeso()` sobrescreve
  `peso_g` com a leitura instantânea a cada volta do loop, então o peso medido
  no estado `Pesando` só sobrevive alguns segundos.

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

### 7.3 Testar o 3D e a interface sem a placa

`tools/simular_esp32.py` sobe um servidor com as **mesmas rotas do firmware**
(`/`, `/3d`, `/status`, `/sensor`, `/servo`, `/rede`, `/descarte`, `/logs`,
`/atuador`), servindo os arquivos de verdade de `src/client/`, e roda o mesmo
ciclo de `src/main.cpp`, com os mesmos estados e tempos. Só biblioteca padrão.

```bash
python tools/simular_esp32.py
```

Depois abra `http://127.0.0.1:8100/` (painel) ou `http://127.0.0.1:8100/3d`.
Para disparar um ciclo à mão:

```bash
curl "http://127.0.0.1:8100/simular?tipo=nao-metal"
```

Se `src/config_backend.h` estiver preenchido, o simulador também **conversa com
o backend PHP** igual à task de `src/nuvem.cpp`: consulta autorizações, reporta
as etapas e envia a conclusão. Dá para escanear o QR Code no celular e ver os
pontos serem creditados — tudo sem hardware.

> **O simulador não é a placa.** Sensores, servos e balança são números
> inventados pelo script; nada obtido ali vale como teste de hardware. Por
> isso `/status` devolve `"simulado": true` e a página de controle mostra uma
> faixa vermelha enquanto estiver falando com ele. O firmware real nunca envia
> esse campo, então a faixa nunca aparece com a placa.

Para conferir o fallback do 3D, derrube o simulador: após três consultas falhas
o modelo cai para a simulação interna dele, e volta sozinho quando o servidor
retorna.

Um aviso que custa tempo: o navegador **cacheia** `ponto_coleta3d.html`, que é
grande. Depois de editar o 3D, recarregue com `Ctrl+Shift+R` — senão você vê o
arquivo antigo e conclui que a mudança não funcionou.

### 7.4 Apontar a interface local para uma placa de verdade

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

## 9.1 Integração com o site Descarte Aqui

O usuário escaneia o QR Code colado no equipamento, faz o descarte físico e os
pontos entram na conta dele no site. O documento completo está em
[`docs/integracao-iot-backend.md`](docs/integracao-iot-backend.md); o guia de
instalação do backend, em [`backend/README.md`](backend/README.md).

O que vale repetir aqui, porque é o que costuma ser perguntado:

- **O ESP32 não deixou de ser servidor.** Continua atendendo clientes da rede
  na porta 80. A integração é uma funcionalidade a mais.
- **O ESP32 não é exposto à internet.** Só faz requisições de saída, por
  polling. Nenhuma porta é aberta no roteador.
- **A comunicação externa roda no core 0**, em task própria, enquanto o
  `loop()` e o servidor HTTP ficam no core 1. `HTTPClient` é bloqueante: sem
  essa separação, cada consulta ao backend deixaria a página local sem resposta
  por vários segundos.
- **Internet caída não trava a máquina.** O ciclo físico não depende de
  autorização: sem sessão, a máquina continua detectando, separando, pesando e
  abrindo a portinha — só não gera pontos. Com `BACKEND_URL` vazio a integração
  fica desligada e o firmware funciona igual.
- **A regra de pontuação é a do banco**, não do backend novo: `concluir.php`
  chama `fn_calcular_pontos()`, a function que a equipe definiu no PjBL 3 — que
  inclui o bônus por volume (1,10 a partir de 500 g, 1,20 a partir de 1 kg). O
  usuário declara só o tipo de pilha; o peso vem da balança; o dispositivo
  nunca envia pontos.
- **Ponto duplicado é impossível pelo banco**, não por cuidado do PHP:
  `descarte.fk_sessao_descarte_id` é `UNIQUE`, então uma sessão nunca gera dois
  descartes, mesmo que o ESP32 reenvie a conclusão por timeout.

Para configurar, ver a seção 7 do documento da integração. Em resumo: montar o
banco com `Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql` e depois
`backend/sql/01_integracao_iot.sql`, copiar `src/config_backend.exemplo.h` para
`src/config_backend.h` e rodar
`php backend/ferramentas/gerar-chave.php "<nome do ponto>"`.

### O QR Code do equipamento

> **Quer só ver funcionando na sua máquina?** O passo a passo completo, do
> ponto de acesso ao descarte com os pontos caindo na conta, está em
> [`docs/configurar-hotspot.md`](docs/configurar-hotspot.md). Os arquivos
> gerados ficam em [`docs/qrcodes/`](docs/qrcodes/) — leia o README de lá antes
> de imprimir, porque o QR carrega o IP de uma rede específica.

Depois que o dispositivo estiver cadastrado no banco:

```bash
python tools/gerar_qrcode.py --dispositivo esp32-bloco7 --base http://192.168.0.15/descarte-aqui/php/iot --nome "Ponto de coleta - Bloco 7"
```

Sai em `docs/qrcodes/`: o QR Code sozinho (SVG) e uma etiqueta de 80×120 mm
pronta para imprimir, recortar e colar no equipamento. Com `--png` sai também
um PNG, útil para slides.

A única dependência é o `segno`, Python puro, que fica **só na máquina de quem
gera** — como o `gerar_pagina_web.py`, esta é uma ferramenta de
desenvolvimento, não entra no firmware nem no site:

```bash
pip install segno
```

Dois detalhes que costumam morder:

- **`localhost` não serve.** O celular não é o PC. Numa rede local, use o IP da
  máquina que roda o XAMPP e deixe o celular no mesmo Wi-Fi. O script avisa se
  você passar `localhost`.
- **O QR Code aponta para o site, nunca para o ESP32.** É o site que sabe quem
  está logado. Por isso o endereço que importa é o do site — e por isso os
  arquivos gerados não são versionados (`docs/qrcodes/` está no `.gitignore`):
  eles dependem da rede do dia. Regere com um comando.



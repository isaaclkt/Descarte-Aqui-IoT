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
│       ├── app.js
│       └── ponto_coleta3d.html   # visualização 3D (Three.js via CDN)
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
| `/3d` | Visualização 3D do ponto de coleta | HTML |
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
- O corpo marcado como **"Sensor previsto"**, na parede do compartimento de
  rejeição, não corresponde a nenhum sensor instalado. O protótipo tem um único
  HC-SR04 e ele fica na entrada (Sensor 1).
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

### 7.3 Testar o 3D sem a placa

Existe um mock em Python que responde `/status` e `/sensor` no formato do
README e percorre os cinco estados do ciclo. Aponte a constante
`ESP32_BASE_URL`, no início do bloco "CAMADA DE DADOS" de
`src/client/ponto_coleta3d.html`, para o endereço do mock e abra o arquivo no
navegador. Para conferir o fallback, basta derrubar o mock: após três consultas
falhas o modelo cai para simulação, e volta sozinho quando o servidor retorna.

### 7.4 Testar a interface sem a placa

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

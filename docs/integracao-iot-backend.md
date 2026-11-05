# Integração ESP32 ↔ backend PHP

Documento de referência da integração entre o dispositivo físico e o sistema
web Descarte Aqui. Escrito para a defesa do TDE: cada decisão tem o motivo ao
lado.

---

## 1. Arquitetura implementada

```
   Celular do usuário                            Navegador na rede local
   (internet, pelo QR Code)                      (Wi-Fi da sala / hotspot)
            │                                              │
            │ HTTPS                                        │ HTTP
            ▼                                              ▼
 ┌──────────────────────────┐                  ┌──────────────────────────────┐
 │  SITE (PHP + MySQL)      │                  │  ESP32                       │
 │                          │                  │                              │
 │  php/login.php           │   requisições    │  core 1: loop()              │
 │  php/minha-area.php      │   DE SAÍDA do    │    WebServer :80             │
 │  php/iot/publico/*       │◀─── ESP32 ───────│    sensores + servos         │
 │  php/iot/dispositivo/*   │                  │                              │
 │                          │─── autorização ─▶│  core 0: task "nuvem"        │
 │  usuario / descarte /    │                  │    cliente do backend        │
 │  pilhas / dispositivo /  │                  │                              │
 │  sessao_descarte         │                  └──────────────┬───────────────┘
 └──────────────────────────┘                                 │
                                                  2× HC-SR04 · sensor indutivo
                                                  HX711 · 2 servos · OLED
```

Quatro responsabilidades, cada uma num lugar:

| Componente | Responsabilidade |
|---|---|
| ESP32 | sensores, servos, servidor HTTP local, controle físico |
| Backend PHP | autenticação, sessões de descarte, validação, pontuação |
| Frontend | interface do usuário |
| MySQL | armazenamento persistente |

### 1.1 Por que polling de saída

O ESP32 fica numa rede local e **não é exposto à internet** — nenhum
redirecionamento de porta no roteador. A conexão é sempre no sentido
ESP32 → backend: o dispositivo pergunta, de tempos em tempos, se há autorização
esperando por ele.

### 1.2 Por que uma task separada

`HTTPClient` é bloqueante. Uma requisição que demora 5 s dentro do `loop()`
deixaria o servidor local 5 s sem responder, a cada ciclo de polling — e a
página consulta `/status` e `/sensor` a cada 2 s.

A comunicação roda numa task FreeRTOS fixada no **core 0**
(`xTaskCreatePinnedToCore`, 8 KB de pilha por causa do HTTPS), enquanto o
`loop()` segue no **core 1** chamando `server.handleClient()`. O estado
compartilhado entre as duas é protegido por mutex (`xSemaphoreCreateMutex`).

Consequência prática, e é o ponto a defender: **o backend pode cair, demorar ou
nem existir, e a página local do ESP32 continua respondendo na mesma
velocidade.**

### 1.3 O servidor local nunca é refém da internet

Três camadas garantem isso:

1. a comunicação externa está em outro núcleo (acima);
2. `BACKEND_URL` vazio desliga a integração inteira — o firmware compila, o
   servidor sobe, `/status` informa `backend_estado: "nao_configurado"`;
3. o ciclo físico **não depende de autorização**. Sem sessão, a máquina
   continua detectando, separando, pesando e abrindo a portinha. A sessão só
   decide se aquele descarte gera pontos.

A decisão 3 foi deliberada: travar o hardware sem internet contrariaria o
requisito do TDE de que a falha de conexão não torne o dispositivo inutilizável,
e inviabilizaria a demonstração numa máquina sem o site publicado.

---

## 2. Fluxo completo de comunicação

| # | Onde | O que acontece |
|---|---|---|
| 1 | site | usuário faz login (`php/login.php`, já existente) |
| 2 | celular | usuário escaneia o QR Code do equipamento |
| 3 | site | `publico/descartar.php?d=<codigo>` identifica o dispositivo |
| 4 | site | sem sessão de cidadão, a página oferece entrar e não inicia nada |
| 5 | site | usuário escolhe o tipo de pilha e confirma |
| 6 | backend | `sessao-abrir.php` cria a sessão: `aguardando`, expira em 3 min |
| 7 | ESP32 | a task do core 0 faz `GET dispositivo/autorizacao.php` |
| 8 | backend | sessão vira `autorizada` e a resposta leva a `chave_conclusao` |
| 9 | ESP32 | mostra no OLED e em `/descarte`; estado `AUTORIZADO` |
| 10 | físico | usuário coloca a pilha; HC-SR04 detecta |
| 11 | ESP32 | `POST evento.php` (`objeto_detectado`) → sessão `em_andamento` |
| 12 | físico | sensor indutivo classifica, servo separa, HX711 pesa. Não sendo pilha, o segundo ultrassônico comanda a portinha e a etapa `porta_aberta` é reportada quando ela abre de fato |
| 13 | ESP32 | `POST concluir.php` com peso, metal e a `chave_conclusao` |
| 14 | backend | valida tudo (seção 5) e calcula os pontos |
| 15 | backend | numa transação: grava `descarte` + `descarte_pilha`, credita `usuario.pontos`, fecha a sessão |
| 16 | site | `sessao-status.php` devolve `concluida` com pontos e saldo |
| 17 | site | a página mostra a confirmação |

Escanear o QR Code ou abrir a sessão **não** gera pontos em nenhum momento.

### 2.1 Máquina de estados no ESP32

```
OCIOSO ──autorização recebida──▶ AUTORIZADO ──objeto detectado──▶ EM_ANDAMENTO
   ▲                                                                   │
   │                                                          ciclo concluído
   │                                                                   ▼
   └────── nada pendente ◀────── CONCLUIDA ◀──backend confirmou── ENVIANDO
```

### 2.2 Estados da sessão no banco

`aguardando` → `autorizada` → `em_andamento` → `concluida`

Saídas sem pontos: `recusada` (material não-metálico ou peso inválido),
`cancelada` (usuário desistiu), `expirada` (passou do prazo).

---

## 3. Endpoints HTTP do ESP32

Porta 80, todas `GET`, testáveis na barra de endereço do navegador.

| Rota | Função | Resposta |
|---|---|---|
| `/` | página de controle | HTML |
| `/3d` | visualização 3D | HTML |
| `/style.css`, `/app.js` | estáticos da página | CSS / JS |
| `/status` | estado geral | JSON |
| `/sensor` | leituras dos sensores | JSON |
| `/atuador?estado=on\|off` | abre / fecha a portinha | JSON |
| `/servo` | **novo** — estado dos dois servos | JSON |
| `/rede` | **novo** — IP, MAC, gateway, RSSI, reconexões | JSON |
| `/descarte` | **novo** — estado da sessão de descarte | JSON |
| `/logs` | **novo** — últimos eventos INFO/WARN/ERROR | JSON |

Rota inexistente responde `404` com JSON.

### `GET /status` (campos novos)

```json
{
  "mac": "A0:B7:65:11:22:33",
  "backend_estado": "ok",
  "backend_configurado": true,
  "backend_ultimo_contato_s": 2,
  "sessao_estado": "autorizada"
}
```

`backend_estado`: `ok`, `sem_rede`, `erro_http` ou `nao_configurado`.

### `GET /rede`

```json
{
  "conectado": true, "modo": "STA", "ssid": "...",
  "ip": "192.168.0.42", "mac": "A0:B7:65:11:22:33",
  "gateway": "192.168.0.1", "mascara": "255.255.255.0", "dns": "192.168.0.1",
  "rssi": -58, "canal": 6, "tentativas_reconexao": 0, "porta_servidor": 80
}
```

### `GET /descarte`

```json
{
  "sessao_estado": "autorizada", "sessao_parcial": "cdbbe9b76d70",
  "pilha": "Pilha comum AA", "pontos_por_grama": 2.000,
  "segundos_restantes": 162, "conclusao_pendente": false,
  "ultimo_descarte": 2, "ultimos_pontos": 60, "ultimo_creditado": true,
  "ultimo_motivo": "", "backend_estado": "ok",
  "backend_configurado": true, "backend_ultimo_contato_s": 1
}
```

Só os 12 primeiros caracteres do código da sessão são publicados, e a
`chave_conclusao` **nunca** aparece em resposta nem em log.

### `GET /logs`

```json
{"linhas":[{"ms":1234,"nivel":"INFO","origem":"WIFI","texto":"Conectado. IP ..."}],
 "total":12,"capacidade":40}
```

Buffer circular de 40 linhas na RAM (~4,4 KB). As requisições HTTP ficam **só**
no Serial: a página consulta a placa a cada 2 s e, no buffer, isso empurraria
para fora os eventos que importam.

---

## 4. Rotas PHP

### Do usuário — exigem login de cidadão e CSRF nos POST

| Rota | Método | Entrada | Saída |
|---|---|---|---|
| `publico/descartar.php` | GET | `?d=<codigo>` | HTML |
| `publico/sessao-abrir.php` | POST | `csrf`, `dispositivo`, `pilha` | `{ok, sessao, expira_em, segundos}` |
| `publico/sessao-status.php` | GET | `?sessao=<codigo>` | `{status, mensagem, pontos, peso_g, saldo, segundos}` |
| `publico/sessao-cancelar.php` | POST | `csrf`, `sessao` | `{ok, status}` |

### Do dispositivo — headers `X-Dispositivo` e `X-Chave`

| Rota | Método | Entrada | Saída |
|---|---|---|---|
| `dispositivo/autorizacao.php` | GET | `?mac=&firmware=` | `{ok, autorizacao}` |
| `dispositivo/evento.php` | POST | `sessao`, `etapa`, leituras | `{ok, status}` |
| `dispositivo/concluir.php` | POST | `sessao`, `chave_conclusao`, `peso_g`, `metal` | `{ok, creditado, descarte, pontos}` |

Códigos: 200, 400, 401, 403, 404, 405, 409, 422, 503.

---

## 5. Segurança

### Pontuação

A regra é a do banco, não deste backend:

```sql
pontos = FLOOR( fn_calcular_pontos(peso_medido_g, pilhas.pontos_por_grama) )
```

`fn_calcular_pontos()` é a function que a equipe definiu no PjBL 3. Ela **não**
é apenas peso × pontos_por_grama: aplica bônus por volume — fator 1,10 a partir
de 500 g e 1,20 a partir de 1 kg. O `FLOOR` é o mesmo que
`sp_registrar_descarte` usa ao creditar, porque `usuario.pontos` é `INT`.

`concluir.php` chama a function em vez de refazer a conta em PHP: duas
implementações da mesma regra divergiriam um dia sem ninguém notar.

Por que não chamar `sp_registrar_descarte`, que faria o INSERT inteiro: ela
abre e fecha a própria transação. Chamada de dentro de `concluir.php`, o
`COMMIT` dela encerraria também a nossa — antes de gravarmos o vínculo com a
sessão, que é justamente o que impede creditar duas vezes. Então reaproveitamos
a parte que de fato muda (a fórmula) e mantemos a transação sob controle,
seguindo a procedure passo a passo no resto: mesmo `STATUS` `'aprovado'`, mesmo
`FLOOR`, mesmas colunas de data.

O usuário declara só o **tipo** de pilha. O peso vem da balança. O que o
dispositivo diga sobre pontos é ignorado.

O banco tem ainda o seu próprio limite: `trg_descarte_pilha_valida_bi` recusa
mais de 5000 g por tipo de pilha. O `peso_maximo_g` da configuração (2000 g por
padrão) é mais apertado de propósito e devolve uma mensagem clara antes de o
trigger precisar barrar.

### Validações em `concluir.php`

1. dispositivo autenticado (`password_verify` contra `chave_hash`) e `ativo`;
2. a sessão pertence àquele dispositivo;
3. `chave_conclusao` confere, com `hash_equals`;
4. status é `autorizada` ou `em_andamento` — nunca já encerrado;
5. `expira_em > NOW()`, reconferido sob `FOR UPDATE`;
6. peso dentro de `peso_minimo_g`..`peso_maximo_g`;
7. material não-metálico é registrado como `recusada`, sem pontos;
8. pontos calculados pelo backend;
9. `descarte.fk_sessao_descarte_id UNIQUE` impede crédito duplo.

### Idempotência

Duas travas, **no banco**, não no PHP:

- `descarte.fk_sessao_descarte_id UNIQUE` — uma sessão, um descarte. Reenvio
  por timeout devolve o descarte original com `"repetido": true`;
- `sessao_descarte.dispositivo_ocupado UNIQUE` — coluna gerada a partir do
  `status`; duas pessoas não ocupam o mesmo equipamento ao mesmo tempo. Quando a
  sessão encerra, a coluna vira `NULL` e o equipamento se libera sozinho.

Mais o `SELECT ... FOR UPDATE`, que serializa conclusões simultâneas.

### QR Code

Contém **só** o código público do dispositivo:

```
https://<dominio>/php/iot/publico/descartar.php?d=esp32-bloco7
```

Sem senha, sem token permanente, sem dado pessoal, sem credencial. Quem
identifica a pessoa é a sessão do site.

A imagem sai de `tools/gerar_qrcode.py`:

```bash
python tools/gerar_qrcode.py --dispositivo esp32-bloco7 \
    --base http://192.168.0.15/descarte-aqui/php/iot \
    --nome "Ponto de coleta - Bloco 7"
```

Gera, em `docs/qrcodes/`, o QR Code em SVG e uma etiqueta de 80×120 mm para
imprimir. Correção de erro **H** (recupera ~30%), porque a etiqueta vai colada
num equipamento e sofre risco, poeira e dedo.

O script valida o layout antes de escrever: confere que nenhum texto cai fora
da etiqueta e que a zona de silêncio em volta do QR tem os 4 módulos que a
norma pede. Essa validação existe porque a primeira versão imprimia o código do
dispositivo em 110 mm numa etiqueta de 105 mm — sumia. E a zona de silêncio não
é detalhe estético: no teste, o mesmo QR sem ela **não decodifica**.

Os arquivos gerados não são versionados (`docs/qrcodes/` está no `.gitignore`):
dependem do endereço do site, que muda de rede para rede. Regerar é um comando.

### Expiração

`expirar_sessoes()` roda no início de cada rota e marca como `expirada` tudo que
passou do prazo. Não depende de cron nem de o usuário voltar à página.

---

## 6. Logs

Níveis INFO, WARN e ERROR nos dois lados. Nenhum deles grava senha de Wi-Fi,
chave de dispositivo ou `chave_conclusao`.

| Evento | ESP32 | Backend |
|---|---|---|
| inicialização | INFO `SETUP` | — |
| conexão Wi-Fi, IP e MAC | INFO `WIFI` | — |
| reconexão | WARN `WIFI` (com o número da tentativa) | — |
| sensor acionado | INFO `SENSOR` | INFO `ciclo_*` |
| servo acionado | INFO `ATUADOR` | — |
| porta do não-metal aberta | INFO `NAO-METAL` | INFO `ciclo_porta_aberta` |
| objeto não chegou ao compartimento (30 s) | WARN `NAO-METAL` | — |
| retirada não confirmada (60 s) | WARN `NAO-METAL` | — |
| sessão iniciada | INFO `NUVEM` | INFO `sessao_iniciada` |
| descarte concluído | INFO `NUVEM` | INFO `descarte_concluido` |
| erro de conexão | WARN/ERROR `NUVEM` | — |
| sessão expirada | — | WARN `conclusao_expirada` |
| descarte cancelado | — | WARN `sessao_cancelada` |
| credencial recusada | — | WARN `auth_dispositivo` |
| balança sem resposta | ERROR `HX711` / `BALANCA` | WARN `peso_invalido` |

No ESP32 o log vai para o Serial **e** para o buffer de `/logs`. No backend, para
`error_log()` **e** para a tabela `evento_iot`.

---

## 7. Configuração

### Wi-Fi (já existia)

```bash
cp src/config_wifi.exemplo.h src/config_wifi.h
```

Preencher `WIFI_SSID` e `WIFI_SENHA`. Só redes de 2,4 GHz. Arquivo no
`.gitignore`; a ausência dele é **erro de compilação**, de propósito.

### Backend no firmware (novo)

```bash
cp src/config_backend.exemplo.h src/config_backend.h
```

| Define | Valor |
|---|---|
| `BACKEND_URL` | raiz das rotas, sem barra final. Vazio = integração desligada |
| `DISPOSITIVO_CODIGO` | código público, o mesmo do QR Code |
| `DISPOSITIVO_CHAVE` | chave secreta |
| `BACKEND_INTERVALO_MS` | intervalo do polling (3000) |
| `BACKEND_TIMEOUT_MS` | tempo máximo de espera (5000) |

Também no `.gitignore`. Ao contrário do Wi-Fi, a **ausência não é erro**: sem o
arquivo o firmware compila e o servidor local funciona igual.

Código e chave saem de:

```bash
php backend/ferramentas/gerar-chave.php "Ponto de coleta - Bloco 7"
```

### Banco de dados

A base é a entrega final do PjBL 3 da equipe, que cria o banco `descarteaqui`
com tabelas, views, functions, procedures e triggers. É reexecutável:

```bash
mysql -u root < "Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql"
```

Depois, a migração da integração, que só acrescenta:

```bash
mysql -u root descarteaqui < backend/sql/01_integracao_iot.sql
```

**Para recomeçar do zero, apague o banco antes.** Rodar só o script do PjBL 3
de novo não serve: ele faz `SET FOREIGN_KEY_CHECKS = 0` e `TRUNCATE descarte`,
o que apaga os descartes e deixa `sessao_descarte.fk_descarte_id` apontando
para linhas inexistentes, sem erro nenhum. Verificado: sobraram 2 sessões
órfãs. O caminho certo é `DROP DATABASE descarteaqui` e refazer os dois passos.

Não rode os scripts de `docs/banco-de-dados/sql/` do repo Web: são de outra
versão do modelo (banco `descarte_aqui`, com underline) e não servem aqui.

### Backend no servidor (novo)

Ver [`backend/README.md`](../backend/README.md). Em resumo: copiar
`config.exemplo.php` para `config.php` e publicar a pasta como `php/iot/`
**dentro do site** — o cookie de sessão só vale no mesmo domínio.

---

## 8. Bibliotecas utilizadas

Nenhuma dependência nova foi acrescentada ao `platformio.ini`.

| Biblioteca | Origem | Uso |
|---|---|---|
| `WiFi.h`, `WebServer.h` | framework Arduino-ESP32 | já usadas |
| `HTTPClient.h`, `WiFiClientSecure.h` | framework Arduino-ESP32 | **novas**, mas vêm do framework |
| `freertos/semphr.h` | framework | mutex do estado compartilhado |
| `ESP32Servo`, `Adafruit SSD1306`, `Adafruit GFX`, `HX711` | `lib_deps` | já usadas |

No backend: PHP 8 com `mysqli` e `session`, sem Composer.

Ferramentas de desenvolvimento, que rodam na máquina de quem gera e **não** são
dependência do firmware nem do site, como já acontecia com
`tools/gerar_pagina_web.py`:

| Ferramenta | Dependência | Instalação |
|---|---|---|
| `tools/gerar_pagina_web.py` | nenhuma | — |
| `tools/gerar_qrcode.py` | `segno` (Python puro, sem dependências) | `pip install segno` |

A leitura do JSON de resposta no firmware é feita por busca direta de chave
(`extrairTexto` / `extrairNumero` em `src/nuvem.cpp`), e não por ArduinoJson. O
motivo é prático: o backend é nosso, responde um JSON plano de chaves únicas, e
assim ninguém precisa baixar uma biblioteca a mais antes da apresentação. Se o
formato da resposta mudar, essas funções mudam junto.

---

## 9. Testes

### 9.1 Executados

| O que | Como | Resultado |
|---|---|---|
| Compilação do firmware | `pio run -e esp32doit-devkit-v1` | sucesso, sem avisos. RAM 15,8%, Flash 73,8% |
| Sintaxe de todo o PHP | `php -l` nos 14 arquivos | sem erros |
| Rotas sem banco | `curl` contra `php -S` | 401 sem credencial, 405 método errado, 401 sem login |
| Migração do banco | aplicada sobre o banco real do PjBL 3 | ok em **MariaDB 10.4.32** (XAMPP) |
| Restrições criadas | `SHOW CREATE TABLE` | coluna gerada, 3 UNIQUE e 4 CHECK confirmados |
| Fluxo do dispositivo | `php ferramentas/testar-fluxo.php` | **41 verificações, 0 falhas** |
| Bônus por volume | 600 g pela integração | 330 pontos, e não os 300 de peso × ponto |
| Views da equipe | `vw_descartes_detalhados` | descarte do dispositivo aparece com protocolo `DSC-202611-00025` |
| Auditoria da equipe | `log_pontos_usuario` | trigger registrou as duas alterações de saldo |
| Fluxo do usuário | cópia descartável do site + `login.php` real | login compartilhado, sessão, crédito, cancelamento e logout ok |
| QR Code gerado | decodificado com o `QRCodeDetector` do OpenCV | devolveu exatamente a URL esperada |
| Desenho da etiqueta | matriz reconstruída dos retângulos do SVG | idêntica à do `segno`; rasterizada, também decodifica certo |
| Zona de silêncio | controle: o mesmo QR sem ela | **não decodifica** — é o que a validação do script evita |

O teste do fluxo do usuário foi feito publicando a pasta como `php/iot/` numa
cópia descartável do site (extraída de `origin/main` do repositório Web, que não
foi alterado), com o `login.php` de verdade. Resultado medido: saldo foi de 48
para 108 pontos com um descarte de 30 g a 2 pontos/g, o `minha-area.php` do site
exibiu o novo saldo e o histórico, cancelar liberou o equipamento e, depois do
logout, as rotas passaram a responder 401.

Dois defeitos foram encontrados por esses testes e corrigidos: uma `CHECK` que
engessava a tabela de sessões, e uma verificação do próprio script que usava
`??` para testar um campo que deve valer `null`.

### 9.2 Dependem do hardware — **não executados**

Não há placa disponível neste ambiente. Nada abaixo foi simulado nem é
apresentado como testado:

- leitura real dos dois HC-SR04, do sensor indutivo e da balança HX711;
- os dois prazos do caminho não-metal (30 s para o objeto chegar, 60 s para a
  retirada). A lógica compila e o raciocínio de rollover do `millis()` está
  correto, mas o comportamento com o sensor real não foi observado;
- movimento dos dois servos e abertura da portinha;
- calibração do `FATOR_CALIBRACAO` (ainda em 420.0, valor não calibrado);
- comportamento da task do core 0 com Wi-Fi real caindo e voltando;
- fluxo ponta a ponta com o dispositivo físico e o site publicado;
- leitura do OLED durante o ciclo.

---

## 10. Como executar e validar

### Firmware

```bash
cp src/config_wifi.exemplo.h src/config_wifi.h && cp src/config_backend.exemplo.h src/config_backend.h
```

```bash
pio run -e esp32doit-devkit-v1 -t upload && pio device monitor
```

O Serial mostra o IP. Abra `http://<ip>/` em um cliente da mesma rede.

Para demonstrar o TDE **sem** o site publicado, deixe `BACKEND_URL` vazio: todo
o resto funciona e o painel diz que a integração está desligada.

### Checklist de validação

| Verificar | Como |
|---|---|
| servidor HTTP local ativo | `http://<ip>/` responde |
| cliente da rede alcança | abrir pelo celular no mesmo Wi-Fi |
| IP e MAC exibidos | `GET /rede` e no OLED |
| sensor consultável | `GET /sensor` |
| atuador controlável | `GET /atuador?estado=on` |
| estado do servo | `GET /servo` |
| estado do descarte | `GET /descarte` |
| eventos registrados | `GET /logs` |
| reconexão automática | desligar o hotspot, ver `tentativas_reconexao` subir |
| internet não derruba o local | parar o Apache e conferir que `/` continua respondendo |
| QR Code identifica o dispositivo | abrir `descartar.php?d=<codigo>` |
| pontos creditados uma vez | rodar `testar-fluxo.php` |
| sessão expirada não pontua | idem, seção 7 do script |

---

## 11. Arquivos criados e modificados

### Criados — backend

```
backend/README.md
backend/bootstrap.php
backend/comum.php
backend/config.exemplo.php
backend/local/sessao.php
backend/sql/01_integracao_iot.sql
backend/publico/descartar.php
backend/publico/sessao-abrir.php
backend/publico/sessao-status.php
backend/publico/sessao-cancelar.php
backend/dispositivo/autorizacao.php
backend/dispositivo/evento.php
backend/dispositivo/concluir.php
backend/ferramentas/gerar-chave.php
backend/ferramentas/testar-fluxo.php
```

### Criados — firmware e documentação

```
src/config_backend.exemplo.h
src/nuvem.h
src/nuvem.cpp
src/registro.h
src/registro.cpp
tools/gerar_qrcode.py
docs/integracao-iot-backend.md
docs/superpowers/specs/2026-11-04-integracao-esp32-backend-design.md
```

### Modificados

```
src/main.cpp            aditivo: registro, task da nuvem, MAC, 4 rotas novas
src/client/index.html    painel "Descarte pelo site" (nada removido)
src/client/app.js        consulta /descarte, fora do Promise.all principal
src/client/style.css     uma regra para a grade do painel novo
src/pagina_web.h         regerado por tools/gerar_pagina_web.py
.gitignore               src/config_backend.h, backend/config.php
README.md                seção da integração
```

### Não modificados

Nada no repositório `Descarte Aqui - Web`. Nenhum pino, tempo ou lógica física
do ciclo de separação. `platformio.ini` sem dependência nova.

---

## 12. Limitações

1. **Latência do polling.** Entre abrir a sessão e o ESP32 receber a
   autorização passam até 3 s.
2. **Balança não calibrada.** `FATOR_CALIBRACAO` continua em 420.0. Como os
   pontos saem do peso, a pontuação só fica correta depois da calibração.
3. **Chave em claro sobre HTTP.** Produção exige HTTPS.
4. **Certificado não validado.** Em HTTPS o firmware usa `setInsecure()`: sem
   isso, a renovação do certificado do site derrubaria a integração no meio da
   apresentação. Protege contra escuta passiva, não contra um ataque ativo na
   rede.
5. **O DDL do banco está no outro repositório.** A fonte é
   `banco/01_descarteaqui_entrega_final.sql`, no repositório
   `Descarte Aqui - Web`. Quem clonar só o repo IoT consegue compilar o
   firmware e ler o código, mas não monta o banco sem clonar o Web também. Os
   caminhos documentados aqui supõem as duas pastas lado a lado, como estão na
   máquina da equipe.
6. **O backend mora no repositório IoT.** Para o fluxo completo funcionar, a
   pasta precisa ser publicada como `php/iot/` no mesmo domínio do site.
7. **Retorno depois do login.** O site volta para `minha-area.php`, então o
   usuário escaneia o QR Code outra vez. Resolver seria uma linha em
   `php/login.php`, do outro repositório, não alterado.
8. **Leitura de JSON simplificada no firmware.** Busca por chave, não parser
   completo. Suficiente para a resposta deste backend; frágil se o formato
   mudar.
9. **Sem tela de administração de dispositivos.** Cadastro por `INSERT`.
10. **Um descarte por sessão.** Para descartar vários tipos de pilha, o usuário
    repete o processo.

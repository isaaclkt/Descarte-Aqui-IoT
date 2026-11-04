# Integração ESP32 ↔ backend PHP — Descarte Aqui

Data: 2026-11-04
Status: aprovado e implementado — ver a seção 14 para o que mudou na execução
Resultado: [`docs/integracao-iot-backend.md`](../../integracao-iot-backend.md)

## 1. Objetivo

Permitir que um descarte físico feito no dispositivo ESP32 gere pontos na conta do
usuário autenticado no site Descarte Aqui, **sem** deixar de atender os requisitos
obrigatórios do TDE de Conectividade em Sistemas Ciberfísicos: o ESP32 continua
sendo um servidor HTTP na rede local, acessível por clientes da rede, mesmo quando
a internet ou o backend estiverem fora do ar.

## 2. Situação encontrada

### 2.1 Repositórios

Três pastas irmãs, dois repositórios Git independentes:

| Pasta | Git | Estado |
|---|---|---|
| `Descarte Aqui - IoT` | `isaaclkt/...` (não verificado) | firmware PlatformIO funcional |
| `Descarte Aqui - Web` | `isaaclkt/Descarte-Aqui-web` | **checkout local 4 commits atrás de `origin/main`** |
| `PjBL3` | — | vazia |

O backend PHP e o login do cidadão existem em `origin/main` do repo Web
(commit `0455565`, 02/11/2026), mas não estão no disco. A equipe precisa de
`git pull` naquela pasta.

### 2.2 Firmware atual (`src/main.cpp`, 576 linhas)

Já atende quase todo o TDE:

- `WiFi.mode(WIFI_STA)` + `setAutoReconnect(true)`, timeout de 20 s na conexão inicial;
- reconexão automática no `loop()` a cada 10 s (`INTERVALO_RECONEXAO_MS`);
- `WebServer server(80)` com rotas `/`, `/3d`, `/style.css`, `/app.js`, `/status`,
  `/sensor`, `/atuador?estado=on|off` e `onNotFound`;
- `aguardar()` chama `server.handleClient()` dentro das esperas, então o servidor
  responde mesmo durante os 4–5 s em que os servos trabalham;
- logs `[WIFI]`, `[HTTP]`, `[ATUADOR]`, `[BALANCA]` no Serial;
- IP exibido no Serial e no OLED.

Faltam para o TDE: **MAC address**, endpoint de estado do servo, endpoint de
informações de rede e registro de eventos consultável pela rede.

Pinagem real (`src/main.cpp:49-96`) — mais componentes do que o enunciado descreve:

| Componente | Pinos |
|---|---|
| HC-SR04 | TRIG 19, ECHO 18 |
| Sensor indutivo de metal | GPIO 32 (`INPUT_PULLUP`, LOW = metal) |
| Balança HX711 | DT 16, SCK 4 (`FATOR_CALIBRACAO 420.0`, não calibrado) |
| OLED SSD1306 | I2C SDA 21, SCL 22, 0x3C |
| Servo de separação | GPIO 25 (20° não-metal, 160° metal, 90° inicial) |
| Servo da portinha | GPIO 26 (0° fechada, 90° aberta) |

Nenhum pino, componente ou tempo será alterado.

`src/pagina_web.h` é **gerado** por `tools/gerar_pagina_web.py` a partir de
`src/client/`. Editar o header à mão é proibido pelo próprio arquivo.

### 2.3 Backend PHP existente (repo Web, `origin/main:php/`)

PHP puro com mysqli, sem framework. Convenções a seguir:

- `conn.php`: `new mysqli($servername, $username, $password, $dbname)`,
  banco `descarteaqui`, `set_charset('utf8mb4')`;
- `sessao.php`: `session.use_strict_mode`, cookie HttpOnly/SameSite=Lax,
  `encerrarSessao()`;
- `cidadao.php`: guarda de autenticação (`$_SESSION['user_id']` +
  `user_role === 'cidadao'`), `h()` para escape, `consulta($sql, $tipos, $valores)`
  para prepared statements, `csrfValido()`, `tokenCampo()`;
- `login.php` / `cadastrar.php`: `password_verify` / `password_hash(PASSWORD_DEFAULT)`,
  `session_regenerate_id(true)`;
- erros vão para `error_log()`, mensagens genéricas para o usuário.

### 2.4 Esquema de banco real

`docs/integracao-minha-area.md` do repo Web é explícito: os scripts em
`docs/banco-de-dados/sql/` descrevem **outra versão** do esquema e não devem ser
executados. O esquema vivo, reconstruído das migrações Laravel em `d6d848b^` e das
queries do PHP atual:

```
usuario(id, nome, email, senha, cpf, telefone, data_nascimento, endereco,
        logradouro, numero, complemento, bairro, cep, cidade, uf,
        pontos, created_at, updated_at)
ponto_coleta(id, nome, endereco, latitude, longitude, fk_coletor_id, timestamps)
pilhas(id, nome, tipo, descricao, pontos_por_grama)
descarte(id, data, status, peso_total_gramas, pontos_gerados,
         fk_usuario_id, fk_ponto_coleta_id, timestamps)
descarte_pilha(fk_descarte_id, fk_pilhas_id, peso_gramas)
```

`status` tem default `'pendente'`; `minha-area.php` conta `'aprovado'` e `'pendente'`.
O saldo é `usuario.pontos` (coluna direta, sem livro-razão).

## 3. Decisões

| # | Decisão | Origem |
|---|---|---|
| D1 | Backend novo fica em `backend/` **neste** repo IoT, em PHP puro + mysqli, seguindo as convenções do repo Web | pedido do usuário |
| D2 | O ESP32 valida e o backend credita os pontos na conclusão (descarte nasce `'aprovado'`) | pedido do usuário |
| D3 | O usuário declara o **tipo de pilha**; o ESP32 fornece o **peso medido**; o backend calcula `pontos_gerados = peso × pilhas.pontos_por_grama` | esquema real do projeto |
| D4 | O ciclo físico atual **não é bloqueado** pela ausência de sessão ou de internet. A sessão só decide se o descarte gera pontos | requisito do TDE: "a falha de internet não pode tornar o ESP32 inutilizável" |
| D5 | Comunicação por **polling de saída** do ESP32, em uma task FreeRTOS separada no core 0 | ESP32 não exposto à internet; servidor local não pode bloquear |
| D6 | A página inicial aprovada do site **não é tocada**. Nada no repo Web é alterado | pedido do usuário |

### Sobre D3

O enunciado pedia que o usuário declarasse tipo e quantidade. O esquema real pontua
**por grama**, e o dispositivo tem balança: a quantidade é desnecessária, o peso a
substitui. O usuário declara apenas o tipo (que define `pontos_por_grama`), o
dispositivo mede, e o backend é a única autoridade sobre os pontos. O dispositivo
nunca envia pontos.

### Sobre D5

`HTTPClient` é bloqueante. Fazer a requisição dentro do `loop()` deixaria o servidor
local sem resposta por até o timeout configurado, a cada ciclo de polling. Rodar o
cliente HTTP em uma task própria (`xTaskCreatePinnedToCore`, core 0) mantém o
`loop()` no core 1 livre para `server.handleClient()`. É também o argumento mais
forte para a defesa: a falha do backend fica isolada em outro núcleo.

Estado compartilhado entre as duas tasks protegido por um `SemaphoreHandle_t`
(mutex). Nada de `String` compartilhada sem lock.

## 4. Arquitetura

```
  Celular do usuário                       Navegador na rede local
         │ (internet)                              │ (Wi-Fi local, HTTP)
         ▼                                         ▼
  ┌──────────────────┐                     ┌───────────────────────┐
  │  Site PHP        │   polling de saída  │  ESP32                │
  │  (XAMPP/hospedag)│◀────────────────────│  WebServer :80        │
  │                  │   HTTPS/HTTP        │  core 1: loop + HTTP  │
  │  MySQL           │────────────────────▶│  core 0: task nuvem   │
  └──────────────────┘    autorização      └───────────────────────┘
                                                     │
                                            sensores / servos
```

O ESP32 **nunca** recebe conexão da internet. Só faz requisições de saída. O
servidor local continua aceitando clientes da rede independentemente disso.

## 5. Fluxo completo

1. Usuário faz login no site (`php/login.php`, já existente).
2. Usuário escaneia o QR Code colado no dispositivo.
3. QR Code = `https://<dominio>/php/iot/descartar.php?d=<codigo_dispositivo>`.
   Contém **só** o código público do dispositivo: sem senha, sem token, sem dados
   pessoais.
4. `descartar.php` identifica o dispositivo por `dispositivo.codigo`.
5. Se não há sessão de cidadão, redireciona para `entrar.html` guardando o retorno.
6. Usuário escolhe o tipo de pilha e confirma. `sessao-abrir.php` (POST + CSRF) cria
   uma `sessao_descarte` com status `aguardando`, código aleatório de 32 hex,
   `expira_em = agora + 3 min`, e uma `chave_conclusao` aleatória.
7. A task de nuvem do ESP32 faz `GET /dispositivo/autorizacao.php`; o backend muda a
   sessão para `autorizada` e devolve `{codigo, pilha, pontos_por_grama, expira_em}`.
8. O ESP32 registra a autorização, mostra no OLED e na página local, e passa para o
   estado `AUTORIZADO`.
9. O usuário faz o descarte físico. O ciclo existente roda sem alteração: HC-SR04
   detecta, sensor indutivo classifica, servo separa, HX711 pesa, portinha abre para
   não-metal.
10. Ao fim do ciclo, se havia sessão autorizada e o objeto era metal com peso > 0, o
    ESP32 monta o evento de conclusão.
11. `POST /dispositivo/concluir.php` com o código da sessão, a `chave_conclusao`, o
    peso em gramas e o resultado da classificação.
12. O backend valida tudo (seção 8), calcula os pontos, grava `descarte` +
    `descarte_pilha` e credita `usuario.pontos` em **uma transação**.
13. `descartar.php` faz polling em `sessao-status.php` e mostra a confirmação com os
    pontos creditados.

Em nenhum momento escanear o QR Code ou abrir a sessão gera pontos.

## 6. Migração de banco

Arquivo novo: `backend/sql/01_integracao_iot.sql`. Só adiciona; não altera nem
remove nada do que existe.

### `dispositivo`

```
id                  INT AUTO_INCREMENT PK
codigo              VARCHAR(32)  NOT NULL UNIQUE   -- vai no QR Code
nome                VARCHAR(120) NOT NULL
chave_hash          VARCHAR(255) NOT NULL          -- password_hash da chave do ESP32
fk_ponto_coleta_id  INT NULL  → ponto_coleta(id)
ativo               TINYINT(1) NOT NULL DEFAULT 1
firmware            VARCHAR(20) NULL
mac                 VARCHAR(17) NULL
ultimo_ip           VARCHAR(45) NULL
ultimo_contato_em   DATETIME NULL
created_at / updated_at
```

A chave do dispositivo é guardada apenas como hash. O `.sql` **não** contém chave
nenhuma: traz um comentário com o comando para a equipe gerar a sua.

### `sessao_descarte`

```
id                  INT AUTO_INCREMENT PK
codigo              CHAR(32) NOT NULL UNIQUE        -- identificador único da sessão
chave_conclusao     CHAR(32) NOT NULL               -- idempotência; nunca vai ao navegador
fk_usuario_id       INT NOT NULL → usuario(id)
fk_dispositivo_id   INT NOT NULL → dispositivo(id)
fk_pilhas_id        INT NOT NULL → pilhas(id)       -- tipo declarado
status              VARCHAR(20) NOT NULL DEFAULT 'aguardando'
                    -- aguardando | autorizada | em_andamento
                    -- | concluida | cancelada | expirada | recusada
peso_gramas         DECIMAL(10,3) NULL
pontos_creditados   DECIMAL(10,2) NULL
motivo              VARCHAR(255) NULL
fk_descarte_id      INT NULL UNIQUE → descarte(id)
criada_em           DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
expira_em           DATETIME NOT NULL
autorizada_em       DATETIME NULL
concluida_em        DATETIME NULL
updated_at          DATETIME ON UPDATE CURRENT_TIMESTAMP

-- Uma só sessão viva por dispositivo, garantido pelo banco:
dispositivo_ocupado INT GENERATED ALWAYS AS (
    IF(status IN ('aguardando','autorizada','em_andamento'), fk_dispositivo_id, NULL)
) STORED,
UNIQUE KEY uq_sessao_dispositivo_ocupado (dispositivo_ocupado)
```

O `UNIQUE` na coluna gerada impede, no nível do banco, que dois usuários ocupem o
mesmo dispositivo ao mesmo tempo — não depende de o PHP acertar a checagem.

### Colunas novas em `descarte`

```
ALTER TABLE descarte
  ADD COLUMN fk_dispositivo_id INT NULL,
  ADD COLUMN fk_sessao_descarte_id INT NULL,
  ADD UNIQUE KEY uq_descarte_sessao (fk_sessao_descarte_id),
  ADD CONSTRAINT fk_descarte_dispositivo  FOREIGN KEY (fk_dispositivo_id)     REFERENCES dispositivo (id),
  ADD CONSTRAINT fk_descarte_sessao       FOREIGN KEY (fk_sessao_descarte_id) REFERENCES sessao_descarte (id);
```

`uq_descarte_sessao` é a garantia dura de idempotência: **uma sessão nunca gera dois
descartes**, portanto nunca gera pontos duas vezes, mesmo que o ESP32 reenvie a
conclusão por timeout de rede.

### `evento_iot` (logs)

```
id                INT AUTO_INCREMENT PK
fk_dispositivo_id INT NULL → dispositivo(id)
fk_sessao_id      INT NULL → sessao_descarte(id)
nivel             VARCHAR(5) NOT NULL        -- INFO | WARN | ERROR
tipo              VARCHAR(40) NOT NULL       -- autorizacao, conclusao, expiracao, ...
mensagem          VARCHAR(255) NOT NULL
ip                VARCHAR(45) NULL
criado_em         DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
```

Nunca grava senha, hash, chave do dispositivo nem `chave_conclusao`.

## 7. Backend: arquivos e rotas

```
backend/
  README.md                   como instalar, configurar e testar
  config.exemplo.php          placeholders; o config.php real fica no .gitignore
  bootstrap.php               carrega sessao.php/conn.php do site, ou os locais
  comum.php                   responder_json(), ler_json(), log_evento(), expirar_sessoes()
  local/sessao.php            cópia de apoio, usada só quando roda isolado
  local/conn.php              idem
  sql/01_integracao_iot.sql
  publico/
    descartar.php             página do QR Code (exige login de cidadão)
    sessao-abrir.php          POST + CSRF  → cria a sessão
    sessao-status.php         GET          → estado da sessão do usuário
    sessao-cancelar.php       POST + CSRF  → cancela
  dispositivo/
    autorizacao.php           GET  → autorização pendente (polling do ESP32)
    evento.php                POST → progresso/telemetria do ciclo
    concluir.php              POST → conclusão: valida, registra, credita
```

`bootstrap.php` resolve a reutilização sem duplicar o login:

```php
$base = is_file(__DIR__ . '/../sessao.php') ? __DIR__ . '/..' : __DIR__ . '/local';
```

Quando a pasta é publicada como `php/iot/` no site, ela usa o `sessao.php` e o
`conn.php` **do próprio site** — mesma sessão PHP, mesmo login, mesmo banco. Isolada,
usa as cópias de apoio. Isso é obrigatório: o cookie de sessão só é compartilhado
dentro do mesmo domínio.

### Rotas do usuário

| Rota | Método | Entrada | Saída |
|---|---|---|---|
| `publico/descartar.php` | GET | `?d=<codigo>` | página HTML no estilo do site |
| `publico/sessao-abrir.php` | POST | `csrf`, `dispositivo`, `pilha` | `{ok, sessao, expira_em}` |
| `publico/sessao-status.php` | GET | `?sessao=<codigo>` | `{status, pontos, peso, motivo}` |
| `publico/sessao-cancelar.php` | POST | `csrf`, `sessao` | `{ok}` |

### Rotas do dispositivo

Autenticação por dois headers: `X-Dispositivo: <codigo>` e `X-Chave: <chave>`,
verificada com `password_verify` contra `dispositivo.chave_hash`. Em HTTP puro isso
trafega em claro — o `README.md` deixará registrado que produção exige HTTPS.

| Rota | Método | Corpo | Saída |
|---|---|---|---|
| `dispositivo/autorizacao.php` | GET | — | `{autorizacao: null}` ou `{autorizacao:{sessao, pilha, pontos_por_grama, expira_em, chave_conclusao}}` |
| `dispositivo/evento.php` | POST | `{sessao, etapa, distancia_cm, metal, peso_g}` | `{ok}` |
| `dispositivo/concluir.php` | POST | `{sessao, chave_conclusao, peso_g, metal, resultado}` | `{ok, descarte, pontos}` |

Todas respondem JSON com código HTTP adequado: 200, 400 (entrada inválida),
401 (dispositivo não autenticado), 404 (sessão inexistente), 409 (sessão já
concluída/expirada/cancelada), 503 (banco fora).

### Transação da conclusão

```
BEGIN
  SELECT ... FROM sessao_descarte WHERE codigo = ? FOR UPDATE
  -- valida status, expiração, chave_conclusao, dispositivo
  INSERT INTO descarte (data, status, peso_total_gramas, pontos_gerados,
                        fk_usuario_id, fk_ponto_coleta_id,
                        fk_dispositivo_id, fk_sessao_descarte_id)
         VALUES (CURDATE(), 'aprovado', ?, ?, ?, ?, ?, ?)
  INSERT INTO descarte_pilha (fk_descarte_id, fk_pilhas_id, peso_gramas) VALUES (...)
  UPDATE usuario SET pontos = pontos + ?, updated_at = NOW() WHERE id = ?
  UPDATE sessao_descarte SET status='concluida', ... WHERE id = ?
COMMIT
```

Se a sessão já estiver `concluida`, a rota **não** repete nada: devolve 200 com o
descarte e os pontos originais. Reenvio é seguro.

## 8. Segurança

Validações em `concluir.php`, todas obrigatórias:

1. Dispositivo autenticado (`X-Chave` confere com `chave_hash`) e `ativo = 1`.
2. Sessão existe e pertence **àquele** dispositivo.
3. `chave_conclusao` confere (`hash_equals`).
4. `status` é `autorizada` ou `em_andamento` — nunca `concluida`, `cancelada`,
   `expirada`.
5. `expira_em > NOW()`.
6. Peso numérico, `> 0` e `<= PESO_MAXIMO_G` (constante no `config.php`), para barrar
   leitura absurda da balança descalibrada.
7. Resultado do dispositivo indica metal; não-metal registra `recusada` com motivo e
   **não** gera pontos.
8. Pontos calculados pelo backend a partir de `pilhas.pontos_por_grama`. O que o
   dispositivo manda sobre pontos é ignorado.
9. `fk_sessao_descarte_id UNIQUE` impede crédito duplo mesmo com reenvio.

Nas rotas do usuário: guarda de `cidadao.php` (sessão + `user_role`), CSRF em todo
POST, `fk_usuario_id` vindo **só** de `$_SESSION['user_id']`, nunca do POST — a mesma
regra que `perfil-salvar.php` já adota.

Expiração: `expirar_sessoes()` roda no início de cada rota e marca como `expirada`
tudo que passou de `expira_em`. Não depende de cron.

## 9. Firmware

### Arquivos novos

| Arquivo | Conteúdo |
|---|---|
| `src/config_backend.exemplo.h` | `BACKEND_URL`, `DISPOSITIVO_CODIGO`, `DISPOSITIVO_CHAVE` — **placeholders** |
| `src/nuvem.h` / `src/nuvem.cpp` | cliente do backend, task do core 0, estado da sessão, mutex |
| `src/registro.h` / `src/registro.cpp` | log com níveis INFO/WARN/ERROR + buffer circular servido em `/logs` |

`config_backend.h` entra no `.gitignore`, como já acontece com `config_wifi.h`.
Nenhuma URL de produção, chave ou credencial será inventada.

### Mudanças em `main.cpp`

Aditivas. A pinagem, os tempos e a lógica física do ciclo ficam intactos.

- `setup()`: inicia o registro, lê o MAC, cria a task de nuvem;
- `loop()`: ao terminar um ciclo com metal e peso válido, chama
  `nuvem_concluir(peso_g, metal)` — que só enfileira, não bloqueia;
- os `Serial.printf` existentes passam pelo `registro` (continuam no Serial e também
  ficam disponíveis em `/logs`).

### Máquina de estados da sessão (no ESP32)

```
OCIOSO ──autorização recebida──▶ AUTORIZADO ──objeto detectado──▶ EM_ANDAMENTO
   ▲                                  │                                │
   └──────── expirou / enviado ◀───────┴────────── ciclo concluído ─────┘
```

Fora de `AUTORIZADO`/`EM_ANDAMENTO` o ciclo físico roda igual, só não reporta.

### Rotas locais

Mantidas: `/`, `/3d`, `/style.css`, `/app.js`, `/status`, `/sensor`,
`/atuador?estado=on|off`.

Novas:

| Rota | Resposta |
|---|---|
| `GET /servo` | posição e estado dos dois servos, pinos, limites |
| `GET /rede` | IP, **MAC**, SSID, RSSI, gateway, máscara, estado da conexão, nº de reconexões |
| `GET /descarte` | estado da sessão: `ocioso`/`autorizado`/`em_andamento`/`enviando`/`concluido`, peso, pilha declarada, últimos pontos |
| `GET /logs` | últimas N linhas do buffer circular, com nível |

`/status` ganha `mac`, `backend_estado` (`ok`/`sem_rede`/`erro_http`/`nao_configurado`),
`backend_ultimo_contato_s` e `sessao_estado`. A chave do dispositivo nunca aparece em
nenhuma resposta nem em log.

### Página de controle local

`src/client/index.html` ganha **um painel adicional** ("Descarte e backend") e
`src/client/app.js` passa a consultar `/descarte`. Nada do layout atual é removido.
Depois de editar, rodar `python tools/gerar_pagina_web.py`.

A página inicial aprovada do site (`Descarte Aqui - Web/index.html`) não é tocada.

## 10. Tratamento de erros e resiliência

| Falha | Comportamento |
|---|---|
| Wi-Fi cai | servidor local para de ser alcançável (não há rede); reconexão automática a cada 10 s, como hoje |
| Backend fora do ar / DNS falha | task de nuvem marca `erro_http`, aplica backoff (3 s → 6 s → 12 s → 30 s, teto), **servidor local intacto** |
| `BACKEND_URL` não configurado | task não sobe; `/status` informa `nao_configurado`; o resto funciona |
| Timeout no envio da conclusão | reenvia até 3 vezes com a mesma `chave_conclusao`; o `UNIQUE` do banco garante crédito único |
| Sessão expira antes do descarte | ESP32 volta a `OCIOSO`; backend marca `expirada`; sem pontos |
| Balança indisponível | conclusão é recusada por peso inválido; registra `ERROR`; sem pontos |
| MySQL fora | rotas respondem 503 com mensagem genérica; detalhe só em `error_log` |

Níveis de log, nos dois lados: INFO (inicialização, conexão, IP/MAC, sensor, servo,
sessão iniciada, descarte concluído), WARN (reconexão, sessão expirada, descarte
cancelado, peso fora da faixa), ERROR (falha de comunicação, erro de banco,
periférico ausente). Sem senhas, sem chaves, sem tokens.

## 11. Testes

Verificáveis nesta máquina:

- `pio run -e esp32doit-devkit-v1` — compilação real do firmware (PlatformIO 6.2.0);
- `php -l` em todos os arquivos PHP (PHP 8.2.12 em `C:\xampp`);
- `php -S` + rotas do dispositivo sem header de autenticação → 401, e com entrada
  inválida → 400, sem precisar de banco;
- com o MySQL do XAMPP ligado: aplicar `01_integracao_iot.sql` e exercitar o fluxo
  com `curl` simulando o ESP32.

Dependem do hardware, e serão declarados como não executados:

- leitura real do HC-SR04, do sensor indutivo e da balança;
- movimento dos servos;
- calibração do `FATOR_CALIBRACAO`;
- o fluxo ponta a ponta com o dispositivo físico e o site publicado.

Nada será simulado nem apresentado como se tivesse rodado em hardware.

## 12. Limitações conhecidas

1. A latência entre abrir a sessão e o ESP32 receber a autorização é o intervalo de
   polling (3 s por padrão).
2. `FATOR_CALIBRACAO 420.0` não foi calibrado; os pontos dependem dessa calibração.
3. A chave do dispositivo em HTTP puro trafega em claro. Produção exige HTTPS.
4. O esquema real do banco não tem DDL versionado no repositório — foi reconstruído
   das migrações Laravel removidas e das queries do PHP. A migração assume esses
   nomes de coluna e deve ser conferida contra o banco da equipe antes de rodar.
5. O backend novo mora no repo IoT. Para o fluxo completo funcionar, a pasta precisa
   ser publicada como `php/iot/` no mesmo domínio do site.
6. Perfis de coletador e administrador continuam sem autenticação, como hoje.

## 13. Arquivos a criar e modificar

Criar:

```
backend/README.md
backend/config.exemplo.php
backend/bootstrap.php
backend/comum.php
backend/local/sessao.php
backend/local/conn.php
backend/sql/01_integracao_iot.sql
backend/publico/descartar.php
backend/publico/sessao-abrir.php
backend/publico/sessao-status.php
backend/publico/sessao-cancelar.php
backend/dispositivo/autorizacao.php
backend/dispositivo/evento.php
backend/dispositivo/concluir.php
src/config_backend.exemplo.h
src/nuvem.h
src/nuvem.cpp
src/registro.h
src/registro.cpp
docs/integracao-iot-backend.md
```

Modificar:

```
src/main.cpp              (aditivo: task, rotas novas, MAC, registro)
src/client/index.html     (painel adicional)
src/client/app.js         (consulta /descarte)
src/client/style.css      (estilos do painel)
src/pagina_web.h          (regenerado pelo script)
.gitignore                (config_backend.h, backend/config.php)
README.md                 (seção da integração)
platformio.ini            (nenhuma lib nova: HTTPClient vem do framework)
```

Não modificar: nada no repo `Descarte Aqui - Web`; nenhum pino, tempo ou lógica
física do ciclo de separação.

---

## 14. O que mudou durante a implementação

A spec foi seguida. Cinco ajustes vieram do contato com o ambiente real:

1. **O SGBD é MariaDB 10.4.32, não MySQL 8.** Descoberto ao subir o servidor do
   XAMPP. A migração foi aplicada e verificada nessa versão: a coluna gerada
   `STORED` com `UNIQUE`, as quatro `CHECK` e as chaves estrangeiras foram
   criadas sem alteração. O arquivo `.sql` continua compatível com os dois.

2. **A `CHECK (expira_em > criada_em)` foi removida.** O teste automatizado a
   derrubou ao encurtar o prazo de uma sessão para exercitar a expiração. A
   restrição engessava a tabela sem proteger nada: o prazo é definido no
   `INSERT` com `DATE_ADD(NOW(), ...)`, e quem decide se a sessão vale é a
   comparação com `NOW()` nas rotas. O motivo está comentado no `.sql`.

3. **`local/conn.php` não existe.** A spec previa uma cópia de apoio do
   `conn.php` do site. Em vez disso, a conexão é construída a partir de
   `config.php`, com uma única fonte de credenciais. O `conn.php` do site
   chama `die()` com HTML quando o banco falha, o que quebraria uma resposta
   JSON; construir a conexão aqui permite devolver 503 corretamente. O
   `sessao.php` continua sendo reaproveitado do site, que é o reuso que
   importa: é dele que vem o compartilhamento do login.

4. **Três arquivos a mais do que a seção 13 previa:**
   `ferramentas/gerar-chave.php` (gera a credencial do dispositivo sem inventar
   chave em nenhum arquivo versionado), `ferramentas/testar-fluxo.php` (as 37
   verificações automatizadas) e `sql/00_estrutura_minima_teste.sql` (andaime
   que permite testar do zero sem depender do banco de outra pessoa, já que o
   DDL real não está versionado em nenhum repositório).

5. **O teste do lado do usuário foi executado**, e não apenas descrito como
   manual: publicando a pasta como `php/iot/` numa cópia descartável do site
   extraída de `origin/main`, com o `login.php` real. Confirmou o
   compartilhamento da sessão PHP, o crédito dos pontos, a exibição no
   `minha-area.php` do site, o cancelamento e o bloqueio após logout.

Um defeito do próprio script de teste também apareceu: `??` trata `null` como
ausente, então não servia para verificar um campo que deve valer `null`.

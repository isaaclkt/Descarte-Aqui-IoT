# Ver o QR Code funcionando — notebook como ponto de acesso

Guia para rodar o fluxo completo na sua máquina: escanear o QR Code com o
celular, fazer o descarte e ver os pontos caírem na conta.

## Por que ponto de acesso, e não o Wi-Fi do lugar

O QR Code carrega o **endereço IP da máquina que serve o site**. Numa rede
comum esse IP vem do roteador por DHCP e muda: trocou de rede, o QR impresso
deixa de funcionar. Pior, redes de faculdade e de condomínio costumam ter
*isolamento de clientes* — os aparelhos não se enxergam, e o celular não
alcança o notebook nem com o IP certo.

Com o notebook criando a rede, os dois problemas somem: o IP passa a ser
sempre o mesmo e o celular está, por definição, na mesma rede.

---

## Passo 0 — Atualizar os dois repositórios

Antes de qualquer coisa, em **cada** um dos dois repositórios:

```bash
git pull
```

Não pule. Uma cópia antiga do repositório **Web** derruba o fluxo inteiro de um
jeito que parece bug do projeto: o login termina na página inicial em vez da
página do equipamento, e o "Criar conta" volta para o formulário sem dizer
nada. Era assim que o código estava há alguns commits; já está corrigido.

## Passo 1 — Ligar o ponto de acesso no Windows

`Configurações` → `Rede e Internet` → `Ponto de acesso móvel`.

Antes de ligar, clique em **Editar** e confira três coisas:

| Campo | O que usar |
|---|---|
| Nome da rede | o que quiser, anote |
| Senha | mínimo 8 caracteres, anote |
| **Banda de rede** | **2,4 GHz** |

> **A banda é obrigatória.** O ESP32-WROOM-32 só fala 2,4 GHz. Se o ponto de
> acesso estiver em 5 GHz, a placa simplesmente não enxerga a rede — e o
> sintoma é o ESP32 reiniciando a busca sem erro nenhum, o que confunde.

Ligue o ponto de acesso e conecte o celular nele.

## Passo 2 — Descobrir o IP (não presuma)

No Windows, o ponto de acesso costuma usar `192.168.137.1`, mas isso **não é
garantido**. Confira:

```bash
ipconfig
```

Procure o adaptador do ponto de acesso — algo como *Conexão Local* ou
*Microsoft Wi-Fi Direct Virtual Adapter* — e anote o **Endereço IPv4**.

Cuidado para não pegar o IP errado: numa máquina com VirtualBox aparece também
um `192.168.56.1`, que não serve.

## Passo 3 — Apontar o firmware para essa rede

Dois arquivos, nenhum deles versionado:

**`src/config_wifi.h`** — o nome e a senha do ponto de acesso que você acabou
de criar:

```c
#define WIFI_SSID  "nome-que-voce-escolheu"
#define WIFI_SENHA "senha-que-voce-escolheu"
```

**`src/config_backend.h`** — o IP do passo 2:

```c
#define BACKEND_URL "http://192.168.137.1/descarte-aqui/php/iot"
```

Se ainda não tem esses arquivos, copie dos modelos:

```bash
cp src/config_wifi.exemplo.h src/config_wifi.h && cp src/config_backend.exemplo.h src/config_backend.h
```

O código e a chave do dispositivo saem de:

```bash
php backend/ferramentas/gerar-chave.php "PUCPR" esp32-pucpr
```

Ele imprime o que colar no `config_backend.h` e o `INSERT` para cadastrar o
equipamento no banco. **A chave aparece uma única vez.**

## Passo 4 — Gerar o QR Code com esse IP

```bash
pip install segno
```

```bash
python tools/gerar_qrcode.py --dispositivo esp32-pucpr --base http://192.168.137.1/descarte-aqui/php/iot --nome "PUCPR"
```

Sai em `docs/qrcodes/`. O script avisa se você passar `localhost`, que o
celular nunca alcança.

## Passo 5 — Subir o site

O site e o banco precisam estar no ar na sua máquina. O passo a passo completo
está em [`backend/README.md`](../backend/README.md); em resumo:

1. publicar o repositório Web em `C:\xampp\htdocs\descarte-aqui`, com a pasta
   `backend/` deste repositório dentro dele como `php/iot`;
2. montar o banco:

```bash
mysql -u root < "../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql"
```

```bash
mysql -u root descarteaqui < backend/sql/01_integracao_iot.sql
```

3. rodar o `INSERT` do dispositivo que o passo 3 imprimiu;
4. ligar **Apache** e **MySQL** no painel do XAMPP.

Monte o banco pela **linha de comando**. Pelo phpMyAdmin a importação costuma
parar no meio: o script define functions num bloco `DELIMITER $$`, e o que vem
depois — inclusive os tipos de pilha — não entra. As tabelas ficam lá, então
parece que deu certo.

Antes de ir para o celular, confirme:

```bash
php backend/ferramentas/conferir-banco.php
```

Ele só lê o banco e diz, item por item, o que falta e qual script resolve.

## Passo 6 — Liberar a porta 80 no Firewall

Quando o Apache sobe pela primeira vez, o Windows pergunta se pode liberar.
Se você negou, ou se o celular não abre a página, é quase sempre isso.

O ponto de acesso cria um perfil de rede próprio, e a regra do Apache precisa
valer para ele. Confira no PowerShell:

```powershell
Get-NetFirewallRule -Direction Inbound -Enabled True -Action Allow | Where-Object { $_.DisplayName -match 'apache' } | Select-Object DisplayName, Profile
```

Compare com o perfil da rede do ponto de acesso:

```powershell
Get-NetConnectionProfile | Select-Object InterfaceAlias, NetworkCategory
```

Se os perfis não baterem, ajuste a regra pelo Firewall do Windows. É
configuração de segurança da máquina — faça você, conscientemente.

## Passo 7 — Testar

1. Abra `http://SEU-IP/descarte-aqui/` **no celular**. Se a página inicial não
   carregar, pare aqui: é rede, não é o projeto.
2. Escaneie o QR Code da etiqueta (dá para escanear da tela do notebook).
3. Deve abrir a página do equipamento mostrando **PUCPR**.
4. Crie uma conta pelo botão da própria página.
5. Você volta **direto** para a tela do equipamento — sem escanear de novo.
6. Escolha o tipo de pilha e toque em *Iniciar descarte*.
7. Faça o descarte na placa. Sem a placa, suba o simulador:

```bash
python tools/simular_esp32.py
```

O simulador percebe a autorização em até 3 segundos e roda o ciclo sozinho. A
tela do celular vira para *Descarte registrado*, com os pontos.

> O simulador **não é a placa**: sensores, servos e balança são números
> inventados por ele. Serve para desenvolver e demonstrar o software; para a
> entrega final o regulamento exige o protótipo físico.

---

## Quando não der certo

| Sintoma | Causa provável |
|---|---|
| ESP32 não acha a rede | ponto de acesso em 5 GHz; mude para 2,4 GHz |
| Celular não abre a página, o notebook abre | Firewall na porta 80, ou perfil de rede diferente |
| Página abre mas diz "Equipamento não encontrado" | o dispositivo não está na tabela `dispositivo` |
| Página abre mas diz "Nenhum tipo de pilha cadastrado" | banco importado pela metade — rode `php backend/ferramentas/conferir-banco.php` |
| QR abre endereço que não responde | o IP mudou; gere o QR de novo |
| Tela fica esperando para sempre | nem a placa nem o simulador estão rodando |
| Login leva para a página inicial, não para o equipamento | repositório Web desatualizado — `git pull` (passo 0) |
| "Criar conta" volta para o formulário sem mensagem | repositório Web desatualizado — `git pull` (passo 0) |
| "Criar conta" acusa e-mail ou CPF em uso | a conta já existe; é só entrar com ela |

## Se o ponto de acesso não estiver disponível

Alguns notebooks corporativos têm o recurso bloqueado. Alternativas, da melhor
para a pior:

1. **Hotspot do celular**, com o notebook conectado nele. O IP do notebook muda,
   então gere o QR depois de conectar.
2. **Reserva de DHCP no roteador**, amarrando o MAC do notebook a um IP fixo.
   Só funciona em rede que vocês controlam.
3. **Publicar o site** num domínio. É a única solução definitiva: o QR passa a
   valer para sempre e resolve o HTTPS junto.

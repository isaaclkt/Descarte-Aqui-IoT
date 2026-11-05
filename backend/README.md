# Backend da integração IoT

Rotas PHP que ligam o dispositivo ESP32 ao sistema web Descarte Aqui: o usuário
escaneia o QR Code do equipamento, faz o descarte físico, e os pontos caem na
conta dele.

PHP puro com `mysqli`, sem framework e sem Composer — as mesmas convenções da
pasta `php/` do repositório `Descarte Aqui - Web`.

> **Esta pasta não tem login próprio.** Ela usa a sessão criada por
> `php/login.php` do site. Não existe um segundo sistema de autenticação.

## 1. Onde publicar

A pasta precisa ficar **dentro do site**, como `php/iot/`:

```
<raiz do site>/
├── index.html
├── assets/
├── paginas/
└── php/
    ├── login.php          ← já existe no repositório Web
    ├── sessao.php         ← já existe
    ├── conn.php           ← já existe
    └── iot/               ← copie esta pasta backend/ para cá
        ├── bootstrap.php
        ├── publico/
        └── dispositivo/
```

Isso não é preferência de organização: **o cookie de sessão do PHP só vale
dentro do mesmo domínio**. Fora do site, o login não atravessa e o usuário
nunca é reconhecido. O `bootstrap.php` detecta a situação:

```php
$base = is_file(__DIR__ . '/../sessao.php') ? __DIR__ . '/..' : __DIR__ . '/local';
```

Publicada em `php/iot/`, ela carrega o `sessao.php` **do site**. Rodando
isolada (teste com `php -S`), usa a cópia de apoio em `local/`.

## 2. Instalação

### 2.1 Banco de dados

Primeiro o banco da equipe — a entrega final do PjBL 3, que cria `descarteaqui`
com tabelas, views, functions, procedures e triggers. É reexecutável:

```bash
mysql -u root < "../../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql"
```

Depois a migração da integração:

```bash
mysql -u root descarteaqui < sql/01_integracao_iot.sql
```

A migração **só adiciona**: cria `dispositivo`, `sessao_descarte` e
`evento_iot`, e acrescenta duas colunas a `descarte`. Nada existente é apagado
ou alterado de forma destrutiva.

> **Não** rode os scripts de `docs/banco-de-dados/sql/` do repositório Web: são
> de outra versão do modelo (banco `descarte_aqui`, com underline). O próprio
> `docs/integracao-minha-area.md` da equipe avisa isso.

Rode a migração uma vez só: MySQL e MariaDB não aceitam
`ADD COLUMN IF NOT EXISTS`. O bloco de rollback está comentado no fim do
arquivo `.sql`.

### 2.2 Configuração

```bash
cp config.exemplo.php config.php     # Windows: copy config.exemplo.php config.php
```

| Chave | O que faz |
|---|---|
| `banco.*` | credenciais do MySQL — os mesmos valores de `php/conn.php` |
| `sessao_minutos` | validade da autorização (padrão 3) |
| `peso_minimo_g` / `peso_maximo_g` | faixa de peso aceita; fora dela não há pontos |
| `url_base` | endereço público, só para montar a URL do QR Code |

`config.php` está no `.gitignore`. Sem ele a integração ainda sobe com os
padrões do XAMPP e registra um `WARN` no log.

### 2.3 Cadastrar o dispositivo

```bash
php ferramentas/gerar-chave.php "Ponto de coleta - Bloco 7"
```

Imprime três coisas: o que colar em `src/config_backend.h` do firmware, o
`INSERT` para cadastrar o dispositivo, e a URL do QR Code.

A chave em texto puro aparece **uma única vez** — o banco guarda só o
`password_hash`. Perdeu, gere outra.

### 2.4 Gerar o QR Code

Com o dispositivo já cadastrado, a imagem sai de uma ferramenta na raiz do
repositório IoT:

```bash
python tools/gerar_qrcode.py --dispositivo esp32-bloco7 --base http://192.168.0.15/descarte-aqui/php/iot --nome "Ponto de coleta - Bloco 7"
```

Precisa de `pip install segno` na máquina de quem gera. Sai em `docs/qrcodes/`
o QR Code em SVG e uma etiqueta de 80×120 mm para imprimir.

Antes de imprimir, escaneie da própria tela e confirme que abre a página certa:
se o dispositivo não estiver na tabela `dispositivo`, a página responde
"Equipamento não encontrado".

## 3. Rotas

### Do usuário (exigem login de cidadão)

| Rota | Método | O que faz |
|---|---|---|
| `publico/descartar.php?d=<codigo>` | GET | página do QR Code |
| `publico/sessao-abrir.php` | POST + CSRF | inicia a sessão de descarte |
| `publico/sessao-status.php?sessao=<codigo>` | GET | andamento, para a página consultar |
| `publico/sessao-cancelar.php` | POST + CSRF | cancela e libera o equipamento |

O `fk_usuario_id` vem **sempre** de `$_SESSION['user_id']`, nunca do POST — a
mesma regra que `php/perfil-salvar.php` já adota.

### Do dispositivo (headers `X-Dispositivo` e `X-Chave`)

| Rota | Método | O que faz |
|---|---|---|
| `dispositivo/autorizacao.php` | GET | o ESP32 pergunta se há autorização pendente |
| `dispositivo/evento.php` | POST | telemetria do ciclo (não pontua) |
| `dispositivo/concluir.php` | POST | conclusão: registra o descarte e credita |

Códigos: 200, 400 (entrada inválida), 401 (dispositivo não autenticado), 403
(chave ou dono errado), 404 (sessão inexistente), 405 (método), 409 (sessão
encerrada ou expirada), 422 (peso fora da faixa), 503 (banco indisponível).

## 4. Como os pontos são calculados

A regra é a do banco:

```sql
pontos = FLOOR( fn_calcular_pontos(peso_medido_g, pilhas.pontos_por_grama) )
```

`fn_calcular_pontos()` é a function da equipe (PjBL 3). Ela aplica **bônus por
volume**: fator 1,10 a partir de 500 g e 1,20 a partir de 1 kg. O `FLOOR` é o
mesmo de `sp_registrar_descarte`, porque `usuario.pontos` é `INT`.

`concluir.php` chama essa function em vez de refazer a conta em PHP — duas
implementações da mesma regra divergiriam um dia. Não chamamos
`sp_registrar_descarte` inteira porque ela abre e fecha a própria transação, o
que encerraria a nossa antes de gravarmos o vínculo com a sessão, que é o que
impede creditar duas vezes.

O usuário declara só o **tipo** de pilha. O peso vem da balança HX711. O
dispositivo nunca envia pontos, e se enviar, são ignorados.

## 5. O que impede ponto duplicado

Duas travas, as duas no banco, não no PHP:

- `descarte.fk_sessao_descarte_id UNIQUE` — uma sessão nunca gera dois
  descartes. Se o ESP32 reenviar a conclusão depois de um timeout de rede, a
  rota devolve o descarte original com `"repetido": true` e não credita nada.
- `sessao_descarte.dispositivo_ocupado UNIQUE` (coluna gerada a partir do
  `status`) — duas pessoas não ocupam o mesmo equipamento ao mesmo tempo.

Mais o `SELECT ... FOR UPDATE` em `concluir.php`, que serializa duas conclusões
simultâneas da mesma sessão.

## 6. Testes

### Automatizado (lado do dispositivo)

O teste cria descartes e credita pontos, então use uma base **descartável**.
Para voltar ao estado inicial, apague o banco e refaça os dois passos:

```bash
mysql -u root -e "DROP DATABASE IF EXISTS descarteaqui;" && mysql -u root < "../../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql" && mysql -u root descarteaqui < sql/01_integracao_iot.sql
```

> **Não basta rodar o script do PjBL 3 de novo.** Ele é reexecutável, mas faz
> `SET FOREIGN_KEY_CHECKS = 0` e `TRUNCATE descarte`. Sobre um banco que já tem
> a integração, isso apaga os descartes e deixa `sessao_descarte.fk_descarte_id`
> apontando para linhas que não existem mais — sem o banco reclamar. Medi isso:
> sobraram 2 sessões órfãs. Apague o banco inteiro.

Suba o servidor e rode:

```bash
php -S 127.0.0.1:8099 -t .
```

```bash
php ferramentas/testar-fluxo.php http://127.0.0.1:8099
```

São 41 verificações: autorização, telemetria, conclusão, crédito, **bônus por
volume**, idempotência do reenvio, chave errada, material não-metálico, peso
fora da faixa, sessão expirada, exclusividade do dispositivo e ausência de
chaves no log.

O número de pontos esperado não é calculado pelo teste: é perguntado ao banco,
com `fn_calcular_pontos()`. O teste verifica que o backend concorda com a regra
da equipe, não com uma fórmula redigitada no arquivo de teste.

### Manual (lado do usuário)

O login precisa de navegador, então estas etapas são conferidas à mão:

1. abrir `publico/descartar.php?d=<codigo>` **sem** estar logado → a página
   oferece entrar, sem iniciar nada;
2. entrar pelo site e abrir a página de novo → aparece o seletor de tipo;
3. iniciar o descarte → a tela passa a acompanhar o andamento;
4. concluir no equipamento → a tela mostra os pontos e o saldo novo;
5. cancelar uma sessão → o equipamento volta a ficar livre;
6. sair da conta e recarregar → as rotas respondem 401.

## 7. Limitações

1. Em HTTP puro a chave do dispositivo trafega em claro. Produção exige HTTPS.
2. Depois do login o site volta para `minha-area.php`, então o usuário precisa
   escanear o QR Code outra vez. Resolver isso seria uma linha em
   `php/login.php`, que é do outro repositório e não foi alterado.
3. Perfis de coletador e administrador continuam sem autenticação, como já era.
4. Não há tela de administração de dispositivos: o cadastro é pelo `INSERT` que
   `ferramentas/gerar-chave.php` imprime.

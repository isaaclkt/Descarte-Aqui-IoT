# QR Codes dos equipamentos

Arquivos gerados por [`tools/gerar_qrcode.py`](../../tools/gerar_qrcode.py).

| Arquivo | Para quê |
|---|---|
| `esp32-pucpr-etiqueta.svg` | **é este que se imprime** — 80 × 120 mm, com nome, código e instrução |
| `esp32-pucpr.svg` | o QR Code sozinho, vetorial |
| `esp32-pucpr.png` | o QR Code em imagem, para slides e documentos |

Abra o `.svg` no navegador e imprima com `Ctrl+P`. Sendo vetorial, não borra em
nenhum tamanho.

## ⚠ Antes de imprimir, leia isto

**O QR Code carrega o endereço de uma rede específica** — o IP que a máquina de
quem gerou por último tinha naquela rede, e o caminho do site nela. Em outro
lugar, na PUCPR ou na casa de outro integrante, o celular não alcança esse
endereço e a etiqueta impressa não serve para nada.

Por isso o arquivo versionado muda de endereço toda vez que alguém regenera, e
não adianta anotar o IP aqui. Para ver para onde o arquivo de agora aponta,
escaneie com o próprio celular.

Então este arquivo existe para vocês **verem como ficou** e conferirem o
layout, não para imprimir e colar em definitivo.

## Gerar o seu

Precisa do `segno` uma vez por máquina (Python puro, não entra no firmware nem
no site):

```bash
pip install segno
```

Descubra o IP da sua máquina e gere:

```bash
python tools/gerar_qrcode.py --dispositivo esp32-pucpr --base http://SEU-IP/descarte-aqui/php/iot --nome "PUCPR"
```

Para a apresentação, o caminho que não depende do Wi-Fi do lugar é o notebook
virar o ponto de acesso: o IP passa a ser sempre o mesmo. O passo a passo está
em [`docs/configurar-hotspot.md`](../configurar-hotspot.md).

## O QR não contém segredo

Só o código público do equipamento (`esp32-pucpr`). Sem senha, sem token, sem
dado pessoal — quem identifica a pessoa é a sessão do site, depois do login.
Por isso dá para versionar e imprimir sem medo.

O que **não** está versionado é `src/config_backend.h`, que tem a chave secreta
do dispositivo. Cada integrante gera a sua com
`php backend/ferramentas/gerar-chave.php`.

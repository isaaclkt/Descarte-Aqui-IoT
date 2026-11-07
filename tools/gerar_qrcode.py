"""
Gera o QR Code que vai colado no equipamento.

O QR Code aponta para a pagina do SITE, nunca para o ESP32: e o site que
identifica o usuario logado. Ele carrega so o codigo publico do dispositivo -
sem senha, sem token, sem dado pessoal.

Como gerar_pagina_web.py, este script e uma ferramenta de desenvolvimento:
roda na maquina de quem gera a etiqueta e NAO e dependencia do firmware nem
do site. Nada aqui entra no ESP32.

Uso tipico (XAMPP na rede local, para a demonstracao):

    python tools/gerar_qrcode.py \\
        --base http://192.168.0.15/descarte-aqui/php/iot \\
        --dispositivo esp32-bloco7 \\
        --nome "Ponto de coleta - Bloco 7"

Com o site publicado:

    python tools/gerar_qrcode.py \\
        --base https://descarteaqui.exemplo.br/php/iot \\
        --dispositivo esp32-bloco7 \\
        --nome "Ponto de coleta - Bloco 7"

Ou passando a URL inteira, se ela ja estiver pronta:

    python tools/gerar_qrcode.py --url "https://..." --dispositivo esp32-bloco7

Saida em docs/qrcodes/:
    <codigo>.svg            QR Code sozinho (vetorial, escala sem borrar)
    <codigo>-etiqueta.svg   etiqueta pronta para imprimir e colar
    <codigo>.png            opcional, com --png (para slides e Word)

ATENCAO: o celular precisa ALCANCAR o endereco. "localhost" nao serve - o
celular nao e o PC. Numa rede local, use o IP da maquina que roda o XAMPP e
deixe o celular no mesmo Wi-Fi.

Instalacao da unica dependencia (so nesta maquina, nao no projeto):

    pip install segno
"""

import argparse
import os
import re
import sys
import tempfile
from pathlib import Path

try:
    import segno
except ImportError:
    sys.exit(
        "ERRO: a biblioteca 'segno' nao esta instalada.\n"
        "      Instale com:  pip install segno\n"
        "      Ela e Python puro, sem dependencias, e fica so nesta maquina:\n"
        "      nao entra no firmware nem no site."
    )

RAIZ = Path(__file__).resolve().parent.parent
DESTINO = RAIZ / "docs" / "qrcodes"

# Mesmo formato aceito por dispositivo.codigo no banco (ck_dispositivo_codigo).
PADRAO_CODIGO = re.compile(r"^[A-Za-z0-9_-]{4,32}$")

# Medidas da etiqueta, em milimetros. Todas as posicoes verticais saem destas
# constantes, nunca escritas a mao no SVG: foi assim que a primeira versao
# acabou com o codigo do dispositivo impresso FORA da etiqueta.
ETIQUETA_LARGURA = 80
ETIQUETA_ALTURA = 120

QR_LADO = 56          # lado do QR Code
QR_TOPO = 34          # onde o QR comeca

# Zona de silencio exigida pela norma do QR Code: 4 modulos de branco em
# volta. Abaixo disso, leitor de celular comeca a falhar. Como o QR tem
# QR_LADO / modulos de lado por modulo, isto vira milimetros no desenho.
QUIET_ZONE_MODULOS = 4

# Linha de base de cada texto, de cima para baixo. conferir_layout() valida
# que estao em ordem, dentro da etiqueta e longe o bastante do QR Code.
LINHA_TITULO = 14
LINHA_CHAMADA_1 = 21
LINHA_CHAMADA_2 = 27
LINHA_NOME = 99       # QR termina em QR_TOPO + QR_LADO = 90
LINHA_CODIGO = 105
LINHA_RODAPE = 114

COR_VERDE = "#04301A"
COR_CLARA = "#3C9A1C"
COR_TEXTO = "#3f5a4a"


def montar_url(base: str, codigo: str) -> str:
    """Monta a URL da pagina de descarte a partir da raiz das rotas."""
    return f"{base.rstrip('/')}/publico/descartar.php?d={codigo}"


def escapar_xml(texto: str) -> str:
    return (
        texto.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def quadrados_do_qr(qr, lado_mm: float, deslocamento_x: float, deslocamento_y: float) -> str:
    """
    Converte a matriz do QR Code em retangulos SVG.

    Desenhamos a matriz nos mesmos (modulos) que o segno calcula, em vez de
    embutir o SVG dele: assim a etiqueta e um arquivo unico, legivel, sem
    imagem aninhada nem data URI.
    """
    matriz = [list(linha) for linha in qr.matrix]
    modulos = len(matriz)
    tamanho = lado_mm / modulos

    partes = []
    for y, linha in enumerate(matriz):
        x = 0
        while x < modulos:
            if not linha[x]:
                x += 1
                continue

            # Junta modulos escuros vizinhos num retangulo so: o arquivo fica
            # bem menor e o resultado impresso e identico.
            inicio = x
            while x < modulos and linha[x]:
                x += 1

            largura = (x - inicio) * tamanho
            partes.append(
                f'<rect x="{deslocamento_x + inicio * tamanho:.4f}" '
                f'y="{deslocamento_y + y * tamanho:.4f}" '
                f'width="{largura:.4f}" height="{tamanho:.4f}"/>'
            )

    return "\n      ".join(partes)


def conferir_layout(qr) -> None:
    """
    Garante que nada da etiqueta cai fora dela e que a zona de silencio do QR
    e respeitada.

    Existe porque a primeira versao imprimia o codigo do dispositivo em
    y = 110 mm numa etiqueta de 105 mm: simplesmente sumia. Um erro de
    aritmetica que so aparece quando alguem vai imprimir.
    """
    modulos = len(qr.matrix)
    quiet_mm = (QR_LADO / modulos) * QUIET_ZONE_MODULOS

    folga_acima = QR_TOPO - LINHA_CHAMADA_2
    folga_abaixo = LINHA_NOME - (QR_TOPO + QR_LADO)
    folga_lateral = (ETIQUETA_LARGURA - QR_LADO) / 2

    problemas = []

    if LINHA_RODAPE > ETIQUETA_ALTURA - 2:
        problemas.append(f"rodape em {LINHA_RODAPE} mm ultrapassa a etiqueta "
                         f"de {ETIQUETA_ALTURA} mm")

    for nome_linha, y in (("nome", LINHA_NOME), ("codigo", LINHA_CODIGO),
                          ("rodape", LINHA_RODAPE)):
        if y > ETIQUETA_ALTURA:
            problemas.append(f"linha '{nome_linha}' em {y} mm fica fora da etiqueta")

    if LINHA_CODIGO <= LINHA_NOME or LINHA_RODAPE <= LINHA_CODIGO:
        problemas.append("as linhas de texto estao fora de ordem e vao se sobrepor")

    for lado, folga in (("acima", folga_acima), ("abaixo", folga_abaixo),
                        ("lateral", folga_lateral)):
        if folga < quiet_mm:
            problemas.append(f"zona de silencio {lado} com {folga:.1f} mm; "
                             f"a norma pede {quiet_mm:.1f} mm "
                             f"({QUIET_ZONE_MODULOS} modulos)")

    if problemas:
        sys.exit("ERRO no layout da etiqueta:\n  - " + "\n  - ".join(problemas))


def gerar_etiqueta(qr, codigo: str, nome: str) -> str:
    """Etiqueta pronta para imprimir, recortar e colar no equipamento."""
    conferir_layout(qr)

    qr_x = (ETIQUETA_LARGURA - QR_LADO) / 2
    meio = ETIQUETA_LARGURA / 2

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="{ETIQUETA_LARGURA}mm" height="{ETIQUETA_ALTURA}mm"
     viewBox="0 0 {ETIQUETA_LARGURA} {ETIQUETA_ALTURA}">

  <title>QR Code - {escapar_xml(nome)}</title>

  <rect width="{ETIQUETA_LARGURA}" height="{ETIQUETA_ALTURA}" fill="#ffffff"/>
  <rect x="1" y="1" width="{ETIQUETA_LARGURA - 2}" height="{ETIQUETA_ALTURA - 2}"
        fill="none" stroke="{COR_VERDE}" stroke-width="0.6" rx="3"/>

  <text x="{meio}" y="{LINHA_TITULO}" text-anchor="middle"
        font-family="Lexend, Verdana, sans-serif" font-size="7"
        font-weight="700" fill="{COR_VERDE}">Descarte Aqui</text>

  <text x="{meio}" y="{LINHA_CHAMADA_1}" text-anchor="middle"
        font-family="Public Sans, Verdana, sans-serif" font-size="3.6"
        fill="{COR_TEXTO}">Aponte a camera do celular</text>

  <text x="{meio}" y="{LINHA_CHAMADA_2}" text-anchor="middle"
        font-family="Public Sans, Verdana, sans-serif" font-size="3.6"
        fill="{COR_TEXTO}">para ganhar pontos pelo seu descarte</text>

  <g fill="{COR_VERDE}" shape-rendering="crispEdges">
      {quadrados_do_qr(qr, QR_LADO, qr_x, QR_TOPO)}
  </g>

  <text x="{meio}" y="{LINHA_NOME}" text-anchor="middle"
        font-family="Lexend, Verdana, sans-serif" font-size="4.5"
        font-weight="600" fill="{COR_VERDE}">{escapar_xml(nome)}</text>

  <text x="{meio}" y="{LINHA_CODIGO}" text-anchor="middle"
        font-family="monospace" font-size="3.2"
        fill="{COR_CLARA}">{escapar_xml(codigo)}</text>

  <text x="{meio}" y="{LINHA_RODAPE}" text-anchor="middle"
        font-family="Public Sans, Verdana, sans-serif" font-size="2.6"
        fill="{COR_TEXTO}">E preciso estar logado no site para receber os pontos</text>
</svg>
"""


def main() -> None:
    analisador = argparse.ArgumentParser(
        description="Gera o QR Code de um dispositivo do Descarte Aqui.",
        epilog="O QR Code carrega so o codigo publico do dispositivo.",
    )
    analisador.add_argument("--dispositivo", required=True,
                            help="codigo publico do dispositivo (dispositivo.codigo no banco)")
    analisador.add_argument("--base",
                            help="raiz das rotas do backend, sem barra final "
                                 "(ex.: http://192.168.0.15/descarte-aqui/php/iot)")
    analisador.add_argument("--url",
                            help="URL completa, no lugar de --base")
    analisador.add_argument("--nome", default="",
                            help="nome do ponto de coleta, impresso na etiqueta")
    analisador.add_argument("--png", action="store_true",
                            help="gera tambem um PNG, para slides e Word")

    args = analisador.parse_args()

    if not PADRAO_CODIGO.match(args.dispositivo):
        sys.exit("ERRO: o codigo do dispositivo deve ter de 4 a 32 caracteres "
                 "entre A-Z a-z 0-9 _ - (mesma regra do banco).")

    if bool(args.base) == bool(args.url):
        sys.exit("ERRO: informe --base OU --url, nunca os dois.")

    url = args.url if args.url else montar_url(args.base, args.dispositivo)

    if not url.lower().startswith(("http://", "https://")):
        sys.exit("ERRO: a URL precisa comecar com http:// ou https://")

    if "localhost" in url or "127.0.0.1" in url:
        print("AVISO: a URL aponta para localhost. O celular nao vai alcancar:")
        print("       use o IP da maquina na rede local, ou o dominio publico.\n")

    nome = args.nome or args.dispositivo

    # Correcao de erro 'H' (recupera ~30%): a etiqueta vai ficar colada num
    # equipamento, sujeita a risco, poeira e dedo em cima.
    qr = segno.make(url, error="h")

    DESTINO.mkdir(parents=True, exist_ok=True)

    caminho_qr = DESTINO / f"{args.dispositivo}.svg"
    qr.save(str(caminho_qr), scale=10, border=4, dark=COR_VERDE)

    caminho_etiqueta = DESTINO / f"{args.dispositivo}-etiqueta.svg"
    caminho_etiqueta.write_text(gerar_etiqueta(qr, args.dispositivo, nome),
                                encoding="utf-8")

    print(f"URL codificada: {url}")
    print(f"Versao do QR:   {qr.version} (correcao de erro H, ~30%)")
    print()
    print(f"  {caminho_qr.relative_to(RAIZ)}")
    print(f"  {caminho_etiqueta.relative_to(RAIZ)}")

    if args.png:
        caminho_png = DESTINO / f"{args.dispositivo}.png"
        # OneDrive pode impedir a abertura direta de um PNG ja existente.
        # Gera em um arquivo novo e troca o antigo somente apos salvar tudo.
        with tempfile.NamedTemporaryFile(
            dir=DESTINO, prefix=f".{args.dispositivo}-", suffix=".png", delete=False
        ) as temporario:
            caminho_temporario = Path(temporario.name)
        try:
            qr.save(str(caminho_temporario), scale=10, border=4, dark=COR_VERDE)
            os.replace(caminho_temporario, caminho_png)
        finally:
            caminho_temporario.unlink(missing_ok=True)
        print(f"  {caminho_png.relative_to(RAIZ)}")

    print()
    print("Antes de imprimir: escaneie da tela e confira que abre a pagina certa.")
    print("O dispositivo precisa estar cadastrado na tabela dispositivo, senao")
    print("a pagina responde \"Equipamento nao encontrado\".")


if __name__ == "__main__":
    main()

"""
Gera src/pagina_web.h a partir dos arquivos de src/client/.

O ESP32 nao tem sistema de arquivos configurado neste projeto: a pagina web
viaja dentro do proprio firmware, como texto na memoria de programa (PROGMEM).
Para nao manter o HTML/CSS/JS duplicado em dois lugares, editamos sempre
src/client/ e rodamos este script para regerar o header.

Uso:
    python tools/gerar_pagina_web.py
"""

from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
ORIGEM = RAIZ / "src" / "client"
DESTINO = RAIZ / "src" / "pagina_web.h"

# nome do arquivo em src/client  ->  nome da constante em C++
ARQUIVOS = [
    ("index.html", "PAGINA_HTML"),
    ("style.css", "PAGINA_CSS"),
    ("app.js", "PAGINA_JS"),
    ("ponto_coleta3d.html", "PAGINA_3D"),
]

DELIMITADOR = "rawliteral"


def ler(nome):
    caminho = ORIGEM / nome
    texto = caminho.read_text(encoding="utf-8")

    # Um raw string literal de C++ termina em )rawliteral" - se essa sequencia
    # aparecesse no conteudo, o firmware nem compilaria.
    if f"){DELIMITADOR}\"" in texto:
        raise SystemExit(f"ERRO: {nome} contem a sequencia )rawliteral\" e quebraria o literal C++.")

    return texto


def main():
    partes = [
        "// ARQUIVO GERADO AUTOMATICAMENTE - NAO EDITE A MAO.",
        "// Edite os arquivos em src/client/ e rode: python tools/gerar_pagina_web.py",
        "",
        "#ifndef PAGINA_WEB_H",
        "#define PAGINA_WEB_H",
        "",
        "#include <Arduino.h>",
        "",
    ]

    for nome, constante in ARQUIVOS:
        conteudo = ler(nome)
        partes.append(f"// ---------- {nome} ----------")
        partes.append(f"static const char {constante}[] PROGMEM = R\"{DELIMITADOR}(")
        partes.append(conteudo.rstrip("\n"))
        partes.append(f"){DELIMITADOR}\";")
        partes.append("")

    partes.append("#endif  // PAGINA_WEB_H")
    partes.append("")

    DESTINO.write_text("\n".join(partes), encoding="utf-8")

    tamanho = DESTINO.stat().st_size
    print(f"Gerado {DESTINO.relative_to(RAIZ)} ({tamanho} bytes)")
    for nome, constante in ARQUIVOS:
        print(f"  {constante:<12} {(ORIGEM / nome).stat().st_size:>7} bytes  ({nome})")


if __name__ == "__main__":
    main()

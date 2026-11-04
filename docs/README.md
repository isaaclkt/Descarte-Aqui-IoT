# Diagramas e imagens do projeto

- `arquitetura.pdf` / `.svg`  - diagrama de blocos da solucao (pronto)
- `esquematico.pdf` / `.svg`  - ligacoes dos sensores e atuadores no ESP32 (pronto)
- `montagem.jpg`              - fotos do prototipo montado (a fazer)

Os dois PDFs sao de uma pagina so, A4 paisagem, com o texto vetorial (da para
selecionar e pesquisar). O SVG e a fonte; o PDF sai dele com:

    chrome --headless=new --no-pdf-header-footer --print-to-pdf=saida.pdf pagina.html

onde `pagina.html` so embute o SVG com `@page { size: A4 landscape }`.
O Word tambem aceita o SVG direto por Inserir > Imagens.

## Simulacao no Wokwi

O `diagram.json` e o `wokwi.toml` ficam na raiz do projeto. Com a extensao
Wokwi do VS Code, compile pelo PlatformIO e rode a simulacao.

Duas trocas em relacao a bancada, porque o Wokwi nao tem as pecas:

- o sensor indutivo virou uma chave deslizante no GPIO 32 (esquerda = metal);
- o peso so muda por cenario de automacao, nao pela tela.

Cuidado: o GPIO 16 (DT do HX711) aparece como **RX2** na placa, nao como D16.

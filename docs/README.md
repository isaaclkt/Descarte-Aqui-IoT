# Diagramas e imagens do projeto

- `arquitetura.pdf` / `.svg`  - diagrama de blocos da solucao (pronto)
- `esquematico.pdf` / `.svg`  - ligacoes dos sensores e atuadores no ESP32
  (DESATUALIZADO: ainda mostra o HC-SR04 da entrada em TRIG 19 / ECHO 18, que
  foi substituido pelo infravermelho E18-D80NK no GPIO 19, e nao mostra o
  ultrassonico da portinha em TRIG 27 / ECHO 33. Precisa ser redesenhado.)
- `montagem.jpg`              - fotos do prototipo montado (a fazer)

Os dois PDFs sao de uma pagina so, A4 paisagem, com o texto vetorial (da para
selecionar e pesquisar). O SVG e a fonte; o PDF sai dele com:

    chrome --headless=new --no-pdf-header-footer --print-to-pdf=saida.pdf pagina.html

onde `pagina.html` so embute o SVG com `@page { size: A4 landscape }`.
O Word tambem aceita o SVG direto por Inserir > Imagens.

## Simulacao no Wokwi

O `diagram.json` e o `wokwi.toml` ficam na raiz do projeto. Com a extensao
Wokwi do VS Code, compile pelo PlatformIO e rode a simulacao.

Tres trocas em relacao a bancada, porque o Wokwi nao tem as pecas:

- o sensor indutivo virou uma chave deslizante no GPIO 32 (esquerda = metal);
- o infravermelho E18-D80NK virou outra chave deslizante, no GPIO 19: a saida
  dele e digital e ativa em nivel baixo, entao a chave faz exatamente o mesmo
  que o sensor real faria;
- o peso so muda por cenario de automacao, nao pela tela.

O ultrassonico da portinha (TRIG 27 / ECHO 33) ainda nao esta no diagrama,
entao o caminho do nao-metal nao da para simular ali.

Cuidado: o GPIO 16 (DT do HX711) aparece como **RX2** na placa, nao como D16.

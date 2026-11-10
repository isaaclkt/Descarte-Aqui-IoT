# Logs persistentes no ESP32

As APIs anteriores, o Serial e os 40 eventos em RAM de GET /logs foram preservados. Cada evento inclui ms (uptime de 64 bits), nivel, origem, texto e data_hora. Antes de um relogio valido (epoch >= 2024-01-01), data_hora e null: eventos antigos nao recebem datas retroativas. A task configura NTP quando o Wi-Fi conecta; o SNTP continua tentando sincronizar quando houver internet. America/Sao_Paulo usa a regra POSIX <-03>3 (UTC-3, sem horario de verao). Uma mudanca legal futura exige atualizar essa regra.

## Armazenamento e concorrencia

LittleFS.begin(false) nunca formata. Particao ausente, nao inicializada, incompativel ou corrompida resulta em diagnostico no Serial e em /logs, HTTP 503 no historico e funcionamento normal da lixeira. Nao existe recuperacao destrutiva automatica. Em uma placa nova, prepare uma imagem LittleFS e provisione a particao separadamente, somente depois de verificar/guardar seu conteudo; uploadfs substitui os dados e nao deve fazer parte de uploads rotineiros. board_build.filesystem = littlefs seleciona o formato da imagem, nao formata a placa nem muda a tabela de particoes.

Somente a task registroFS acessa o FS, sem compartilhar handles com HTTP ou nuvem. Produtores usam fila de 64 eventos com espera zero; fila cheia descarta o evento persistente, mantendo Serial e tentando o buffer RAM. A cada 500 ms, a task escreve ate 8 eventos em lote, fecha o arquivo e atende no maximo uma consulta. Nenhum mutex de RAM permanece tomado durante I/O; mensagens de falha sao geradas fora das travas. A gravacao e assincrona, mas o acesso fisico a flash ainda pode causar pequenas pausas de cache do ESP32: validar tempos em hardware.

Rotacao: /eventos0.jsonl (atual) ate /eventos3.jsonl (mais antigo), cada um com ate 16 KiB; teto de 64 KiB de conteudo, alem dos metadados LittleFS. Arquivos de outras funcionalidades nao sao apagados. Falha de escrita/rotacao suspende persistencia ate reiniciar. Queda durante rotacao pode perder arquivos envolvidos; nao ha garantia transacional entre renomeacoes. Filas em RAM e escritas em andamento podem ser perdidas ao cortar energia. Cauda sem newline e ignorada na leitura; cada lote inicia com newline para isolar uma cauda incompleta.

Polling HTTP e leituras periodicas ficam fora da flash. Espera repetitiva da portinha fica so no Serial; falhas de polling do backend sao limitadas a um aviso por minuto e recuperacao. Sensores sem diagnostico proprio (IR e indutivo) nao sao declarados defeituosos por inferencia. HC-SR04 registra transicoes de ausencia/retorno do eco, sem mudar a decisao de seguranca. Textos livres do backend e nome da autorizacao nao sao registrados. Nao passar credenciais, tokens ou dados pessoais para registrar().

## Consulta

GET /logs/historico?offset=0&limite=20 retorna JSON, do arquivo mais antigo ao atual. offset: 0..10000; limite: 1..20 (padrao 20). Parametros invalidos retornam 400. HTTP 202 indica processamento: repetir exatamente a URL apos 1 segundo; 200 contem linhas, offset, limite, retornados, proximo_offset (null no fim) e descartados_boot. 503 indica indisponibilidade/ocupacao. Existe um unico slot global de consulta: clientes concorrentes devem repetir em 202. Paginas prontas ficam em cache por 5 segundos; nao sao snapshots entre paginas, e uma rotacao durante navegacao pode deslocar offsets. A resposta nao inclui polling HTTP e nao expoe o FS inteiro. A rota usa o mesmo acesso local das rotas existentes, sem autenticacao adicional.

## Roteiro de validacao em hardware (pendente)

1. Provisione LittleFS preservando previamente dados existentes; suba somente firmware nas atualizacoes seguintes. Ligue com Serial a 115200 e confirme Persistencia ativa.
2. Gere eventos: inicializacao, abrir/fechar porta com /atuador, descarte metal/nao-metal, sensor sem eco e falha de HX711/OLED quando seguro. Confira /logs (no maximo 40) e /logs/historico, repetindo em 202 e percorrendo proximo_offset.
3. Aguarde a fila drenar (pelo menos 5 segundos para eventos normais), reinicie por EN e confira eventos anteriores junto aos novos eventos de setup. Repita desligando e religando a alimentacao. Compare mensagens e timestamps, nao apenas ms, que reinicia por boot.
4. Desligue o hotspot: confira desconexao/tentativas de reconexao; religue e confira reconexao. Inicialize sem internet/NTP e confirme data_hora:null; habilite internet e aguarde Relogio sincronizado, verificando UTC-3 nos novos eventos. Eventos anteriores devem continuar null. Apos sincronizar, perder Wi-Fi nao invalida imediatamente o relogio local.
5. Faca polling continuo de /sensor e /status sem gerar eventos fisicos: nao deve crescer o historico por requisicao/leitura. Durante ciclos, consulte historico e observe atendimento HTTP, sensores e nuvem.
6. Gere mais de 64 KiB de eventos controlados: confirme rotacao, preservacao dos mais recentes e limite dos arquivos. Em rajada maior que a fila, confira descartados_boot sem travamento. Uma consulta pode refletir o contador em cache por ate 5 segundos.
7. Teste montagem invalida em placa de ensaio com conteudo conhecido: /logs/historico deve retornar 503, sem formatacao e com sensores/atuadores funcionando. Teste corte durante escrita: registros ja fechados devem continuar legiveis; os ultimos podem ser perdidos. Nao realizar esses testes destrutivos com dados unicos.

## Validacao local

Compilacao PlatformIO executada para esp32doit-devkit-v1/Arduino. O checkout nao possui config_wifi.h; foi usada uma copia temporaria de config_wifi.exemplo.h, sem credenciais reais, removida ao terminar. Nao houve upload, teste de sensores ou confirmacao de persistencia fisica neste ambiente.

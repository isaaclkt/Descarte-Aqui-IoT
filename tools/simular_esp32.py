"""
Simulador do ESP32 - para desenvolver e demonstrar SEM a placa.

O que ele faz:

  1. sobe um servidor HTTP com as MESMAS rotas do firmware (/, /3d, /status,
     /sensor, /servo, /rede, /descarte, /logs, /atuador), servindo os arquivos
     de verdade de src/client/. A pagina de controle e o modelo 3D nao sabem
     que nao ha placa;

  2. roda o mesmo ciclo de src/main.cpp, com os mesmos estados e tempos, para
     o 3D animar de ponta a ponta;

  3. se src/config_backend.h estiver preenchido, conversa com o backend PHP
     igual a task de src/nuvem.cpp: consulta autorizacoes, reporta as etapas e
     envia a conclusao. Ou seja, da para ver o fluxo completo - QR Code no
     celular, descarte, pontos creditados - sem hardware nenhum.

ISTO NAO E A PLACA. Nenhum resultado obtido aqui vale como teste de hardware:
os sensores, os servos e a balanca sao numeros inventados por este arquivo.
/status devolve "simulado": true e a pagina de controle mostra um aviso em
vermelho enquanto estiver falando com o simulador - de proposito, para
ninguem confundir uma demonstracao com uma medicao real.

Uso:

    python tools/simular_esp32.py

    python tools/simular_esp32.py --porta 8100 --peso 35

Depois abra  http://127.0.0.1:8100/  (pagina de controle) ou
             http://127.0.0.1:8100/3d  (modelo 3D).

Dispara um ciclo:
  * sozinho, quando o backend entrega uma autorizacao (fluxo do QR Code);
  * a mao, abrindo  /simular?tipo=metal  ou  /simular?tipo=nao-metal .

Sem dependencias: so a biblioteca padrao do Python.
"""

import argparse
import json
import re
import threading
import time
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
CLIENTE = RAIZ / "src" / "client"
CONFIG_BACKEND = RAIZ / "src" / "config_backend.h"

# Os mesmos tempos de src/main.cpp.
TEMPO_ESPERA = 4.0
TEMPO_ESTABILIZACAO = 3.0
TEMPO_ATE_MAO = 4.0                # quanto o "usuario" demora a levar a mao
CONFIRMACAO_RETIRADA = 1.5         # CONFIRMACAO_RETIRADA_MS: abertura livre ate fechar

MAC_SIMULADO = "A0:B7:65:00:00:01"
FIRMWARE = "0.2.0-sim"

# Espelham os #define de src/main.cpp. Se mudarem la, mudam aqui: o simulador
# so serve enquanto responder o mesmo que a placa responderia.
GPIO_SENSOR_IR = 19                # E18-D80NK, confirma a chegada na bandeja
GPIO_SENSOR_METAL = 32             # indutivo, primeiro do caminho (na boca)
PORTA_FECHADA = 170
PORTA_ABERTA = 10


# -----------------------------------------------------------------------------
# Configuracao do backend, lida do mesmo arquivo que o firmware usa
# -----------------------------------------------------------------------------

def ler_config_backend() -> dict:
    vazio = {"url": "", "codigo": "", "chave": ""}

    if not CONFIG_BACKEND.is_file():
        return vazio

    texto = CONFIG_BACKEND.read_text(encoding="utf-8", errors="replace")

    def define(nome: str) -> str:
        achado = re.search(rf'#define\s+{nome}\s+"([^"]*)"', texto)
        return achado.group(1) if achado else ""

    return {
        "url": define("BACKEND_URL").rstrip("/"),
        "codigo": define("DISPOSITIVO_CODIGO"),
        "chave": define("DISPOSITIVO_CHAVE"),
    }


# -----------------------------------------------------------------------------
# Estado do "dispositivo"
# -----------------------------------------------------------------------------

class Dispositivo:
    def __init__(self, peso_padrao: float, porta: int):
        self.trava = threading.Lock()
        self.inicio = time.time()
        self.porta_http = porta
        self.peso_padrao = peso_padrao

        self.estado = "Iniciando"
        # Nenhum sensor do caminho mede distancia desde que o HC-SR04 saiu
        # da entrada. O firmware mantem distancia_cm em -1 por
        # compatibilidade e o simulador faz igual.
        self.distancia_cm = -1.0
        self.objeto_presente = False
        self.confirmacao_armada = True
        self.metal_recente = False
        self.metal_detectado = False
        self.peso_g = 0.0
        self.distancia_nao_metal_cm = 20.0
        self.objeto_nao_metal_presente = False
        self.porta_aberta = False
        self.controle_automatico_porta = False
        self.servo_separacao = 90

        # Espelha o que nuvem.cpp publica em /descarte.
        self.sessao_estado = "ocioso"
        self.sessao_codigo = ""
        self.chave_conclusao = ""
        self.pilha_nome = ""
        self.pontos_por_grama = 0.0
        self.segundos_restantes = 0
        self.ultimo_descarte = 0
        self.ultimos_pontos = 0
        self.ultimo_creditado = False
        self.ultimo_motivo = ""
        self.backend_estado = "nao_configurado"
        self.ultimo_contato = 0.0

        self.logs = []
        self.ciclo_em_andamento = False

    # -- log, no formato de src/registro.cpp ---------------------------------

    def registrar(self, nivel: str, origem: str, texto: str) -> None:
        linha = {
            "ms": int((time.time() - self.inicio) * 1000),
            "nivel": nivel,
            "origem": origem,
            "texto": texto,
        }
        with self.trava:
            self.logs.append(linha)
            del self.logs[:-40]
        print(f"[{nivel}][{origem}] {texto}", flush=True)

    # -- JSON das rotas ------------------------------------------------------

    def json_status(self) -> dict:
        with self.trava:
            return {
                "dispositivo": "Descarte Aqui - ESP32 (SIMULADO)",
                "simulado": True,
                "firmware": FIRMWARE,
                "estado": self.estado,
                "wifi_conectado": True,
                "modo_rede": "STA",
                "ssid": "simulador",
                "ip": f"127.0.0.1:{self.porta_http}",
                "mac": MAC_SIMULADO,
                "rssi": -55,
                "uptime_s": int(time.time() - self.inicio),
                "heap_livre": 180000,
                "oled_ok": True,
                "balanca_ok": True,
                "porta": "aberta" if self.porta_aberta else "fechada",
                "servo_separacao": self.servo_separacao,
                "backend_estado": self.backend_estado,
                "backend_configurado": bool(CONFIG["url"]),
                "backend_ultimo_contato_s": (
                    -1 if self.ultimo_contato == 0 else int(time.time() - self.ultimo_contato)
                ),
                "sessao_estado": self.sessao_estado,
            }

    def json_sensor(self) -> dict:
        with self.trava:
            return {
                "distancia_cm": round(self.distancia_cm, 1),
                "sensor_entrada_tipo": "indutivo",
                "sensor_entrada_gpio": GPIO_SENSOR_METAL,
                "sensor_confirmacao_tipo": "infravermelho",
                "sensor_confirmacao_gpio": GPIO_SENSOR_IR,
                "objeto_presente": self.objeto_presente,
                "confirmacao_armada": self.confirmacao_armada,
                "metal_recente": self.metal_recente,
                "distancia_nao_metal_cm": round(self.distancia_nao_metal_cm, 1),
                "objeto_nao_metal_presente": self.objeto_nao_metal_presente,
                "metal_detectado": self.metal_detectado,
                "peso_bruto": int(self.peso_g * 420),
                "peso_g": round(self.peso_g, 1),
                "balanca_ok": True,
                "atualizado_ms": int((time.time() - self.inicio) * 1000),
            }

    def json_servo(self) -> dict:
        with self.trava:
            return {"servos": [
                # Angulos iguais aos do firmware: a porta fecha em 170 e
                # abre em 10, entao minimo > maximo aqui tambem.
                {"nome": "porta", "pino": 26,
                 "posicao": PORTA_ABERTA if self.porta_aberta else PORTA_FECHADA,
                 "estado": "aberta" if self.porta_aberta else "fechada",
                 "minimo": PORTA_FECHADA, "maximo": PORTA_ABERTA},
                {"nome": "separacao", "pino": 25, "posicao": self.servo_separacao,
                 "estado": ("metal" if self.servo_separacao == 160
                            else "nao_metal" if self.servo_separacao == 20 else "centro"),
                 "minimo": 20, "maximo": 160},
            ]}

    def json_rede(self) -> dict:
        return {
            "conectado": True, "modo": "STA", "ssid": "simulador",
            "ip": f"127.0.0.1:{self.porta_http}", "mac": MAC_SIMULADO,
            "gateway": "127.0.0.1", "mascara": "255.255.255.0", "dns": "127.0.0.1",
            "rssi": -55, "canal": 6, "tentativas_reconexao": 0, "porta_servidor": self.porta_http,
        }

    def json_descarte(self) -> dict:
        with self.trava:
            return {
                "sessao_estado": self.sessao_estado,
                "sessao_parcial": self.sessao_codigo[:12],
                "pilha": self.pilha_nome,
                "pontos_por_grama": self.pontos_por_grama,
                "segundos_restantes": self.segundos_restantes,
                "conclusao_pendente": False,
                "ultimo_descarte": self.ultimo_descarte,
                "ultimos_pontos": self.ultimos_pontos,
                "ultimo_creditado": self.ultimo_creditado,
                "ultimo_motivo": self.ultimo_motivo,
                "backend_estado": self.backend_estado,
                "backend_configurado": bool(CONFIG["url"]),
                "backend_ultimo_contato_s": (
                    -1 if self.ultimo_contato == 0 else int(time.time() - self.ultimo_contato)
                ),
            }

    def json_logs(self) -> dict:
        with self.trava:
            return {"linhas": list(self.logs), "total": len(self.logs), "capacidade": 40}


# -----------------------------------------------------------------------------
# Conversa com o backend PHP - espelha src/nuvem.cpp
# -----------------------------------------------------------------------------

def chamar_backend(caminho: str, corpo: dict = None) -> tuple:
    url = CONFIG["url"] + caminho
    dados = json.dumps(corpo).encode() if corpo is not None else None

    requisicao = urllib.request.Request(url, data=dados, method="POST" if dados else "GET")
    requisicao.add_header("X-Dispositivo", CONFIG["codigo"])
    requisicao.add_header("X-Chave", CONFIG["chave"])
    if dados:
        requisicao.add_header("Content-Type", "application/json")

    try:
        with urllib.request.urlopen(requisicao, timeout=5) as resposta:
            return resposta.status, json.loads(resposta.read().decode())
    except urllib.error.HTTPError as erro:
        try:
            return erro.code, json.loads(erro.read().decode())
        except Exception:
            return erro.code, {}
    except Exception as erro:
        return -1, {"erro": str(erro)}


def tarefa_nuvem(disp: Dispositivo) -> None:
    """Equivalente da task do core 0: pergunta ao backend, nao recebe conexao."""
    if not CONFIG["url"]:
        disp.registrar("WARN", "NUVEM", "Integracao desligada: src/config_backend.h vazio.")
        return

    disp.registrar("INFO", "NUVEM", f"Consultando {CONFIG['url']} a cada 3 s.")

    while True:
        codigo, corpo = chamar_backend(
            f"/dispositivo/autorizacao.php?mac={MAC_SIMULADO}&firmware={FIRMWARE}"
            # A placa serve na 80; aqui nao da, porque nesta maquina a 80 e
            # do Apache. Informar a porta real e o que faz o link do
            # monitoramento apontar para o simulador em vez do XAMPP.
            f"&porta={disp.porta_http}")

        with disp.trava:
            if codigo == 200:
                disp.backend_estado = "ok"
                disp.ultimo_contato = time.time()
            else:
                disp.backend_estado = "erro_http"

        if codigo == 200:
            autorizacao = corpo.get("autorizacao")

            if autorizacao:
                nova = autorizacao["sessao"] != disp.sessao_codigo
                with disp.trava:
                    disp.sessao_codigo = autorizacao["sessao"]
                    disp.chave_conclusao = autorizacao["chave_conclusao"]
                    disp.pilha_nome = autorizacao.get("pilha_nome", "")
                    disp.pontos_por_grama = float(autorizacao.get("pontos_por_grama", 0))
                    disp.segundos_restantes = int(autorizacao.get("segundos_restantes", 0))
                    if disp.sessao_estado in ("ocioso", "concluida"):
                        disp.sessao_estado = "autorizada"

                if nova:
                    disp.registrar("INFO", "NUVEM",
                                   f"Autorizacao recebida: {disp.pilha_nome}, "
                                   f"{disp.segundos_restantes} s para descartar.")
                    # O "usuario" coloca a pilha alguns segundos depois.
                    threading.Thread(target=ciclo, args=(disp, True), daemon=True).start()

            elif disp.sessao_estado in ("autorizada", "concluida"):
                with disp.trava:
                    disp.sessao_estado = "ocioso"
                    disp.sessao_codigo = ""

        elif codigo == -1:
            disp.registrar("WARN", "NUVEM", f"Backend inalcancavel: {corpo.get('erro', '')[:60]}")

        time.sleep(3)


def avisar_etapa(disp: Dispositivo, etapa: str) -> None:
    if not CONFIG["url"] or not disp.sessao_codigo:
        return

    chamar_backend("/dispositivo/evento.php", {
        "sessao": disp.sessao_codigo,
        "etapa": etapa,
        "distancia_cm": disp.distancia_cm,
        "metal": disp.metal_detectado,
        "peso_g": disp.peso_g,
    })


def concluir(disp: Dispositivo, peso: float, metal: bool, resultado: str) -> None:
    if not CONFIG["url"] or not disp.sessao_codigo:
        disp.registrar("INFO", "NUVEM", "Ciclo sem sessao autorizada: nada a pontuar.")
        return

    with disp.trava:
        disp.sessao_estado = "enviando"

    codigo, corpo = chamar_backend("/dispositivo/concluir.php", {
        "sessao": disp.sessao_codigo,
        "chave_conclusao": disp.chave_conclusao,
        "peso_g": peso,
        "metal": metal,
        "resultado": resultado,
    })

    with disp.trava:
        disp.sessao_estado = "concluida"
        disp.ultimo_creditado = bool(corpo.get("creditado"))
        disp.ultimos_pontos = int(corpo.get("pontos") or 0)
        disp.ultimo_descarte = int(corpo.get("descarte") or 0)
        disp.ultimo_motivo = corpo.get("motivo") or corpo.get("erro") or ""

    if disp.ultimo_creditado:
        disp.registrar("INFO", "NUVEM",
                       f"Descarte {disp.ultimo_descarte} registrado: "
                       f"{disp.ultimos_pontos} pontos creditados.")
    else:
        disp.registrar("WARN", "NUVEM",
                       f"Descarte nao pontuou (HTTP {codigo}): {disp.ultimo_motivo}")


# -----------------------------------------------------------------------------
# Ciclo fisico - mesma sequencia de estados de src/main.cpp
# -----------------------------------------------------------------------------

def ciclo(disp: Dispositivo, metal: bool, atraso: float = 2.0) -> None:
    if disp.ciclo_em_andamento:
        return
    disp.ciclo_em_andamento = True

    try:
        time.sleep(atraso)

        # Mesma ordem do firmware: o indutivo fica na boca, ANTES do
        # infravermelho. Quem classifica e a memoria do indutivo; o IR so
        # confirma a chegada. Sem registro recente de metal, e NAO PILHA.
        if metal:
            with disp.trava:
                disp.metal_recente = True
            disp.registrar("INFO", "INDUTIVO",
                           "Metal passou na boca. Aguardando confirmacao do IR.")
            time.sleep(0.3)

        with disp.trava:
            disp.objeto_presente = True
            disp.confirmacao_armada = False
            disp.metal_detectado = disp.metal_recente
            disp.metal_recente = False

        disp.registrar("INFO", "CLASSIFICACAO",
                       f"IR confirmou objeto. Indutivo antes do IR: "
                       f"{'SIM (PILHA)' if metal else 'NAO (NAO PILHA)'}.")
        disp.registrar("INFO", "SENSOR",
                       f"Chegada confirmada pelo IR (GPIO {GPIO_SENSOR_IR}); "
                       f"classificacao {'metal' if metal else 'nao-metal'}.")
        avisar_etapa(disp, "objeto_detectado")

        if metal:
            _caminho_metal(disp)
        else:
            _caminho_nao_metal(disp)

        with disp.trava:
            disp.estado = "Aguardando objeto"
            disp.objeto_presente = False
            disp.confirmacao_armada = True
            disp.metal_detectado = False
            disp.servo_separacao = 90
    finally:
        disp.ciclo_em_andamento = False


def _caminho_metal(disp: Dispositivo) -> None:
    with disp.trava:
        disp.estado = "Separando metal"
        disp.servo_separacao = 160
    avisar_etapa(disp, "separando")
    time.sleep(TEMPO_ESPERA)

    with disp.trava:
        disp.estado = "Pesando"
    avisar_etapa(disp, "pesando")
    time.sleep(TEMPO_ESTABILIZACAO)

    with disp.trava:
        disp.peso_g = disp.peso_padrao
    disp.registrar("INFO", "BALANCA", f"Bruto {int(disp.peso_g * 420)}, peso {disp.peso_g:.1f} g.")

    concluir(disp, disp.peso_g, True, "pilha separada")
    time.sleep(2)


def _caminho_nao_metal(disp: Dispositivo) -> None:
    with disp.trava:
        disp.estado = "Separando nao-metal"
        disp.servo_separacao = 20
    avisar_etapa(disp, "separando")
    time.sleep(TEMPO_ESPERA)

    # controlarRetiradaNaoMetal(): a porta abre JA, sem esperar o objeto
    # chegar ao fundo, e so fecha depois que a mao e vista e se afasta.
    with disp.trava:
        disp.estado = "Aguardando mao"
        disp.controle_automatico_porta = True
        disp.porta_aberta = True
    disp.registrar("INFO", "NAO-METAL",
                   "Porta aberta; aguardando mao no ultrassonico 2 (GPIO 33).")
    avisar_etapa(disp, "porta_aberta")
    time.sleep(TEMPO_ATE_MAO)

    # Mao chega a menos de DISTANCIA_DETECCAO_2 (14 cm).
    with disp.trava:
        disp.distancia_nao_metal_cm = 3.0
        disp.objeto_nao_metal_presente = True
        disp.estado = "Mao detectada"
    disp.registrar("INFO", "NAO-METAL",
                   "Mao detectada a 3.0 cm. Aguardando afastamento.")
    time.sleep(CONFIRMACAO_RETIRADA)

    # Abertura livre pelo tempo de confirmacao: so entao a porta fecha.
    with disp.trava:
        disp.distancia_nao_metal_cm = 20.0
        disp.objeto_nao_metal_presente = False
        disp.porta_aberta = False
        disp.controle_automatico_porta = False
        disp.estado = "Mao retirada"
    disp.registrar("INFO", "NAO-METAL",
                   f"Abertura livre por {int(CONFIRMACAO_RETIRADA * 1000)} ms apos detectar mao; porta fechada.")

    concluir(disp, 0, False, "nao e pilha")
    time.sleep(1)


# -----------------------------------------------------------------------------
# Servidor HTTP com as rotas do firmware
# -----------------------------------------------------------------------------

TIPOS = {".html": "text/html", ".css": "text/css", ".js": "application/javascript"}


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, formato, *args):
        pass  # o ruido de HTTP fica fora; o log do ciclo ja aparece

    def _enviar(self, codigo: int, corpo: bytes, tipo: str) -> None:
        self.send_response(codigo)
        self.send_header("Content-Type", tipo)
        self.send_header("Content-Length", str(len(corpo)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(corpo)

    def _json(self, dados: dict, codigo: int = 200) -> None:
        self._enviar(codigo, json.dumps(dados).encode(), "application/json")

    def _arquivo(self, nome: str) -> None:
        caminho = CLIENTE / nome
        if not caminho.is_file():
            self._json({"ok": False, "erro": "arquivo nao encontrado"}, 404)
            return
        self._enviar(200, caminho.read_bytes(), TIPOS.get(caminho.suffix, "text/plain"))

    def do_GET(self):
        rota = self.path.split("?")[0]
        parametros = dict(
            p.split("=", 1) for p in self.path.split("?")[1].split("&")
            if "=" in p
        ) if "?" in self.path else {}

        if rota == "/":
            self._arquivo("index.html")
        elif rota == "/3d":
            self._arquivo("ponto_coleta3d.html")
        elif rota == "/style.css":
            self._arquivo("style.css")
        elif rota == "/app.js":
            self._arquivo("app.js")
        elif rota == "/status":
            self._json(DISPOSITIVO.json_status())
        elif rota == "/sensor":
            self._json(DISPOSITIVO.json_sensor())
        elif rota == "/servo":
            self._json(DISPOSITIVO.json_servo())
        elif rota == "/rede":
            self._json(DISPOSITIVO.json_rede())
        elif rota == "/descarte":
            self._json(DISPOSITIVO.json_descarte())
        elif rota == "/logs":
            self._json(DISPOSITIVO.json_logs())
        elif rota == "/atuador":
            self._atuador(parametros)
        elif rota == "/simular":
            self._simular(parametros)
        else:
            self._json({"ok": False, "erro": "rota nao encontrada"}, 404)

    def _atuador(self, parametros: dict) -> None:
        estado = parametros.get("estado")

        if estado not in ("on", "off"):
            self._json({"ok": False, "erro": "estado invalido, use on ou off"}, 400)
            return

        if DISPOSITIVO.controle_automatico_porta:
            self._json({"ok": False, "erro": "porta sob controle automatico da retirada"}, 409)
            return

        with DISPOSITIVO.trava:
            DISPOSITIVO.porta_aberta = (estado == "on")

        DISPOSITIVO.registrar("INFO", "ATUADOR",
                              f"Porta -> {'ABERTA' if estado == 'on' else 'FECHADA'} (GPIO 26).")
        self._json({"ok": True, "atuador": "porta",
                    "estado": "aberta" if estado == "on" else "fechada", "pino": 26})

    def _simular(self, parametros: dict) -> None:
        tipo = parametros.get("tipo", "metal")

        if DISPOSITIVO.ciclo_em_andamento:
            self._json({"ok": False, "erro": "ja ha um ciclo em andamento"}, 409)
            return

        threading.Thread(target=ciclo, args=(DISPOSITIVO, tipo == "metal", 0.2),
                         daemon=True).start()
        self._json({"ok": True, "ciclo": tipo})


# -----------------------------------------------------------------------------

CONFIG = {}
DISPOSITIVO = None


def main() -> None:
    global CONFIG, DISPOSITIVO

    analisador = argparse.ArgumentParser(description="Simulador do ESP32 do Descarte Aqui.")
    analisador.add_argument("--porta", type=int, default=8100)
    analisador.add_argument("--peso", type=float, default=35.0,
                            help="peso em gramas que a balanca simulada reporta")
    args = analisador.parse_args()

    CONFIG = ler_config_backend()
    DISPOSITIVO = Dispositivo(args.peso, args.porta)
    DISPOSITIVO.estado = "Aguardando objeto"

    print()
    print("=" * 70)
    print(" SIMULADOR - isto NAO e o ESP32")
    print(" Sensores, servos e balanca sao numeros inventados por este script.")
    print(" Nada obtido aqui vale como teste de hardware.")
    print("=" * 70)
    print()
    print(f"  Pagina de controle : http://127.0.0.1:{args.porta}/")
    print(f"  Modelo 3D .........: http://127.0.0.1:{args.porta}/3d")
    print(f"  Disparar um ciclo .: http://127.0.0.1:{args.porta}/simular?tipo=metal")
    print(f"                       http://127.0.0.1:{args.porta}/simular?tipo=nao-metal")
    print()

    if CONFIG["url"]:
        print(f"  Backend ...........: {CONFIG['url']}")
        print(f"  Dispositivo .......: {CONFIG['codigo']}")
        print("  Abra o QR Code no celular: o ciclo dispara sozinho quando a")
        print("  autorizacao chegar.")
    else:
        print("  Backend ...........: desligado (src/config_backend.h vazio)")
        print("  Use /simular para disparar os ciclos a mao.")
    print()

    threading.Thread(target=tarefa_nuvem, args=(DISPOSITIVO,), daemon=True).start()

    servidor = ThreadingHTTPServer(("0.0.0.0", args.porta), Handler)
    DISPOSITIVO.registrar("INFO", "HTTP", f"Servidor simulado ativo na porta {args.porta}.")

    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        print("\n  encerrado")


if __name__ == "__main__":
    main()

-- =============================================================================
-- Descarte Aqui - IoT · migracao 02: em que porta o equipamento serve
--
-- Rode DEPOIS de 01_integracao_iot.sql, uma vez so:
--
--     mysql -u root descarteaqui < backend/sql/02_porta_dispositivo.sql
--
-- Por que existe
-- -------------
-- A pagina de monitoramento e servida pelo proprio equipamento. O site ja
-- aprendia o endereco dele (dispositivo.ultimo_ip, gravado a partir do IP de
-- origem das consultas que a placa faz), mas nao a porta - e porta nao da
-- para deduzir: a porta de ORIGEM de uma requisicao de saida e efemera e nao
-- diz nada sobre onde o equipamento escuta.
--
-- Sem isso, o link do monitoramento so funcionava para quem serve na 80. O
-- firmware serve, mas o simulador nao pode: nesta maquina a 80 e do Apache.
-- Resultado: o botao abria o painel do XAMPP em vez do monitoramento.
--
-- Agora o proprio equipamento informa a porta na consulta de autorizacao,
-- do mesmo jeito que ja informa o MAC e a versao do firmware. Nada e
-- deduzido nem chutado pelo site.
--
-- NULL significa "o equipamento ainda nao informou"; nesse caso o site
-- assume 80, que e onde o firmware serve.
--
-- Para desfazer:
--     ALTER TABLE dispositivo DROP COLUMN ultimo_porta;
-- =============================================================================

ALTER TABLE dispositivo
    ADD COLUMN ultimo_porta SMALLINT UNSIGNED NULL
        COMMENT 'Porta HTTP informada pelo equipamento; NULL = nao informada, assume 80'
        AFTER ultimo_ip;

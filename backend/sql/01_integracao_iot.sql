-- =============================================================================
-- Descarte Aqui - Integracao IoT
-- Migracao 01: dispositivo, sessao de descarte, log de eventos
--
-- SGBD: MySQL 8.0+ (usa coluna gerada STORED com UNIQUE)
-- Banco: descarteaqui (o mesmo que php/conn.php usa)
--
--     mysql -u root descarteaqui < 01_integracao_iot.sql
--
-- IMPORTANTE
--   * Esta migracao SO ADICIONA. Nenhuma tabela ou coluna existente e alterada
--     de forma destrutiva, nenhum dado e apagado.
--   * Ela assume o esquema que o PHP do site realmente usa:
--       usuario(id, pontos)                     -> php/minha-area.php
--       descarte(id, data, status, peso_total_gramas, pontos_gerados,
--                fk_usuario_id, fk_ponto_coleta_id)
--       descarte_pilha(fk_descarte_id, fk_pilhas_id, peso_gramas)
--       pilhas(id, nome, tipo, pontos_por_grama)
--       ponto_coleta(id, ...)
--     NAO e o esquema de docs/banco-de-dados/sql/ do repo Web, que descreve
--     outra versao do modelo e nao deve ser executado nesta integracao.
--   * Confira os nomes das colunas no seu banco antes de rodar:
--       SHOW COLUMNS FROM descarte;
--   * Rode uma unica vez. O MySQL 8 nao aceita ADD COLUMN IF NOT EXISTS.
--     Para desfazer, veja o bloco comentado no fim do arquivo.
--
--   * CUIDADO AO RECOMECAR DO ZERO. O script do PjBL 3 e reexecutavel, mas ele
--     faz SET FOREIGN_KEY_CHECKS = 0 e TRUNCATE em descarte. Rodado sozinho
--     sobre um banco que ja tem esta integracao, ele apaga os descartes e
--     DEIXA PARA TRAS as sessoes que apontavam para eles - sessao_descarte
--     fica com fk_descarte_id orfao, sem o banco reclamar.
--
--     Para recomecar limpo, apague o banco inteiro e refaca os dois passos:
--
--         mysql -u root -e "DROP DATABASE IF EXISTS descarteaqui;"
--         mysql -u root < ../../../Descarte Aqui - Web/banco/01_descarteaqui_entrega_final.sql
--         mysql -u root descarteaqui < 01_integracao_iot.sql
--   * Nenhuma chave, senha ou credencial aparece aqui. A chave do dispositivo
--     e gerada por  php ferramentas/gerar-chave.php  e gravada como hash.
-- =============================================================================

SET NAMES utf8mb4;

-- -----------------------------------------------------------------------------
-- dispositivo
-- Um registro por ponto de descarte fisico (ESP32).
--   codigo      identificador publico; e o unico dado que vai no QR Code.
--   chave_hash  password_hash() da chave que o firmware envia no header
--               X-Chave. A chave em texto puro NUNCA e gravada.
-- -----------------------------------------------------------------------------
CREATE TABLE dispositivo (
    id                  INT          NOT NULL AUTO_INCREMENT,
    codigo              VARCHAR(32)  NOT NULL,
    nome                VARCHAR(120) NOT NULL,
    chave_hash          VARCHAR(255) NOT NULL,
    fk_ponto_coleta_id  INT          NULL,
    ativo               TINYINT(1)   NOT NULL DEFAULT 1,
    firmware            VARCHAR(20)  NULL,
    mac                 VARCHAR(17)  NULL,
    ultimo_ip           VARCHAR(45)  NULL,
    ultimo_contato_em   DATETIME     NULL,
    created_at          DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at          DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
                                     ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT pk_dispositivo PRIMARY KEY (id),
    CONSTRAINT uq_dispositivo_codigo UNIQUE (codigo),
    CONSTRAINT fk_dispositivo_ponto_coleta FOREIGN KEY (fk_ponto_coleta_id)
        REFERENCES ponto_coleta (id),
    -- O codigo vai numa URL de QR Code: so caracteres seguros.
    CONSTRAINT ck_dispositivo_codigo CHECK (codigo REGEXP '^[A-Za-z0-9_-]{4,32}$'),
    CONSTRAINT ck_dispositivo_nome CHECK (CHAR_LENGTH(TRIM(nome)) >= 2),
    -- Formato PHC/crypt ($2y$..., $argon2id$...). Impede, por acidente,
    -- gravar a chave do dispositivo em texto puro.
    CONSTRAINT ck_dispositivo_chave_hash CHECK (
        chave_hash LIKE '$%' AND CHAR_LENGTH(chave_hash) >= 50)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- -----------------------------------------------------------------------------
-- sessao_descarte
-- Autorizacao temporaria que liga UM usuario a UM dispositivo por alguns
-- minutos. Nasce em 'aguardando'; o ESP32 a recebe e ela vira 'autorizada'.
--
--   codigo           identificador unico da sessao (vai para o navegador).
--   chave_conclusao  segredo que so o ESP32 recebe; exigido para concluir.
--                    Garante que a conclusao vem do dispositivo autorizado.
--                    NUNCA e enviada ao navegador.
--
-- Status possiveis:
--   aguardando    criada pelo usuario, o ESP32 ainda nao buscou
--   autorizada    entregue ao ESP32, esperando o descarte fisico
--   em_andamento  o ESP32 detectou objeto e esta processando
--   concluida     descarte registrado e pontos creditados
--   recusada      o dispositivo concluiu sem material valido (sem pontos)
--   cancelada     cancelada pelo usuario
--   expirada      passou de expira_em sem conclusao
-- -----------------------------------------------------------------------------
CREATE TABLE sessao_descarte (
    id                  INT           NOT NULL AUTO_INCREMENT,
    codigo              CHAR(32)      NOT NULL,
    chave_conclusao     CHAR(32)      NOT NULL,
    fk_usuario_id       INT           NOT NULL,
    fk_dispositivo_id   INT           NOT NULL,
    fk_pilhas_id        INT           NOT NULL,
    status              VARCHAR(20)   NOT NULL DEFAULT 'aguardando',
    peso_gramas         DECIMAL(10,3) NULL,
    pontos_creditados   DECIMAL(10,2) NULL,
    motivo              VARCHAR(255)  NULL,
    fk_descarte_id      INT           NULL,
    criada_em           DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP,
    expira_em           DATETIME      NOT NULL,
    autorizada_em       DATETIME      NULL,
    concluida_em        DATETIME      NULL,
    updated_at          DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP
                                      ON UPDATE CURRENT_TIMESTAMP,

    -- Uma sessao viva ocupa o dispositivo; sessoes encerradas valem NULL e o
    -- UNIQUE as ignora. E o banco, nao o PHP, que impede dois usuarios de
    -- usarem o mesmo dispositivo ao mesmo tempo.
    dispositivo_ocupado INT GENERATED ALWAYS AS (
        IF(status IN ('aguardando', 'autorizada', 'em_andamento'),
           fk_dispositivo_id, NULL)) STORED,

    CONSTRAINT pk_sessao_descarte PRIMARY KEY (id),
    CONSTRAINT uq_sessao_codigo UNIQUE (codigo),
    CONSTRAINT uq_sessao_dispositivo_ocupado UNIQUE (dispositivo_ocupado),
    -- Uma sessao gera no maximo um descarte.
    CONSTRAINT uq_sessao_descarte_gerado UNIQUE (fk_descarte_id),
    CONSTRAINT fk_sessao_usuario FOREIGN KEY (fk_usuario_id)
        REFERENCES usuario (id),
    CONSTRAINT fk_sessao_dispositivo FOREIGN KEY (fk_dispositivo_id)
        REFERENCES dispositivo (id),
    CONSTRAINT fk_sessao_pilhas FOREIGN KEY (fk_pilhas_id)
        REFERENCES pilhas (id),
    CONSTRAINT fk_sessao_descarte FOREIGN KEY (fk_descarte_id)
        REFERENCES descarte (id),
    CONSTRAINT ck_sessao_status CHECK (status IN (
        'aguardando', 'autorizada', 'em_andamento',
        'concluida', 'recusada', 'cancelada', 'expirada')),
    -- Nao existe CHECK (expira_em > criada_em) de proposito: ela engessaria a
    -- tabela sem proteger nada. O prazo e definido no INSERT com
    -- DATE_ADD(NOW(), INTERVAL n MINUTE), e uma CHECK dessas impediria
    -- encurtar o prazo de uma sessao (o que o teste automatizado faz para
    -- exercitar a expiracao) e tambem uma eventual tela de administracao.
    -- Quem decide se a sessao vale e a comparacao com NOW() nas rotas.
    CONSTRAINT ck_sessao_peso CHECK (peso_gramas IS NULL OR peso_gramas >= 0),
    CONSTRAINT ck_sessao_pontos CHECK (pontos_creditados IS NULL OR pontos_creditados >= 0),
    -- Coerencia: concluida exige descarte, peso e pontos.
    CONSTRAINT ck_sessao_concluida CHECK (
        status <> 'concluida' OR (
            fk_descarte_id IS NOT NULL AND concluida_em IS NOT NULL
            AND peso_gramas IS NOT NULL AND pontos_creditados IS NOT NULL))
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- Polling do ESP32: "ha sessao para este dispositivo?"
CREATE INDEX ix_sessao_dispositivo_status ON sessao_descarte (fk_dispositivo_id, status);
-- Varredura de expiracao.
CREATE INDEX ix_sessao_status_expira ON sessao_descarte (status, expira_em);
-- Historico do usuario.
CREATE INDEX ix_sessao_usuario_data ON sessao_descarte (fk_usuario_id, criada_em);

-- -----------------------------------------------------------------------------
-- descarte: colunas de rastreabilidade da origem IoT
--
-- uq_descarte_sessao e a garantia DURA de idempotencia: uma sessao nunca gera
-- dois descartes, portanto nunca gera pontos duas vezes, mesmo que o ESP32
-- reenvie a conclusao depois de um timeout de rede.
-- -----------------------------------------------------------------------------
ALTER TABLE descarte
    ADD COLUMN fk_dispositivo_id     INT NULL AFTER fk_ponto_coleta_id,
    ADD COLUMN fk_sessao_descarte_id INT NULL AFTER fk_dispositivo_id,
    ADD CONSTRAINT uq_descarte_sessao UNIQUE (fk_sessao_descarte_id),
    ADD CONSTRAINT fk_descarte_dispositivo FOREIGN KEY (fk_dispositivo_id)
        REFERENCES dispositivo (id),
    ADD CONSTRAINT fk_descarte_sessao FOREIGN KEY (fk_sessao_descarte_id)
        REFERENCES sessao_descarte (id);

-- -----------------------------------------------------------------------------
-- evento_iot
-- Registro de eventos e erros da comunicacao, exigido pelo TDE.
-- NUNCA grava senha, hash, chave do dispositivo ou chave_conclusao.
-- -----------------------------------------------------------------------------
CREATE TABLE evento_iot (
    id                  BIGINT       NOT NULL AUTO_INCREMENT,
    fk_dispositivo_id   INT          NULL,
    fk_sessao_id        INT          NULL,
    nivel               VARCHAR(5)   NOT NULL,
    tipo                VARCHAR(40)  NOT NULL,
    mensagem            VARCHAR(255) NOT NULL,
    ip                  VARCHAR(45)  NULL,
    criado_em           DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT pk_evento_iot PRIMARY KEY (id),
    CONSTRAINT fk_evento_dispositivo FOREIGN KEY (fk_dispositivo_id)
        REFERENCES dispositivo (id),
    CONSTRAINT fk_evento_sessao FOREIGN KEY (fk_sessao_id)
        REFERENCES sessao_descarte (id),
    CONSTRAINT ck_evento_nivel CHECK (nivel IN ('INFO', 'WARN', 'ERROR'))
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

CREATE INDEX ix_evento_data ON evento_iot (criado_em);
CREATE INDEX ix_evento_dispositivo_data ON evento_iot (fk_dispositivo_id, criado_em);

-- =============================================================================
-- Para desfazer (na ordem, por causa das chaves estrangeiras):
--
--   ALTER TABLE descarte
--       DROP FOREIGN KEY fk_descarte_sessao,
--       DROP FOREIGN KEY fk_descarte_dispositivo,
--       DROP INDEX uq_descarte_sessao,
--       DROP COLUMN fk_sessao_descarte_id,
--       DROP COLUMN fk_dispositivo_id;
--   DROP TABLE evento_iot;
--   DROP TABLE sessao_descarte;
--   DROP TABLE dispositivo;
-- =============================================================================

-- =============================================================================
-- Descarte Aqui - Integracao IoT
-- Estrutura MINIMA so para TESTE da integracao
--
-- ATENCAO: NAO rode isto no banco de producao da equipe.
--
-- Para que serve
--   A integracao depende de tabelas que ja existem no banco do site
--   (usuario, pilhas, ponto_coleta, descarte, descarte_pilha). Esse banco
--   vive na maquina de quem o criou, e o repositorio Web nao versiona o DDL
--   dele: as migracoes Laravel que o originaram foram removidas no commit
--   "Retira LARAVEL do projeto".
--
--   Este arquivo reconstroi apenas o minimo dessas tabelas, com os nomes de
--   coluna que o PHP do site realmente consulta, para que qualquer integrante
--   possa testar a integracao do zero, numa base descartavel, sem precisar da
--   maquina de outra pessoa.
--
-- Como usar
--   mysql -u root -e "CREATE DATABASE descarteaqui_teste_iot CHARACTER SET utf8mb4;"
--   mysql -u root descarteaqui_teste_iot < 00_estrutura_minima_teste.sql
--   mysql -u root descarteaqui_teste_iot < 01_integracao_iot.sql
--
--   Depois aponte backend/config.php para 'descarteaqui_teste_iot' e rode
--   php ferramentas/testar-fluxo.php
--
-- O que NAO esta aqui
--   Colunas e tabelas que a integracao nao usa (coletor, recompensa, troca,
--   administrador, enderecos detalhados do usuario...). Este arquivo nao e um
--   modelo de dados: e um andaime de teste.
-- =============================================================================

SET NAMES utf8mb4;

CREATE TABLE usuario (
    id              INT          NOT NULL AUTO_INCREMENT,
    nome            VARCHAR(255) NOT NULL,
    email           VARCHAR(255) NOT NULL,
    senha           VARCHAR(255) NOT NULL,
    cpf             VARCHAR(255) NULL,
    telefone        VARCHAR(255) NULL,
    data_nascimento DATE         NULL,
    -- endereco (legado) + campos detalhados: php/cidadao.php consulta os dois
    -- formatos. Sem eles, as proprias telas do site respondem 503 nesta base.
    endereco        VARCHAR(255) NULL,
    logradouro      VARCHAR(255) NULL,
    numero          VARCHAR(255) NULL,
    complemento     VARCHAR(255) NULL,
    bairro          VARCHAR(255) NULL,
    cep             VARCHAR(255) NULL,
    cidade          VARCHAR(255) NULL,
    uf              VARCHAR(255) NULL,
    pontos          INT          NOT NULL DEFAULT 0,
    created_at      DATETIME     NULL,
    updated_at      DATETIME     NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_usuario_email (email)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

CREATE TABLE ponto_coleta (
    id         INT          NOT NULL AUTO_INCREMENT,
    nome       VARCHAR(255) NULL,
    endereco   VARCHAR(255) NULL,
    latitude   DECIMAL(10,7) NULL,
    longitude  DECIMAL(10,7) NULL,
    created_at DATETIME     NULL,
    updated_at DATETIME     NULL,
    PRIMARY KEY (id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- pontos_por_grama: e daqui que sai a pontuacao. O dispositivo mede o peso,
-- o backend multiplica. Nem o usuario nem o ESP32 escolhem pontos.
CREATE TABLE pilhas (
    id               INT           NOT NULL AUTO_INCREMENT,
    nome             VARCHAR(255)  NULL,
    tipo             VARCHAR(255)  NULL,
    descricao        VARCHAR(255)  NULL,
    pontos_por_grama DECIMAL(10,3) NOT NULL,
    created_at       DATETIME      NULL,
    updated_at       DATETIME      NULL,
    PRIMARY KEY (id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

CREATE TABLE descarte (
    id                 INT           NOT NULL AUTO_INCREMENT,
    data               DATE          NULL,
    status             VARCHAR(255)  NOT NULL DEFAULT 'pendente',
    peso_total_gramas  DECIMAL(10,3) NULL,
    pontos_gerados     DECIMAL(10,2) NULL,
    fk_usuario_id      INT           NULL,
    fk_ponto_coleta_id INT           NULL,
    created_at         DATETIME      NULL,
    updated_at         DATETIME      NULL,
    PRIMARY KEY (id),
    CONSTRAINT fk_descarte_usuario FOREIGN KEY (fk_usuario_id) REFERENCES usuario (id),
    CONSTRAINT fk_descarte_ponto_coleta FOREIGN KEY (fk_ponto_coleta_id) REFERENCES ponto_coleta (id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

CREATE TABLE descarte_pilha (
    fk_descarte_id INT           NOT NULL,
    fk_pilhas_id   INT           NOT NULL,
    peso_gramas    DECIMAL(10,3) NOT NULL,
    created_at     DATETIME      NULL,
    updated_at     DATETIME      NULL,
    PRIMARY KEY (fk_descarte_id, fk_pilhas_id),
    CONSTRAINT fk_dp_descarte FOREIGN KEY (fk_descarte_id) REFERENCES descarte (id),
    CONSTRAINT fk_dp_pilhas FOREIGN KEY (fk_pilhas_id) REFERENCES pilhas (id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- -----------------------------------------------------------------------------
-- Dados de teste.
-- A senha e um hash de "senha-de-teste-12345" gerado com password_hash();
-- serve apenas para esta base descartavel.
-- -----------------------------------------------------------------------------

INSERT INTO usuario (nome, email, senha, pontos, created_at, updated_at)
VALUES ('Cidadao de Teste', 'teste@exemplo.local',
        '$2y$10$gXBaqhm7gi9E/iw/ra7/Zeb0X0XQPc.BSRv9jj0V2.BvBzWXGwK2G', 0, NOW(), NOW());

INSERT INTO ponto_coleta (nome, endereco, created_at, updated_at)
VALUES ('Ponto de Teste - Laboratorio', 'Rua Imaginaria, 100', NOW(), NOW());

INSERT INTO pilhas (nome, tipo, descricao, pontos_por_grama, created_at, updated_at)
VALUES ('Pilha comum AA', 'comuns', 'Alcalina de uso geral', 2.000, NOW(), NOW()),
       ('Pilha recarregavel', 'recarregaveis', 'NiMH', 3.000, NOW(), NOW()),
       ('Bateria de botao', 'botao', 'Litio tipo moeda', 5.000, NOW(), NOW());

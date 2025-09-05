-- Habilita o uso de nomes de tabelas com aspas duplas, mantendo a capitalização
SET standard_conforming_strings = on;

-- Tabela de Endereços
CREATE TABLE "Endereco" (
    id_endereco BIGSERIAL PRIMARY KEY,
    dns VARCHAR(255),
    ip_interno INET,
    ip_externo INET,
    apelido VARCHAR(255)
);

-- Tabela de Serviços
CREATE TABLE "Servico" (
    id_servico BIGSERIAL PRIMARY KEY,
    id_endereco BIGINT NOT NULL,
    porta INTEGER,
    nome_servico VARCHAR(255),
    FOREIGN KEY (id_endereco) REFERENCES "Endereco"(id_endereco)
);

-- Tabela de Histórico
CREATE TABLE "Historico" (
    id_historico BIGSERIAL PRIMARY KEY,
    id_endereco BIGINT NOT NULL,
    id_servico BIGINT,
    data_hora TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    estado_anterior VARCHAR(255),
    estado_atual VARCHAR(255),
    detalhes TEXT,
    FOREIGN KEY (id_endereco) REFERENCES "Endereco"(id_endereco),
    FOREIGN KEY (id_servico) REFERENCES "Servico"(id_servico)
);

-- Tabela de Alertas
CREATE TABLE "Alertas" (
    id_alerta BIGSERIAL PRIMARY KEY,
    id_servico BIGINT NOT NULL,
    tipo_alerta VARCHAR(255),
    data_hora TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    descricao TEXT,
    estado_servico VARCHAR(255),
    enviado BOOLEAN DEFAULT false,
    FOREIGN KEY (id_servico) REFERENCES "Servico"(id_servico)
);

-- Tabela de Contatos
CREATE TABLE "Contatos" (
    id_contato BIGSERIAL PRIMARY KEY,
    nome VARCHAR(255),
    email VARCHAR(255),
    trigger VARCHAR(255)
);

-- Tabela de junção para a relação "notifica" entre Alertas e Contatos
CREATE TABLE "Alertas_Contatos" (
    id_alerta BIGINT NOT NULL,
    id_contato BIGINT NOT NULL,
    PRIMARY KEY (id_alerta, id_contato),
    FOREIGN KEY (id_alerta) REFERENCES "Alertas"(id_alerta),
    FOREIGN KEY (id_contato) REFERENCES "Contatos"(id_contato)
);

-- Criação de Índices para Chaves Estrangeiras
CREATE INDEX idx_servico_endereco_id ON "Servico"(id_endereco);
CREATE INDEX idx_historico_endereco_id ON "Historico"(id_endereco);
CREATE INDEX idx_alertas_servico_id ON "Alertas"(id_servico);
CREATE INDEX idx_alertas_contatos_alerta_id ON "Alertas_Contatos"(id_alerta);
CREATE INDEX idx_alertas_contatos_contato_id ON "Alertas_Contatos"(id_contato);

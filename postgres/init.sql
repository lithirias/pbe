-- Habilita o uso de nomes de tabelas com aspas duplas, mantendo a capitalização
SET standard_conforming_strings = on;

-- Tabela de Endereços
CREATE TABLE "Endereco" (
    id_endereco BIGSERIAL PRIMARY KEY,
    dns VARCHAR(255),
    ip INET,
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

-- Criação do Usuário para realizar CRUD dentro do Node-RED
CREATE USER ze_colmeia WITH PASSWORD 'catatau';

-- Função para verificar contato
CREATE OR REPLACE FUNCTION inserir_contato_retorno(p_nome TEXT, p_email TEXT)
RETURNS TEXT AS $$
DECLARE
    valor_existente INTEGER;
BEGIN
    -- Verifica se o email já existe
    SELECT count(*) INTO valor_existente FROM "Contatos" WHERE email = p_email;

    -- Se o email já existir, retorna uma mensagem de erro
    IF valor_existente > 0 THEN
        RETURN 'ERRO: O email "' || p_email || '" já existe.';
    ELSE
        -- Se não existir, realiza o INSERT e retorna uma mensagem de sucesso
        INSERT INTO "Contatos" (nome, email) VALUES (p_nome, p_email);
        RETURN 'SUCESSO: Contato "' || p_nome || '" inserido.';
    END IF;
END;
$$ LANGUAGE plpgsql;


-- Função para verificar endereço

CREATE OR REPLACE FUNCTION public.inserir_endereco_retorno(p_nome text, p_ip inet, p_servico text, p_porta integer, p_dns text)
 RETURNS text
 LANGUAGE plpgsql
AS $function$
DECLARE
    valor_existente INTEGER;
    id_endereco_salvo INTEGER;
BEGIN
    -- Se a porta não foi preenchida (é NULL), verificamos a unicidade do IP
    IF p_porta IS NULL THEN
        -- Verifica se o IP já existe na tabela Endereco
        SELECT COUNT(*)
        INTO valor_existente
        FROM "Endereco"
        WHERE ip = p_ip;

        -- Se o IP já existe, retorna um erro.
        IF valor_existente > 0 THEN
            RETURN 'ERRO: O IP ' || p_ip || ' já está cadastrado.';
        ELSE
            -- Se não, insere um novo registro em Endereco
            INSERT INTO "Endereco" (apelido, ip, dns)
            VALUES (p_nome, p_ip, p_dns)
            RETURNING id_endereco INTO id_endereco_salvo; -- Captura o ID do novo registro

            -- E insere o novo serviço, com a porta NULL
            INSERT INTO "Servico" (nome_servico, porta, id_endereco)
            VALUES (p_servico, p_porta, id_endereco_salvo);

            RETURN 'SUCESSO: Recurso salvo com o IP ' || p_ip || '.';
        END IF;

    -- Se a porta foi preenchida, verificamos a unicidade da combinação IP/Porta
    ELSE
        -- Verifica se a combinação IP e Porta já existe.
        -- Para isso, fazemos um JOIN entre as tabelas Endereco e Servico.
        SELECT COUNT(*)
        INTO valor_existente
        FROM "Endereco" AS e
        JOIN "Servico" AS s ON e.id_endereco = s.id_endereco
        WHERE e.ip = p_ip AND s.porta = p_porta;

        -- Se a combinação IP/Porta já existe, retorna um erro.
        IF valor_existente > 0 THEN
            RETURN 'ERRO: A combinação IP ' || p_ip || ' e Porta ' || p_porta || ' já está em uso.';
        ELSE
            -- Se não, insere um novo registro em Endereco
            INSERT INTO "Endereco" (apelido, ip, dns)
            VALUES (p_nome, p_ip, p_dns)
            RETURNING id_endereco INTO id_endereco_salvo; -- Captura o ID do novo registro

            -- E insere o novo serviço, com a porta especificada
            INSERT INTO "Servico" (nome_servico, porta, id_endereco)
            VALUES (p_servico, p_porta, id_endereco_salvo);

            RETURN 'SUCESSO: Endereço salvo com o IP ' || p_ip || ' e Porta ' || p_porta || '.';
        END IF;
    END IF;
END;
$function$
;

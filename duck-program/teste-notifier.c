// Compile com:
// gcc notifier.c -I/usr/include/postgresql -lpq -lmosquitto -ljson-c -o notifier -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <mosquitto.h>
#include <json-c/json.h> // Biblioteca para manipular JSON

// Protótipo da função
void processar_notificacao(PGconn *conn, struct mosquitto *mosq, PGnotify *notify);

// Função de conexão com o banco de dados
PGconn *db_connect(const char *conninfo) {
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Erro ao conectar no BD: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    return conn;
}

int main() {
    struct mosquitto *mosq = NULL;
    int rc;

    // --- INICIALIZAÇÃO DO MQTT ---
    mosquitto_lib_init();
    mosq = mosquitto_new("pg_processor_bridge", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Erro ao criar o cliente Mosquitto!\n");
        return 1;
    }

    printf("Conectando ao broker MQTT...\n");
    rc = mosquitto_connect(mosq, "172.27.155.6", 1884, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Erro de conexao com o broker: %s\n", mosquitto_strerror(rc));
        return 1;
    }
    printf("Broker conectado.\n");

    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Erro ao iniciar o loop do Mosquitto: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    // --- INICIALIZAÇÃO DO POSTGRESQL ---
    const char *conninfo = "host=172.27.155.6 port=8081 user=postgres password=fuleco dbname=postgres";
    PGconn *conn = db_connect(conninfo);
    if (!conn) {
        return 1;
    }
    printf("Conectado ao PostgreSQL.\n");

    PGresult *res = PQexec(conn, "LISTEN historico_channel");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Comando LISTEN falhou: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return 1;
    }
    PQclear(res);
    printf("Iniciando o listener no canal 'historico_channel'...\n");

    // --- LOOP PRINCIPAL DE NOTIFICAÇÃO ---
    while (1) {
        PQconsumeInput(conn);
        PGnotify *notify;
        while ((notify = PQnotifies(conn)) != NULL) {
            // Chama a função que processa e envia
            processar_notificacao(conn, mosq, notify);
            PQfreemem(notify);
        }
        sleep(1);
    }

    // --- LIMPEZA ---
    PQfinish(conn);
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}

/**
 * @brief Processa uma notificação do PG: busca contatos, monta um novo JSON e publica no MQTT.
 *
 * @param conn Conexão ativa com o banco de dados.
 * @param mosq Cliente Mosquitto conectado.
 * @param notify O objeto de notificação recebido do PostgreSQL.
 */
void processar_notificacao(PGconn *conn, struct mosquitto *mosq, PGnotify *notify) {
    printf("\n--- Nova Notificacao Recebida! ---\n");
    printf("Payload Original do PG: %s\n", notify->extra);

    // ETAPA 1: Parsear o JSON do alerta e verificar se há erros.
    enum json_tokener_error jerr;
    json_object *json_alerta = json_tokener_parse_verbose(notify->extra, &jerr);

    if (jerr != json_tokener_success) {
        fprintf(stderr, "Erro ao parsear o JSON do alerta: %s\n", json_tokener_error_desc(jerr));
        // Se não conseguir parsear o alerta, não podemos continuar.
        return;
    }
    
    // ETAPA 2: Buscar os contatos no banco de dados
    PGresult *res = PQexec(conn, "SELECT email FROM \"Contatos\"");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Falha ao buscar contatos: %s\n", PQerrorMessage(conn));
        PQclear(res);
        json_object_put(json_alerta); // Libera a memória do JSON do alerta
        return;
    }

    // ETAPA 3: Montar o novo payload JSON enriquecido
    // Cria o objeto JSON principal: { "alerta": ..., "contatos": [...] }
    json_object *json_root = json_object_new_object();

    // Adiciona o objeto de alerta (já parseado e validado)
    json_object_object_add(json_root, "alerta", json_alerta);

    // Cria um array JSON para os e-mails
    json_object *json_contatos_array = json_object_new_array();
    int num_contatos = PQntuples(res);
    for (int i = 0; i < num_contatos; i++) {
        const char *email = PQgetvalue(res, i, 0);
        json_object_array_add(json_contatos_array, json_object_new_string(email));
    }
    json_object_object_add(json_root, "contatos", json_contatos_array);

    // Converte o objeto JSON completo para uma string
    const char *novo_payload = json_object_to_json_string_ext(json_root, JSON_C_TO_STRING_PLAIN);
    printf("Payload Enriquecido para MQTT: %s\n", novo_payload);

    // ETAPA 4: Publicar o novo payload no MQTT
    const char *topic = "monitoramento/historico/notificacoes";
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(novo_payload), novo_payload, 1, false);

    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Erro ao publicar no MQTT: %s\n", mosquitto_strerror(rc));
    } else {
        printf("Publicado com sucesso no tópico MQTT: '%s'\n", topic);
    }

    // ETAPA 5: Limpeza
    PQclear(res);               // Libera a memória do resultado da query
    json_object_put(json_root); // Libera a memória de todos os objetos JSON aninhados
}

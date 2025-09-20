#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

int main() {
    struct mosquitto *mosq = NULL;
    int rc;

    // 1. Inicializa a biblioteca Mosquitto
    mosquitto_lib_init();

    // 2. Cria o cliente Mosquitto
    mosq = mosquitto_new("ExemploPublisher", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Erro: Falha ao criar o cliente.\n");
        return 1;
    }

    // 3. Conecta ao broker
    // Use "localhost" se o broker estiver na mesma máquina
    // A porta 1883 é a padrão para MQTT
    rc = mosquitto_connect(mosq, "172.27.155.6", 1884, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Erro de conexão: %s\n", mosquitto_strerror(rc));
        return 1;
    }
    printf("Conectado com sucesso ao broker.\n");

    // 4. Publica a mensagem
    const char *topic = "teste";
    const char *message = "25.5";
    // O último parâmetro (false) indica que a mensagem não é retida
    rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 1, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Erro de publicação: %s\n", mosquitto_strerror(rc));
        return 1;
    }
    printf("Mensagem publicada no tópico '%s': %s\n", topic, message);

    // 5. Desconecta do broker e limpa a biblioteca
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}

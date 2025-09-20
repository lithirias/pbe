// gcc -I/usr/include/postgresql notifier.c -L/usr/lib/postgresql -lpq -lmosquitto -o notifier

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>
#include <mosquitto.h>

PGconn *db_connect(const char *conninfo){
	PGconn *conn = PQconnectdb(conninfo);
	if(PQstatus(conn) != CONNECTION_OK){
		fprintf(stderr, "Erro ao conectar no BD: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		return NULL;
	}
	return conn;
}

void sendTrigger(int *rc, struct mosquitto *mosq, const char *message){
	const char *topic = "trigger";

	rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 1, false);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Erro de publicacao %s\n", mosquitto_strerror(rc));
		return 1;
	}
	printf("Trigger enviado\n");
	return 0;
}

void readContacts(PGconn *conn, PGnotify *notify, int *rc, struct mosquitto *mosq){
	PGresult *response = PQexec(conn, "SELECT email FROM \"Contatos\"");
	int n_linhas = PQntuples(response);
	char emails[1024] = "";

	for(int i = 0; i < n_linhas; i++){
		if(i != 0)
			strcat(emails, ", ");
		strcat(emails, PQgetvalue(response, i, 0));
	}

	printf("Contatos: %s\n", emails);

	const char *to = emails;
	const char *subject = "Alteracao de estado";
	const char *body = notify->extra;

	sendTrigger(rc, mosq, to);

	return 0;
}

int main(){
	struct mosquitto *mosq = NULL;
	int rc;

	mosquitto_lib_init();
	mosq = mosquitto_new("trigger", true, NULL);
	if(!mosq){
		fprintf(stderr, "Erro ao criar o cliente!\n");
		return 1;
	}

	rc = mosquitto_connect(mosq, "172.27.155.6", 1884, 60);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Erro de conexao: %s\n", mosquitto_strerror(rc));
		return 1;
	}
	printf("Broker conectado.\n");

	// ___________________Declarações do Mosquitto acima______________

	const char *conninfo = "host=172.27.155.6 port=8081 user=postgres password=fuleco dbname=postgres";

	PGconn *conn = db_connect(conninfo);
	if(!conn){
		return 1;
	}

	printf("Iniciando o listener no canal...\n");
	PQexec(conn, "LISTEN trigger_novo_historico");

	while(1){
		// Manda requisição de notificação
		PQconsumeInput(conn);

		// Verifica notificações pendentes
		PGnotify *notify = PQnotifies(conn);
		if(notify){
			printf("Nova notificacao recebida!\n	ID: %s\n", notify->extra);
			// Chamar função de alerta aqui
			readContacts(conn, notify, rc, mosq);
			PQfreemem(notify);
		}

		sleep(10);
	}

	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	PQfinish(conn);
	return 0;
}

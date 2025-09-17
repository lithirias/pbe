#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>

PGconn *db_connect(const char *conninfo){
	PGconn *conn = PQconnectdb(conninfo);
	if(PQstatus(conn) != CONNECTION_OK){
		fprintf(stderr, "Erro ao conectar no BD: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		return NULL;
	}
	return conn;
}

void readContacts(PGconn *conn, PGnotify *notify){
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

	char command[1024];
	sprintf(command, "echo \"%s\" | mail -s \"%s\" \"%s\"", body, subject, to);
	printf("-----Email deve ser enviado-----\n");
	printf("echo \"%s\" | mail -s \"%s\" \"%s\"\n", body, subject, to);
	int result = system(command);

	if(result == -1){
		fprintf(stderr, "Erro ao executar comando.\n");
	}
	else{
		printf("Comando executado com sucesso. Resultado: %d\n", result);
	}

	return 0;
}

int main(){
	const char *conninfo = "host=172.27.155.6 port=8081 user=postgres password=fuleco dbname=postgres";

	PGconn *conn = db_connect(conninfo);
	if(!conn){
		return 1;
	}

	printf("Iniciando o listener no canal 'state_change'...\n");
	PQexec(conn, "LISTEN state_change");

	while(1){
		// Manda requisição de notificação
		PQconsumeInput(conn);

		// Verifica notificações pendentes
		PGnotify *notify = PQnotifies(conn);
		if(notify){
			printf("Nova notificacao recebida!\n	ID: %s\n", notify->extra);
			// Chamar função de alerta aqui
			readContacts(conn, notify);
			PQfreemem(notify);
		}

		sleep(60);
	}

	PQfinish(conn);
	return 0;
}

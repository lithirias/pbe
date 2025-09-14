// Para rodar com o libpq-fe.h utilize:
// gcc bd-listener.c -o bd-listener -I/usr/include/postgresql/ -lpq

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>	// Biblioteca padrão C para operar o PostgreSQL

#define TAM_MAX_LINE 256

void write_retorno_bd(PGresult *response){
	FILE *fp;
	char linha[TAM_MAX_LINE];
	fp = fopen("enderecos.txt", "w");


	if(PQresultStatus(response) != PGRES_TUPLES_OK){
		fprintf(stderr, "Select não retornou dados.\n");
		PQclear(response);
		return;
	}

	int n_linhas = PQntuples(response);
	int n_colunas = PQnfields(response);

	// Itera linhas
	for(int i = 0; i < n_linhas; i++){
		printf("	%s", PQgetvalue(response, i, 0));	// I é a linha e 0 é a coluna
		printf("\n");
		fprintf(fp, "%s\n", PQgetvalue(response, i, 0)); // 	Aponta para o doc e escreve nele
	}
	fclose(fp);
	PQclear(response);
}

// Fecha conexão com o BD
void close_connection(PGconn *connection, int code){
	PQfinish(connection);
	exit(code);
}

// Error Handler para problemas em comandos
void command_error_handler(PGconn *conn, PGresult *res){
	if(PQresultStatus(res) != PGRES_COMMAND_OK){
		fprintf(stderr, "%s\n", PQresultErrorMessage(res));
		PQclear(res);
		close_connection(conn, 1);
	};
}

// Se o dado returnado for um Tuple
void result_error_handler(PGconn *conn, PGresult *res){
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
		fprintf(stderr, "%s\n", PQresultErrorMessage(res));
		PQclear(res);
		close_connection(conn, 1);
	} else if(PQntuples(res) == 0){
		printf("There's no such a user\n");
		PQclear(res);
		close_connection(conn, 0);
	}
}

int main(){
	while(1){
		// Credenciais do BD
		PGconn *conn = PQconnectdb("host=172.27.155.6 port=8081 user=postgres password=fuleco dbname=postgres");

		// Testa status da conexão
		if(PQstatus(conn) == CONNECTION_BAD){
			fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
			close_connection(conn, 1);
		}

		// Inicia Transaction
		PGresult *response = PQexec(conn, "BEGIN");
		command_error_handler(conn, response);
		PQclear(response);

		// Lê lista de IPs
		char *stm = "SELECT ip FROM \"Endereco\"";
		response = PQexecParams(conn, stm, 0, NULL, NULL, NULL, NULL, 0);
		result_error_handler(conn, response);
		write_retorno_bd(response);
		//PQclear(response);

		// Commit da Transaction
		response = PQexec(conn, "COMMIT");
		command_error_handler(conn, response);
		//PQclear(response);
		PQfinish(conn);
		printf("Rodada de leitura/escrita concluida\n");
		sleep(60);
	}
}

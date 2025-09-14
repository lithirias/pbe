// Para rodar com o libpq-fe.h utilize:
// gcc bd-listener.c -o bd-listener -I/usr/include/postgresql/ -lpq

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>	// Biblioteca padrão C para operar o PostgreSQL

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

int main(int argc, char *argv[]){
	const int LEN = 10;
	const char *paramValues[2];

	if(argc != 3){
		fprintf(stderr, "Usage: need and Id and a withdraw\n");
		exit(1);
	}

	int rowId;
	int ret = sscanf(argv[1], "%d", &rowId);

	if(ret != 1){
		fprintf(stderr, "Id must be an interger\n'");
		exit(1);
	}

	if(rowId < 0){
		fprintf(stderr, "Error passing a negaative rowId\n");
		exit(1);
	}

	int price;
	int rep = sscanf(argv[2], "%d", &price);

	if(rep != 1){
		fprintf(stderr, "Withdraw must be an interger");
	}

	char str[LEN];
	snprintf(str, LEN, "%d", rowId);
	paramValues[0] = str;

	char pstr[LEN];
	snprintf(pstr, LEN, "%d", price);
	paramValues[1] = pstr;

	// Credenciais do BD
	PGconn *conn = PQconnectdb("host=172.27.155.6 port=8081 user=ze_colmeia password=catatau dbname=postgres");

	// Testa status da conexão
	if(PQstatus(conn) == CONNECTION_BAD){
		fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
		close_connection(conn, 1);
	}

	// Inicia Transaction
	PGresult *response = PQexec(conn, "BEGIN");
	command_error_handler(conn, response);
	PQclear(response);

	// Verifica se o usuário existe
	char *stm = "SELECT * FROM account WHERE user_id=$1";
	response = PQexecParams(conn, stm, 1, NULL, paramValues, NULL, NULL, 0);
	result_error_handler(conn, response);
	PQclear(response);

	// Update
	char *udt = "UPDATE account SET balance = balance - $2 WHERE user_id =$1";
	response = PQexecParams(conn, udt, 2, NULL, paramValues, NULL, NULL, 0);
	command_error_handler(conn, response);
	PQclear(response);

	// Commit da Transaction
	response = PQexec(conn, "COMMIT");
	command_error_handler(conn, response);
	PQclear(response);

	// Verifica se funcionou o update
	response = PQexecParams(conn, stm, 1, NULL, paramValues, NULL, NULL, 0);
	result_error_handler(conn, response);
	printf("Following user updated:\n %s %s %s\n", PQgetvalue(response, 0, 0), PQgetvalue(response, 0, 1), PQgetvalue(response, 0, 2));

	PQclear(response);
	PQfinish(conn);
	return 0;
}

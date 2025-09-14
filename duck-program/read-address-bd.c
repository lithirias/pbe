// Para rodar com o libpq-fe.h utilize:
// gcc bd-listener.c -o bd-listener -I/usr/include/postgresql/ -lpq

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>	// Biblioteca padrão C para operar o PostgreSQL

void print_retorno_bd(PGresult *response){
	if(PQresultStatus(response) != PGRES_TUPLES_OK){
		fprintf(stderr, "Select não retornou dados.\n");
		PQclear(response);
		return;
	}

	int n_linhas = PQntuples(response);
	int n_colunas = PQnfields(response);

	printf("---Resultado da Consulta---\n");

	for(int j = 0; j < n_colunas; j++){
		printf("%-20s", PQfname(response, j)); 	// PQfname retorna o nome da coluna
	}
	printf("\n");

	for(int j = 0; j < n_colunas; j++){
		printf("---------------------");
	}
	printf("\n");

	// Itera linhas
	for(int i = 0; i < n_linhas; i++){
		// Itera colunas
		for(int j = 0; j < n_colunas; j++){
			printf("%-20s", PQgetvalue(response, i, j));
		}
		printf("\n");
	}

	printf("-----------------------------------\n");
	printf("Total de linhas: %d\n", n_linhas);

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

	// Verifica se o usuário existe
	//char *stm = "SELECT * FROM account WHERE user_id=$1";
	//response = PQexecParams(conn, stm, 1, NULL, paramValues, NULL, NULL, 0);
	//result_error_handler(conn, response);
	//PQclear(response);

	// Lê lista de IPs
	char *stm = "SELECT * FROM \"Endereco\"";
	response = PQexecParams(conn, stm, 0, NULL, NULL, NULL, NULL, 0);
	result_error_handler(conn, response);
	print_retorno_bd(response);
	PQclear(response);

	// Update
	//char *udt = "UPDATE account SET balance = balance - $2 WHERE user_id =$1";
	//response = PQexecParams(conn, udt, 2, NULL, paramValues, NULL, NULL, 0);
	//command_error_handler(conn, response);
	//PQclear(response);

	// Commit da Transaction
	response = PQexec(conn, "COMMIT");
	command_error_handler(conn, response);
	PQclear(response);

	// Verifica se funcionou o update
	//response = PQexecParams(conn, stm, 1, NULL, paramValues, NULL, NULL, 0);
	//result_error_handler(conn, response);
	//printf("Following user updated:\n %s %s %s\n", PQgetvalue(response, 0, 0), PQgetvalue(response, 0, 1), PQgetvalue(response, 0, 2));

	PQclear(response);
	PQfinish(conn);
	return 0;
}

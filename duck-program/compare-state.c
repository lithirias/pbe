/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libpq-fe.h>



void attDatabase(PGconn *conn, char *ip, char *value, char *oldvalue){
	// Inicia transação
	PGresult *response;
	response = PQexec(conn, "BEGIN");
	PQclear(response);

	// Realiza o select do ID
	const char *paramValues[1];
	paramValues[0] = ip;
	char *query = "SELECT id_endereco FROM \"Endereco\" WHERE ip like '$1'";
	response = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);
	PQclear(response);

	// Realiza o inserto do estado
	const char *secParamValues[3];
	secParamValues[0] = PQgetvalue(response, 0, 0);
	secParamValues[1] = oldvalue;
	secParamValues[2] = value;
	*query = "INSERT INTO \"Histórico\"(id_historico, estado_anterior, estado_atual) VALUES ($1, $2, $3)";
	response = PQexecParams(conn, query, 3, NULL, paramValues, NULL, NULL, 0);
	PQclear(response);

	// Realiza o commit
	response = PQexec(conn, "COMMIT");
	PQclear(response);

	return 0;
}

void copyFile(){
	FILE *sc;
	sc = popen("cp status.csv status-compare.csv", "w");
	printf("I'm here!\n");
	pclose(sc);
	return 0;
}

#define MAX_LINHA 256
#define MAX_DADOS 100 // Tamanho máximo para o array de dados

// Estrutura para armazenar o endereço IP e o valor
typedef struct {
    char ip[20];
    int valor;
} DadosCSV;

// Protótipo da função para ler o arquivo
int ler_arquivo(const char *nome_arquivo, DadosCSV dados[]);

int main() {
	// Conecta com o DB
	PGconn *conn = PQconnectdb("host=172.27.155.6 port=8081 user=postgres password=fuleco");

    	DadosCSV dados1[MAX_DADOS];
    	DadosCSV dados2[MAX_DADOS];
    	int total_dados1, total_dados2;

    	// 1. Ler os dados do primeiro arquivo CSV
    	total_dados1 = ler_arquivo("status.csv", dados1);
    		if (total_dados1 == -1) {
        	return 1;
    	}

    	// 2. Ler os dados do segundo arquivo CSV
    	total_dados2 = ler_arquivo("status-compare.csv", dados2);
    	if (total_dados2 == -1) {
        	return 1;
    	}

    	// 3. Comparar os dados e imprimir as diferenças
    	printf("Comparando os arquivos...\n\n");
    	printf("Resultados que NÃO SÃO IGUAIS:\n");

    	// Itera sobre o primeiro array para comparar com o segundo
    	for (int i = 0; i < total_dados1; i++) {
        	// Encontrar o IP correspondente no segundo array
        	int j;
        	int encontrado = 0;
        	for (j = 0; j < total_dados2; j++) {
            		if (strcmp(dados1[i].ip, dados2[j].ip) == 0) {
                		encontrado = 1;
                		// Se os valores não forem iguais, imprime a diferença
                		if (dados1[i].valor != dados2[j].valor) {
                    			printf("IP: %s | Valor CSV1: %d | Valor CSV2: %d\n", dados1[i].ip, dados1[i].valor, dados2[j].valor);
					attDatabase(conn, dados1[i].ip, dados1[i].valor, dados2[j].valor);
                		}
                		break; // Sai do loop interno, pois o IP foi encontrado
            		}
        	}
        	// Caso o IP não exista no segundo arquivo
        	if (!encontrado) {
        		printf("IP: %s | Valor CSV1: %d | IP não encontrado no CSV2\n", dados1[i].ip, dados1[i].valor);
			attDatabase(conn, dados1[i].ip, dados1[i].valor, '0');
        	}
    	}

    	// Opcional: Verifica IPs que estão no CSV2 mas não no CSV1
    	printf("\nIPs que estão no CSV2 mas não no CSV1:\n");
    	for (int i = 0; i < total_dados2; i++) {
        	int encontrado = 0;
		for (int j = 0; j < total_dados1; j++) {
            		if (strcmp(dados2[i].ip, dados1[j].ip) == 0) {
                		encontrado = 1;
                		break;
            		}
        	}
        	if (!encontrado) {
            		printf("IP: %s | Valor CSV2: %d\n", dados2[i].ip, dados2[i].valor);
        	}
	}
	copyFile();
    	return 0;
}

// Implementação da função para ler o arquivo
int ler_arquivo(const char *nome_arquivo, DadosCSV dados[]) {
    FILE *arquivo;
    char linha[MAX_LINHA];
    int count = 0;

    arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo");
        return -1;
    }

    // Lê cada linha do arquivo
    while (fgets(linha, MAX_LINHA, arquivo) != NULL) {
        if (count >= MAX_DADOS) {
            fprintf(stderr, "Aviso: Limite de dados atingido. Apenas os primeiros %d itens foram lidos.\n", MAX_DADOS);
            break;
        }

        // Remove a quebra de linha
        linha[strcspn(linha, "\n")] = '\0';

        char *ip_str = strtok(linha, ",");
        char *valor_str = strtok(NULL, ",");

        if (ip_str != NULL && valor_str != NULL) {
            strcpy(dados[count].ip, ip_str);
            dados[count].valor = atoi(valor_str);
            count++;
        }
    }

    fclose(arquivo);
    return count;
}
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libpq-fe.h>

#define MAX_LINHA 256
#define MAX_DADOS 100 // Tamanho máximo para o array de dados

// Estrutura para armazenar o endereço IP e o valor
typedef struct {
    char ip[20];
    int valor;
} DadosCSV;

// Protótipo da função para ler o arquivo
int ler_arquivo(const char *nome_arquivo, DadosCSV dados[]);

// Função para atualizar o banco de dados (agora com tratamento de erros)
void attDatabase(PGconn *conn, const char *ip, int value, int oldvalue) {
    PGresult *response_select;
    PGresult *response_insert;
    const char *param_select[1];
    const char *param_insert[3];
    char valor_str[10], oldvalor_str[10];

    // Converte os valores inteiros para string para passar para PQexecParams
    snprintf(valor_str, sizeof(valor_str), "%d", value);
    snprintf(oldvalor_str, sizeof(oldvalor_str), "%d", oldvalue);

    // 1. Inicia a transação
    PQexec(conn, "BEGIN");

    // 2. Realiza o SELECT do ID do endereço
    param_select[0] = ip;
    response_select = PQexecParams(conn, "SELECT id_endereco FROM \"Endereco\" WHERE ip = $1", 1, NULL, param_select, NULL, NULL, 0);

    if (PQresultStatus(response_select) != PGRES_TUPLES_OK || PQntuples(response_select) == 0) {
        fprintf(stderr, "Erro ou IP nao encontrado na tabela Endereco: %s", PQerrorMessage(conn));
        PQclear(response_select);
        PQexec(conn, "ROLLBACK"); // Reverte a transação
        return;
    }

    // Armazena o ID do endereço
    const char *id_endereco = PQgetvalue(response_select, 0, 0);

    // 3. Realiza a inserção no histórico
    param_insert[0] = id_endereco;
    param_insert[1] = oldvalor_str;
    param_insert[2] = valor_str;

    response_insert = PQexecParams(conn, "INSERT INTO \"Historico\" (id_endereco, estado_anterior, estado_atual) VALUES ($1, $2, $3)", 3, NULL, param_insert, NULL, NULL, 0);

    if (PQresultStatus(response_insert) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Erro na insercao no Historico: %s", PQerrorMessage(conn));
        PQclear(response_select);
        PQclear(response_insert);
        PQexec(conn, "ROLLBACK"); // Reverte a transação
        return;
    }

    // Libera os resultados após o uso
    PQclear(response_select);
    PQclear(response_insert);

    // 4. Realiza o commit
    PQexec(conn, "COMMIT");
}

void copyFile() {
    FILE *sc;
    sc = popen("cp status.csv status-compare.csv", "w");
    pclose(sc);
}

// ... (A função ler_arquivo permanece a mesma)

// Implementação da função para ler o arquivo
int ler_arquivo(const char *nome_arquivo, DadosCSV dados[]) {
    FILE *arquivo;
    char linha[MAX_LINHA];
    int count = 0;

    arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo");
        return -1;
    }

    while (fgets(linha, MAX_LINHA, arquivo) != NULL) {
        if (count >= MAX_DADOS) {
            fprintf(stderr, "Aviso: Limite de dados atingido. Apenas os primeiros %d itens foram lidos.\n", MAX_DADOS);
            break;
        }

        linha[strcspn(linha, "\n")] = '\0';
        char *ip_str = strtok(linha, ",");
        char *valor_str = strtok(NULL, ",");

        if (ip_str != NULL && valor_str != NULL) {
            strcpy(dados[count].ip, ip_str);
            dados[count].valor = atoi(valor_str);
            count++;
        }
    }

    fclose(arquivo);
    return count;
}


int main() {
    // Conecta com o DB
    PGconn *conn = PQconnectdb("host=172.27.155.6 port=8081 user=postgres password=fuleco");
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Erro de conexao: %s", PQerrorMessage(conn));
        return 1;
    }

    DadosCSV dados1[MAX_DADOS];
    DadosCSV dados2[MAX_DADOS];
    int total_dados1, total_dados2;

    total_dados1 = ler_arquivo("status.csv", dados1);
    if (total_dados1 == -1) {
        PQfinish(conn);
        return 1;
    }

    total_dados2 = ler_arquivo("status-compare.csv", dados2);
    if (total_dados2 == -1) {
        PQfinish(conn);
        return 1;
    }

    printf("Comparando os arquivos...\n\n");
    printf("Resultados que NÃO SÃO IGUAIS:\n");

    for (int i = 0; i < total_dados1; i++) {
        int encontrado = 0;
        for (int j = 0; j < total_dados2; j++) {
            if (strcmp(dados1[i].ip, dados2[j].ip) == 0) {
                encontrado = 1;
                if (dados1[i].valor != dados2[j].valor) {
                    printf("IP: %s | Valor CSV1: %d | Valor CSV2: %d\n", dados1[i].ip, dados1[i].valor, dados2[j].valor);
                    attDatabase(conn, dados1[i].ip, dados1[i].valor, dados2[j].valor);
                }
                break;
            }
        }
        if (!encontrado) {
            printf("IP: %s | Valor CSV1: %d | IP nao encontrado no CSV2\n", dados1[i].ip, dados1[i].valor);
            attDatabase(conn, dados1[i].ip, dados1[i].valor, 0); // Passa o valor 0 (inteiro)
        }
    }

    printf("\nIPs que estao no CSV2 mas nao no CSV1:\n");
    for (int i = 0; i < total_dados2; i++) {
        int encontrado = 0;
        for (int j = 0; j < total_dados1; j++) {
            if (strcmp(dados2[i].ip, dados1[j].ip) == 0) {
                encontrado = 1;
                break;
            }
        }
        if (!encontrado) {
            printf("IP: %s | Valor CSV2: %d\n", dados2[i].ip, dados2[i].valor);
            attDatabase(conn, dados2[i].ip, dados2[i].valor, 0);
        }
    }
    
    copyFile();
    PQfinish(conn);
    return 0;
}

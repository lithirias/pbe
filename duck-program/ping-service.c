#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TAM_MAX_LINE 256

typedef struct{
	char **address;
	int count;
}objAddress;

void writeStatus(FILE *fp, char *address, bool status){
	fprintf(fp, "%s,%d\n", address, status);
}

bool checkPing(const char* target){
	// Cria o comando e salva num buffer
	char command[100];
	sprintf(command, "ping -c 1 %s", target);

	// A função popen(comando, permissão) é utilizada para executar funções no shell 
	FILE *fp = popen(command, "r");
	if(fp == NULL){
		return false;
	}

	char output[500];
	bool success = false;
	while(fgets(output, sizeof(output), fp) != NULL){
		if(strstr(output, "1 received") != NULL){
			success = true;
			break;
		}
	}

	pclose(fp);
	return success;
}

objAddress readAddressList(const char *arquivo){
	FILE *fa;
	char linha[TAM_MAX_LINE];
	int count = 0;
	objAddress info = {NULL, 0};

	fa = fopen(arquivo, "r");
	if(fa == NULL){
		perror("Erro ao abrir arquivo");
		return info;
	}

	while(fgets(linha, sizeof(linha), fa) != NULL){
		count++;
	}
	fclose(fa);

	if(count == 0){
		return info;
	}

	info.address = malloc(count * sizeof(char *));
	if(info.address == NULL){
		perror("Erro ao alocar memoria");
		return info;
	}
	info.count = count;

	fa = fopen(arquivo, "r");
	if(fa == NULL){
		perror("Erro ao reabrir o arquivo");
		free(info.address);
		info.address = NULL;
		return info;
	}

	int i = 0;
	while(fgets(linha, sizeof(linha), fa) != NULL){
		// Tira o \n
		linha[strcspn(linha, "\n")] = 0;

		// Aloca memória para a string
		info.address[i] = strdup(linha);
		if(info.address[i] == NULL){
			perror("Erro de alocacao de memoria para a linha");
			for(int j = 0; j < i; j++){
				free(info.address[j]);
			}
			free(info.address);
			info.address = NULL;
			info.count = 0;
			fclose(fa);
			return info;
		}
		i++;
	}

	fclose(fa);
	return info;
}


void freeAddress(objAddress *info){
	if(info->address != NULL){
		for(int i = 0; i < info->count; i++){
			free(info->address[i]);
		}
		free(info->address);
		info->address = NULL;
		info->count = 0;
	}
}

int main() {
	objAddress obj_address;

	while(1){
		FILE *fp;
		fp = fopen("status.csv", "w");

		obj_address = readAddressList("enderecos.txt");

		if(obj_address.address == NULL || obj_address.count == 0){
			printf("Nenhum endereco encontrado");
		}
		else{
			printf("Verificando %d enderecos...\n", obj_address.count);
			for(int i = 0; i < obj_address.count; i++){
				if(checkPing(obj_address.address[i])){
					printf("	%s esta online.\n", obj_address.address[i]);
					writeStatus(fp, obj_address.address[i], true);
				}
				else{
					fprintf(stderr, "	%s esta offline ou houve um erro.\n", obj_address.address[i]);
					writeStatus(fp, obj_address.address[i], false);
				}
			}
		}

		freeAddress(&obj_address);
		fclose(fp);
		printf("Rodada de pings concluida.\n");
		sleep(60);
	}

	return 0;
}

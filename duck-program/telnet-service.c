#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

bool checkTelnet(const char* target, int port){
	// Cria o socket pra abrir conexão
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		return false;
	}

	// Tenta resolver o nome de domínio para IP
	struct hostent *server = gethostbyname(target);
	if(server == NULL){
		close(sockfd);
		return false;
	}

	// Aqui começa a putaria pra configurar o server
	struct sockaddr_in serv_addr;
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	// Tenta abrir conexão
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		close(sockfd);
		return false; // Se falhar
	}

	close(sockfd);
	return true; // Conexão estabelecida
}

int main(){
	if(checkTelnet("google.com", 80)){
		printf("Telnet bem sucedido!\n");
	}
	else{
		printf("Telnet falhou\n");
	}
}

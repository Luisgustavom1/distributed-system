#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    struct sockaddr_in server;
    char message[1024] = "Hello from client", buffer[1024];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
      printf("not created\n");
      return 1;
    }

    printf("socket created with fd: %d\n", sock);

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
      perror("Erro ao conectar");
      return 1;
    }

    printf("Conectado\n");

    if (send(sock, message, strlen(message), 0) < 0) {
      perror("Erro ao enviar mensagem");
      return 1;
    }
    printf("Mensagem enviada\n");

    if (recv(sock, buffer, 1024, 0) < 0) {
      perror("Erro ao receber resposta");
      return 1;
    }

    printf("Resposta do servidor: %s\n", buffer);

    close(sock);

    return 0;
}

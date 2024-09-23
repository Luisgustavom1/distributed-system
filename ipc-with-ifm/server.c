#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef enum {
    WAITING,
    PROCESSING,
    SENDING,
} State;

typedef struct {
    State state;
    int sock;
    double operand;
    char operation;
    char buffer[BUFFER_SIZE];
} ServerContext;

double calculate(char operation, double operand);

void my_sleep(int seconds) {
    struct timespec req;
    req.tv_sec = seconds;          
    req.tv_nsec = 0;               

    nanosleep(&req, NULL);        
}

void handleClient(int client_socket) {
    ServerContext ctx;
    ctx.state = WAITING;
    ctx.sock = client_socket;
    memset(ctx.buffer, 0, BUFFER_SIZE);

    while (1) {
        switch (ctx.state) {
            case WAITING:
                printf("Waiting for client...\n");
                read(ctx.sock, ctx.buffer, BUFFER_SIZE);
                sscanf(ctx.buffer, "%c %lf", &ctx.operation, &ctx.operand);
                ctx.state = PROCESSING;
                break;
            case PROCESSING:
                printf("Processing...\n");
                double result = calculate(ctx.operation, ctx.operand);
                sprintf(ctx.buffer, "%lf", result);
                ctx.state = SENDING;
                break;
            case SENDING:   
                send(ctx.sock, ctx.buffer, strlen(ctx.buffer), 0);
                memset(ctx.buffer, 0, sizeof(ctx.buffer));
                ctx.state = WAITING;
                break;
        }
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
 
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {        
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
 
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
 
    while(1) {
        handleClient(client_socket);
    }

    close(server_fd);
 
    return 0;
}
 
double calculate(char operation, double operand) {
    my_sleep(5);
    double result = 0;
    switch(operation) {
        case '+':
            result = operand + operand;
            break;
        case '*':
            result = operand * operand;
            break;
        case 's':
            result = sqrt(operand);
            break;
        case '^':
            result = exp(operand);
            break;
        case '!':
            result = 1;
            for(int i = 1; i <= operand; i++) {
                result *= i;
            }
            break;
        default:
            printf("Invalid operation\n");
    }
    return result;
}
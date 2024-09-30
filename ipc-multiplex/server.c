#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30

const int OPT = 1;

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
    int buffer_len;
} ClientContext;

double calculate(char operation, double operand);

void my_sleep(int seconds) {
    struct timespec req;
    req.tv_sec = seconds;          
    req.tv_nsec = 0;               

    nanosleep(&req, NULL);        
}

void handle_client(ClientContext* ctx) {
    printf("Handling client %d in state %s\n", ctx->sock, ctx->state == WAITING ? "WAITING" : ctx->state == PROCESSING ? "PROCESSING" : "SENDING");

    switch (ctx->state) {
        case WAITING: {
            ssize_t read_size = read(ctx->sock, ctx->buffer, BUFFER_SIZE - 1);
            if (read_size > 0) {
                sscanf(ctx->buffer, "%c %lf", &ctx->operation, &ctx->operand);
                ctx->buffer[read_size] = '\0';
                ctx->state = PROCESSING;
                printf("operation: %c, operand: %lf\n", ctx->operation, ctx->operand);
            } else if (read_size == 0) {
                // Client disconnected
                close(ctx->sock);
                ctx->sock = -1;
            }
            break;
        }
        case PROCESSING: {
            double result = calculate(ctx->operation, ctx->operand);
            sprintf(ctx->buffer, "%lf", result);
            ctx->buffer_len = strlen(ctx->buffer);
            ctx->state = SENDING;
            break;
        }
        case SENDING: {
            ssize_t sent_size = send(ctx->sock, ctx->buffer, ctx->buffer_len, 0);
            if (sent_size > 0) {
                ctx->state = WAITING; // Go back to waiting for the next request
                memset(ctx->buffer, 0, BUFFER_SIZE); // Clear buffer
            } else if (sent_size < 0) {
                perror("send failed");
                close(ctx->sock);
                ctx->sock = -1;
            }
            break;
        }
    }
}

void set_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    ClientContext clients[MAX_CLIENTS];
    fd_set read_fds, write_fds;
    int max_sd;
    
    // Initialize all client sockets to -1 (not used)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = -1;
        clients[i].state = WAITING;
    }
 
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &OPT, sizeof(OPT))) {        
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
    
    set_non_blocking(server_fd);

    while(1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        // Add the server socket to the set
        FD_SET(server_fd, &read_fds);
        max_sd = server_fd;

        // Add client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            ClientContext c = clients[i];
            int sock = c.sock;

            if (sock > 0) {
                FD_SET(sock, &read_fds);
                if (c.state == SENDING) {
                    FD_SET(sock, &write_fds);
                }
                if (sock > max_sd) {
                    max_sd = sock;
                }
            }
        }

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        int newConnection = FD_ISSET(server_fd, &read_fds);
        if (newConnection) {
            int acceptClient = (client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen));
            if (acceptClient < 0) {
                perror("accept failed");
                continue;
            }

            printf("New connection, socket fd is %d\n", client_socket);

            // Add new client to the client array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == -1) {
                    clients[i].sock = client_socket;
                    set_non_blocking(client_socket);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = clients[i].sock;

            if (FD_ISSET(sock, &read_fds) || FD_ISSET(sock, &write_fds)) {
                handle_client(&clients[i]);
            }
        }
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
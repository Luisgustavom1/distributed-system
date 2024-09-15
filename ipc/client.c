#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
int THREADS_COUNT = 0;

typedef struct {
    char *buffer;
    char operation; 
    double operand; 
    int sock;
} threadArgs;

void* calculate(void* arg);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
 
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "192.168.1.3", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
 
    while(1){
        char buffer[1024] = {0};
        char operation;
        double operand;

        printf("\n\nEnter operation and operand: ");
        scanf("%c %lf", &operation, &operand);
        
	    pthread_t th;
        threadArgs *args = (threadArgs *)malloc(sizeof(threadArgs));
        args->buffer = buffer;
        args->operation = operation;
        args->operand = operand;
        args->sock = sock;

	    if (pthread_create(&th, NULL, calculate, args) != 0) {
        	perror("pthread_create failed");
        	exit(EXIT_FAILURE);
    	}

        operation = "";
        operand = 0;
     }

    return 0;
}

void* calculate(void* arg) {
    threadArgs* args = (threadArgs*) arg;
    char *buffer = args->buffer;
    char operation = args->operation;
    double operand = args->operand;
    int sock = args->sock;

	THREADS_COUNT++;
	
    sprintf(buffer, "%c %lf", operation, operand);
	send(sock, buffer, strlen(buffer), 0);
    printf("Request sent %d -> %c %f\n", THREADS_COUNT, operation, operand);
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, 1024);
    printf("Result: %s\n", buffer);
    memset(buffer, 0, sizeof(buffer));
	return NULL;
}

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

typedef struct {
    char operation;
    double operand; 
    int sock;
    int id;
} threadArgs;

void* calculate(void* arg);

int main() {
    int sock = 0;
    int THREADS_COUNT = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
 
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
 
    while(1){
        char operation;
        double operand;

        printf("\nEnter operation and operand: ");
        scanf(" %c %lf", &operation, &operand);
        
        THREADS_COUNT++;
        
	    pthread_t th;
        threadArgs *args = (threadArgs *)malloc(sizeof(threadArgs));
        args->operation = operation;
        args->operand = operand;
        args->sock = sock;
        args->id = THREADS_COUNT;

	    if (pthread_create(&th, NULL, calculate, args) != 0) {
        	perror("pthread_create failed");
        	exit(EXIT_FAILURE);
    	}

        pthread_detach(th);
     }

    return 0;
}

void* calculate(void* arg) {
    char buffer[1024] = {0};

    threadArgs* args = (threadArgs*) arg;
    char operation = args->operation;
    double operand = args->operand;
    int sock = args->sock;
    int thread_id = args->id;

    sprintf(buffer, "%c %lf", operation, operand);
	send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, 1024);
    printf("\n[%d] Result: %f %c %f = %s\n", thread_id, operand, operation, operand, buffer);
    memset(buffer, 0, sizeof(buffer));
	return NULL;
}

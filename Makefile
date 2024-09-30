CC = gcc

SERVER_BINARY = server
CLIENT_BINARY = client

SERVER_SRC = server.c
CLIENT_SRC = client.c

all: $(SERVER_BINARY) $(CLIENT_BINARY)

$(SERVER_BINARY): ${F}/$(SERVER_SRC)
	$(CC) -o ${F}/$(SERVER_BINARY) ${F}/$(SERVER_SRC)

$(CLIENT_BINARY): ${F}/$(CLIENT_SRC)
	$(CC) -o ${F}/$(CLIENT_BINARY) ${F}/$(CLIENT_SRC)

start_server: $(SERVER_BINARY)
	./${F}/$(SERVER_BINARY)

start_client: $(CLIENT_BINARY)
	./${F}/$(CLIENT_BINARY)

clean:
	rm -f ${F}/$(SERVER_BINARY) ${F}/$(CLIENT_BINARY)

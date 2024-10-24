#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "8080"
#define MAX_CLIENTS 10
#define MAX_EVENTS 10
#define MAX_MSG_SIZE 1024

struct client_data {
  int fd; 
} clients[MAX_CLIENTS];

int get_connection(int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].fd == fd) return i;
  }

  return -1;
}

int add_connection(int fd) {
  if (fd < 1) return -1;
  int i = get_connection(0);
  if (i == -1) return -1;
  clients[i].fd = fd;
  return 0;
}

int del_connection(int fd) {
  int i = get_connection(fd);
  if (i == -1) return -1;
  clients[i].fd = 0;
  return close(fd);
}

void receive_msg(int fd) {
  char buffer[MAX_MSG_SIZE];
  int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
  buffer[bytes_read] = 0;
  printf("[%d] Received: %s\n", get_connection(fd), buffer);
  fflush(stdout);
}

void send_message(int fd) {
  char msg[80];
  sprintf(msg, "Hello, client %d", get_connection(fd));
  send(fd, msg, strlen(msg), 0);
}

void broadcast_message() {
  char msg[MAX_MSG_SIZE];
  fgets(msg, sizeof(msg), stdin);
  
  for (int i = 0; i < MAX_CLIENTS; i++) {
    struct client_data c = clients[i];
    if (c.fd > 0) {
      send(c.fd, msg, strlen(msg), 0);
    }
  }

  fflush(stdout);
}

int create_socket_and_listen() {
  struct addrinfo *addr;
  struct addrinfo addr_hints;

  memset(&addr_hints, 0, sizeof(addr_hints));
  addr_hints.ai_flags = AI_PASSIVE;
  addr_hints.ai_family = PF_UNSPEC;
  addr_hints.ai_socktype = SOCK_STREAM;

  getaddrinfo("127.0.0.1", PORT, &addr_hints, &addr);
  int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  bind(sock, addr->ai_addr, addr->ai_addrlen);
  listen(sock, 3);

  printf("listening on fd %d\n", sock);
  return sock;
}

void run_event_loop(int kq, int sock) {
  struct kevent evSet;
  struct kevent evList[MAX_EVENTS];
  struct sockaddr_storage addr;
  socklen_t addr_size = sizeof(addr);

  while(1) {
    int num_events = kevent(kq, NULL, 0, evList, MAX_EVENTS, NULL);
    for (int i = 0; i < num_events; i++) {
      struct kevent ev = evList[i];

      // new connection
      printf("received event from fd %lu\n", ev.ident);
      if (ev.ident == sock) {
        int fd = accept(evList[i].ident, (struct sockaddr *) &addr, &addr_size);
        if (add_connection(fd) == 0) {
          EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
          kevent(kq, &evSet, 1, NULL, 0, NULL);
          send_message(fd);
        } else {
          printf("Connection refused\n");
          close(fd);
        }
      } else if (ev.flags & EV_EOF) {
        int fd = ev.ident;
        printf("Closing connection on descriptor %d\n", fd);
        EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(kq, &evSet, 1, NULL, 0, NULL);
        del_connection(fd);
      // client events
      } else if (ev.filter == EVFILT_READ & ev.ident != 0) {
        receive_msg(ev.ident);
      // stdin events
      } else if (ev.filter == EVFILT_READ & ev.ident == STDIN_FILENO) {
        printf("broadcasting message...\n");
        broadcast_message();
      }
    }
  }
}

int main(int argc, char *argv[]) {
  int sock = create_socket_and_listen();
  int kq = kqueue();
  struct kevent evSet[2];

  EV_SET(&evSet[0], sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
  EV_SET(&evSet[1], STDIN_FILENO, EVFILT_READ, EV_ADD, 0, 0, NULL);

  if (kevent(kq, evSet, 2, NULL, 0, NULL) == -1) {
    perror("kevent");
    exit(EXIT_FAILURE);
  };
  
  run_event_loop(kq, sock);
  close(sock);
  printf("Server shutting down...\n");

  return EXIT_SUCCESS;
}
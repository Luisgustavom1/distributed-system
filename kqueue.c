#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define READ_SIZE 10

int main() {
  int kq = kqueue();
  if (kq == -1) {
    perror("kqueue");
    exit(EXIT_FAILURE);
  }

  struct kevent change;
  EV_SET(&change, STDIN_FILENO, EVFILT_READ, EV_ADD, 0, 0, NULL);

  if (kevent(kq, &change, 1, NULL, 0, NULL) == -1) {
    perror("kevent");
    exit(EXIT_FAILURE);
  }

  struct kevent event;
  char buffer[READ_SIZE + 1];

  while (1) {
    printf("Waiting for input...\n");

    int receivedEvent = kevent(kq, NULL, 0, &event, 1, NULL);
    if (receivedEvent == -1) {
      perror("kevent");
      exit(EXIT_FAILURE);
    }

    if (receivedEvent > 0 && event.filter == EVFILT_READ) {
      int bytes_read = read(event.ident, buffer, READ_SIZE);
      if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Read from stdin: %s\n", buffer);
      }
    }
  }

  close(kq);
  return 0;
}

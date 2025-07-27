#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>

#define SB_SOCKET_PATH "/tmp/sb_socket"

int main(){
  int ret; // return value
  int down_flag = 0;
  int connection_socket;
  int data_socket;
  int result;

  connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (connection_socket == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /*
  * For portability clear the whole structure, since some
  * implementations have additional (nonstandard) fields in
  * the structure.
  */

  memset(&name, 0, sizeof(name));

  /* Bind socket to socket name. */

  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

  ret = bind(connection_socket, (const struct sockaddr *) &name,
                      sizeof(name));
  if (ret == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  /*
  * Prepare for accepting connections. The backlog size is set
  * to 20. So while one request is being processed other requests
  * can be waiting.
  */

  ret = listen(connection_socket, 20); // 연결 대기 함수..
  if (ret == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

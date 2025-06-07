#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#define LIS_ADDR = 0
#define LIS_PORT = 0

int main(int argc, char *argv[]){
  struct sockaddr_in sock;
  memset(&sock, 0, sizeof(sock));

  sock.sin_family = AF_INET;
  sock.sin_port = LIS_PORT;
  sock.sin_addr.s_addr = LIS_ADDR;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    printf("Socket creation error: %s\n", strerr(errno));
    return 1;
  }

  if(bind(sockfd, (struct sockaddr *)&sock, sizeof(sock)) == -1){
    printf("Bind error: %s\n", strerr(errno));
    close(sockfd);
    return 1;
  }

  if(listen(sockfd, SOMAXCONN) == -1){
    printf("Listen error: %s\n", strerror(errno));
    close(sockfd);
    return 1;
  }

  while(1 == 1){
    int sockfd_acc = accept(sockfd, NULL, NULL);
    if(sockfd_acc == -1){
        printf("Accept error: %s\n", strerr(errno));
        continue;
    }
    
  }

  return 0;
}

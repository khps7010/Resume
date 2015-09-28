#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFSIZE 4096

int status;

static void handle(int fd){
  char buf[BUFSIZE]; //讀寫用BUFFER
  char *ptr;	//暫存檔名
  int file_fd, len; //檔案指標、讀寫長度
  long file_offset; //檔案位子
  long file_size;	//檔案大小

  /*Set select*/
  int ret;  //select return
  struct timeval tv;  //waiting time
  fd_set wfd; //read set
  
  while(1){
    /*receive command*/
    memset(buf, '\0', BUFSIZE);
    if(read(fd, buf, BUFSIZE) <= 0)
      break;
    printf("%d: %s\n", fd, buf);//print receive command

    /*quit*/
    if(strncasecmp(buf, "quit", 4) == 0) //字串比較，不考慮大小寫
      break;

    /*get*/
    else if(strncasecmp(buf, "get ", 4) == 0){
      /*file name*/
      ptr = buf+4;
      strtok(ptr, "\n");

      /*open file*/
      file_fd = open(ptr, O_RDONLY, 0);
      //fail
      if(file_fd == -1){
        memset(buf, '\0', BUFSIZE);
        sprintf(buf, "File not exist");
        write(fd, buf, strlen(buf));
        continue;
      }
      //success
      else{
        /*send file size*/
        memset(buf, '\0', BUFSIZE);
        file_size = lseek(file_fd, 0, SEEK_END);  //取得檔案大小，從0到檔案結尾
        sprintf(buf, "%ld", file_size); 
        write(fd, buf, strlen(buf));
      }

      /*receive file offset*/
      memset(buf, '\0', BUFSIZE);
      read(fd, buf, BUFSIZE); //接收CLIENT端已存在的檔案大小
      printf("%d: Offset %s/%ld\n", fd, buf, file_size);
      file_offset = atol(buf);  //字串轉長整數
      lseek(file_fd, file_offset, SEEK_SET); //位移檔案指標到CLINET端要求的位子
      while((len = read(file_fd, buf, BUFSIZE)) > 0) {
        FD_ZERO(&wfd);  //清空
        FD_SET(fd,&wfd);  //將fd加入
        /* Set timeout */
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        
        ret = select(fd+1,NULL,&wfd,NULL,&tv);
        if(ret <= 0){
          perror("select");
          printf("transfer fail\n");
          close(fd);
          close(file_fd);
          exit(1);
        }
        else
          len = write(fd, buf, len);
      }
      close(file_fd);
      printf("File transfer is successful\n");
    }

    /*error command*/
    else
      continue;
  }
}

int main(int argc, char *argv[])
{
  int sd, client_fd; //serverfd, clientfd
  int one = 1; 
  int port; //port號碼
  struct sockaddr_in svr_addr, cli_addr; //sockaddr_in地址結構
  socklen_t sin_len = sizeof(cli_addr);

  if(argc > 1)
   port = atoi(argv[1]);
  else{
   fprintf (stderr, "usage: %s <port>\n", argv[0]);
   exit(0);
  }

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0){
    perror("open socket");
    exit(1);
  }
 
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
 
  svr_addr.sin_family = AF_INET; //IPV4 協定
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(port); //port號碼
  
  if (bind(sd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(sd);
    perror("bind");
    exit(1);
  }

  if (listen(sd, 20) == -1){
    close(sd);
    perror("listen");
    exit(1);
  }
    
  while (1) {
    client_fd = accept(sd, (struct sockaddr *) &cli_addr, &sin_len);

    if (client_fd == -1) {
      perror("Can't accept");
      continue;
    }

    printf("Client from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    switch (fork()){
      /*失敗*/
      case -1:
        perror("fork Error");
      /*成功*/
      case 0:
        close(sd);
        handle(client_fd);
        printf("Client %d close\n",client_fd);
        close(client_fd);
        exit(status);
      default:
        close(client_fd);
    }
  }

  close(sd);
  return 0;
}


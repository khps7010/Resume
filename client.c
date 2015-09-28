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

static void handle(int fd){
  char buf[BUFSIZE],file_name[256]; //接收buf
  int file_fd, len = 0; //檔案標籤, 接收到的訊息長度
  long re_size; //目前檔案大小
  long file_size; //檔案大小
  char *ptr; //暫存檔名

  /*Set select*/
  int ret;  //select return
  struct timeval tv;  //waiting time
  fd_set rfd; //read set

  while(1){
    printf("input command: get filename\n");
  
    fgets(buf, BUFSIZE, stdin); //取得輸入的指令

    /*quit*/
    if(strncasecmp(buf, "quit", 4) == 0){
      /*send command*/
      write(fd, buf, BUFSIZE);
      break;
    }

    /*get*/
    else if(strncasecmp(buf, "get ", 4) == 0){  //字串比較
      ptr = buf + 4;
      strtok(ptr, "\n");
      strcpy(file_name, ptr);

      /*send command*/
      write(fd, buf, strlen(buf));
      printf("Waitint sever\n");

      /*receive sever response*/
      memset(buf, '\0', BUFSIZE);
      read(fd, buf, BUFSIZE);

      /*server回傳檔案大小，表示檔案存在*/
      if(strstr(buf, "File not exist") == NULL){
        printf("Size: %s\n", buf);
        file_size = atol(buf);
        
        /*open file*/
        file_fd = open(file_name, O_WRONLY|O_CREAT, S_IRWXU);
        //fail
        if(file_fd == -1){
          perror("Open file");
          break;
        }
        //success
        else{
          re_size = lseek(file_fd, 0, SEEK_END);
          memset(buf, '\0', BUFSIZE);
          sprintf(buf, "%ld", re_size);
          write(fd, buf, strlen(buf));
        }
        memset(buf, '\0', BUFSIZE);
        while(1){
          if(re_size == file_size) break; //如果client端已存在檔案就離開
          
          memset(buf, '\0', BUFSIZE);
          FD_ZERO(&rfd);  //清空
          FD_SET(fd,&rfd);  //將fd加入
          /* Set timeout */
          tv.tv_sec = 30; //30秒
          tv.tv_usec = 0;
          ret = select(fd+1,&rfd,NULL,NULL,&tv);
          if(ret <= 0){
            perror("select");
            printf("transfer fail\n");
            close(fd);
            close(file_fd);
            exit(1);
          }
          else
            len = read(fd,buf,BUFSIZE);

          len = write(file_fd, buf, len);//寫入檔案
          printf("\e[?25l");  //隱藏指標
          /*顯示檔案傳輸進度*/
          printf("Progress %4.2f %%\r", (float)(re_size += len)/(float)file_size*100);   
        }

        close(file_fd);
        printf("\nFile transfer is successful\n");
      }
    
      /*server回傳檔案不存在*/
      else{
        printf("%s\n", buf);
        continue;
      }
    }

    /*輸入的指令錯誤*/
    else{
      printf("Error command\n");
      continue; 
    }
  }
}

int main(int argc, char *argv[])
{
  int port; //port號碼
  int sd; //serverfd
  int one = 1; 
  struct sockaddr_in svr_addr; //sockaddr_in地址結構
  socklen_t sin_len = sizeof(svr_addr);

  if(argc > 2)
   port = atoi(argv[2]);
  else{
   fprintf (stderr, "usage: %s <ip> <port>\n", argv[0]);
   exit(0);
  }
 
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0){
    perror("open socket");
    exit(1);
  }

  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
 
  svr_addr.sin_family = AF_INET; //IPV4 協定
  svr_addr.sin_addr.s_addr = inet_addr(argv[1]); //伺服器IP
  svr_addr.sin_port = htons(port); //port號碼

  if(connect(sd, (struct sockaddr *) &svr_addr, sin_len) != 0){
    perror("Can't accept");
    exit(0);
  }

  printf("connect server\n");

  handle(sd);
  close(sd);

  return 0;
}

//解析命令行参数中的主机名和端口号，并在调试模式下打印它们
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<math.h>
#include <netdb.h>
#include <sys/time.h> 
#include <cerrno>
#include <stdbool.h>
#include <regex.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<string>

/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG

//定义版本号
#define PROTOCOL_VERSION "TEXT TCP 1.0\n\n"


using namespace std;

int CAP=2000;
int MAX_CLIENTS =5;//设置最大客户端数量

//判断是否是域名
bool is_domain(const std::string&  input) {
  printf("Execute IPv6.\n");
  if(isdigit(input[0])){
    return false;
  }
  return  true;
}

//域名转ipv4
char* domain_to_ipv4(const char* domain) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\\n", gai_strerror(status));
        return NULL;
    }

    // 遍历结果，找到第一个IPv4地址
    for (p = res; p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        void *addr = &(ipv4->sin_addr);
        inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
        break; // 找到第一个地址就退出循环
    }

    freeaddrinfo(res); // 释放内存
    return strdup(ipstr); // 返回IPv4地址的副本
}


int main(int argc, char *argv[]){
  //检查命令行参数是否符合要求
  if (argc != 2){
    fprintf(stderr, "Incorrect format:%s\n",argv[0]);
    return 1;
  }
  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
 //解析主机名和端口号
  char* splits[CAP];
  char* p = strtok(argv[1],":");//根据：进行分割
  int delimCounter=0;//这个标志用来记录：的数量，来判断是ipv6还是ipv4
  char* Desthost=NULL;//目标服务器的ip
  char* Destport=NULL;//目标服务器的端口
  int ip_version=0;
  while(p!=NULL){
    splits[delimCounter++]=p;
    p=strtok(NULL,":");
  }


  
  Destport = splits[--delimCounter];//填充端口号
  Desthost = splits[0];//填充ip地址
  printf("deliCounter: %d\n",delimCounter);
  if(delimCounter==1){
    ip_version=4;
    if(is_domain(Desthost)){
      Desthost=domain_to_ipv4(Desthost);
      printf("Is Domain Name.\n");
    }

  }else if(delimCounter>1){
    ip_version=6;
  }
  printf("Receive IPv%d.\n",ip_version);
  for(int i=1;i<delimCounter;i++){
    // sprintf(Desthost,"%s:%s",Desthost,splits[i]);
    char  tempBuffer[100];//创建一个临时缓冲区
  	sprintf(tempBuffer, "%s:%s", Desthost, splits[i]);//将格式化后的字符串存储在临时缓冲区中
	  strcpy(Desthost,tempBuffer);//将临时缓冲区中的内容复制到目标缓冲区中
  }
  int port=atoi(Destport);//将字符串转化为int
  
  
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 
  //检查主机名和端口号
  if(Desthost == NULL || Destport == NULL){
    fprintf(stderr,"Deskhost or Deskport is empty\n");
    return 1;
  }


  /* Do magic */
  //int port=atoi(Destport);
#ifdef DEBUG  
  printf("Host %s, and port %d.\n",Desthost,port);
#endif
  //把端口号从string转换到int
  port = atoi(Destport);



int sockfd;
if(ip_version==4){
sockfd = socket(AF_INET, SOCK_STREAM, 0);// 创建TCP套接字
struct sockaddr_in server_addr; // 使用IPv4的地址结构体
memset(&server_addr, 0, sizeof(server_addr)); // 初始化为0
server_addr.sin_family = AF_INET; // 设置地址族为IPv4
server_addr.sin_port = htons(port); // 设置端口号
inet_pton(AF_INET, Desthost, &server_addr.sin_addr);// 将字符串形式的IPv4地址转换为网络字节序
bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));// 绑定套接字到服务器地址
//printf("Socket bound to %s:%d\n",Desthost,port);//打印绑定信息
listen(sockfd,5);
}else if(ip_version==6){
sockfd = socket(AF_INET6, SOCK_STREAM, 0);
struct sockaddr_in6 server_addr; // 使用IPv6的地址结构体
memset(&server_addr, 0, sizeof(server_addr)); // 初始化为0
server_addr.sin6_family = AF_INET6; // 设置地址族为IPv6
server_addr.sin6_port = htons(port); // 设置端口号
inet_pton(AF_INET6, Desthost, &server_addr.sin6_addr);
bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
//printf("Socket bound to %s:%d\n", Desthost, port);
listen(sockfd, 5);
}




  //记录已连接的客户端的数量
  int connectedClients =0;


  //循环接受客户端的连接
  while(1){


    struct sockaddr_in client_addr;//存储客户端地址信息的结构体
    socklen_t client_len = sizeof(client_addr);//获取客户端地址结构体的大小
    int client_sockfd = accept(sockfd,(struct sockaddr *)&client_addr,&client_len);
    if(client_sockfd == -1){
      perror("accept");
      continue;
    }
    //成功建立连接，与客户端进行通信
    //发送协议版本号给客户端
    send(client_sockfd, PROTOCOL_VERSION,strlen(PROTOCOL_VERSION),0);

    //检查客户端数量是否已经达到上限
    if(++connectedClients > MAX_CLIENTS){
      printf("Already reach the max number of clients.Rejecting new connections.\n");
      send(client_sockfd, "Connection rejected: Server is full\n", strlen("Connection rejected: Server is full\n"), 0);
      close(client_sockfd);
      connectedClients--;
      //continue;
    }else{
      //没有超过最大连接数
      printf("Accept. Now connection num: %d\n",connectedClients);
    }



    //接收客户端的响应
    char client_response_1[100];
    //超时检测
    fd_set rfds;
    struct timeval tv;
    int retval;

    //记录输入时间
    FD_ZERO(&rfds);
    FD_SET(client_sockfd,&rfds);

    //设置超时时间为5秒
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    //等待客户端响应
    retval = select(client_sockfd + 1,&rfds,NULL,NULL,&tv);
    printf("retval: %d\n",retval);
    if(retval == -1){
      perror("select()");
    }
    else if(retval){
      //客户端有数据可读
      recv(client_sockfd,client_response_1,sizeof(client_response_1),0);
      printf("ClientResponse1:%s\n",client_response_1);
    
    }else{
      //超时
      printf("Timeout\n");
      send(client_sockfd,"ERROR TO\n",strlen("ERROR TO\n"),0);
      close(client_sockfd);
      connectedClients--;
    }

    if(strcmp(client_response_1,"OK\n")==0){
      //收到客户端传来的OK\n
    
      //使用随机的运算类型和操作数生成随机分配
      char *operation = randomType();
      int value1 = randomInt();
      int value2 = randomInt();
      char assignment[100];
      sprintf(assignment,"%s %d %d\n",operation,value1,value2);
      printf("Assignment: %s\n",assignment);


      //发送随机分配给客户端
      send(client_sockfd,assignment,strlen(assignment),0);
      //接收客户端的答案
      char client_response[100];
    
      //超时检测
      fd_set rfds2;
      struct timeval tv2;
      int retval2;

      //记录输入时间
      FD_ZERO(&rfds2);
      FD_SET(client_sockfd,&rfds2);

      //设置超时时间为5秒
      tv2.tv_sec = 5;
      tv2.tv_usec = 0;

      //等待客户端响应
      retval2 = select(client_sockfd + 1,&rfds2,NULL,NULL,&tv2);
      printf("retval: %d\n",retval2);
      if(retval2 == -1){
        perror("select()");
      }
      else if(retval2){
        //客户端有数据可读
        recv(client_sockfd,client_response,sizeof(client_response),0);
  
    
      }else{
        //超时
        printf("Timeout\n");
        send(client_sockfd,"ERROR TO\n",strlen("ERROR TO\n"),0);
        close(client_sockfd);
        connectedClients--;
      }




    printf("client_respoonse2:%s \n",client_response);
    //检查客户端的答案是否正确
    int result;
    char *ptr;
    int server_result = 0;
    //strncmp：用于比较两个字符串的前n个字符是否相等
    if(strncmp(operation,"add",3)==0){
      server_result = value1 + value2;
    }else if(strncmp(operation,"sub",3)==0){
      server_result = value1-value2;
    }else if(strncmp(operation,"mul",3)==0){
      server_result = value1 * value2;
    }else if(strncmp(operation,"div",3)==0){
      server_result = value1 / value2;
    }else if(strncmp(operation,"fadd",4)==0){
      server_result = value1+value2;
    }else if(strncmp(operation,"fsub",4)==0){
      server_result = value1-value2;
    }else if(strncmp(operation,"fmul",4)==0){
      server_result = value1*value2;
    }else if(strncmp(operation,"fdiv",4)==0){
      server_result = value1/value2;
    }
    //将客户端答案转换成整数
    result=strtol(client_response,&ptr,10);
    //printf("Client result: %d \n",result);
    //printf("Server result: %d \n",server_result);

    //检查客户端答案是否和服务器端答案一致
    float difference=abs(server_result-result);
    if(difference<0.0001){
      printf("OK.\n");
    }else{
      printf("Wrong.\n");
    }
       //关闭客户端套接字
    close(client_sockfd);
    connectedClients--;
    

    }else{
             
    //关闭客户端套接字
    close(client_sockfd);
    connectedClients--;
    }
    
    
  }
  //freeaddrinfo(serverinfo); // 释放地址信息链表
  //关闭服务器套接字
  close(sockfd);
  return 0;


}

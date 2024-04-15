#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <iostream>

// Additional include for calcLib
#include "calcLib.h"

#define BACKLOG 5   // how many pending connections queue will hold
#define MAXCLIENTS 5
#define SECRETSTRING "konijiwa00"

#define WAIT_TIME_SEC 10005
#define WAIT_TIME_USEC 0
#define MAXRCVTIME 5
#define DEBUG


using namespace std;

void sigchld_handler(int s)
{
    (void)s;

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
	{
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool isIPv6(const char* str) 
{
    return (strchr(str, ':') != nullptr);
}

// transform domain into ipv4
char* domain_to_ipv4(const char* domain) 
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain, NULL, &hints, &res)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\\n", gai_strerror(status));
        return NULL;
    }

    for (p = res; p != NULL; p = p->ai_next) 
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        void *addr = &(ipv4->sin_addr);
        inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
        break; // find the first addr
    }

    freeaddrinfo(res); 

    char *result = (char*)malloc(INET_ADDRSTRLEN);
    if (result != NULL) 
    {
        strcpy(result, ipstr);
    }

    return result; // 返回IPv4地址的副本
}

char* generateRandom(char* ptr,double f1,double f2,int i1,int i2)
{
    double fresult;
    int iresult;

	// printf("  Int Values: %d %d \n",i1,i2);
	// printf("Float Values: %8.8g %8.8g \n",f1,f2);

	
	/* Act differently depending on what operator you got, judge type by first char in string. If 'f' then a float */
	
	if(ptr[0]=='f')
    {
		/* At this point, ptr holds operator, f1 and f2 the operands. Now we work to determine the reference result. */
	
		if(strcmp(ptr,"fadd")==0)
		{
			fresult=f1+f2;
		} 
		else if (strcmp(ptr, "fsub")==0)
		{
			fresult=f1-f2;
		} 
		else if (strcmp(ptr, "fmul")==0)
		{
			fresult=f1*f2;
		} 
		else if (strcmp(ptr, "fdiv")==0)
		{
			fresult=f1/f2;
		}

		char *operation = (char*)malloc(100); // Adjust buffer size accordingly
        snprintf(operation, 100, "%s %8.8g %8.8g = %8.8g\n", ptr, f1, f2, fresult);
        return operation;
	} 
	else
	{
		if(strcmp(ptr,"add")==0)
		{
			iresult=i1+i2;
		} 
		else if (strcmp(ptr, "sub")==0)
		{
			iresult=i1-i2;
		} else if (strcmp(ptr, "mul")==0)
		{
			iresult=i1*i2;
		} else if (strcmp(ptr, "div")==0)
		{
			iresult=i1/i2;
		}

		char *operation = (char*)malloc(100); // Adjust buffer size accordingly
        snprintf(operation, 100, "%s %d %d = %d \n", ptr, i1, i2, iresult);
		return operation;
	}
}

bool checkDoubleRandom(char* respondResultString,char* fresult_str)
{   
    double respondResult;
    sscanf(respondResultString, "%lg", &respondResult);
    double real_fresult;
    sscanf(fresult_str, " %lg", &real_fresult);
    
#ifdef DEBUG  
    // printf("DOUBLE re:%fend;\n",respondResult);
    // printf("correct fresult_str: %s\n",fresult_str);
    // printf("server: correct result: %8.8g\n",real_fresult);
#endif
    double difference = abs(respondResult-real_fresult);
    if(difference < 0.0001)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool checkIntRandom(char* respondResultString,char* iresult_str)
{
    int respondResult;
    sscanf(respondResultString, "%d", &respondResult);
    int real_iresult;
    sscanf(iresult_str, " %d", &real_iresult);

#ifdef DEBUG  
    // printf("INT re:%dend;\n",respondResult);
    // printf("correct iresult_str: %s\n",iresult_str);
    // printf("server: correct result: %d\n",real_iresult);
#endif
    int difference = abs(respondResult-real_iresult);
    if(difference < 0.0001)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main(int argc, char *argv[]) 
{
    // Check if command-line argument is provided
    if (argc != 2) 
	{
        fprintf(stderr, "Usage: %s <ip>:<port>\n", argv[0]);
        exit(1);
    }
   
    // silpt
    char *Desthost = NULL;
    char *Destport = NULL;
    char* last_colon = strrchr(argv[1], ':');

    if (last_colon != nullptr) 
    {
        *last_colon = '\0'; // 将冒号替换为字符串结束符，从而分割字符串.
        Desthost = new char[strlen(argv[1]) + 1];
        strcpy(Desthost, argv[1]);
        Destport = new char[strlen(last_colon + 1) + 1];
        strcpy(Destport, last_colon + 1);
        // Desthost = argv[1];
        // Destport = last_colon + 1;

    }

    // ipv4 or ipv6 or domain
    char* temp = new char[strlen(Desthost) + 1]; // 创建Host字符串的拷贝
    strcpy(temp, Desthost);
    bool isIPv6_flag = false;
    if (isIPv6(temp)) 
    {
        isIPv6_flag = true;
        // std::cout << "IPv6 address detected." << std::endl;
    }
    else if (isdigit(temp[0])) 
    {
        isIPv6_flag = false;
        // std::cout << "IPv4 address detected." << std::endl;
    } 
    else 
    {
        isIPv6_flag = false;
        // std::cout << "Domain" << std::endl;
    }
    

        
//     if(isdigit(Desthost[0]) == false)
//     {
//         Desthost=domain_to_ipv4(Desthost);
//     }


    if (Desthost == NULL || Destport == NULL) 
	{
        fprintf(stderr, "Invalid IP:Port format\n");
        exit(1);
    }


#ifdef DEBUG  
    // Convert port to integer
    // int port = atoi(Destport);
    // printf("Host %s, and port %d.\n",Desthost,port);
#endif
    printf("server on %s:%s \n",Desthost,Destport);
    int port = atoi(Destport);
    // Setup random seed
    // srand(time(NULL));

    // Setup signal handler
    // struct sigaction sa;
    // sa.sa_handler = sigchld_handler;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART;
    // if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	// {
    //     perror("sigaction");
    //     exit(1);
    // }
    
    int rv;
    int yes=1;
    int sockfd;
    
    // Set up hints for getaddrinfo
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if(isIPv6_flag)
    {
        sockfd = socket(AF_INET6, SOCK_STREAM, 0);
        struct sockaddr_in6 server_addr; // 使用IPv6的地址结构体
        memset(&server_addr, 0, sizeof(server_addr)); // 初始化为0
        server_addr.sin6_family = AF_INET6; // 设置地址族为IPv6
        server_addr.sin6_port = htons(port); // 设置端口号
        inet_pton(AF_INET6, Desthost, &server_addr.sin6_addr);
        bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }
    else
    {
        // Get address information
        if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) 
        {

            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // Loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) 
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
            {
                perror("server: socket");
                continue;
            }

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
            {
                perror("setsockopt");
                exit(1);
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
            {
                close(sockfd);
                perror("server: bind");
                continue;
            }

            break;
        }

        freeaddrinfo(servinfo);

        if (p == NULL) 
        {
            fprintf(stderr, "server: failed to bind\n");
            exit(1);
        }
    }


    if (listen(sockfd, BACKLOG) == -1) 
	{
        perror("listen");  
        exit(1);
    }

    printf("server: waiting for connections...\n");

//-----------------------------
    fd_set redset;
    FD_ZERO(&redset);
    FD_SET(sockfd, &redset);
    int maxfd = sockfd;

    int num = 0;


    while(1)
    {
        
        if(num<MAXCLIENTS)
        {
            // client connection
            // struct sockaddr_storage their_addr; // connector's address information
            // socklen_t sin_size;
            // int cfd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);
            int cfd = accept(sockfd,NULL,NULL);
            if(cfd==-1)
            {
                // printf("sockfd: %d\n",sockfd);
                perror("accept");
                continue;
            }
            
            num++;

            // 打印连接信息
            // char s[INET6_ADDRSTRLEN];
            // inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
            // printf("server: got connection from %s\n", s);
            printf("server: got connection %d\n", num);

            // FD_SET(cfd, &redset);
            // // 更新最大值
            // maxfd = cfd > maxfd ? cfd : maxfd;

            // printf("maxfd:%d \n",maxfd);
            // printf("sockfd: %d,cfd: %d \n",sockfd,cfd);

            // Send supported protocol to client
            std::string protocolMessage = "TEXT TCP 1.0\n";
            // if (write(i, protocolMessage.c_str(), protocolMessage.length()) == -1) 
            if (send(cfd, protocolMessage.c_str(), protocolMessage.length(), 0) == -1) 
            {
                // printf("protocol\n");
                perror("send");
                close(cfd);
                continue;
            }
        }
        else
        {
            // client connection
            struct sockaddr_storage their_addr; // connector's address information
            socklen_t sin_size;
            int cfd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);

            std::string protocolMessage = "Reject, out of queue\n";
            if (send(cfd, protocolMessage.c_str(), protocolMessage.length(), 0) == -1) 
            {
                perror("send");
                close(cfd);
                continue;
            }

            close(cfd);
        }
        // time out
        struct timeval timeout;
        timeout.tv_sec = WAIT_TIME_SEC;
        timeout.tv_usec = 0;
        
        //select
        fd_set tmp = redset;
        int ret = select(maxfd+1,&tmp,NULL,NULL,&timeout);
        
        //
        char buf[5]={0};
        if(ret == -1)
        {
            perror("select()");
        }
        else if(ret)
        {
            int len = recv(maxfd, buf, sizeof(buf), 0);
            if (len == -1) 
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN) 
                {
                    // Timeout occurred
                    printf("server: protocol reply timeout occurred, closing connection\n");
                } 
                else 
                {
                    perror("recv");
                }
                FD_CLR(maxfd, &redset);
                close(maxfd);
                num--;
                continue;
            }
            else if(len == 0)
            {
                printf("client closed connection...\n");
                FD_CLR(maxfd, &redset);
                close(maxfd);
                num--;
            }          
        }
        else
        {
            printf("server: protocol reply timeout occurred, closing connection\n");
            FD_CLR(maxfd, &redset);
            close(maxfd);
            num--;
            continue;
        }

        char buf_pre3[4];
        for (int i = 0; i < 3; ++i) 
        {
            buf_pre3[i] = buf[i];
        }
        buf_pre3[3] = '\0'; // end
        
        // Check if client accepted the protocol
        if (strcmp(buf_pre3, "OK\n") != 0) 
        {
            printf("Client can not accept this protocol.\nDisconnecting.\n");
            FD_CLR(maxfd, &redset);
            close(maxfd);
            num--;
            continue;
        }
        
        // 清空缓存区,理论上没有用
        memset(buf, 0, sizeof(buf));


        fd_set redset_calculator;
        FD_ZERO(&redset_calculator);
        FD_SET(maxfd,&redset_calculator);

        struct timeval timeout_calculator;
        timeout_calculator.tv_sec = WAIT_TIME_SEC;
        timeout_calculator.tv_usec = 0;
            
        // Initialize the library, this is needed for this library. 
        char *ptr;
        ptr=randomType(); // Get a random arithemtic operator. 
        double f1,f2;
        int i1,i2;
        i1=randomInt();
        i2=randomInt();
        f1=randomFloat();
        f2=randomFloat();

        // Generate random number
        char* operation = generateRandom(ptr,f1,f2,i1,i2);
        if (operation == NULL) 
        {
            return -1;
        }
        char *formula = strtok(operation, "=");
        char *result = strtok(NULL, "=");
        char *result_copy = strdup(result);
        if (result_copy == NULL) 
        {
            return -1;
        }

        // Send formula to client
        int tmpLength = strlen(formula);
        formula[tmpLength] = '\n';
        formula[tmpLength + 1] = '\0'; // 添加 null 终止符
        if (send(maxfd, formula, strlen(formula), 0) == -1) 
        {
            perror("send");
            close(maxfd);
            continue;
        }
        printf("server: send %s",formula);

        char respondResultString[1024];
        memset(respondResultString, 0, sizeof(respondResultString));

        int checkReceived = select(maxfd + 1,&redset_calculator,NULL,NULL,&timeout_calculator);
        if (checkReceived == -1)
        {
            perror("select()");
        }
        else if(checkReceived)
        {
            int bytesReceived = recv(maxfd, respondResultString, sizeof(respondResultString), 0);
            if (bytesReceived == -1) 
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN) 
                {
                    // Timeout occurred
                    printf("server: result reply timeout occurred, closing connection\n");
                    // close(i);
                } 
                else 
                {
                    perror("recv");
                }
                FD_CLR(maxfd, &redset_calculator);
                num--;
                close(maxfd);
                continue;;
            } 
            else if (bytesReceived == 0) 
            {
                // Connection closed by the client
                FD_CLR(maxfd, &redset_calculator);
                close(maxfd);
                continue;
            }

            printf("server: receive respond: %s",respondResultString);
            printf("server: correct result:%s",result_copy);

            bool checkflag = NULL;     
            if(ptr[0]=='f')
            {
                checkflag = checkDoubleRandom(respondResultString,result_copy);
                
            } 
            else
            {
                checkflag = checkIntRandom(respondResultString,result_copy);
            }

            free(result_copy);    

            if(checkflag)
            {
                printf("server: OK\n");
            }
            else
            {
                printf("server: ERROR\n");
                
            }

            // 顺手关掉
            close(maxfd);
            FD_CLR(maxfd, &redset_calculator);
            num--;
        }
        else
        {
            printf("server: result reply timeout occurred, closing connection\n");
            FD_CLR(maxfd, &redset_calculator);
            num--;
            close(maxfd);
            continue;
        }
        

        
        /*
        for(int i=0;i<=maxfd;++i)
        {
            if(i!=sockfd && FD_ISSET(i, &tmp))
            {
                struct timeval timeout_calculator;
                timeout_calculator.tv_sec = WAIT_TIME_SEC;
                timeout_calculator.tv_usec = 0;
                if (setsockopt(i, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_calculator, sizeof(timeout_calculator)) < 0) 
                {
                    perror("setsockopt");
                    FD_CLR(i, &redset);
                    close(i);
                    num--;
                    continue;
                }
                    
                // Initialize the library, this is needed for this library. 
                char *ptr;
                ptr=randomType(); // Get a random arithemtic operator. 
                double f1,f2;
                int i1,i2;
                i1=randomInt();
                i2=randomInt();
                f1=randomFloat();
                f2=randomFloat();

                // Generate random number
                char* operation = generateRandom(ptr,f1,f2,i1,i2);
                if (operation == NULL) 
                {
                    return -1;
                }
                char *formula = strtok(operation, "=");
                char *result = strtok(NULL, "=");
                char *result_copy = strdup(result);
                if (result_copy == NULL) 
                {
                    return -1;
                }

                // Send formula to client
                int tmpLength = strlen(formula);
                formula[tmpLength] = '\n';
                formula[tmpLength + 1] = '\0'; // 添加 null 终止符
                if (send(i, formula, strlen(formula), 0) == -1) 
                {
                    perror("send");
                    close(i);
                    continue;
                }
                printf("server: send %s",formula);

                char respondResultString[1024];
                memset(respondResultString, 0, sizeof(respondResultString));
                // if (recv(i, respondResultString, sizeof(respondResultString), 0) == -1) 
                // {
                //     perror("recv");
                //     close(i);
                //     continue;
                // }

                int bytesReceived = recv(i, respondResultString, sizeof(respondResultString), 0);
                if (bytesReceived == -1) 
                {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) 
                    {
                        // Timeout occurred
                        printf("server: result reply timeout occurred, closing connection\n");
                        // close(i);
                    } 
                    else 
                    {
                        perror("recv");
                    }
                    FD_CLR(i, &redset);
                    num--;
                    close(i);
                    break;
                } 
                else if (bytesReceived == 0) 
                {
                    // Connection closed by the client
                    FD_CLR(i, &redset);
                    close(i);
                    // continue;
                    break;
                }

                printf("server: receive respond: %s",respondResultString);
                printf("server: correct result:%s",result_copy);

                bool checkflag = NULL;     
                if(ptr[0]=='f')
                {
                    checkflag = checkDoubleRandom(respondResultString,result_copy);
                    
                } 
                else
                {
                    checkflag = checkIntRandom(respondResultString,result_copy);
                }

                free(result_copy);    

                if(checkflag)
                {
                    printf("server: OK\n");
                }
                else
                {
                    printf("server: ERROR\n");
                    
                }

                // 顺手关掉
                close(i);
                FD_CLR(i, &redset);
                num--;
            }
        }
        */
    }
//-----------------------------

    close(sockfd);

    return 0;
}

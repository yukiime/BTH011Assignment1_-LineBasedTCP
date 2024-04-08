#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <time.h>
#include <chrono>

// Additional include for calcLib
#include "calcLib.h"

#define BACKLOG 5   // how many pending connections queue will hold
#define MAXCLIENTS 5
#define SECRETSTRING "konijiwa00"
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

    // Parse IP and port from command-line argument
    char *Desthost = strtok(argv[1], ":");
    char *Destport = strtok(NULL, ":");
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

    // Setup random seed
    // srand(time(NULL));

    // Setup signal handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	{
        perror("sigaction");
        exit(1);
    }

    // Set up hints for getaddrinfo
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    // Get address information
    int rv;
    if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) 
    {

        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all the results and bind to the first we can
    int yes=1;
    int sockfd;
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

    while(1)
    {
        fd_set tmp = redset;
        int ret = select(maxfd+1,&tmp,NULL,NULL,NULL);
        // 判断是否listen
        if(FD_ISSET(sockfd,&tmp))
        {
            // client connection
            struct sockaddr_storage their_addr; // connector's address information
	        socklen_t sin_size;
            int cfd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);
            
            char s[INET6_ADDRSTRLEN];
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
            printf("server: got connection from %s\n", s);

            FD_SET(cfd, &redset);
            // 更新最大值
            maxfd = cfd > maxfd ? cfd : maxfd;
        }

        for(int i=0;i<maxfd;++i)
        {
            if(i!=sockfd && FD_ISSET(i, &tmp))
            {
                // Send supported protocol to client
                std::string protocolMessage = "TEXT TCP 1.0\n";
                if (send(i, protocolMessage.c_str(), protocolMessage.length(), 0) == -1) 
                {
                    perror("send");
                    close(i);
                    continue;
                }

                // Receive response from client
                char buf[5];
                if (recv(i, buf, sizeof(buf), 0) == -1) 
                {
                    perror("recv");
                    close(i);
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
                    close(i);
                    continue;
                }
                // 清空缓存区,理论上没有用
                memset(buf, 0, sizeof(buf));

                while(1)
                {
                    /* Initialize the library, this is needed for this library. */
                    initCalcLib();
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

                    bool checkflag = NULL;     
                    char respondResultString[1024];
                    memset(respondResultString, 0, sizeof(respondResultString));
                    if (recv(i, respondResultString, sizeof(respondResultString), 0) == -1) 
                    {
                        perror("recv");
                        close(i);
                        continue;
                    }
                    printf("server: receive respond: %s",respondResultString);
                    printf("server: correct result:%s",result_copy);

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
                    
                }

                close(i);
                break;
            }
        }
    }
//-----------------------------

    close(sockfd);

    return 0;
}

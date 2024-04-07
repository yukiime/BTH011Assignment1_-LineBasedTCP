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
#include <time.h>
#include <chrono>

// Additional include for calcLib
#include "calcLib.h"

#define BACKLOG 5   // how many pending connections queue will hold
#define SECRETSTRING "gimboid"

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

char* generateRandom()
{
	/* Initialize the library, this is needed for this library. */
	initCalcLib();
	char *ptr;
	ptr=randomType(); // Get a random arithemtic operator. 

	double f1,f2,fresult;
	int i1,i2,iresult;
	/*
	printf("ptr = %p, \t", ptr );
	printf("string = %s, \n", ptr );
	*/
	//  printf("Int\t");
	i1=randomInt();
	i2=randomInt();
	//  printf("Float\t");
	f1=randomFloat();
	f2=randomFloat();

	printf("  Int Values: %d %d \n",i1,i2);
	printf("Float Values: %8.8g %8.8g \n",f1,f2);

	
	/* Act differently depending on what operator you got, judge type by first char in string. If 'f' then a float */
	
	if(ptr[0]=='f'){
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

		char *operation = malloc(100); // Adjust buffer size accordingly
        snprintf(operation, 100, "%s %d %d = %d \n", ptr, i1, i2, iresult);
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

		char *operation = malloc(100); // Adjust buffer size accordingly
		snprintf("%s %d %d = %d \n",ptr,i1,i2,iresult);
		return operation;
	}
}


int main(int argc, char *argv[]) {
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

    // Convert port to integer
    int port = atoi(Destport);

    // Setup random seed
    srand(time(NULL));

    // Variable to store current time
    auto startTime = std::chrono::steady_clock::now();

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
    if ((rv = getaddrinfo(NULL, Destport, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all the results and bind to the first we can
    int sockfd;
    for (p = servinfo; p != NULL; p = p->ai_next) 
	{
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
            perror("server: socket");
            continue;
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

    while (1) 
	{
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) 
		{
            perror("accept");
            continue;
        }

        char s[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Send supported protocol to client
        std::string protocolMessage = "TEXT TCP 1.0\n\n";
        if (send(new_fd, protocolMessage.c_str(), protocolMessage.length(), 0) == -1) 
		{
            perror("send");
            close(new_fd);
            continue;
        }

        // Receive response from client
        char buf[3];
        if (recv(new_fd, buf, sizeof(buf), 0) == -1) 
		{
            perror("recv");
            close(new_fd);
            continue;
        }

        // Check if client accepted the protocol
        if (strcmp(buf, "OK\n") != 0) 
		{
            printf("Client did not accept the protocol. Disconnecting.\n");
            close(new_fd);
            continue;
        }

        // Generate random number
        char* operation = generateRandom()
		printf(operation)

        // Send operation to client
        if (send(new_fd, operation.c_str(), operation.length(), 0) == -1) 
		{
            perror("send");
            close(new_fd);
            continue;
        }

        // Check if client takes longer than 5 seconds to complete
        auto currentTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
        if (duration > 5) 
		{
            printf("Client took longer than 5 seconds. Terminating connection.\n");
            close(new_fd);
            break;
        }

        close(new_fd);
    }

    return 0;
}

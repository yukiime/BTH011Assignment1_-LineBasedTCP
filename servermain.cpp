#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG


using namespace std;


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
		
		return ("%s %8.8g %8.8g = %8.8g\n",ptr,f1,f2,fresult);
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

		return ("%s %d %d = %d \n",ptr,i1,i2,iresult);
	}
}


int main(int argc, char *argv[]){
  
	if (argc < 2)
	{
		fprintf(stderr, "Port number not provided. Program Terminated\n");
		exit(1);
	}

	/*
		Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
		Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
	*/
	char delim[]=":";
	char *Desthost=strtok(argv[1],delim);
	char *Destport=strtok(NULL,delim);
	// *Desthost now points to a sting holding whatever came before the delimiter, ':'.
	// *Dstport points to whatever string came after the delimiter. 

	/* Do magic */
	int port=atoi(Destport);

// print debug message
#ifdef DEBUG  
	printf("Host %s, and port %d.\n",Desthost,port);
#endif

    // sock
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	//char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

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


	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	{
		perror("sigaction");
		exit(1);
	}

	// message
	printf("server: waiting for connections from clients...\n");
	char msg[1500];
	int MAXSZ=sizeof(msg)-1;


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

        // Generate random
		char* operation = generateRandom();
		printf(operation);

        // Send operation to client
        if (send(new_fd, operation.c_str(), operation.length(), 0) == -1) {
            perror("send");
            close(new_fd);
            continue;
        }

        // Check if client takes longer than 5 seconds to complete
        auto currentTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
        if (duration > 5) {
            printf("Client took longer than 5 seconds. Terminating connection.\n");
            close(new_fd);
            break;
        }

        close(new_fd);
    }

}

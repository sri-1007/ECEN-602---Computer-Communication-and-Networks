/*
This is an echo server that services mutiple clients.

Authors: Srividhya Balaji , Sanjana Srinivasan
September 9/7/2018
*/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include<sys/wait.h>
#include<signal.h>

#define MAX_DATA_SIZE 200

/************************************************************
Function Name: writen 
Parameters   : client_socket, *str, n
Description  : Used to write n bytes. Prevents the shortcoming
               of std write sometimes sending out a lesser 
			   number of bytes.
************************************************************/
int writen(int client_socket, char *str, int n)
{
int sent_bytes, bytes_left;
bytes_left = n;  

while(bytes_left != 0)	
	{
	   sent_bytes = write(client_socket,str,bytes_left);
	   /* if error in transferring bytes*/
	   if(sent_bytes<=0)
	   {
		 if(errno == EINTR) 
		   sent_bytes = 0; // resetting the str . On exitting if will try to resend the bytes
		 else  
		   return -1;      // returing error if error not due to EINTR		   
	   }
	   str+= sent_bytes;
	   bytes_left-= sent_bytes;
	}
    
	printf("SERVER : Echoed %d bytes \n",n);
	return n;
}

/************************************************************
Function Name: sigchld_handler 
Parameters   : s
Description  : Signal Handler to effectively handle Zombie child
               processes
************************************************************/
void sigchld_handler(int st)
{
	 int s_errno= errno;
	 int pid;
	 while((pid = waitpid(-1,NULL,WNOHANG ))>0)
	 {
	 	   printf("Child Process ID: %d \n",pid);
	 }
	 errno =s_errno;

/* \n is necessary at the end of printf in C, so that it knows it is the end of buffer */	 
}


/************************************************************
Function Name: main
Parameters   : argc, argv
Description  : 
************************************************************/

int main(int argc, char *argv[]) 
{
	int socket_server,socket_client,portno,bytes_sent;  // Socket descriptor , port Number
	struct sockaddr_in server_addr,client_addr; // Server address object of sockaddr_in type
	unsigned int addrlen;
	char str[INET_ADDRSTRLEN]; //  String to print the IP ADDRESS
	char buff[MAX_DATA_SIZE];
    int bytes_recv;
	struct sigaction sa;

	if (argc != 2) 
	{
       		printf("Incorrect number of arguments.Use echos <PortNumber>\n" );
       		return 1;
 	}

	memset(&server_addr,0,sizeof(server_addr)); // Storing empty

	/* Configuring the Server Socket address */
	server_addr.sin_family = AF_INET;
	portno = atoi(argv[1]);                    // Port number will be given as an argument
	server_addr.sin_port = htons(portno);     //Converting port number in host byte order to port number in network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY; // Contains the IP of the host. INADDR_ANY, gets the address of the machine auto

	/* Creating a new Socket descriptor of required TCP type*/
	if((socket_server = socket(AF_INET,SOCK_STREAM,0))<0)
	{
  		printf("Could not Create SERVER Socket\n" );
  		if(errno)
    			printf("Exiting due to error : %s\n", strerror(errno));
  		return 1;
	}


	/* Bind Socket to listen to requests */
	if((bind(socket_server,(struct sockaddr*)&server_addr,sizeof(server_addr)))<0)
	{
  		printf("Could not bind SERVER Socket\n" );
  		if(errno)
    			printf("Exiting due to error : %s\n", strerror(errno));
  		return 1;
	}

	int wait_size = 16;  // maximum number of waiting clients, after which dropping begins
	if (listen(socket_server, wait_size) < 0) 
	{
		printf("Could not open socket for listening\n");
    		if(errno)
      			printf(" Exiting due to error : %s\n", strerror(errno));
		return 1;
	}
	
  	sa.sa_handler =sigchld_handler;	
    sigemptyset(&sa.sa_mask);
    sa.sa_flags= SA_RESTART ;									
    if(sigaction(SIGCHLD ,&sa,NULL)==-1) 
	{								
      printf(" sigaction \n");
      return 1;
     }

	printf("Server Ready. Waiting for client on %d\n ",portno);

	/*Accept connection and print IP*/
	while(1)
	{

		addrlen = sizeof(client_addr);
		if((socket_client =accept(socket_server, (struct sockaddr*)&client_addr, &addrlen)) <0)  
		{
 			 printf("Could not accept data through socket\n");
  			if(errno)
   				 printf("Exiting due to error : %s\n", strerror(errno));
  			return 1;
		}
  		
  		printf("Connection Established\n");
					
  		inet_ntop(AF_INET, &client_addr.sin_addr, str, INET_ADDRSTRLEN);
  		printf("IP ADDRESS OF CLIENT : %s\n",str);
		
	if(!fork()){
	
		while(1){ // To continuously keep receiving
			 
			bytes_recv	= read(socket_client,buff,MAX_DATA_SIZE-1);	// Reading data from the buffer
			if(bytes_recv == -1)
			{
				printf("DATA READ ERROR\n");
				if(errno)
					 printf("Exiting due to error : %s\n", strerror(errno));
				return 1;
			}
			else if(bytes_recv == 0)
			{
				/*EOF returns 0*/
				printf("EOF for client\n");
				printf("Disconnecting Clinet \n"); 			
				close(socket_client);
				return 1;
				
			}			
			buff[bytes_recv] = '\0';
			printf("Server Received : %s \n ",buff);
			bytes_sent=writen(socket_client,buff,strlen(buff));
			
		}

	}
	}
	close(socket_client);
	close(socket_server);
  	return 0;
}

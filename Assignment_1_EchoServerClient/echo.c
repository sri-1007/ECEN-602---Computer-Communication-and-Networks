/*
This is an echo client

Authors: Srividhya Balaji , Sanjana Srinivasan
September 9/7/2018
*/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   // Warning for close()
#include <errno.h>


#define MAX_DATA_SIZE 200
static int text_buffer_count;
static char *text_buffer_ptr;
static char buftext[MAX_DATA_SIZE];


/************************************************************
Function Name: writen 
Parameters   : client_socket, *str, n
Description  : Writes n bytes to the socket and returns n on 
               success and -1 on error. Since Blocking TCP is 
			   considered, no check on n is maintained at the 
			   functioncall place to check if write is 
			   complete for the entire n bytes
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
    printf("CLIENT : Wrote %d bytes \n",n);	
	return n;
}

/************************************************************
Function Name: localRead 
Parameters   : client_socket, *	ptr
Description  : Reads data character wise for READLINE function
************************************************************/
int localRead(int client_socket, char *ptr)
{
if(text_buffer_count<=0)
{
x:
	text_buffer_count = read(client_socket, buftext, sizeof(buftext));
	if(text_buffer_count<0)
	{
		if(errno == EINTR)
			goto x ;
		else
			return -1 ;
	}
	else if(text_buffer_count == 0)
	    return 0;
	
	text_buffer_ptr = buftext;
}

text_buffer_count--;
*ptr = *text_buffer_ptr;
text_buffer_ptr++;	

return 1;

}

/************************************************************
Function Name: readline 
Parameters   : client_socket, *str, nMax
Description  : Calls localRead to read data from the socket
************************************************************/
int readline(int client_socket, char *str, int nMax)
{	
 int bytes_recv,i;
 char ch,*localptr;
 localptr = str;
 
 for(i = 1; i<nMax ; i++)
 {
     bytes_recv = localRead(client_socket, &ch); // Reads every character from buffer into ch
     if(bytes_recv ==1)
	 {
        *localptr = ch;
		 localptr += 1;
		 if(ch == '\n') // Newline character termination
			 break;

	 }
     else if(bytes_recv == 0)
	 {
         *localptr = 0; // EOF situation
		 return(i-1);
	 }
     else
      return -1; // ERROR		 
	 	 
 }
	
*localptr = 0;
return i; // Number of bytes read	

}

/************************************************************
Function Name: main
Parameters   : argc, argv
Description  : 
************************************************************/
int main(int argc, char *argv[])
{
  	char *server_IP;
	int servPort, socket_client;
  	struct sockaddr_in server_address;
	char msg[MAX_DATA_SIZE];
	char buff[MAX_DATA_SIZE];
	
	
	if(argc !=3) 
	{
		printf("Incorrect number of arguments. Use: echo <IP Address> <PortNumber>\n");
	        return 1;
	}

	server_IP = argv[1];
	servPort = atoi(argv[2]);

	if((socket_client = socket(AF_INET, SOCK_STREAM,0))<0)
	{
			printf("Client socket not created\n");
			if(errno)
		       printf("Exiting due to error : %s\n", strerror(errno));
			return 1;
	}

	memset(&server_address,0,sizeof(server_address));

	server_address.sin_family = AF_INET;
  	server_address.sin_addr.s_addr = inet_addr(server_IP);
	server_address.sin_port = htons(servPort);

	if(connect(socket_client, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		printf("Connection not established\n");
		if(errno)
	    		printf("Exiting due to error : %s\n", strerror(errno));
		return 1;
	}

	printf("Connection Established\n");
    printf("ENTER MESSAGES TO BE SENT\n ");	
	while(1)
	{
       bzero(msg,MAX_DATA_SIZE); // intializing the msg to 0
       if((fgets(msg, MAX_DATA_SIZE, stdin) == NULL)||feof(stdin)) // feof(stdin) returns true on Ctrl+D ,fgets returns NULL on EOF
	   {
		 printf("EOF Cntrl+D \n ");
		 printf("Disconnecting client \n");
		 close(socket_client);
		 return 1;
	   } 

       /* Writing DATA */
	   if((writen(socket_client,msg,strlen(msg))) == -1)
	   {
		   printf("WRITE ERROR \n");
		   if(errno)
		       printf("Exiting due to error : %s\n", strerror(errno));
			return 1;
	   }
	   
	   /*Reading the echoed data*/
	   if((readline(socket_client,buff,MAX_DATA_SIZE)) == -1)
	   {
		 		
		 printf("CLIENT : READ ECHO ERROR \n");
		   if(errno)
		       printf("Exiting due to error : %s\n", strerror(errno));
			return 1;   
		   
	   }
	   
	  printf("%s \n",buff);
	}
			
	close(socket_client);
	return 0;	
		
	}	
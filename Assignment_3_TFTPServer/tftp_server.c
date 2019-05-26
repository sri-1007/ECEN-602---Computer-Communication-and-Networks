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


#define MAXBUFFERLENGTH 520
#define MAXDATASIZE 512
#define TIMEOUT 1

int next_char = -1;

/*UNp reference*/
int timeout_check(int fd)
{
	fd_set rset;
	struct timeval tv;
	
	FD_ZERO(&rset);
	FD_SET(fd,&rset);
	
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	return (select(fd+1,&rset,NULL, NULL, &tv));
	/* >0 if descriptor is readable */	
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
int main(int argc,char *argv[])
{
	
	int socket_server,portno,server_ip,recvbytes; 
	struct sockaddr_in server_addr,client_addr;
	socklen_t client_size;
	struct sigaction sa;
	
	char recv_buf[MAXBUFFERLENGTH];
	char str[INET_ADDRSTRLEN];
	
	char send_buffer[MAXBUFFERLENGTH];
	
	int childHandler;
	char data_sent[MAXDATASIZE];
	
	
	if (argc != 3) 
	{
       		printf("Incorrect number of arguments.Use tftp_server IP <PortNumber>\n" );
       		return 1;
 	}	
	
	memset(&server_addr,0,sizeof(server_addr)); // Storing empty
	memset(&client_addr, 0, sizeof client_addr);
    memset(recv_buf, 0, sizeof(recv_buf));

	server_ip = atoi(argv[1]);
	portno = atoi(argv[2]);
	
	/* Configuring the Server Socket address */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);     //Converting port number in host byte order to port number in network byte order

	/* Creating a new Socket descriptor of required UDP type*/
	if((socket_server = socket(AF_INET,SOCK_DGRAM,0))<0)
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
		close(socket_server);
  		return 1;
		
	}

	printf("Server is waiting for connections... \n");
	
	while(1)
	{
		 client_size = sizeof client_addr;
		 if((recvbytes = recvfrom(socket_server, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&client_addr, &client_size)) < 0)
       {
            printf("Could not RECEIVE OVER SOCKET\n" );
  		if(errno)
    			printf("Exiting due to error : %s\n", strerror(errno));
  		return 1;
       }
       
       inet_ntop(AF_INET, &client_addr.sin_addr, str, INET_ADDRSTRLEN);
       printf("Server established connection with CLIENT from: %s \n",str);
	   

	   sa.sa_handler = sigchld_handler; // reap all dead processes
       sigemptyset(&sa.sa_mask);
       sa.sa_flags = SA_RESTART;
       if (sigaction(SIGCHLD, &sa, NULL) == -1)
		   {
			perror("sigaction");
			exit(1);
		   }

		if(!fork())
		{
			char fileName[] = " ";
			FILE *fileptr;
			int opcode,mode;
			size_t len_fileName;
			char tempFileName[10];
				
			opcode = recv_buf[1]; 
			strcpy(fileName, &recv_buf[2]);
			//fileptr = fopen(fileName,"r");
			len_fileName = strlen(fileName);
			
			printf("FILENAME :%s \n" ,fileName);
			
			/*checking MODE of incoming FILE*/
				if(strcasecmp(&recv_buf[len_fileName+3] , "netascii" ) == 0)
					 mode =1 ;
				else if(strcasecmp(&recv_buf[len_fileName+3],"octet") == 0)
					 mode =2;
				else
				{
					/*1.  ERROR HANDLING : MODE NOT SPECIFIED*/ 
					 printf("UNABLE TO UNDERSTAND MODE ::ILLEGAL OPERATION \n ");
					 memset(send_buffer , 0, sizeof(send_buffer));
					 
					 send_buffer[1] = 5; // OPCODE FOR ERROR ;
					 send_buffer[3] = 4; // ERROR NUMBER FOR ILLEGAL OPERATION
					 
					 stpcpy(&send_buffer[4], "MODE NOT SPECIFICIED" );
					 len_fileName = strlen("MODE NOT SPECIFIED");
					 
					if(sendto(socket_server,send_buffer,len_fileName+5,0,(struct sockaddr *)&client_addr, client_size)<0)
					{
						 if(errno)
							printf("Exiting due to error : %s\n", strerror(errno)); 
						 close(socket_server);
						 return(1); ;
					}
					 
					 
				}
			
			/*Checking if file is not valid and sending error message*/
			 /*2.  ERROR HANDLING : FILE SPECIFIED NOT FOUND*/ 
				 
				 if((fileptr = fopen(fileName,"r")) == NULL)
				 {
					printf("FILE NOT FOUND \n");
					memset(send_buffer,0,sizeof(send_buffer));
					
					send_buffer[1] = 5; // opcode for error
					send_buffer[3] = 1; //errnumber for file not found
					
					strcpy(&send_buffer[4],"FILE NOT FOUND");
					len_fileName = strlen("FILE NOT FOUND");
					
					if(sendto(socket_server,send_buffer,len_fileName+5,0,(struct sockaddr *)&client_addr, client_size)<0)
					{
						if(errno)
							printf("Exiting due to error : %s\n", strerror(errno)); 
						 close(socket_server);
						 return(1); 	
						
					}
					 
				}
				
				
			/*Processing based on MODE REQUEST*/
				if(mode == 1)
				{
					fseek(fileptr , 0, SEEK_END);
					int fileLength = ftell(fileptr);
					printf("Length of File = %d Bytes \n" ,fileLength);
					fseek(fileptr , 0, SEEK_SET);
					
					
					FILE *newTempFile;
					newTempFile = fopen(tempFileName, "w");
					int ch = 1;
					
			    /*UNP REFERENCE*/	
					while(ch != EOF)
					{
						if(next_char >= 0)
						{
						   fputc(next_char,newTempFile)	;
						   next_char = -1;
						   continue;
						}
						
						ch = fgetc(fileptr);
						if(ch == EOF)
						{
							if(ferror(fileptr))
								printf("READ ERROR :: fgetc");
							break;
							
						}
						else if(ch == '\n')
						{
						   ch = '\r';
						   next_char = '\n';
						}
						else if(ch == '\r')
						{
							next_char = '\0';
						}
						else
						   next_char = -1;
						
						
						fputc(ch,newTempFile);
						
					}
					
					fseek(newTempFile, 0,SEEK_SET);
					fileptr = newTempFile;
					fileptr = fopen(tempFileName , "r");
							 
				}
			
			
			close(socket_server);
				
				
				/* FOR EPHEMERAL PORT : ANY PORT CAN BE USED BY BOTH SERVER AND CLIENT*/
				server_addr.sin_port = htons(0);	
				
				/*Socket creation for child process. Messages are sent using this*/
				
				if((childHandler = socket(AF_INET,SOCK_DGRAM,0))<0)
				{
					 printf("Could not Create CHILD Socket\n" );
					 if(errno)
					   printf("Exiting due to error : %s\n", strerror(errno));
					 return 1;
				}
		
				/* Bind Socket to listen to requests */
				if((bind(childHandler,(struct sockaddr*)&server_addr,sizeof(server_addr)))<0)
				{
					printf("Could not bind child Socket\n" );
					if(errno)
						printf("Exiting due to error : %s\n", strerror(errno));
					close(childHandler);
					return 1;
					
				}
				
				
			/*set of statements for sending data, ACK, indicating EOF and timeouts*/
				/*to keep track of packets*/
				unsigned short int sent_blockNum, sentNum;
				sent_blockNum = 0;
				sentNum = 0;
				
				unsigned short int block_fin,ack_num;
				block_fin = 0;
				ack_num = 0;
				
				unsigned short int recv_num;
				
				/*to keep tack of file offset*/
				unsigned int f_offset = 0;
				int bytes_read;
				
				/*To send*/
				int sentBytesHandler,recvBytesHandler;
				
				int time_out_count = 0;
				int tv;
				
			while(1)
			{
				
				memset(send_buffer,0,sizeof(send_buffer));
				sprintf(send_buffer,"%c%c",0x00,0x03);	
				
					sentNum = sent_blockNum + 1; //Keeping track of the number of sends and number of blocks sents
					/*writing the 2nd and 3rd entry in the send_buffer to indicate this number*/
					
					send_buffer[2] = (sentNum & 0xFF00) >>8;
					send_buffer[3] = (sentNum & 0x00FF);
					
					fseek(fileptr,f_offset*MAXDATASIZE,SEEK_SET);
					memset(data_sent,0,sizeof(data_sent));
					
					bytes_read = fread(data_sent,1,MAXDATASIZE,fileptr);
					
					
					/*check for LAST BLOCK*/
					if(bytes_read < 512) 
					{
					   /*INDICATES THAT IT IS THE LAST BLOCK*/
						if(bytes_read == 0)
						{
						   send_buffer[4] = '\0' ; // data block in the buffer
						   printf("ZERO BYTES BLOCK \n");
						}
						else
						{
						   memcpy(&send_buffer[4],data_sent,bytes_read);
						   printf("LESS THAN 512 BYTES : LAST BLOCK READ \n");
							
						}
						 block_fin = sentNum;
						 printf("block_fin : %d \n" ,block_fin);
											
					}
					else					
					{
						memcpy(&send_buffer[4],data_sent,bytes_read);
					
					}
					
					if((sentBytesHandler = sendto(childHandler,send_buffer,bytes_read+4,0,(struct sockaddr *)&client_addr,client_size)) < 0)
					{	
					   printf("SEND ERROR \n");
					   if(errno)
						  printf("Exiting due to error : %s\n", strerror(errno)); 
						break;
					
					}
					
					/*CHECK FOR TIMEOUTS*/
					if(tv = timeout_check(childHandler) == 0)
					{
						if(time_out_count == 10)
						{
							printf("MAX NUMBER OF 10 TIMEOUTS OCCURRED \n");
							break;
							
						}
						else
						{
							printf("TIMEOUT OCCURRED \n");
							time_out_count++;
							continue;
						}
					
					}
					else if(tv == -1)
					{
					   printf("CHILD SELECT ERROR \n");
					   if(errno)
							printf("Exiting due to error : %s\n", strerror(errno)); 
						break;
					}
					else
					{
					
						memset(recv_buf,0,sizeof(recv_buf));
					   if((recvBytesHandler = recvfrom(childHandler,recv_buf,sizeof(recv_buf),0,(struct sockaddr*)&client_addr,&client_size)) < 0)
						{
						 printf("CHILD RECEIVE ERROR \n");
						  if(errno)
							printf("Exiting due to error : %s\n", strerror(errno)); 
						 break;   
						}
						
						opcode = recv_buf[1];
						memcpy(&recv_num, &recv_buf[2],2);
						ack_num = ntohs(recv_num);
						
						printf("OPCODE RECEIVED %d \n " , opcode);
						printf("ACK RECEIVED : ACK_NUM %d \n",ack_num);
						if(opcode == 4) /* opcode for ack*/
						{
								
								
								if(ack_num == block_fin && bytes_read <512)
								{
									printf("FINAL ACK RECEIVED \n");
									break;
								
								}
								
								sent_blockNum++;
								f_offset++;
						}
						else
						{
							printf("WRONG ACK FORMAT \n");
							break;
					   
						}
					}
					
			}
		  
		  if(mode ==1)
			{
			    if(remove(tempFileName) !=0)
					printf("ERROR DELETING TEMP FILE \n");
				else
					printf("TEMP FILE IS DELETED \n");
			}	
			
		 close(childHandler);
		 printf("CHILD DISCONNECTED \n");
		 exit(0); //exit child process
			
		}

	}
}
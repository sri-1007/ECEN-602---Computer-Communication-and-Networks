/*
This is an chat server that services mutiple clients.

Authors: Srividhya Balaji , Sanjana Srinivasan
Date   : 9/24/2018

1. CHECKS IF ANY USER WANTS TO CONNECT. REGULAR ACCEPT AND SOCKET CONNECTION
2. IF AN ALREADY CONNECTED USER TRIES TO COMMUNICATE, 
   2.1. JOIN REQUEST. PROCESS BASED ON MAXCLIENTS AND SEND ACK OR NAK 
   2.2  ONCE JOINED, SEND ONLINE MESSAGE TO OTHER CLIENTS
   2.3  WHEN NUMBYTES RECEIVED = 0, CHECK AND SEND OFFLINE MESSAGE
   2.4  SEND IDLE WHEN CLIENT DOESN'T SEND MESSAGE FOR LONG

sbcp_msg.type=='5' - NAK OR REJECT MESSAGE
sbcp_msg.type=='7' - ACK MESSAGE
sbcp_msg.type=='8' - ONLINE MESSAGE
sbcp_msg.type=='6' - OFFLINE MESSAGE
sbcp_msg.type=='9' - IDLE MESSAGE
sbcp_msg.type=='3' - FWD MESSAGE
sbcp_msg.type=='4'  - SEND MESSAGE

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
#include<ctype.h>
#include <stdarg.h>  // va_list, var_arg : For variable argument lists

#define MAX_DATA_SIZE 200
char active_users[160];
char current_usernames[10][16];
char list[10][16];

struct SBCP_attribute{
	uint16_t type;
	uint16_t length;
	uint16_t client_count;
	char reason[32];
	char *attr_payload;
};
	
struct SBCP_message{
	uint8_t vrsn; // we have assigned a bit field to version.vrsn=3 in our case
	uint8_t type; //assign type=2 for JOIN,3 for forward and 4 for send
	uint16_t length;
	struct SBCP_attribute *attribute;
};
/************************************************************
Function Name: initialize_sbcp_attr 
Description  : To initialize the sbcp_attr
************************************************************/
void initialize_sbcp_attr(struct SBCP_attribute *struct_ptr, int type_attr ,char *ptr_to_buffer,int len){
		(struct_ptr->type)= type_attr;
		(struct_ptr->attr_payload)= ptr_to_buffer; 
		struct_ptr->length= 4+len;
}

/************************************************************
Function Name: initialize_sbcp_msg 
Description  : To initialize the sbcp_msg
************************************************************/
void initialize_sbcp_msg(struct SBCP_message *struct_msg_ptr,int version,int type_msg,struct SBCP_attribute *attr123){
		(struct_msg_ptr->vrsn)=version;
		(struct_msg_ptr->type)= type_msg;
		(struct_msg_ptr->attribute)=attr123;
		(struct_msg_ptr->length)=(attr123->length) +4;
}

/************************************************************
Function Name: packi16, pack 
Description  : To pack the message in the given SBCP format
************************************************************/
void packi16(unsigned char *packed_buff, unsigned int i)
{
    	*packed_buff++ = i>>8; *packed_buff++ = i;
}

int32_t pack(char *packed_buff, char *format, ...)
{
    va_list ap; // type to hold information about variable argument parameters
    int16_t h;  // represents short - 2 bytes - convert to 8*2 = 16 bits information
	int8_t c;   // represents char - 1 byte - convert to 8 bits of information
    char *str;    // String to which the result packed string is going to be stored locally
    int32_t size = 0, length;
    va_start(ap, format);
    for(; *format != '\0'; format++) {
		switch(*format) {
			case 'h': // 16-bit
				size += 2;
				h = (int16_t)va_arg(ap, int); 
				packi16(packed_buff, h);
				packed_buff += 2;
				break;
			case 'c': // 8-bit
				size += 1;
				c = (int8_t)va_arg(ap, int);
				*packed_buff++ = (c>>0)&0xff;
				break;
			case 's': // string
				str = va_arg(ap, char*);
				length = strlen(str);
				size += length + 2;
				packi16(packed_buff, length);
				packed_buff += 2;
				memcpy(packed_buff, str, length);
				packed_buff += length;
				break;
		}
	}
	va_end(ap);
	return size;
}

/************************************************************
Function Name: unpacki16, unpack 
Description  : To unpack the message in SBCP format
************************************************************/
unsigned int unpacki16(char *packed_buff)
{
   return (packed_buff[0]<<8) | packed_buff[1];
}

void unpack(char *packed_buff, char *format, ...)
{
	va_list ap;
	int16_t *h;
	int8_t *c;
	char *str;
	int32_t length, count, max_strlen=0;
	va_start(ap, format);
	for(; *format != '\0'; format++) {
		switch(*format) {
            case 'h': // 16-bit
				h = va_arg(ap, int16_t*);
				*h = unpacki16(packed_buff);
				packed_buff += 2;
				break;
			case 'c': // 8-bit
				c = va_arg(ap, int8_t*);
				*c = *packed_buff++;
				break;
			case 's': // string
				str = va_arg(ap, char*);
				length = unpacki16(packed_buff);
				packed_buff += 2;
				if (max_strlen > 0 && length > max_strlen) 
					count = max_strlen - 1;
				else 
					count = length;
				memcpy(str, packed_buff, count);
				str[count] = '\0';
				packed_buff += length;
				break;
			default:
				if (isdigit(*format)) 
				{ // track max str len
					max_strlen = max_strlen * 10 + (*format-'0');
				}
		}
		
		if (!isdigit(*format)) 
			max_strlen = 0;
	}
	va_end(ap);
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

	int max_clients, server_ip;
	struct sigaction sa;
	
	fd_set master;
    fd_set read_fds;
    int fdmax,i,j,total_count;  //maximum file descriptor number
	char username_buffer[16];
	int size;
	char offline_message[100]="The following user has left the chat room:";
    struct SBCP_message sbcp_msg;
    struct SBCP_attribute sbcp_attr1;
    struct SBCP_attribute sbcp_attr2;
	uint16_t count=0;
	int g,g1,x,nbytes;
    int comp_res;
	char user[16];
 
    char reject_message[512];
    char online_message[100]="Our newly joined member is:";
    char message_buffer[530];
	
	int reject_counter;
	
	if(argc !=4)
	{
		printf("Incorrect no. of arguments include server_ip server_port ma_clients\n");
		return 1;
	}
	memset(&server_addr,0,sizeof(server_addr)); // Storing empty

	server_ip = atoi(argv[1]);
	portno = atoi(argv[2]);
    max_clients = atoi(argv[3]);

	/* Configuring the Server Socket address */
	server_addr.sin_family = AF_INET;//AF_UNSPEC;
	server_addr.sin_port = htons(portno);     //Converting port number in host byte order to port number in network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY; // Contains the IP of the host. INADDR_ANY, gets the address of the machine auto

	/* Creating a new Socket descriptor of required TCP type*/
	if((socket_server = socket(/*AF_UNSPEC*/AF_INET,SOCK_STREAM,0))<0)
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

	if (listen(socket_server, max_clients) < 0) 
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

	printf("Server Ready. Waiting for connections on %d\n ",portno);

    FD_ZERO(&master); // initialize the set
	FD_ZERO(&read_fds);
	FD_SET(socket_server, &master);//add socket_server to master set
	
	fdmax = socket_server; //find biggest file descriptor
	
	while(1)
	{
		
		read_fds = master;
/*continously checking for read activity*/
		if(select(fdmax+1, &read_fds, NULL,NULL,NULL)==-1)
		{
			printf("SERVER :: Select ERROR\n");
			return 1;
		}
			
/*find if there is data being sent from existing connections*/
		for(i=0;i<=fdmax;i++)
		{			
		   if(FD_ISSET(i,&read_fds)) //there is data to read
		   {	
/*SCENARIO 1: NEW CLIENT TRYING TO CONNECT. REGULAR SOCKET ACCEPT OPERATION*/
		     if(i==socket_server) 
				{
						addrlen = sizeof(client_addr);
						if((socket_client = accept(socket_server,(struct sockaddr*)&client_addr, &addrlen)) <0)
						{
 			 				printf("Could not accept data through socket\n");
  							if(errno)
                                printf("Exiting due to error : %s\n", strerror(errno));
  							return 1;
						}
  						else
						{
							FD_SET(socket_client, &master); //add master to the set
							if(socket_client >fdmax) // find max
							{
								fdmax = socket_client;
							}							
						}
				}
				
				else
				{
/*SCENARIO:2  SEQUENCE OF OPERATIONS FOR AN ALREADY EXISTING CLIENT IN THE CHAT ROOM

SCENARIO 2.1 : WHEN NUMBER OF BYTES RECEIVED IS LESS THAN OR EQUAL TO ZERO. THIS MEANS EITHER CLIENT IS DISCONNECTED
OR RECEIVE ERROR. IF CLIENT IS OFFLINE REMOVE FROM THE EXISTING USERS LIST*/
				    if((nbytes =recv(i,buff,sizeof buff,0)) <=0) 
					{ 
				       int f,k1,z1;
					   int flag=1; 
					   int h;
					   
					   if(nbytes ==0) 
					    {
							for(h=0;h<i;h++) 
							{
						       if(!(strcmp(list[h],username_buffer))) 
							   {
							       flag=0;
						       }
					        }
							
							if(flag!=0) 
							{	
						        for(g1=0;g1<=fdmax;g1++) 
								{
			   	        				
									if(FD_ISSET(g1,&master)) 
									{
										if(g1!=socket_server && g1!=i) 
										{
/*FEATURE 1: OFFLINE MESSAGE :SENDING THE OFFLINE STATUS TO OTHER CLIENTS*/										
											size= pack(buff,"cchhhhshhs",'3','6',sbcp_msg.length,2,sbcp_attr1.length,count,list[i],4,sbcp_attr2.length,offline_message);
										    printf("%s, left the chatroom \n",username_buffer);
											if(send(g1,buff,size,0) ==-1) 
											{
												printf("SERVER:: SEND ERROR IN NUMBYTES = 0 \n");
											} 
										}
								    
									} 
								}

/*REMOVING THE OFFLINE CLIENT FROM THE LISTS STORED*/
                                strcpy(user,list[i]);
					            count--;
					            for(f=0;f<10;f++) 
							    {
						            if(!strcmp(current_usernames[f],user)) 
									{
							           for(k1=f;k1<9;k1++) 
									   {
								         strcpy(current_usernames[k1],current_usernames[k1+1]);
							           }
						            }	
					            } 
					            memset(list[i],0,sizeof(list[i]));	
							}
						}
						else if(!reject_counter)
						{
                              printf(" RECEIVE ERROR \n");	  
                        }
                        close(i);
				        FD_CLR(i,&master);						
				    }
					else
					{
/*SCENARIO : 2.2 : WHEN A MESSAGE IS RECEIVED FROM THE CONNECTED CLIENTS.
THIS CAN BE JOIN REQUESTS OR MESSAGES.*/					
					  
					  unpack(buff,"cchhhshhs",&sbcp_msg.vrsn,&sbcp_msg.type,&sbcp_msg.length,&sbcp_attr1.type,&sbcp_attr1.length,username_buffer,&sbcp_attr2.type,&sbcp_attr2.length,message_buffer);

/*Processing JOIN REQUEST*/					  
					    if(sbcp_msg.type=='2')  
					    {
					        int u,current_pos;
							comp_res=0;
						    int t9=0;
							if(count==0) 
							{
								strcpy(current_usernames[0],username_buffer);
								strcpy(list[i],username_buffer);
								current_pos=1;
								char welcome_message[512]="Your username for the chat is";
								
/*FEATURE 1 : SENDING ACK MESSAGE :ACCEPTING JOIN AND SENDING IT ITS USERNAME MESSAGE*/								

								pack(buff,"cchhhhshhs",'3','7',sbcp_msg.length,2,sbcp_attr1.length,count,username_buffer,4,sbcp_attr2.length,welcome_message);
								t9=1;
								count++;
								printf("%s has joined \n",username_buffer);
						    }
							else 
							{						
								for(u=0;u<10;u++) 
								{
									 if(!strcmp(current_usernames[u],username_buffer)) 
									{
										comp_res=10;
									}
								}
						    
							}
							
							if(t9!=1)
							{
/*PROCESSING THE JOIN REQUEST. ADDING TO USERS_LIST, ACTIVE_USERS_LIST etc.
CASE 1: ADDED ONLY IF LESS THAN MAX COUNT. SENDING ONLINE USER MESSAGE TO ALL USERS 
CASE 2: IF MORE THAN MAX COUNT SENDING NAK OR REJECT MESSAGE*/								
								if(comp_res!=10 && count <max_clients) // max_clients is max number of clients that could be connected
								{
									
								 strcpy(list[i],username_buffer);
								 strcpy(current_usernames[current_pos],username_buffer);
							 	 current_pos++;
								 count++;
								 printf("%s has joined \n",username_buffer);
                                 reject_counter = 0;								 
								 
								 for(x=0;x<=9;x++) 
									{
										strcat(active_users,current_usernames[x]) ;
										strcat(active_users," ");
									}

								 for(g=0;g<=fdmax;g++) 
									{
										if(FD_ISSET(g,&master)) 
										{
											if(g!=socket_server && g!=i) 
											{
/*FEATURE 1: SENDING ONLINE MESSAGE*/
											  size= pack(buff,"cchhhhshhs",'3','8',sbcp_msg.length,2,sbcp_attr1.length,count,username_buffer,4,sbcp_attr2.length,online_message);
												if(send(g,buff,size,0) ==-1) 
												{
												 printf("SERVER :: SEND ERROR\n ");
												} 
											} 
										} 
									}								 
								
								  char welcome_message[512]="List of Online users :\n";
							 	  pack(buff,"cchhhhshhs",'3','7',sbcp_msg.length,2,sbcp_attr1.length,count,active_users,4,sbcp_attr2.length,welcome_message);
								  memset(active_users,0,sizeof(active_users));
								}
								else
								{
									if(count>=max_clients) 
									{					
									 strcpy(reject_message,"MAX USERS to join the chat room reached....");		
									}
									if(comp_res==10)
									{
									    strcpy(reject_message,"USERNAME already exists....");
								    }	
									reject_counter = 1;
/*FEATURE 1 : NAK. SENDING REJECT MESSAGE*/ 									
								 pack(buff,"cchhhhshhs",'3','5',sbcp_msg.length,2,sbcp_attr1.length,count,username_buffer,4,sbcp_attr2.length,reject_message);
								
				
						        }
							}
					
					        if(send(i,buff,200,0)==-1) 
							{
							 		printf("SERVER :: SEND ERROR\n ");
							}
						}
						
/*SENDING MESSAGE SENT BY ONE CLIENT TO ALL CLIENTS*/						
						if(sbcp_msg.type=='4')
						{
						
							for(j=0;j<=fdmax;j++)
							{
									
								if(FD_ISSET(j,&master)) 
								{
										if(j!=socket_server && j!=i)
										{
											initialize_sbcp_attr(&sbcp_attr1,2,username_buffer,16);
											initialize_sbcp_msg(&sbcp_msg,'3','3',&sbcp_attr1);
											initialize_sbcp_attr(&sbcp_attr2,4,message_buffer,530);
											size=pack(buff,"cchhhhshhs", sbcp_msg.vrsn, sbcp_msg.type,sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,count,username_buffer,sbcp_attr2.type,sbcp_attr2.length,message_buffer);
											if(send(j,buff,size,0) ==-1) 
											{
					    				       printf("SERVER :: SEND ERROR \n");
											}
							
											
										}
									
							    }
								
						    }
						
					    }
						
/*FEATURE 2: SENDING IDLE STATUS OF NON-ACTIVE CLIENTS TO OTHERS*/						
						if(sbcp_msg.type=='9')
						{
							for(j=0;j<=fdmax;j++)
							{
								if(FD_ISSET(j,&master))
								{
									if(j!=socket_server && j!=i)
									{
										initialize_sbcp_attr(&sbcp_attr1,2,username_buffer,16);
								        initialize_sbcp_msg(&sbcp_msg,'3','9',&sbcp_attr1);
								        initialize_sbcp_attr(&sbcp_attr2,4,message_buffer,530);
			                            size=pack(buff,"cchhhhshhs", sbcp_msg.vrsn, sbcp_msg.type,sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,count,username_buffer,sbcp_attr2.type,sbcp_attr2.length,message_buffer);
										
										if(send(j,buff,size,0) ==-1) 
										{
					    				    printf("SERVER :: SEND ERROR \n");
										}
									}
								
								}
							 
							}
		                }
						

					}
				}
		   
		   }		   
			
	    }
			
    }
return 0;
}
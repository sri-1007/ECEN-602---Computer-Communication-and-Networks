/*
This is an chat client that participates in a chat room.

Authors: Srividhya Balaji , Sanjana Srinivasan
Date   : 9/24/2018
*/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   // Warning for close()
#include <errno.h>
#include <stdarg.h>  // va_list, var_arg : For variable argument lists
#include<ctype.h>    //isdigit()

#define MESSAGE_LEN_LIMIT 512 //512 Bytes is the maximum message length allowed : Given. 
#define MAX_DATA_SIZE 200

struct SBCP_attribute{
	unsigned int type;   
	unsigned int length;
	char *attr_payload;
};

struct SBCP_message{
	unsigned int vrsn; // vrsn protocol version =3
	unsigned int type; //assign type=2 for JOIN,3 for forward and 4 for send
	unsigned int length;
	struct SBCP_attribute *attribute;
};


/*Methods to initialize Structures objects */

void initialize_sbcp_attr(struct SBCP_attribute *struct_ptr, int _type, int _length, char *_payload)
{
	
struct_ptr->type = _type;
struct_ptr->length = _length;
struct_ptr->attr_payload = _payload;	
	
}

void initialize_sbcp_msg(struct SBCP_message *msg_ptr, int _version, int _type, int _length, struct SBCP_attribute *_attrobj)
{
	
msg_ptr->vrsn = _version;
msg_ptr->type = _type;
msg_ptr->length = _length;
msg_ptr->attribute = _attrobj;
}

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
Function Name: main
Parameters   : argc, argv
Description  : 
************************************************************/
int main(int argc, char *argv[])
{
	
    char *server_IP;
	int servPort, socket_client;
  	struct sockaddr_in server_address;
	char str[INET6_ADDRSTRLEN]; 
	char username[16]; // gIVEN : uSERNAME SIZE = 1 TO 16 BYTES
	char message[MESSAGE_LEN_LIMIT];
	struct SBCP_message sbcp_msg;
	struct SBCP_attribute sbcp_attr1,sbcp_attr2;
	
	int packed_size; 
	char packed_buff[MAX_DATA_SIZE];
	
	fd_set read_set;
	struct timeval tv; // TIME VALUE FOR select(). Tells how long to check the sets
	
	struct SBCP_message sbcp_msg1;
	struct SBCP_attribute sbcp_attr11,sbcp_attr22;
	char data[530];
	char data1[20];
	uint16_t count;
	int cnt_var=0;
	
	/* zero out the master set and the read set  . REFERENCE : select() beej's guide */
	FD_ZERO(&read_set);
	
	
	if(argc !=4) 
	{
		printf("Incorrect number of arguments. Use: chatc Username <IP Address> <PortNumber>\n");
	        return 1;
	}	
	
		
	server_IP = argv[2];
	servPort = atoi(argv[3]);

	if((socket_client = socket(AF_INET/*AF_UNSPEC*/, SOCK_STREAM,0))<0)
	{
			printf("Client socket not created\n");
			if(errno)
		       printf("Exiting due to error : %s\n", strerror(errno));
			return 1;
	}

	memset(&server_address,0,sizeof(server_address));

	server_address.sin_family = AF_INET;//AF_UNSPEC; //ipv4 or ipv6
  	server_address.sin_addr.s_addr = inet_addr(server_IP);
	server_address.sin_port = htons(servPort);

	if(connect(socket_client, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		printf("Connection not established\n");
		if(errno)
	    		printf("Exiting due to error : %s\n", strerror(errno));
		return 1;
	}

	
	//printf("Connection Established\n");
	inet_ntop(AF_INET, &server_address.sin_addr, str, sizeof(str));
    printf("client:connecting to %s and my username is  %s\n",str,argv[1]);
	
	
	strcpy(username,argv[1]);  // Copy the arg[1] to username variable
	 
	/*SEND JOIN REQUEST
	data for JOIN :  
	initialize_sbcp_attr(struct SBCP_attribute *struct_ptr, int _type, int _length, char *_payload)
	
	type = 2, length = 16+2+2= 20 bytes (msg+type+length) , payload = username entered in console
	
	initialize_sbcp_msg(struct SBCP_message *msg_ptr, int _version, int _type, int _length, struct SBCP_attribute _attrobj)
	
	version =3, type =2 , length = 2+2+20 (vrsn, type + length + payload object)	
	
	*/
	
	initialize_sbcp_attr(&sbcp_attr1,2,20,username); 
	initialize_sbcp_msg(&sbcp_msg,'3','2',24,&sbcp_attr1); 
	
	/* 
	Packing and Unpacking majorly to get the entire structure data into a single
	entity and in a format that other programs and machines can understand. So we read and write buffers of data according to a template string which 
	contains the entire data in the format we wish to send. The format mentions the type of data we would like to pack.
	
	Parameters : Pack takes in a string of characters which describe the format in which data needs to be packed followed by the list of values that needs to 
	be packed. The packed_buff contains the packed string with size returning its size.
	
	Format : c : represents char with standard size of 1 byte
	         h : represents a short integer with standard size of 2 bytes
			 s : represnts string, char[]
			 
	Here, cchhhshhs : vrsn , type using c as these use around just 1 byte. hhh represents type of length, attr_type, attr_length.
	                  s represents username string, 3-version.   

	Ref for format characters : https://docs.python.org/2/library/struct.html   
	                            https://www.stat.berkeley.edu/~spector/extension/perl/notes/node87.html
	        
	*/
	packed_size = pack(packed_buff,"cchhhshhs", sbcp_msg.vrsn, sbcp_msg.type, sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,username,3,530," ");
	
	/*Send data of USERNAME*/
		
    if (send(socket_client, packed_buff, packed_size, 0) == -1)
	{
			printf("CLIENT : USERNAME : SEND ERROR \n");		
			return 1;
    }
	
	/*Next send operation to send message */
	initialize_sbcp_attr(&sbcp_attr1,4,520,message);
	initialize_sbcp_msg(&sbcp_msg,'3', '4', 24, &sbcp_attr1);
	initialize_sbcp_attr(&sbcp_attr2,2,16,username);
	
	int ret_val;
	while(1)
	{		
		FD_ZERO(&read_set);
		FD_SET(0,&read_set);	
		FD_SET(socket_client,&read_set);
		
		tv.tv_sec = 10; //timeout
		
		if((ret_val=select(socket_client+1,&read_set,NULL,NULL,&tv))==-1) 
		   {
				printf("CLIENT :: SELECT ERROR \n");
				return 1;
		   }
		   
		   
		if(ret_val==0) 
		{
		  //nothing is sec for 10 s elapsed time 
		  packed_size=pack(packed_buff,"cchhhshhs",'3','9',300,2,200,username,4,300,message);
			  if(send(socket_client,packed_buff,packed_size,0) ==-1) 
			  {
				 printf(" CLIENT :: SEND ERROR :MESSAGE");
			  }
		}
		
		
		if(FD_ISSET(0,&read_set)) 
		{	
			 if((fgets(message,512,stdin)==NULL)|| feof(stdin))  
			   {
				 printf("EOF Cntrl+D \n ");
		         printf("Disconnecting client \n");
		         close(socket_client);
		         return 1;
				}

			else
			   {
				strcpy(username,argv[1]);
				packed_size=pack(packed_buff,"cchhhshhs", sbcp_msg.vrsn, sbcp_msg.type,sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,username,sbcp_attr2.type,sbcp_attr2.length,message);
			 	if(send(socket_client,packed_buff,packed_size,0) ==-1) 
					{
					 printf(" CLIENT :: SEND ERROR :username");
					}

				}
		}


		/* There is data to read from the server */
		int numbytes;
        if (FD_ISSET(socket_client,&read_set))
		{
						
			if ((numbytes = recv(socket_client, packed_buff, MAX_DATA_SIZE-1, 0)) <= 0) 
			{
				printf(" CLIENT :: RECEIVE ERROR");
				return 1;
			}

			/* Unpack the received data*/
			unpack(packed_buff,"cchhhhshhs",&sbcp_msg1.vrsn,&sbcp_msg1.type,&sbcp_msg1.length,&sbcp_attr11.type,&sbcp_attr11.length,&count,data,&sbcp_attr22.type,&sbcp_attr22.length,data1);					

			if(sbcp_msg1.type=='5') 
			{
				printf("%s\n",data1);
				close(socket_client);
				return 1;
			}
						
			if(cnt_var==2) 
			{
				cnt_var=0;
			}
			  		
			if(sbcp_msg1.type=='7' && cnt_var!=1) 
			{
				cnt_var++;
				printf("%s:",data1);
				printf("%s\n",data);
				if(count==0)
					count=1;
				printf("The count is :%d\n",count);
					
			}

			if(sbcp_msg1.type=='8') 
			{
			    printf("ONLINE:%s",data1);
				printf("%s\n",data);
			}
				
			if(sbcp_msg1.type=='6') 
			{
			    printf("OFFLINE:%s",data1);
			    printf("%s\n",data);
			}
					
			if(sbcp_msg1.type=='9') 
			{
			    printf("IDLE MESSAGE FOR CLIENT:");
			    printf("%s has been idle for more than 10 seconds\n",data);
			}
			else if(sbcp_msg1.type=='3') 
			{
			    printf("Received MESSAGE from %s  : %s \n",data,data1);
			}
		
		}
		
			
		
	}
	
	
	return 0;
	
}




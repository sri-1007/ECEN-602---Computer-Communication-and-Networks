#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<netinet/in.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<map>



using namespace std;

char * http(char *url)
{
	int length = strlen(url);
	char *out = (char *)malloc((length+50)*sizeof(char));
	int i=0, j=4;
	
	out[0]='G';out[1]='E';out[2]='T';out[3]=' ';
	
	if(length>7)
	{
		if(url[0]=='h' && url[1]=='t' && url[2]=='t' && url[3]=='p' && url[4]==':' && url[5]=='/' && url[6]=='/')
			i = 7;
	}
	
	int temp = i;								
	
	while(i<length && url[temp]!='/')
		temp++;
	
	out[j++]='/';
	temp++;
	
	while(temp<length)
		out[j++]=url[temp++];
	
	out[j++]=' ';out[j++]='H';out[j++]='T';out[j++]='T';out[j++]='P';out[j++]='/';
	out[j++]='1';out[j++]='.';out[j++]='0';out[j++]='\r';out[j++]='\n';
	out[j++]='H';out[j++]='o';out[j++]='s';out[j++]='t';out[j++]=':';out[j++]=' ';
	
	while(i<length && url[i]!='/')
		out[j++] = url[i++];
	
	out[j++]='\r';out[j++]='\n';out[j++]='\r';out[j++]='\n';out[j]='\0';
	
return out;
}

void *get_in_addr(struct sockaddr *sa)				
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char *get_filename(char *file)
{
	char *file_name = (char *)malloc(512*sizeof(char));
	int j=0;
	int last = 0;
	int i=0;
	int length = strlen(file);
	
	while(i < length)
	{
		if(file[i]=='/')
			last = i;
		i++;
	}
	if(last!=0)
	{
		memcpy(file_name,&file[last+1],(length-last-1));
		file_name[length-last]='\0';
	}
	if(strlen(file_name)==0)
	{
		file_name[j++]='i';file_name[j++]='n';file_name[j++]='d';file_name[j++]='e';file_name[j++]='x';file_name[j++]='.';		
		file_name[j++]='h';file_name[j++]='t';file_name[j++]='m';file_name[j++]='l';file_name[j]='\0';
		return file_name;
	}
	return file_name;
}


int main(int argc, char *argv[])
{
	/*regular connection sequence*/
	int error;
	if(argc!=4)									
	{
		fprintf(stderr,"Enter server IP, port number, URL\n");
		return 1;
	}
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;				

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if( (error = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0){	
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		exit(1);
	}

	int sockfd;
	for(p=servinfo; p!=NULL; p=p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){	
			perror(" ERROR : client: socket");
			continue;
		}
		break;
	}
	
	if(p==NULL)
	{
		fprintf(stderr, "Connect failed\n");
		return 2;
	}

	int size = 2048, bytes_num;
	char *buffer = (char *)malloc((size+1)*sizeof(char));
	unsigned short int length;
	
	if( (error = connect(sockfd,p->ai_addr,p->ai_addrlen)) == -1)			
	{
		perror("ERROR :client: connect");
	}
	freeaddrinfo(servinfo);
	
	char *query = http(argv[3]);

    length = strlen(query);
    cout << "Request: " << argv[3] << endl;
	if( (error = send(sockfd,query,length,0)) == -1)			
	{
		perror("ERROR :client: send");
	}
	
	ofstream testFile;
	char *filename = get_filename(argv[3]);
	stringstream ss;
	ss << filename;

	testFile.open(ss.str().c_str(),ios::out | ios::binary);
	if(!testFile.is_open())
	{
		cout << "Unable to open file in filesystem" << endl;
		exit(0);
	}
	
	bool header = true;
	bool first=true;
	
	while(1)
	{
		bytes_num = recv(sockfd,buffer,2048,0);			
		if(first)
		{
			stringstream tempstream;
			tempstream << buffer;
			string line;
			getline(tempstream,line);
			if(line.find("404")!=string::npos)
				cout << " Unable to  find the page " << line << endl;
			first = false;
		}
		int i=0;
		if(header)
		{
			if(bytes_num>=4)
			{
				for(i=3; i<bytes_num; i++)
				{
					if(buffer[i]=='\n' && buffer[i-1]=='\r')
					{
						if(buffer[i-2]=='\n' && buffer[i-3]=='\r')
						{
							header=false;				
							i++;
							break;
						}
					}
				}	
			}
		}
		
		if(!header)
			testFile.write(buffer+i,bytes_num-i);			 
		
		if(bytes_num == -1)
		{
			perror("client: recv");
		}
		else if(bytes_num == 0)
		{						
			close(sockfd);
			cout << "File name:	" << filename << endl;
			testFile.flush();
			testFile.close();
			break;							
		}
		testFile.flush();				
	}
	testFile.close();					
	return 0;
}
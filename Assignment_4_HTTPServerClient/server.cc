#include<sys/types.h>
#include<sys/socket.h>

#include<arpa/inet.h>
#include<netinet/in.h>

#include<netdb.h>
#include<unistd.h>
#include<fcntl.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sstream>
#include<fstream>
#include<algorithm>
#include<iostream>
#include<string>
#include<map>


using namespace std;
int max_cache_capacity;
unsigned short int client_num;

/*For a map of information from the client. Its requested Filename and the hostname*/
struct http{
	char file_name[1024];
	char host_name[1024];
};

struct cache_block{
	string expire_date;
	string host_file;
	int lastused;
	int current;
	int expire;
};
struct cache_block cache[10];

/*Structure to represent the request parameters*/
struct condition{
	int expires;
	int length;
	int cb_index;
	int index;
	
	char filename[200];
	bool boolean;
	bool conditional;
};

/*Function to initialize the Cache*/
void cache_setup(){
	for(int j=0; j<10; j++){
		cache[j].lastused=j+1;
		cache[j].current=0;
	}
}


map<int,struct condition> req_parameters; //creating a map of condition type
map<int,int> client_request;
map<int,int> get_request;
map<int,int> type;
map<string,int> block_number;
map<int,string> URL;
map<int,bool> Random_number;


/*Updates the Cache objects*/
void update(int block)
{
	if(cache[block].lastused==1)
		return;
	cache[block].lastused = 1;
	for(int i=0; i<10; i++)
	{
		if(i!=block)
		{
			cache[i].lastused++;
			if(cache[i].lastused>10)
				cache[i].lastused = 10;
		}
	}
	return;
}

/*Updating the LRU Cache*/
int update_kick()
{
	int end = 10;
	int ans = -1;
	while(end>0)
	{
		for(int i=0; i<10; i++)
		{
			if(cache[i].lastused==end && cache[i].current==0)
			{
				
				ans = i;
				cache[ans].current=-2;
				end=0;
				break;
			}
		}
		end--;
	}
	return ans;
}



// Below we shall check for the stale cachee
bool calc_expire_bool(int cacheBlock_index)
{
	time_t raw_time;
	time(&raw_time);
	struct tm * utc;
	utc = gmtime(&raw_time);
	raw_time = mktime(utc);
	if(cache[cacheBlock_index].expire-raw_time>0)
		return false;
	return true;
}

// Check for the exisiting request in the cache
int checkCache(string req)
{
	if(block_number.find(req)==block_number.end()){
		return -1;
	}
	int cacheBlock_index = block_number[req];
	if(cache[cacheBlock_index].host_file == req){
		return cacheBlock_index;
	}
	return -1;
}


int getRandomNumber()
{
	srand(time(NULL));
	return rand();
}

// Decoding the host name and the file name by parsing through the HTTP request 
struct http * parse_http_request(char *buf, int bytes_num)
{
	struct http* out = (struct http *)malloc(sizeof(struct http));
	int temp;
	
	for(temp=0; temp<bytes_num; temp++)
	{
		if(buf[temp]==' ')
		{
			temp++;
			break;
		}
	}
	int j=0;
	out->file_name[0]='/';
	
	if(buf[temp+8]=='\r' && buf[temp+9]=='\n')
	{
		out->file_name[1]='\0';
	}
	else
	{
		while(buf[temp]!=' ')
			out->file_name[j++]=buf[temp++];
		out->file_name[j]='\0';
	}
	temp+=1;
	while(buf[temp++]!=' ');
	
	j=0;
	
	while(buf[temp]!='\r')
		out->host_name[j++]=buf[temp++];
	
	out->host_name[j]='\0';
	
return out;
}

int main(int argc, char *argv[])
{
	
	max_cache_capacity = 10;
	client_num = 0;
	int sock_fd, error;
	int i, bytes_num;
	char buffer[2048],TB[256],tmp[512];

	struct addrinfo hints, *server_info, *p;
	
	
	if(argc!=3)
	{
		fprintf(stderr,"Invalid number of arguments : server ip port\n");
		return 1;
	}
	
	
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	// Accessing the portno and IP
	if((error = getaddrinfo(argv[1], argv[2], &hints, &server_info)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		exit(1);
	}
	
	for(p=server_info; p!=NULL; p=p->ai_next)
	{
		
		if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){
			perror("server: socket error");
			continue;
		}
		
		if((error = bind(sock_fd, p->ai_addr, p->ai_addrlen))== -1){
			perror("bind error\n");
			continue;
		}
		break;
	}
	// REPRESENTS BIND failure
	if(p==NULL){
		fprintf(stderr, "Bind failed\n");
		return 2;
	}
													
	if( (error = listen(sock_fd, max_cache_capacity)) == -1){
		perror("server: listen failed");
	}
	
	
	freeaddrinfo(server_info);
	
	fd_set master, read_fds, write_fds, master_write;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&master_write);
	FD_SET(sock_fd,&master);
	
	int fdmax = sock_fd, c_sockfd;
	struct sockaddr_storage client_addr;
	socklen_t addr_length;
	
	/*Function to initialize the Cache*/
	cache_setup();
	
	
	struct tm tm,*utc;
	time_t rawtime;
	
	printf("Proxy is up and running.....! \n");
	
	
	while(1)
	{
	   read_fds =  master;
       write_fds = master_write;
	   
	   /*Performing select for multiclient operations*/
		if( (error = select(fdmax+1, &read_fds, &write_fds, NULL, NULL)==-1)){
			perror("Server: Select");
			exit(4);
		}	   
		
		
		
		for(i=0; i<=fdmax; i++)
		{
		   if(FD_ISSET(i,&write_fds))
		    {			   
				if(FD_ISSET(i,&master)==false)		       
				{
				   FD_CLR(i,&master);
					FD_CLR(i,&master_write);
					close(i);
					
					
					if(req_parameters[i].cb_index==-1)
					{
						if(req_parameters[i].boolean==true)
								remove(req_parameters[i].filename);	
					}
							
					else if(req_parameters[i].cb_index != -1)
					{
						update(req_parameters[i].cb_index);
						cache[req_parameters[i].cb_index].current-=1;
					}
					continue;
			    }
				
				/*Opening the file and putting the contents*/
				ifstream first_file;
				first_file.open(req_parameters[i].filename,ios::in | ios::binary);
				
				if(!first_file.is_open())
				{
					cout << req_parameters[i].filename << endl;
					FD_CLR(i,&master_write);
					cout << "Unable to Open Cache File" << endl;
				}
				else
				{
					first_file.seekg((req_parameters[i].index)*2048,ios::beg);		
					req_parameters[i].index+=1;
					first_file.read(buffer,2048);
                    bytes_num = first_file.gcount();
                    if(bytes_num!=0)
					{
						if( (error = send(i,buffer,bytes_num,0)) == -1)
						{			
								perror("Client send error");
						}
					}
					
					// Disconnecting once end of file is reached
					if(bytes_num<2048)
					{						
						FD_CLR(i,&master_write);
						FD_CLR(i,&master);			
						close(i);
						
						cout << "------------------------------------------------ " << endl;
						
						if(req_parameters[i].cb_index==-1)
						{
							if(req_parameters[i].boolean==true)		
								remove(req_parameters[i].filename);	
						}
						else if(req_parameters[i].cb_index != -1)
						{
							update(req_parameters[i].cb_index);	
							cache[req_parameters[i].cb_index].current-=1;		
						}
					}	
					
				}
				
				first_file.close();
			}
            else if(FD_ISSET(i,&read_fds))
			{
				/*Handling the new clients getting connected to the server*/
				
				if(i==sock_fd)
				{
					
					addr_length = sizeof(client_addr);		
					
					if( (c_sockfd = accept(sock_fd, (struct sockaddr *)&client_addr,&addr_length)) == -1)
					{
						perror("Accept error");
					}
					else
					{
						FD_SET(c_sockfd,&master);
						type[c_sockfd]=0;
						client_num++;
						if(c_sockfd > fdmax)
							fdmax = c_sockfd;
						cout << "New connection established at Socket: " << c_sockfd << endl;
					}
				}
				else
				{
				   	if((bytes_num = recv(i,buffer,sizeof(buffer),0)) <=0)
					{
							if(bytes_num == 0)
							{				
						    }
						    else{
							perror("Recevie error");
						    }	
						    
							close(i);
						    FD_CLR(i,&master);
							
							if(type[i]==1)
							{
									
									FD_SET(client_request[i],&master_write);
									int j = client_request[i];
									int a = req_parameters[j].cb_index;
									if(a==-1){
									}
									else
									{// now we shall serve from the cache block after copying tmp file to cache block
									
										ifstream third_file;
										third_file.open(req_parameters[j].filename,ios::in | ios::binary);
										
										string line;
										bool boo=false;
										
										if(third_file.is_open())
										{
											getline(third_file,line);
											// now we shall check if the cache is modified or not
											
											if(line.find("304")!=string::npos)		
												boo = true;
											bool flag = false;
											
											while(!third_file.eof())
											{
												getline(third_file,line);
												
												if(line.compare("\r") == 0)
												{
													break;
												}
												
												string line_cp = line;
												transform(line.begin(), line.end(), line.begin(), ::tolower);
												
												//Checking and getting the expiry field
												if(line.find("expires:")==0)
												{
													flag = true;
													int pos = 8;
													
													if(line[pos]==' ')
														pos++;
													
													stringstream s2;
													s2 << line.substr(pos);
													
													getline(s2,line);
													line = line_cp.substr(pos);
													cache[a].expire_date = line;
													memset(&tm, 0, sizeof(struct tm));
													
													strptime(line.c_str(), "%a, %d %b %Y %H:%M:%S ", &tm);
													cache[a].expire = mktime(&tm);
													break;
												}
											}
											third_file.close();
											
											
											if(!flag)
											{
												
												time(&rawtime);
												utc = gmtime(&rawtime);
												
												strftime(TB, sizeof(TB), "%a, %d %b %Y %H:%M:%S ",utc);	
												cache[a].expire = mktime(utc);
												cache[a].expire_date = string(TB) + string("GMT");
											}
										}
										
										time(&rawtime);
										utc = gmtime(&rawtime);
										strftime(TB, sizeof(TB), "%a, %d %b %Y %H:%M:%S ",utc);
										cout << "Access Time: " << TB << "GMT" << endl;
										cout << "File expiry time is: " << cache[a].expire_date << endl;

										if(boo==true)
										{
											//Request served from the cache block
											printf("Serving from the cache block \n");
											
											if(req_parameters[j].conditional)
											{
												remove(req_parameters[j].filename);
												stringstream s4;
												s4 << a;
												strcpy(tmp,s4.str().c_str());
												strcpy(req_parameters[j].filename,tmp);									
												req_parameters[j].boolean = false;
											}
										}

										if(boo==false)
										{
											
											if(cache[a].current!=1)
											{
												if(cache[a].current>1)
												{
													cache[a].current-=1;
													int new_cb;
													new_cb = update_kick();
													
													if(new_cb==-1)
													{
														req_parameters[j].boolean = true;
														req_parameters[j].cb_index = new_cb;
														a = new_cb;
													}
													else
													{
														req_parameters[j].cb_index = new_cb;
														cache[new_cb].expire = cache[a].expire;
														cache[new_cb].expire_date = cache[a].expire_date;
														a = new_cb;
													}
												}
											}
											else
											{
												cache[a].current = -2;
											}
										}
										/*Updating the cache block and removing temp*/
										if(a!=-1 && cache[a].current==-2)
										{
											stringstream s3;
											s3 << a;
											strcpy(tmp,s3.str().c_str());
											
											ofstream mf;
											mf.open(tmp,ios::out | ios::binary);
											mf.close();
											remove(tmp);
											
											rename(req_parameters[j].filename,tmp);
											strcpy(req_parameters[j].filename,tmp);
											
											cache[a].host_file = string(URL[client_request[i]]);
											block_number[cache[a].host_file]=a;
											req_parameters[j].boolean = false;
											cache[a].current = 1;
										}
									}
						    
							}
							else
							{
								cout << "Client number " << i << " is disconnected" << endl;		
						    }
								
					}
					
					else if(type[i]==1)
					{
						ofstream tmp_file;
						tmp_file.open(req_parameters[client_request[i]].filename,ios::out | ios::binary | ios::app);
						
						if(!tmp_file.is_open())
						{ 
							cout << "Error opening the file!!!!!" << endl;
						}
						else
						{
							tmp_file.write(buffer,bytes_num);
							tmp_file.close();
						}
					}
					
					else if(type[i]==0)
					{
						struct http* tmp;
						tmp = parse_http_request(buffer,bytes_num);
						
						URL[i] = string(tmp->host_name)+string(tmp->file_name);
						cout <<endl<< "Client "<< i << ": requested " << URL[i] << endl;
						
						int cacheBlock_index = checkCache(URL[i]);
						bool expired = false;
						req_parameters[i].conditional = false;
						
						if(cacheBlock_index!=-1)
						{
							if(cache[cacheBlock_index].current>=0)
							{
								
								expired = calc_expire_bool(cacheBlock_index);
								
								if(expired)
								{
									req_parameters[i].conditional = true;
								}
								
								else
								{
									req_parameters[i].cb_index = cacheBlock_index;		
									req_parameters[i].index = 0;
									req_parameters[i].boolean = false;
									
									cache[cacheBlock_index].current += 1;
									
									stringstream ss;
									ss << cacheBlock_index;
									
									strcpy(req_parameters[i].filename,ss.str().c_str());
									FD_SET(i,&master_write);
									
									cout << "Cache Block Hit at block number :  "<< cacheBlock_index << endl;
									continue;
								}
							}
						}
						if(cacheBlock_index==-1 || expired)
						{
							string tmp_str = "GET "+string(tmp->file_name)+" HTTP/1.0\r\nHost: "+string(tmp->host_name)+"\r\n\r\n";
							bytes_num = tmp_str.length();
							strcpy(buffer,tmp_str.c_str());
							
							if(!expired)
							{
								cacheBlock_index = update_kick();
								
								cout << "Client "<< i << ": Cache Missed/ Cache Entry is not present. Cache Block Allocated " << cacheBlock_index << endl;
							}
							else
							{
								cache[cacheBlock_index].current+=1;
								buffer[bytes_num-2]='\0';
								string req = string(buffer);
								
								// now if the request if stale we shall send a GET request which shall conatain If-modified-since in header 

								string new_req = req+"If-Modified-Since: "+cache[cacheBlock_index].expire_date +"\r\n\r\n\0";
								bytes_num=new_req.length();
								strcpy(buffer,new_req.c_str());
								cout << "Client " << i << ": Already present in Cache , Cache Hit at Block "<< cacheBlock_index << endl;
								cout << "Client " << i << ": If-Modified-Since: "<< cache[cacheBlock_index].expire_date << endl;
								//cout<<"Status 304 :Served from proxy because of IF-modified"<<endl;
							}
							req_parameters[i].cb_index = cacheBlock_index;
							
							
							int new_socket;
							memset(&hints,0,sizeof(hints));		
							hints.ai_family = AF_INET;
							hints.ai_socktype = SOCK_STREAM;
							// fetching the address
							if((error = getaddrinfo(tmp->host_name, "80", &hints, &server_info)) != 0)
							{	
								fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
								exit(1);
							}
							for(p=server_info; p!=NULL; p=p->ai_next)
							{
								if((new_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1)
								{	// we shall create socket for the server
									perror("socket fail");
									continue;
								}
								break;
							}
							if(p==NULL)
							{
								fprintf(stderr, "bind error\n");
								return 2;
							}
							if( (error = connect(new_socket,p->ai_addr,p->ai_addrlen)) == -1)
							{
								perror("connect error\n");
							}	
							freeaddrinfo(server_info);
							
							type[new_socket]=1;
							get_request[i] = new_socket;
							client_request[new_socket] = i;
							
							req_parameters[i].boolean = true;				
							req_parameters[i].index = 0;
							stringstream ss;
                            
							// we shall generate random numbers corresppnding to the tmp file names
							
							int rr = getRandomNumber();		
							while(Random_number.find(rr) != Random_number.end() )
							{
								
								rr = (rr+1)%1000000007;		
							}
							
							Random_number[rr]=true;
							ss << "tmp_"<< rr;
							strcpy(req_parameters[i].filename,ss.str().c_str());
							
							ofstream touch;
							touch.open(req_parameters[i].filename,ios::out | ios::binary);
							
							if(touch.is_open())
								touch.close();

							FD_SET(new_socket,&master);
							if(new_socket > fdmax)
								fdmax = new_socket;
							cout<<endl;
							//cout << "Fetcher started at socket " << new_socket << ": for client " << i << endl;
							if( (error = send(new_socket,buffer,bytes_num,0)) == -1)
							{
								
								perror("send error");
							}
						}
						free(tmp);
					}
					
				}
				
				
				
			}


			
		}
		
		
	}
		
return 0;	
}
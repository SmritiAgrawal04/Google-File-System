#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include<iostream>
#include <unistd.h> 
#include<vector>
#include <string.h> 
#include<cstring>
#include <stdlib.h>
#include<string>
#include <bits/stdc++.h> 
#include<fstream>
#include<unistd.h>
#include <netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<vector>
#include<algorithm>
#include <openssl/sha.h>
#include<cstring>
#include <chrono>
using namespace std; 

int listen_port;
char *listen_ip;

struct threads_info
{
	int client_fd;
	int cport;
	char *cip;
	// struct sockaddr ad;
// 	ad.sin_port;
// 	ad.sin_addr.s_addr;
// };
};
void * start_serv(void *);
void * start_client(void *);
void * write_fn(void *);
int create_socket()
{
	return socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
}

int main(int argc, char  *argv[])
{
	
	char **args_token=(char **)malloc(1024*sizeof(char *));
	 int i=0;
   char* str=strtok(argv[1],":");
  	while(str!=NULL)
  	{
   		args_token[i]=str;
   	//cout<<spaced_token[i]<<"hhh"<<endl;
   		i++;
   		str=strtok(NULL,":");
  	}
  	args_token[i]=NULL;

//  	cout<<args_token[0]<<args_token[1]<<endl;
  	listen_ip=new char[strlen(args_token[0])];
  	listen_ip=args_token[0];
	listen_port=atoi(args_token[1]);

	struct sockaddr_in add;
	add.sin_family = AF_INET;
	add.sin_port = htons(8087);


	
	pthread_t serv_thread,client_thread;
	
	//pthread_create(&client_thread,NULL,start_client,NULL);
	pthread_create(&serv_thread,NULL,start_serv,NULL);
	//pthread_join(client_thread,NULL);
	// cout<<"bye"<<endl;
	// int d;
	// cin>>d;
	pthread_join(serv_thread,NULL);
}



void * start_serv(void *cs)
{   
	cout<<"Inside t_p2S"<<endl; 
	int server_fd,status,queue_size=5;
	struct sockaddr_in addr,c_addr;

	cout<<"INside t_p2s_2";
	int addrl=sizeof(addr);
	int c_addrl=sizeof(c_addr);
	server_fd=socket(AF_INET,SOCK_STREAM,0);
	if(server_fd==0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
		addr.sin_family=AF_INET;
		addr.sin_port=htons(listen_port);
		addr.sin_addr.s_addr=inet_addr(listen_ip);
		cout<<"going to bind\n";

		status=bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
		if(status<0)
		{
			perror("Failed to bind");
			exit(EXIT_FAILURE);
		}
		cout<<"Bind successful"<<endl;
	
		int lstatus=listen(server_fd,queue_size);
		if(lstatus<0)
		{
			perror("Failed to listen");
			exit(EXIT_FAILURE);
		}
		cout<<"lsattus"<<lstatus<<endl;
		cout<<"Listen successful"<<endl;
		
//Commented while (1) here t check if multiple clients are able to connect
		int to_client_fd;
		cout<<"came vack to while 1 serv thread"<<endl;
		char buffer[512];
		while(1)
		{	
		
		to_client_fd=accept(server_fd,(struct sockaddr*)&c_addr,(socklen_t*)&c_addrl);
		if(to_client_fd<0)
		{
			perror("Accept failed");
			exit(EXIT_FAILURE);
		}
		cout<<"connection accepted"<<endl;
		  int recv_bytes=recv(to_client_fd , buffer , 512 , 0);

		  char *recvd_val=buffer;
			cout<<"The received value is "<<recvd_val<<endl;
			
			if(strcmp(recvd_val,"read")==0)
			{	
				//read_from_file();
				cout<<"read"<<endl;
			}
			else if(strcmp(recvd_val,"write")==0)
			{
				//write_to_file();
				pthread_t th;
				//write_thread
				threads_info *tinf;
				cout<<"creating thread"<<endl;
				tinf=(threads_info*)malloc(sizeof(struct threads_info));
		
				char *str1=inet_ntoa(c_addr.sin_addr);
		
				int port_v=ntohs(c_addr.sin_port);
		
				tinf->client_fd=to_client_fd;
				tinf->cport=port_v;
				tinf->cip=str1;
		
				pthread_create(&th,NULL,write_fn,(void*)tinf);
			}
			else if(strcmp(recvd_val,"heartbeat")==0)
			{
					//int readbytes=recv(to_client_fd,buffer,512,0);
					cout<<"recvd heartbeat"<<recv_bytes<<endl;
						//cout<<"rcvd "<<buffer<<endl;
					//cout<<val+1<<")"<<buffer<<endl;
					memset(buffer,'\0',512);
					//Sleep (1000); //Sleeps for 1000ec
					
					int sentbytes=send(to_client_fd,"ok",sizeof("ok"),0);
					cout<<"sent heartbeat"<<sentbytes<<endl;
					//cout<<"hbno "<<++hbno<<endl;

			}

		 }
		/*tinf=(threads_info*)malloc(sizeof(struct threads_info));
		
		char *str1=inet_ntoa(c_addr.sin_addr);
		
		int port_v=ntohs(c_addr.sin_port);
	
		tinf->client_fd=to_client_fd;
		tinf->cport=port_v;






		tinf->cip=str1;
	
		pthread_create(&th,NULL,start_fn,(void*)tinf);//removed &
	



	}*/
	
}

void * start_client(void *y)
{  
	int client_fd,status;
	struct sockaddr_in addr;
	cout<<"inside start client"<<endl;

	client_fd=socket(AF_INET,SOCK_STREAM,0);
	cout<<"client fd "<<client_fd<<endl;
	if(client_fd==0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(8087);
	addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	
	
	status=connect(client_fd,(struct sockaddr*)&addr,sizeof(addr));
	if(status<0)
	{
		perror("connection failed");
		exit(EXIT_FAILURE);
	}	
	cout<<"connection successful"<<endl;
	string data;
	char buffer[512];
	int hbno=0;
	while(1)
	{	cout<<"inside while 1"<<endl;
		

			int readbytes=recv(client_fd,buffer,512,0);
			cout<<"recvd heartbeat"<<readbytes<<endl;
				//cout<<"rcvd "<<buffer<<endl;
			//cout<<val+1<<")"<<buffer<<endl;
			memset(buffer,'\0',512);
			//Sleep (1000); //Sleeps for 1000ec
			usleep(1000000);
			int sentbytes=send(client_fd,"ok",sizeof("ok"),0);
			cout<<"sent heartbeat"<<sentbytes<<endl;
			cout<<"hbno "<<++hbno<<endl;



	}
//	cout<<"came out of while"<<endl;

}

void *write_fn(void *cs)
{

	threads_info *sti=(threads_info*)cs;
	int to_client_fd= sti-> client_fd;
	int port_no=sti->cport;
	char *ip=sti->cip;

	int sentbytes=send(to_client_fd,"ok",sizeof("ok"),0);
	char buffer[512];
	int recv_bytes=recv(to_client_fd , buffer , 512 , 0);//file name received

	string filename=buffer;
	cout<<"filename "<<filename<<endl;
	memset(buffer,'\0',512);
	sentbytes=send(to_client_fd,"ack",sizeof("ack"),0);
	int filesize=0;
	recv_bytes=recv(to_client_fd , &filesize , sizeof(filesize) , 0);//file size received
	cout<<"recd filesize"<<recv_bytes<<endl;
	sentbytes=send(to_client_fd , "ok" , sizeof("ok") , 0);

	char data_buffer[1024*32];

	FILE *fp;
		string filepath=filename;
		string filep="";
		
		char filepth[filepath.size()+1];
		strcpy(filepth,filepath.c_str());
		fp=fopen(filepth,"w+");
		if(!fp)
		{
			perror("Cant create file\n");
		}
		int x;
		int n;
		int count=0;
		int bytes_done=0;
		cout<<"sizeof data buffer"<<sizeof(data_buffer)<<endl;
		while(bytes_done <filesize&&(n=recv(to_client_fd,data_buffer,sizeof(data_buffer),0))>0)
	    {	cout<<"n "<<n<<endl;
			 count++;
        	cout<<count<<endl;
	        fwrite(data_buffer,sizeof(char),n,fp);
	        memset(data_buffer,'\0',sizeof(data_buffer));
	       //file_size=file_size-n;
	        x=send(to_client_fd,"ok",sizeof("ok"),0);
	       
	        bytes_done=bytes_done+n;
	        cout<<"bytes_done"<<bytes_done<<"filesize"<<filesize<<endl;
    	}
    	cout<<"n now is "<<n<<endl;
    	fclose(fp);
    	cout<<"done with recv"<<endl;


}

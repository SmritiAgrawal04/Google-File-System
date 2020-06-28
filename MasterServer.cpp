#include <stdio.h>
#include<iostream>
#include<bits/stdc++.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include<cstring>
#include<bits/stdc++.h>
#include<unordered_map>
#include<openssl/sha.h>
#include<unistd.h>
#include <deque>
#include"register.h"
using namespace std;

string path="/home/meenu/gitrepos/";
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

struct file_info
{
	string filename;
	int filesize;
	string hash;
};

struct to_chunk_server_details
{
	int lport;
	int seek_pos;
	string filename;
	int filesize;
};

unordered_map<string,file_info> file_map;

deque<int>chunk_serv;//deque to maintain RR way of allocation
unordered_map<int,int>chunk_listen_ip;//map containing chunkserver <index,Listen ip>
unordered_map<string,int> modified;//hash and if the file is modified or not
unordered_map<string,vector<pair<int,int>>>chunk_allocation;//string is hash <int,int> is the name of the chunk servers where we need to store the data
//the vector's index gives the chunk number. Eg:0th index corresponds to 0th chunk and so on.....
unordered_map<int,pair<string,int>>chunk_files;//

int alive[7]={1};
unordered_map<int,int> alive_map;//client_port(or chunk_server_accept_fd) , index

void *to_chunk_server(void *ch);

void *heartbeat_fn(void *cs)

{		int hbno=0;
		
		 threads_info *sti=(threads_info*)cs;
		
		cout<<"entering start fn"<<endl<<endl;	
		 int from_client=(sti->client_fd);

		 cout<<"client fd"<<from_client<<" "<<sti->client_fd<<endl;
		
		 char buffer[512];
		 int readbytes=0;

		
		while(1)
		{	

			cout<<"while 1 inside thread "<<endl;
			
			int sentbytes=send(sti->client_fd,"ok",sizeof("ok"),0);
			cout<<"sent heartbeat"<<sentbytes<<endl;

			readbytes=recv(from_client,buffer,512,0);
			cout<<"recvd hrtbt "<<readbytes<<endl;
			//DOUBT HOW TO WAIT??? as recv is a blocking call

			cout<<"hbno "<<++hbno<<endl;
			memset(buffer,'\0',512);

		}
}

void *client_fn(void *cs)

{	

		threads_info *p=(threads_info*)cs;
		
		cout<<"entering start fn"<<endl<<endl;	
		// int from_client=(sti->client_fd);
		
		 char buffer[512];
		 int readbytes=0;

		int from_client=p->client_fd;
		int port_no=p->cport;
		string my_ip=p->cip;
		
		int sentbytes=send(from_client,"hi",sizeof("hi"),0);
		//char buffer[512];
		readbytes=recv(from_client,buffer,512,0);
		
		string filename=buffer;
		cout<<"filename "<<filename<<endl;

		memset(buffer,'\0',512);
		sentbytes=send(from_client,"ack",sizeof("ack"),0);
		int filesize=99;
		
		readbytes=recv(from_client,&filesize,sizeof(filesize),0);
		sentbytes=send(from_client,"ack",sizeof("ack"),0);	
		cout<<"filesize "<<filesize<<endl;
		int temp;

		int n;
		int bytes_done=0;
		//int chunk=512*1024;
		int data_buffer[32*1024];
		FILE *fp;
		string filepath="/home/meenu/gitrepos/"+filename;
		char filepth[filepath.size()+1];
		strcpy(filepth,filepath.c_str());
		fp=fopen(filepth,"w+");
		if(!fp)
		{
			perror("Cant create file\n");
		}
		int x;
		int count=0;
		cout<<"sizeof data buffer"<<sizeof(data_buffer)<<endl;
		while(bytes_done <filesize&&(n=recv(from_client,data_buffer,sizeof(data_buffer),0))>0)
	    {	cout<<"n "<<n<<endl;
			 count++;
        	cout<<count<<endl;
	        fwrite(data_buffer,sizeof(char),n,fp);
	        memset(data_buffer,'\0',sizeof(data_buffer));
	       //file_size=file_size-n;
	        x=send(from_client,"ok",sizeof("ok"),0);
	       
	        bytes_done=bytes_done+n;
	        cout<<"bytes_done"<<bytes_done<<"filesize"<<filesize<<endl;
    	}
    	cout<<"n now is "<<n<<endl;
    	fclose(fp);
    	cout<<"done with recv"<<endl;
    	fp=fopen(filepth,"rb+");
    	if(!fp)
    	{
    		cout<<"unable to open file to calculate sha"<<endl;
    	}

    	int filesz=filesize;
				//calculate sha1

    	char buffer_sha[512*1024];
				int sent_bytes;
				string  final;
				string final_sha;
				unsigned char digest[SHA_DIGEST_LENGTH];
				int no_of_times=0;

		cout<<"going to calculate sha1"<<endl;
		//string final_sha="";
		while((sent_bytes=fread(buffer_sha,sizeof(char),512*1024,fp))>0&&filesz>0)
		{ 	no_of_times++;
					//cout<<"senntbytes "<<sent_bytes<<endl;
			    	SHA1((unsigned char*)&buffer_sha, strlen(buffer_sha), (unsigned char*)&digest);    
			 	
				//cout<<digest;
				    char mdString[SHA_DIGEST_LENGTH*2+1];
			 	
			    	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
			        	 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
			 
			    	//cout<<mdString;
			    	 final=mdString;
			 		 final_sha=final_sha+final.substr(0,20);
					
					//send(client_fd,buffer,sizeof(buffer),0);
					memset(buffer_sha,'\0',512*1024);
					filesz=filesz-sent_bytes;
		}
			modified[final_sha]=0;	
		cout<<final_sha<<endl;

		file_info f;
		f.filename=filename;
		f.filesize=filesize;
		f.hash=final_sha; //this stores the entire sha of the file

		file_map[filename]=f; //add the file details to the filemap indexing the filename

		int failed=-1;
		int failflag=0;
		for(int i=1;i<7;i++)
		{

			if(alive[i]==0)
			{
				failed=alive_map[i];
				failflag=1;
				break;

				//Need to handle the failure case here !!!
			}
		}
			if(failflag==0)
			{
				cout<<"No failure"<<endl;

				//take out two chunkservers at a time from the deque
				//then allocate the same chunk to these two popped chunkservers(maintaing the repliaction factor as 2)
				//continue the same until no of chunks are exhausted(RR)
				//32 MB is the chunksize per chunkserver
				//now keeping it as 64KB to check....
				int no_of_pairs=filesize/(64*1024);

				int mod=0;
				mod=filesize%(64*1024);

				if(mod>0)
					no_of_pairs++;
				cout<<"noof pairs"<<no_of_pairs<<endl;

				int pos=0;
					int fsize=filesize;
					int to_send_size=0;
				while(no_of_pairs--)
				{
					int first_chunk_server=chunk_serv.front();
					chunk_serv.pop_front();
					chunk_serv.push_back(first_chunk_server);
					int second_chunk_server=chunk_serv.front();
					chunk_serv.pop_front();
					chunk_serv.push_back(second_chunk_server);

					// get the listen ips of the corresponding chink servers
					int first_ip=chunk_listen_ip[first_chunk_server];
					int second_ip=chunk_listen_ip[second_chunk_server];
					cout<<"first ip"<<first_ip<<endl;
					cout<<"second ip"<<second_ip<<endl;
					to_chunk_server_details *f,*s;
					

					pthread_t th1,th2;
					cout<<"creating thread"<<endl;
					f=(to_chunk_server_details*)malloc(sizeof(struct to_chunk_server_details));
					s=(to_chunk_server_details*)malloc(sizeof(struct to_chunk_server_details));
					
					f->lport=first_ip;
					s->lport=second_ip;
					f->seek_pos=pos;
					s->seek_pos=pos;
					f->filename=filename;
					s->filename=filename;
					cout<<"filename  "<<filename<<endl;
					fsize=fsize-pos;
					pos=pos+64*1024;
					
					if(fsize>=64*1024)
					{
						to_send_size=64*1024;
					}
					else
					{
						to_send_size=fsize;
					}
					f->filesize=to_send_size;
					s->filesize=to_send_size;
					pthread_create(&th1,NULL,to_chunk_server,(void*)f);//removed &
					pthread_create(&th2,NULL,to_chunk_server,(void*)s);

					pthread_join(th1,NULL);
					pthread_join(th2,NULL);





				}




			}	
		


}


int main()
{
	
	int tracker_fd,to_client_fd,choice,status,queue_size=10;
	for(int i=0;i<6;i++)
	{	
		
		chunk_serv.push_back(i+1); //chunk number deque
	}
	//map mai chunk serv k saath listen ip daalo;
	for(int i=1;i<7;i++)
		alive[i]=1;
	chunk_listen_ip[1]=8088;
	chunk_listen_ip[2]=8082;
	chunk_listen_ip[3]=8083;
	chunk_listen_ip[4]=8084;
	chunk_listen_ip[5]=8085;
	chunk_listen_ip[6]=8086;
	
	struct sockaddr_in addr,c_addr;

	int addrl=sizeof(addr);
	int c_addrl=sizeof(c_addr);

	tracker_fd=socket(AF_INET,SOCK_STREAM,0);
	if(tracker_fd==0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(8087);
	addr.sin_addr.s_addr=inet_addr("127.0.0.1");


	status=bind(tracker_fd,(struct sockaddr*)&addr,sizeof(addr));
	if(status<0)
	{
		perror("Failed to bind");
		exit(EXIT_FAILURE);
	}
	

	status=listen(tracker_fd,queue_size);
	if(status<0)
	{
		perror("Failed to listen");
		exit(EXIT_FAILURE);
	}

	int cl=0;//this gives the total number of chunk servers that can connect
	threads_info *tinf;
	int  temp_count=0;
	while(cl<6)
	{
		

		to_client_fd=accept(tracker_fd,(struct sockaddr*)&c_addr,(socklen_t*)&c_addrl);
		if(to_client_fd<0)
		{
			perror("Accept failed");
			exit(EXIT_FAILURE);
		}
		cout<<"tempcoutn"<<++temp_count<<endl;
		pthread_t th;
		cout<<"creating thread"<<endl;
		alive_map[cl+1]=to_client_fd;
		tinf=(threads_info*)malloc(sizeof(struct threads_info));
		
		char *str1=inet_ntoa(c_addr.sin_addr);
		
		int port_v=ntohs(c_addr.sin_port);
		
		tinf->client_fd=to_client_fd;
		tinf->cport=port_v;

		cout<<port_v<<endl;
		cout<<to_client_fd<<endl;




		tinf->cip=str1;
		
		pthread_create(&th,NULL,heartbeat_fn,(void*)tinf);//removed &
	

		cl++;

	}
	//For accepting incoming client requests.Run in infinte loop.
	while(1)
	{

		to_client_fd=accept(tracker_fd,(struct sockaddr*)&c_addr,(socklen_t*)&c_addrl);
		if(to_client_fd<0)
		{
			perror("Accept failed");
			exit(EXIT_FAILURE);
		}
		
		pthread_t th;
		cout<<"creating thread"<<endl;
		tinf=(threads_info*)malloc(sizeof(struct threads_info));
		
		char *str1=inet_ntoa(c_addr.sin_addr);
		
		int port_v=ntohs(c_addr.sin_port);
		
		tinf->client_fd=to_client_fd;
		tinf->cport=port_v;






		tinf->cip=str1;
		
		pthread_create(&th,NULL,client_fn,(void*)tinf);//removed &




	}

}

void *to_chunk_server(void *ch)
{
	cout<<"inside to chunk server\n";
	to_chunk_server_details *fi=(to_chunk_server_details*)ch;
	int s_pos=fi->seek_pos;
	int port_no=fi->lport;
	int client_fd,status;
	struct sockaddr_in addr;
	string fname=fi->filename;
	int size=fi->filesize;

	client_fd=socket(AF_INET,SOCK_STREAM,0);
	if(client_fd==0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port_no);
	// char clientip[100];
	// strcpy(clientip,cip.c_str());
	addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	
	
	status=connect(client_fd,(struct sockaddr*)&addr,sizeof(addr));
	if(status<0)
	{
		perror("connection failed");
		exit(EXIT_FAILURE);
	}	

	
	string total_path=path+fname;
	cout<<"total path is  "<<total_path<<endl;
	char fbuf[total_path.size()+1];
	strcpy(fbuf,total_path.c_str());
	FILE *fp=fopen(fbuf,"rb+");

	if(!fp)
	{
		perror("cannot open file\n");
	}

	int ret=fseek(fp , s_pos , SEEK_SET);
	int z=ftell(fp);
	cout<<"now at position "<<z<<endl;
	if(ret!=0)
	{
		perror("seek unsuccesful\n");
	}

	  int count=0;
	  unsigned char buffer[1024*32];
	  int n=0;
	  int to_read_size=64*1024;
	char ack[512];

	int sentbytes= send(client_fd , "write" , sizeof("write") , 0);
    memset(ack , '\0' , 512);
    int recv_bytes=recv(client_fd , ack , 512 , 0);//ok recvd

	

    char fname_buf[fname.size()+1];
    strcpy(fname_buf,fname.c_str());
	sentbytes= send(client_fd , fname_buf , sizeof(string(fname_buf)) , 0);
    memset(ack , '\0' , 512);
    recv_bytes=recv(client_fd , ack , 512 , 0);

    sentbytes=send(client_fd , &size , sizeof(size) , 0);
    cout<<"sent filesize"<<size<<endl;

  	
  	int m;
    memset(ack , '\0' , 512);
    recv_bytes=recv(client_fd , ack , 512 , 0);
    cout<<"recvd"<<ack<<endl;
    while((m = fread(buffer , sizeof(char) , 32*1024 , fp)) > 0 &&  to_read_size> 0)
    {
    	count++;
        cout<<"count"<<count<<endl;
        int x=send(client_fd , buffer , m , 0);
    	cout<<"x"<<x<<endl;
        memset(buffer , '\0' , 32*1024);
    	to_read_size-=m;
        recv(client_fd , ack , 512 , 0);
        cout<<"ack is "<<ack<<endl;
        memset(ack , '\0' , 512);
    }
}
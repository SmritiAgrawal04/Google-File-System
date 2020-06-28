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
#include <chrono>
#include<sstream>

using namespace std;

string path="/home/meenu/";
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
	string listen_ip;
	string filename;
	int filesize;
	int chunk_num;
};

unordered_map<string,file_info> file_map;//file name indexed

deque<int>chunk_serv;//deque to maintain RR way of allocation
unordered_map<int,pair<string, int>>chunk_listen_ip;//map containing chunkserver <index,Listen ip>
unordered_map<string,int> modified;//hash and if the file is modified or not
unordered_map<string,vector<pair<int,int>>>chunk_allocation;//string is hash <int,int> is the name of the chunk servers where we need to store the data
//the vector's index gives the chunk number. Eg:0th index corresponds to 0th chunk and so on.....
unordered_map<int,vector<pair<string,int>>>chunk_files;//Chubk server number , vector<filename/hash ,chunk number>

int current_chunk_number[7];//to store the current chnunk numver for each of the chunkservers



int alive[7]={1};
unordered_map<int,int> alive_map;//client_port(or chunk_server_accept_fd) , index

void *to_chunk_server(void *ch);

void *heartbeat_fn(void *cs)
{	
	int hbno=0;int client_fd,status;

	while(1)
	{	
		for(int i=1;i<7;i++)
		{
			
			//cout<<"again inside for where i ="<<i<<endl;
			
			struct sockaddr_in addr;


			client_fd=socket(AF_INET,SOCK_STREAM,0);
			if(client_fd==0)
			{
				perror("Socket creation failed");
				exit(EXIT_FAILURE);
			}
			addr.sin_family=AF_INET;
			int ip=chunk_listen_ip[i].second; //port
			string ip_true=chunk_listen_ip[i].first;

			//cout<<"string ip_true"<<ip_true<<endl;
			//cout<<"i is "<<i<<endl;
			//cout<<"ip is "<<ip<<"i = "<<i<<endl;
			addr.sin_port=htons(ip);
			char ip_add[ip_true.size()+1];
			strcpy(ip_add,ip_true.c_str());
			addr.sin_addr.s_addr=inet_addr(ip_add);
			
			
			status=connect(client_fd,(struct sockaddr*)&addr,sizeof(addr));
			if(status<0)
			{
				//perror("connection failed");
				//exit(EXIT_FAILURE);
				cout<<"not availbale";
				alive[i]=0;
			}	
			else
			{	//cout<<"connceted "<<i<<endl;
				char buffer[512];
				int sentbytes=send(client_fd,"heartbeat",sizeof("heartbeat"),0);
				int readbytes=recv(client_fd,buffer,512,0);
				//cout<<"recvd hrtbt "<<readbytes<<endl;
				//DOUBT HOW TO WAIT??? as recv is a blocking call
				alive[i]=1;
				//cout<<"hbno "<<++hbno<<endl;
				memset(buffer,'\0',512);
			}
			//cout<<"else ke bahar"<<endl;
			close(client_fd);
			usleep(1000000);
			
		}
			

				
	}
}

void read_file(int from_client)
{
	int sentbytes=send(from_client,"ok",sizeof("ok"),0);
	char buffer[512];
			
	int readbytes=recv(from_client,buffer,512,0);
	string filename=buffer;
	cout<<"filename "<<filename<<endl;

	memset(buffer,'\0',512);
	
	int ret=0;

	if(file_map.find(filename)==file_map.end())
	{
		ret=-1;
		sentbytes=send(from_client,&ret,sizeof(ret),0);
		return;

	}	

	else
	{
		sentbytes=send(from_client,&ret,sizeof(ret),0); //0 case
		memset(buffer,'\0',512);
		readbytes=recv(from_client,buffer,512,0);


		file_info f1=file_map[filename];

		string file_hash=f1.hash;

		cout<<"hash is "<<file_hash<<endl;

		if(chunk_allocation.find(file_hash)!=chunk_allocation.end())
			cout<<"not empty "<<endl;
		else
		{
			cout<<"chunk "<<chunk_allocation[file_hash].size();

		}
		
		vector<pair<int,int>> temp=chunk_allocation[file_hash];

		cout<<"chunk allocation wala size ";
		cout<< "tempsize="<<temp.size()<<endl;

		int chunk_server=-1;
		int failed=-1;
		string list_of_ip="";
		cout<<"list of ip i s "<<list_of_ip<<endl;
		for(int i=1;i<temp.size();i++)
		{
			chunk_server=temp[i].first;
			cout<<"chunk server "<<chunk_server<<endl;
			if(alive[chunk_server]==0)
			{
				failed=chunk_server;	
				chunk_server=temp[i].second;

			}	
			pair<string,int> p=chunk_listen_ip[chunk_server];
			cout<<"p.first "<<p.first <<"p.second "<<p.second<<endl;
			
			list_of_ip=list_of_ip+p.first+":"+to_string(p.second);
			cout<<"list of ip now "<<list_of_ip<<endl;
			if(i!=temp.size()-1)
			{
				list_of_ip=list_of_ip+" ";
			}




		}

		cout<<"list of ips = "<<list_of_ip<<endl;


		char listofip[list_of_ip.size()+1];
		strcpy(listofip,list_of_ip.c_str());
		cout<<"char list of ip :"<<endl;
		cout<<listofip<<endl;
		cout<<"thats it "<<endl;
		cout<<"len is "<<strlen(listofip)<<endl;
		sentbytes=send(from_client,listofip,strlen(listofip),0);
		cout<<"sentbyres "<<sentbytes<<endl;
		readbytes=recv(from_client,buffer,512,0);

		int total_size=f1.filesize;

		sentbytes=send(from_client,&total_size,sizeof(total_size),0);






	}


	


}

void update_file(int from_client)
{
		cout<<"inside update function"<<endl;
		char buffer[512];
    	int sent_bytes= send(from_client ,"ok" , strlen("ok") , 0);
		char filename[512];
		bzero(filename,'\0');
		int readbytes=recv(from_client,filename,512,0);
		cout<<"printing buffer which is receiving filename "<<filename<<endl;
		// char buffer[512];
		bzero(buffer,'\0');
    	sent_bytes= send(from_client ,"ok" , strlen("ok") , 0);
    	cout<<"set ack"<<endl;
    	int content_filesize=0;
    	readbytes = recv(from_client,&content_filesize,sizeof(content_filesize),0);
    	cout<<"filesize is "<<content_filesize<<endl;
    	int ret=1;

    	if(file_map.find(filename)==file_map.end())
		{
		ret=-1;
		sent_bytes=send(from_client,&ret,sizeof(ret),0);
		cout<<"doesnt exist"<<endl;
		return;
		}	

	else
	{
		sent_bytes=send(from_client,&ret,sizeof(ret),0); //1 case
		memset(buffer,'\0',512);
		readbytes=recv(from_client,buffer,512,0);


		file_info f1=file_map[filename];

		string file_hash=f1.hash;

		cout<<"hash is "<<file_hash<<endl;

		if(chunk_allocation.find(file_hash)!=chunk_allocation.end())
			cout<<"not empty "<<endl;
		else
		{
			cout<<"chunk "<<chunk_allocation[file_hash].size();

		}
		
		vector<pair<int,int>> temp=chunk_allocation[file_hash];
		pair<int,int> p = temp[temp.size()-1];

		int last_chunknum= temp.size()-1;


		int remaining_size= 65536-(f1.filesize%65536);

		string lastchunk_details=to_string(remaining_size)+":"+string(filename)+"_"+to_string(last_chunknum);

		int chunk_server=p.first;
		pair<string,int> piss=chunk_listen_ip[chunk_server];

		string list_of_ip= piss.first+":"+to_string(piss.second)+" ";
		chunk_server=p.second;
		piss=chunk_listen_ip[chunk_server];
		list_of_ip +=piss.first+":"+to_string(piss.second);
		cout<<"list of ip is "<<list_of_ip<<endl;

		cout <<"content_filesize =" << content_filesize<< endl;
		cout << "remaining_size = " << remaining_size << endl;
		int numof_chunkservers_required=(content_filesize- remaining_size)/65536;
		if(!((content_filesize - remaining_size)%65536))
			numof_chunkservers_required++;

		cout << "Num of chunks = " << numof_chunkservers_required << endl;

		for(int i = 0; i < numof_chunkservers_required ; i++)
		{
			chunk_server=chunk_serv.front();
			chunk_serv.pop_front();
			chunk_serv.push_back(chunk_server);

			piss=chunk_listen_ip[chunk_server];
			list_of_ip +=" "+piss.first+":"+to_string(p.second);
			// cout<<"list of ip is "<<list_of_ip<<endl;
		}

		cout<<"list of ips now = "<<list_of_ip<<endl;


		char listofip[list_of_ip.size()+1];
		strcpy(listofip,list_of_ip.c_str());
		cout<<"char list of ip :"<<endl;
		cout<<listofip<<endl;
		cout<<"thats it "<<endl;
		cout<<"len is "<<strlen(listofip)<<endl;
		cout<<"listof ip "<<listofip<<endl;
		sent_bytes=send(from_client,listofip,strlen(listofip),0);
		cout<<"sentbyres "<<sent_bytes<<endl;
		readbytes=recv(from_client,buffer,512,0);

		//find remaining size : lastchunknumber
		char last_chunk_details[lastchunk_details.size()+1];
		strcpy(last_chunk_details,lastchunk_details.c_str());
		sent_bytes = send(from_client,last_chunk_details,strlen(last_chunk_details),0);

 }

}


void write_file(int from_client)

{	
		cout<<"inside write function"<<endl;
		char buffer[512];
		bzero(buffer,'\0');
		int sentbytes=send(from_client,"hi",sizeof("hi"),0);
		//char buffer[512];
		int readbytes=recv(from_client,buffer,512,0);
		cout<<"printing buffer which is receiving filename "<<buffer<<endl;
		string filename=buffer;
		cout<<"filename recieved"<<filename<<endl;

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
		string filepath="/home/meenu/"+filename;
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
		fclose(fp);
		vector<pair<int,int>> chunk_server_pair;
		chunk_server_pair.push_back(make_pair(0,0));

		file_info f;
		f.filename=filename;
		f.filesize=filesize;
		f.hash=final_sha; //this stores the entire sha of the file

		file_map[filename]=f; //add the file details to the filemap indexing the filename

		cout<<"now going to write to chunk servers"<<endl;
		int failed=-1;
		int failflag=0;
		for(int i=1;i<7;i++)
		{

			if(alive[i]==0)
			{
				failed=alive_map[i];
				failflag=1;
				cout<<"failed "<<i<<endl;
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
				cout<<"no of pairs"<<no_of_pairs<<endl;

				int pos=0;
				int fsize=filesize;
				int to_send_size=0;
				int chunk_no=0;
				while(no_of_pairs--)
				{
					chunk_no++;
					int first_chunk_server=chunk_serv.front();
					chunk_serv.pop_front();
					chunk_serv.push_back(first_chunk_server);
					int second_chunk_server=chunk_serv.front();
					chunk_serv.pop_front();
					chunk_serv.push_back(second_chunk_server);
					chunk_server_pair.push_back(make_pair(first_chunk_server, second_chunk_server));

					// get the listen ips of the corresponding chunk servers
					int first_ip=chunk_listen_ip[first_chunk_server].second;
					int second_ip=chunk_listen_ip[second_chunk_server].second;
					string first_ip_true=chunk_listen_ip[first_chunk_server].first;
					string second_ip_true=chunk_listen_ip[second_chunk_server].first;

					cout<<"first port"<<first_ip<<endl;
					cout<<"second port"<<second_ip<<endl;
					cout<<"first ip "<<first_ip_true<<endl;
					cout<<"second ip "<<second_ip_true<<endl;
					to_chunk_server_details *f,*s;
					

					pthread_t th1,th2;
					cout<<"creating thread"<<endl;
					//f=(to_chunk_server_details*)malloc(sizeof(struct to_chunk_server_details));
					//s=(to_chunk_server_details*)malloc(sizeof(struct to_chunk_server_details));
					f=new to_chunk_server_details();
					s=new to_chunk_server_details();
					cout<<"after memory allocation"<<endl;
	
					f->lport=first_ip;
					cout<<f->lport<<endl;
					f->chunk_num=chunk_no;
					cout<<f->chunk_num<<endl;
					s->chunk_num=chunk_no;
					cout<<s->chunk_num<<endl;
					f->listen_ip=first_ip_true; //seg fault here aarahai
					cout<<"f ka listen_ip"<<f->listen_ip<<endl;
					s->listen_ip=second_ip_true;
					s->lport=second_ip;
					f->seek_pos=pos;
					s->seek_pos=pos;
					f->filename=filename;
					s->filename=filename;
					cout<<"filename  "<<filename<<endl;
					if(fsize>pos)
						{
							fsize=fsize-pos;
						}
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

				chunk_allocation[final_sha]=chunk_server_pair;
				cout<<"chuk allocation size afterwrite "<<chunk_allocation[final_sha].size()<<endl;
				chunk_server_pair.clear();
			//	unordered_map<string,vector<pair<int,int>>>::iterator itr;

	/*	if((itr=chunk_allocation.find(final_sha))!=chunk_allocation.end())
		{
			for(int i=0;i<(itr->second).size();i++)
			{
				cout<<itr->second[i].first<<" ";
				cout<<itr->second[i].second<<endl;
			}
		}
		cout<<endl;*/




			}	
		

cout<<"donewriting \n";
}




void *client_fn(void *cs)
{
	while(1)
	{	
		threads_info *p=(threads_info*)cs;
		
		cout<<"entering start fn"<<endl<<endl;	
		// int from_client=(sti->client_fd);
		
		char buffer[512];
		int readbytes=0;

		int from_client=p->client_fd;
		int port_no=p->cport;
		string my_ip=p->cip;	
		recv(from_client , buffer , 512 , 0);
		

		if(strcmp(buffer , "write") == 0)
		{
			cout<<"calling write  \n";
			write_file(from_client);
			cout<<"calling write done"<<endl;
		}

		else if(strcmp(buffer , "read") == 0)
		{
			// cout << "in if condition\n";
			read_file(from_client);
		}

		else if(strcmp(buffer , "update") == 0)
		{
			update_file(from_client);
		}

		//close(from_client);
		cout<<"leaving client fn "<<endl;
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
	chunk_listen_ip[1]=make_pair("127.0.0.1",8088);
	chunk_listen_ip[2]=make_pair("127.0.0.1",8082);
	chunk_listen_ip[3]=make_pair("127.0.0.1",8083);
	chunk_listen_ip[4]=make_pair("127.0.0.1",8084);
	chunk_listen_ip[5]=make_pair("127.0.0.1",8085);
	chunk_listen_ip[6]=make_pair("127.0.0.1",8086);
	
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
	pthread_t th[6];
	while(cl<1)
	{
		pthread_t th;

		// to_client_fd=accept(tracker_fd,(struct sockaddr*)&c_addr,(socklen_t*)&c_addrl);
		// if(to_client_fd<0)
		// {
		// 	perror("Accept failed");
		// 	exit(EXIT_FAILURE);
		// }
		// cout<<"tempcoutn"<<++temp_count<<endl;
		// // pthread_t th;
		// cout<<"creating thread"<<endl;
		// alive_map[cl+1]=to_client_fd;
		// tinf=(threads_info*)malloc(sizeof(struct threads_info));
		
		// char *str1=inet_ntoa(c_addr.sin_addr);
		
		// int port_v=ntohs(c_addr.sin_port);
		
		// tinf->client_fd=to_client_fd;
		// tinf->cport=port_v;

		// cout<<port_v<<endl;
		// cout<<to_client_fd<<endl;




		// tinf->cip=str1;
		
		// pthread_create(&th[cl],NULL,heartbeat_fn,(void*)tinf);//removed &
		// //pthread_join()

		cl++;
		// cout<<"last  of main while 6"<<endl;
		pthread_create(&th,NULL,heartbeat_fn,NULL);


	}
	//For accepting incoming client requests.Run in infinte loop.
	while(1)
	{
		cout<<"inside while 1 main wala "<<endl;

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
		cout<<"calling thread "<<endl;
		pthread_create(&th,NULL,client_fn,(void*)tinf);//removed &
		cout<<"inside while 1"<<endl;



	}
	cout<<"last  of main "<<endl;




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
	int chunk_no=fi->chunk_num;
	string ip_addr=fi->listen_ip;
	char ip_add[ip_addr.size()+1];
	strcpy(ip_add,ip_addr.c_str());

	cout<<"inside to_chunk_server ip_add chap rha h"<<ip_add<<endl;
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
	addr.sin_addr.s_addr=inet_addr(ip_add);
	
	
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

	
    fname=fname+"_"+to_string(chunk_no);
    char fname_buf[fname.size()+1];
    strcpy(fname_buf,fname.c_str());
	sentbytes= send(client_fd , fname_buf , sizeof(string(fname_buf)) , 0);//sending file_name
    memset(ack , '\0' , 512);
    recv_bytes=recv(client_fd , ack , 512 , 0);

    sentbytes=send(client_fd , &size , sizeof(size) , 0);//sending filesize
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
    fclose(fp);
    close(client_fd);
    cout<<"leaving to client fd"<<endl;
}
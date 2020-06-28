#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include<iostream>
#include <unistd.h> 
#include<vector>
#include <string.h> 
#include<cstring>
#include <stdlib.h>
#include <string>
#include <bits/stdc++.h> 
#include<fstream>
#include<unistd.h>
#include <netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<vector>
#include<algorithm>
#include <openssl/sha.h>
#include <stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include<cstring>

#define chunkSize 32*1024

using namespace std;
string user_id;

string default_path="/home/meenu/Music/GFS_seekers_apk-master/";

struct parameters
{
    string chunk_indices;
    string ip_port;
    string filename;
    // int filesize;
};
struct parameters_update
{
    string chunk_indices;
    string ip_port;
    string filename;
    int last_ch_num;
    int last_size;

};

bool flag_setnull =0;

void *get_from_chunkserver(void *);
void *write_updates_to_chunkserver(void *);
void cal_sha(int client_fd , char* filename)
{
    FILE *fp;
    fp=fopen(filename,"rb+");
    if(!fp)
    {
        cout<<"unable to open file to calculate sha"<<endl;
    }
    fseek(fp , 0 , SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);
    char buffer_sha[512*1024];
    int sent_bytes;
    string  final;
    string final_sha;
    unsigned char digest[SHA_DIGEST_LENGTH];
    // int no_of_times=0;
    // cout << "filesize=" << filesize << endl;
    cout<<"going to calculate sha1"<<endl;
        //string final_sha="";
    while((sent_bytes=fread(buffer_sha,sizeof(char),512*1024,fp))>0 && filesize>0)
    {   
        // no_of_times++;
        // cout << "in sha";
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
        filesize=filesize-sent_bytes;
    }
    // modified[final_sha]=0;   
    // cout<<final_sha<<endl;
    char sha[512];
    strcpy(sha,final_sha.c_str());

    send(client_fd , sha , strlen(sha) , 0);
    fclose(fp);
}
void update_last_cs(string ip_port,string lastname,string last_size,string update_file)
{
    cout<<"lastname "<<lastname<<endl;
    cout<<"update last cs "<<endl;
    char ipprt[ip_port.size()+1];
    strcpy(ipprt,ip_port.c_str()); 
   
    vector<char*> connection_details;
    char* temp = strtok(ipprt, ":");
    while(temp != NULL)
    {
        connection_details.push_back(temp);
        temp = strtok(NULL, ":");
    }

    char *ip=connection_details[0];
    char *port=connection_details[1];
    int prt=atoi(port);


    int size=stoi(last_size);

    struct sockaddr_in addr;

    int client_fd=socket(AF_INET,SOCK_STREAM,0);
    if(client_fd==0)
    { 
           perror("Socket creation failed");
            exit(EXIT_FAILURE);
    }
    addr.sin_family=AF_INET;
        // int ip=chunk_listen_ip[i];
            //cout<<"i is "<<i<<endl;
            //cout<<"ip is "<<ip<<"i = "<<i<<endl;
    addr.sin_port=htons(prt);
    addr.sin_addr.s_addr=inet_addr(ip);
            
    cout<<"going to connect to "<<ip<<" and port number  "<<prt<<endl;
            
    int status=connect(client_fd,(struct sockaddr*)&addr,sizeof(addr));
    if(status<0)
        {
             perror("connection failed");
               exit(EXIT_FAILURE);
                   
        } 
        cout<<"connected"<<endl;

    send(client_fd , "update_last" , strlen("update_last") , 0);
    char buf[512];
    int readbytes= recv(client_fd , buf , 512 , 0);
    memset(buf,'\0',512);

    char name[lastname.size()+1];
    strcpy(name,lastname.c_str());
    cout<<"lastname is "<<name<<endl;
    int sent_bytes= send(client_fd , name , strlen(name) , 0);
    //char buf[512];
    readbytes= recv(client_fd , buf , 512 , 0);
    memset(buf,'\0',512);
    //send the size to be written
    int last_chunk_size=stoi(last_size);
    cout<<"last chunk size"<<last_chunk_size<<endl;
    sent_bytes= send(client_fd , &last_chunk_size, sizeof(last_chunk_size) , 0);
    readbytes= recv(client_fd , buf , 512 , 0);

    char update_content_file[update_file.size()+1];
    strcpy(update_content_file,update_file.c_str());
    FILE *f=fopen(update_content_file,"r");
    if(!f)
    {
        perror("Cant open file");
    }
    char data_buf[32*1024];
    int n=0;
    char ack[100];
    while((n = fread(data_buf , sizeof(char) , 32*1024 , f)) > 0 && last_chunk_size > 0)
    {
        cout<<"sending the data "<<data_buf<<endl;
        int x=send(client_fd , data_buf , n , 0);
        cout<<"x"<<x<<endl;
        memset(data_buf , '\0' , 32*1024);
        last_chunk_size -= n;

        recv(client_fd , ack , 512 , 0);
        cout<<"ack is "<<ack<<endl;
        memset(ack , '\0' , 512);
    }
    fclose(f);

    close(client_fd);










}

void update_file(int client_fd , char* filename1 ,char* filename2)
{

    int sent_bytes= send(client_fd , "update" , sizeof("update") , 0);
    char buf[512];
    int readbytes= recv(client_fd , buf , 512 , 0);
    memset(buf,'\0',512);

    sent_bytes= send(client_fd , filename1 , strlen(filename1) , 0);//to append filename
    readbytes= recv(client_fd , buf , 512 , 0);

    FILE *f = fopen(filename2 , "rb");
    if(!f)
    {
        perror("cant open file");
    }

    fseek(f , 0 , SEEK_END);
    int size = ftell(f) ;
    rewind(f);

    
    send(client_fd , &size , sizeof(size) , 0);
    cout<<"sent filesize"<<size<<endl;
    memset(buf,'\0',512);
    int ret;
    readbytes= recv(client_fd , &ret , sizeof(ret) , 0);
    cout<<"revd ret value"<<ret<<endl;

    send(client_fd , "ok" , sizeof("ok") , 0);
    
    char buffer[4096];
    readbytes= recv(client_fd , buffer , 4096 , 0);//list of all ip ports <last chunk ip port,last chunk ip port ,remaining chunks in rr......>
    cout<<"buffer "<<buffer<<endl;
    string chunkservers_tmp=buffer;
    char chunkservers[chunkservers_tmp.size()+1];
    strcpy(chunkservers,chunkservers_tmp.c_str());
    
    sent_bytes= send(client_fd ,"ok" , strlen("ok") , 0);
    memset(buffer,'\0',512);
    
    
    readbytes= recv(client_fd , buffer , 4096 , 0);
    sent_bytes= send(client_fd ,"ok" , strlen("ok") , 0);
     cout<<"buffer 2 "<<buffer<<endl;
    string last_chunk_tmp=buffer;//contains remaining size:last chunk number
    char last_chunk_details[last_chunk_tmp.size()+1];
    strcpy(last_chunk_details,last_chunk_tmp.c_str());
    memset(buffer,'\0',512);

    

    int update_size=0;
    readbytes= recv(client_fd , &update_size , sizeof(update_size) , 0);
    sent_bytes= send(client_fd ,"ok" , strlen("ok") , 0);

    //LOOP HERE


    int last_chunk_number_update=0;
    readbytes= recv(client_fd , &last_chunk_number_update , sizeof(last_chunk_number_update) , 0);
    cout<<"last_chunk_num_update "<<last_chunk_number_update<<endl;
    sent_bytes= send(client_fd ,"ok" , strlen("ok") , 0);
    


   
    cout<<"last "<<last_chunk_details<<endl;
    vector<char *> chunkservers_name;
    
    char* t = strtok(chunkservers, " ");

    while(t != NULL)
    {
        chunkservers_name.push_back(t);
        t = strtok(NULL, " ");
    }
    cout<<"chunkservers[0]"<<chunkservers_name[0]<<endl;
    vector<char*> temporary;
    char* temp = strtok(last_chunk_details, ":");
    while(temp != NULL)
    {
        temporary.push_back(temp);
        temp = strtok(NULL, ":");
    }

    string lastfilename=string(temporary[1]);//last file name
    cout<<"last filename "<<lastfilename<<endl;
    string last_size=temporary[0];
    cout<<"last sixe "<<last_size<<endl;

     int remaining_size=size-stoi(last_size);
     cout<<"remaining_size ="remaining_size<<endl;

    // int total_chunks=remaining_size/65536;
    // if(remaining_size%65536!=0)
    // {
    //     total_chunks=total_chunks+1;
    // }

     int last_update_size=remaining_size%65536;

    // int last_chunk_no=atoi(temporary[1]);
    // cout<<"last chunk no "<<endl;
    update_last_cs(chunkservers_name[0],lastfilename,last_size,filename2);
    update_last_cs(chunkservers_name[1],lastfilename,last_size,filename2);

//vector<string> update_details
       
    int i=0;
    pthread_t th;
    parameters_update *p;
    if(update_size>0)
    {
        while(i<update_size)
        {
            string ip_port=strtok(char*(update_details[i]),"_");
            string list_of_chunk_num=strtok(NULL,"_");
            cout<<"ip port "<<ip_port<<endl;
            cout<<"list of chunks "<<list_of_chunk_num<<endl;
            // string ip;
            // ip = strtok(ip_port, ":");
            // char ip_l[ip.size()+1];
            // strcpy(ip_l,ip.c_str());
            // cout<<"string ip is "<<ip_l<<endl;
            // string port;
            // port=strtok(NULL, ":");
            // int prt=stoi(port);
            // cout<<"port is "<<prt<<endl;
            //string fname=strtok(temporary[1],"_");
            p=(parameters_update*)malloc(sizeof(struct parameters_update));
            if(p==NULL)
            {
                cout<<"no memory allocated"<<endl;
            }
            else
                cout<<"mem allocated"<<endl;
            p->chunk_indices=list_of_chunk_num;
            cout<<"chunk indices "<<p->chunk_indices<<endl;
            p->ip_port=ip_port;
            cout<<"p->ip_port "<<p->ip_port<<endl;
            p->filename=lastfilename;
            cout<<"filename "<<p->filename<<endl;
            p->last_ch_num=last_chunk_number_update;
            p->last_size=last_update_size;
            cout<<"p->last_size "<<p->last_size<<endl;
            // if(last_chunk_number_update==i+1)
            // {
            //     p->filesize=last_update_size;
            // }
            // else
            //     p->filesize=65536;
            // p->filesize=total_size;
            // cout << "filesize " << p->filesize << endl;
            pthread_create(&th,NULL,write_updates_to_chunkserver,(void*)p);

            




            i++;
        }
    }



   







    


}

void read_file(int client_fd , char* filename)
{
   int sent_bytes= send(client_fd , "read" , sizeof("read") , 0);
    // cal_sha(client_fd , filename);
   char buf[512];
   int readbytes= recv(client_fd , buf , 512 , 0);

   sent_bytes= send(client_fd , filename , sizeof(string(filename)) , 0);
   int ret;
   readbytes= recv(client_fd , &ret , sizeof(ret) , 0);

   if(ret==-1)
   {
        cout<<"No such file exists\n";
        return;
   }
   else
   {    memset(buf,'\0',512);
        int sentbytes=send(client_fd , "ok" , sizeof("ok") , 0);
       //string ip_list;
        char buffer[32*1024];

        readbytes=recv(client_fd , buffer , sizeof(buffer) , 0);
        cout<<"buffer is "<<buffer<<endl;
        sent_bytes= send(client_fd , "ok" , sizeof("ok") , 0);

        int total_size=0;
        readbytes= recv(client_fd , &total_size , sizeof(total_size) , 0);

        vector<char*> ip_list_pair;//ip:port is here eg 127.0.0.1:8908<space>next ip:port<space>
        char* temp = strtok(buffer, " ");
        while(temp != NULL)
        {
            // cout << "temp = " << temp << endl;
            ip_list_pair.push_back(temp);
            temp = strtok(NULL, " ");
            // cout << "tempafter = " << temp;

        }

        vector<string> v; //this contains the list of chunks
        //This guy has to open a file and append null. 
        vector<string> ip_port;//ip:port
        vector<int> visited(ip_list_pair.size(),0);
        cout<<"visired size"<<visited.size()<<endl;
        for(int i=0;i<ip_list_pair.size();i++)

        {   int flag=0;
            string temp;
            if(visited[i]==0)
             {  temp=to_string(i+1);
                cout<<"first visted [i]=0"<<endl;
                ip_port.push_back(ip_list_pair[i]);
                visited[i]=1;
                 cout<<"i is "<<i<<endl;
                cout<<"ip "<<ip_port[i]<<endl;
                flag=1;
             }   
           
            
            for(int j=i+1;j<ip_list_pair.size();j++)
            {
                if(strcmp(ip_list_pair[i],ip_list_pair[j])==0 && visited[j]==0)
                {
                    cout<<"inside second visited "<<endl;
                    temp=temp+":"+to_string(j+1);
                    //ip_list_pair.erase(ip_list_pair.begin()+j);
                    visited[j]=1;

                }
            }
            
            cout<<"temp "<<temp<<endl;
            if(flag==1)
                v.push_back(temp);

        }

        cout<<"v[0] "<<v[0]<<endl;
        string fpath=default_path+string(filename);

        char file[fpath.size()+1];
        strcpy(file,fpath.c_str());
        FILE *fp=fopen(file,"w");
      //  write(data_buffer,sizeof(char),n,fp);
        int filesize=total_size;
        cout<<"total size"<<total_size<<endl;
        int w=0;
        //w=-2028;
       //filesize=filesize;
       while(w<filesize)
        {
            int x=fwrite("x",sizeof(char),1,fp);
            cout<<"x"<<x<<endl;
            cout<<"sizeof char "<<sizeof(char)<<endl;
            w=w+x;
            cout<<"w is "<<w<<endl;
        }
        
        
        //fwrite("\0",sizeof(char),filesize,fp);
        

       // filesize=filesize-1024*512;
         pthread_t th;
         parameters *p;
         int i=0;
         cout<<"v.size "<<v.size()<<endl;
        while(i<v.size())
        {

            cout<<"at i"<<i<<endl;
            //p=(parameters*)malloc(sizeof(struct parameters));
            p=new parameters;
            if(p==NULL)
            {
                cout<<"no memory allocated"<<endl;
            }
            else
                cout<<"mem allocated"<<endl;
            cout<<"v[i] "<<v[i]<<endl;

            p->chunk_indices=v[i];
            cout<<"chunk indices "<<p->chunk_indices<<endl;
            p->ip_port=ip_port[i];
            cout<<"p->ip_port "<<p->ip_port<<endl;
            p->filename=filename;
            cout<<"filename "<<p->filename<<endl;
            // p->filesize=total_size;
            // cout << "filesize " << p->filesize << endl;
            pthread_create(&th,NULL,get_from_chunkserver,(void*)p);
            i++;

        }
    }

}


void write_file(int client_fd , char* filename)
{
    cout<<"inside write file function"<<endl;
    cout<<"filename from argument"<<filename<<endl;
    char ack[512];

    int sent_bytes=send(client_fd , "write" , sizeof("write") , 0);
    cout<<"sent" <<sent_bytes<<endl;

    int recvbytes=recv(client_fd , ack , 512 , 0);
    cout<<"recvd   "<<ack<<endl;
    // string filename="wick.mp3";
    // char fname[filename.size()+1];
    string tempo = string(filename);
    char fname[tempo.size()+1];
    strcpy(fname,tempo.c_str());
    cout <<"fname is" <<fname << endl;

    vector<char*> parameters;
    char* temp = strtok(fname, "/");
    while(temp != NULL)
    {
        parameters.push_back(temp);
        temp = strtok(NULL, "/");
    }
    cout<<"name to be sent"<<parameters[parameters.size()-1]<<endl;
    send(client_fd , parameters[parameters.size()-1] , sizeof(string(parameters[parameters.size()-1])) , 0);

    memset(ack , '\0' , 512);
    recv(client_fd , ack , 512 , 0);
    cout<<"recv "<<ack<<endl;

    cout << filename << endl;
    FILE *f = fopen(filename , "rb");
    fseek(f , 0 , SEEK_END);
    int size = ftell(f) ;
    rewind(f);
    
    send(client_fd , &size , sizeof(size) , 0);
    cout<<"sent filesize"<<size<<endl;

    unsigned char buffer[chunkSize];
    // unsigned char main_buffer[200000];
    int n;
    memset(ack , '\0' , 512);
    recv(client_fd , ack , 512 , 0);
    cout<<"recvd ack "<<ack<<endl;
    // string hash;
    int count=0;
    while((n = fread(buffer , sizeof(char) , chunkSize , f)) > 0 && size > 0)
    {
        count++;
        cout<<"count"<<count<<endl;
        int x=send(client_fd , buffer , n , 0);
        cout<<"x"<<x<<endl;
        memset(buffer , '\0' , chunkSize);
        size -= n;
        recv(client_fd , ack , 512 , 0);
        cout<<"ack is "<<ack<<endl;
        memset(ack , '\0' , 512);
    }
    fclose(f);
}


void *get_from_chunkserver(void *cs)
{

    parameters *p=(parameters *)cs;

    cout<<"inside get from chunk server "<<endl;
    string chunk_indices=p->chunk_indices;
    cout<<"get from  chunk_indices "<<chunk_indices<<","<<endl;
    string ip_port=p->ip_port;
    cout<<"get from ip port "<<ip_port<<endl;
    string filename=p->filename;
   // filename = default_path+filename;

    char fname[filename.size()+1];

    strcpy(fname,filename.c_str());

    FILE *fp=fopen(fname,"r+");

    /*if(!flag_setnull){
        cout << "I'm here\n";
        fwrite("\0",sizeof(char),p->filesize,fp);
        flag_setnull =1;
    }*/
    
    char chu_ind[chunk_indices.size()+1];
    strcpy(chu_ind , chunk_indices.c_str());
    

     vector<char*> ch;
        char* temp = strtok(chu_ind, ":");
        while(temp != NULL)
        {
            // cout << "temp = " << temp << endl;
            ch.push_back(temp);
            temp = strtok(NULL, ":");
            // cout << "tempafter = " << temp;

        }

    
    string ip;
    char dummy[ip_port.size()+1];
    strcpy(dummy,ip_port.c_str());
    ip = strtok(dummy, ":");
    char ip_l[ip.size()+1];
    strcpy(ip_l,ip.c_str());
    cout<<"string ip is "<<ip_l<<endl;


    string port;
   
    port=strtok(NULL, ":");
    int prt=stoi(port);
    cout<<"port is "<<prt<<endl;

        struct sockaddr_in addr;

        int client_fd=socket(AF_INET,SOCK_STREAM,0);
        if(client_fd==0)
            {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
            }
        addr.sin_family=AF_INET;
        // int ip=chunk_listen_ip[i];
            //cout<<"i is "<<i<<endl;
            //cout<<"ip is "<<ip<<"i = "<<i<<endl;
        addr.sin_port=htons(prt);
        addr.sin_addr.s_addr=inet_addr(ip_l);
            
        cout<<"going to connect to "<<ip_l<<" and port number  "<<prt<<endl;
            
        int status=connect(client_fd,(struct sockaddr*)&addr,sizeof(addr));
        if(status<0)
            {
                perror("connection failed");
                exit(EXIT_FAILURE);
               
            } 
            cout<<"connected"<<endl;
            int vec_size = ch.size();
            char buff[64];
            cout<<"ch.size :in "<<vec_size<<":"<<prt<<endl;
            int sentbytes=send(client_fd , "read" , sizeof("read"), 0);
            cout<<"sent "<<sentbytes<<"bytes "<<endl;
            recv(client_fd , buff , 64 ,0);
            memset(buff ,'\0' , 64);

            sentbytes=send(client_fd , &vec_size , sizeof(vec_size) , 0);
            cout<<"sent bytes for vecsize"<<sentbytes<<endl;
            recv(client_fd , buff , 64 , 0);
        for(int i = 0 ; i < ch.size(); ++i)
        {
            cout << "ch[i]=" << ch[i]<<endl;
            int to_int = atoi(ch[i]);
            int to_seek_upto=(to_int-1)*64*1024;
            cout<<"seek upto & ch[i]="<<to_seek_upto<<":"<<ch[i]<<endl;
            int ret=fseek(fp , to_seek_upto , SEEK_SET);
            int z=ftell(fp);
            cout<<"now at position "<<z<<endl;
            if(ret!=0)
            {
                perror("seek unsuccesful\n");
            }


            string temp_fname = filename;
            temp_fname += "_" + to_string(atoi(ch[i])+0);
            char buffer[temp_fname.size()+1];
            strcpy(buffer , temp_fname.c_str());
            send(client_fd , buffer , sizeof(buffer) , 0);
            int fsize=0;
            recv(client_fd , &fsize , sizeof(fsize) , 0);
            cout<<"filesize rcvd  is "<<fsize<<endl;
            send(client_fd , "ok" , sizeof("ok") , 0);
            int bytes_done = 0;
            char data_buffer[32*1024];
            int count = 0;
            int x , n;
            while(bytes_done <fsize&&(n=recv(client_fd,data_buffer,sizeof(data_buffer),0))>0)
            { 
              cout<<"n "<<n<<endl;
                 count++;
                cout<<count<<endl;
                fwrite(data_buffer,sizeof(char),n,fp);
                memset(data_buffer,'\0',sizeof(data_buffer));
               //file_size=file_size-n;
                x=send(client_fd,"ok",sizeof("ok"),0);
               
                bytes_done=bytes_done+n;
                cout<<"bytes_done"<<bytes_done<<"filesize"<<fsize<<endl;
            }


        }
        fclose(fp);

        close(client_fd);
}

int create_socket()
{
    return socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
}

int main(int argc, char const *argv[])
{
    
    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_port = htons(8087);


    int client_fd = create_socket();
    if(client_fd < 0)
    {
        perror("Socket Creation Failed!");
        exit(0);
    }

    if(inet_pton(AF_INET, "127.0.0.1", &add.sin_addr) <= 0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    }

    int is_connect = connect(client_fd, (struct sockaddr *)&add, sizeof(add));

    if(is_connect < 0 )
    {
        perror("Error Connecting!\n");
        exit(0);
    }

    cout << "Connection with MasterServer successful!\n";

    while(1)
    {
        // cout << "Enter your choice:\n1. Write File\n2. Read File\n3. Upload File\n";
        // int choice;

        // cin >> choice;
        char command[512];
        cout << "Enter your command:\n";
        cin.getline(command, 512);

        vector<char*> parameters;
        char* temp = strtok(command, " ");
        while(temp != NULL)
        {
            // cout << "temp = " << temp << endl;
            parameters.push_back(temp);
            temp = strtok(NULL, " ");
            // cout << "tempafter = " << temp;

        }

        // cout<<"the len is "<<parameters.size()<<endl;

        // cout << parameters[0] << endl;

         // cout <<"ghghgh"<<parameters[1] << endl;

        if(strcmp(parameters[0] , "write") == 0)
        {
            write_file(client_fd , parameters[1]);
        }

        else if(strcmp(parameters[0] , "read") == 0)
        {
            read_file(client_fd , parameters[1]);
        }

        else if(strcmp(parameters[0] , "update") == 0)
        {
            update_file(client_fd , parameters[1],parameters[2]);
        }

        else 
        {
            continue;
        }
    }

    return 0;
}

void *write_updates_to_chunkserver(void *cs)
{

    parameters_update *p=(parameters_update *)cs;

    cout<<"inside get from chunk server "<<endl;
    string chunk_indices=p->chunk_indices;
    cout<<"get from  chunk_indices "<<chunk_indices<<","<<endl;
    string ip_port=p->ip_port;
    cout<<"get from ip port "<<ip_port<<endl;
    string filename=p->filename;
   // filename = default_path+filename;
    //int file_size=p->filesize;
    int last_chunk_num=p->last_chunk_num;
    int last_size=p->last_size;

    char fname[filename.size()+1];

    strcpy(fname,filename.c_str());
    cout<<"fname is "<<fname<endl;

    //fname format---> abc.cpp_14:3456 <lastfilechunk:no of bytes of free space in last chunk>
    char* f=strtok(fname,"_");
    char* next=strtok(NULL,"_");cout<<"next "<<next<<endl;

    char* last=strtok(next,":");
    cout<<"last "<<last<<endl;
    char* seek_by=strtok(NULL,":");
    cout<<"seek by"<<seek_by<<endl;
    int last_no=atoi(last);
    cout<<"last no "<<last_no<<endl;
    int seek_const=atoi(seek_by);
    cout<<"seek_const"<<seek_const<<endl;
    
    //int last_no=stoi(last);

    FILE *fp=fopen(f,"r");
    if(!fp)
    {
        perror("cant open file");
    }
    
    char chu_ind[chunk_indices.size()+1];
    strcpy(chu_ind , chunk_indices.c_str());
    

    vector<char*> ch;
    char* temp = strtok(chu_ind, "-");
    while(temp != NULL)
    {
        // cout << "temp = " << temp << endl;
         ch.push_back(temp);
        temp = strtok(NULL, "-");
            // cout << "tempafter = " << temp;

    }
    cout<<"ch[0]"<<ch[0]<<endl;
    
    string ip;
    char dummy[ip_port.size()+1];
    strcpy(dummy,ip_port.c_str());
    ip = strtok(dummy, ":");
    char ip_l[ip.size()+1];
    strcpy(ip_l,ip.c_str());
    cout<<"string ip is "<<ip_l<<endl;


    string port;
   
    port=strtok(NULL, ":");
    int prt=stoi(port);
    cout<<"port is "<<prt<<endl;

    struct sockaddr_in addr;

    int client_fd=socket(AF_INET,SOCK_STREAM,0);
        if(client_fd==0)
            {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
            }
    addr.sin_family=AF_INET;
        // int ip=chunk_listen_ip[i];
            //cout<<"i is "<<i<<endl;
            //cout<<"ip is "<<ip<<"i = "<<i<<endl;
    addr.sin_port=htons(prt);
    addr.sin_addr.s_addr=inet_addr(ip_l);
            
    cout<<"going to connect to "<<ip_l<<" and port number  "<<prt<<endl;
            
    int status=connect(client_fd,(struct sockaddr*)&addr,sizeof(addr));
    if(status<0)
    {
                perror("connection failed");
                exit(EXIT_FAILURE);
               
    } 
    cout<<"connected"<<endl;

    int sentbytes= send(client_fd , "write_update" , sizeof("write_update") , 0);
    memset(ack , '\0' , 512);
    
    int recv_bytes=recv(client_fd , ack , 512 , 0);//ok recvd

    //send the no of chunks to chunkserver

    sentbytes= send(client_fd , &ch.size() , sizeof(ch.size()) , 0);
    cout<<"sent bytes "<<sentbytes<<endl;
    recv_bytes=recv(client_fd , ack , 512 , 0);//ok

    int j=0;
    char buffer[512];
    while(j<ch.size())
    {
       string tempfile=filename+string(ch[i]);
       cout<<"filename to cs"<<tempfile<<endl;
       strcpy(buffer,tempfile.c_str());
       sentbytes= send(client_fd , buffer , strlen(buffer) , 0);
       recv_bytes=recv(client_fd , ack , 512 , 0);//ok recvd

       int temp_file_size;
       if(atoi(ch[i])==last_chunk_num)
       {
        temp_file_size=last_size;
       }
       else
        temp_file_size=65536;

        sentbytes= send(client_fd , &temp_file_size , sizeof(temp_file_size) , 0);
        cout<<"temp file size "<<temp_file_size<<endl;
        recv_bytes=recv(client_fd , ack , 512 , 0);//ok recvd

        int n;
        int bytes_done=0;
        //int chunk=512*1024;
        int data_buffer[32*1024];
        FILE *fp;
        string filepath="/home/meenu/client/"+filename;
        char filepth[filepath.size()+1];
        strcpy(filepth,filepath.c_str());
        fp=fopen(filepth,"r");
        if(!fp)
        {
            perror("Cant open file\n");
        }
        int m;
        int count=0
        cout<<"sizeof data buffer"<<sizeof(data_buffer)<<endl;
       char ack[512];

       int ret=fseek(fp , seek_const+(ch[j]-last_no-1) , SEEK_SET);
       int z=ftell(fp);
        cout<<"now at position "<<z<<endl;
        if(ret!=0)
        {
            perror("seek unsuccesful\n");
        }
       while((m = fread(data_buffer , sizeof(char) , 32*1024 , fp)) > 0 &&  temp_file_size> 0)
        {
            
            int x=send(client_fd , data_buffer , m , 0);
            cout<<"x"<<x<<endl;
            memset(data_buffer , '\0' , 32*1024);
            temp_file_size-=m;
            recv(client_fd , ack , 512 , 0);
            cout<<"ack is "<<ack<<endl;
            memset(ack , '\0' , 512);
        }




        j++;

    }
    fclose(fp);
    close(client_fd);










    
}
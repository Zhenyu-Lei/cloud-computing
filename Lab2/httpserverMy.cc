#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>					
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>					
#include <pthread.h>
#include <iostream>
#include <string>
#include <string.h>
#include <sstream>
#include <fcntl.h>
#include <linux/tcp.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <getopt.h>
using namespace std;
const int RunTask=100;
pthread_mutex_t myMutex;// 定义互斥锁，全局变量
pthread_mutex_t myLock;//calculate the number of masks 
int total=0;//calculate the number of masks

void putMessage(int type,string file_path,int sockfd,string message);

void Error_dealer(string method,string url,int sockfd)
{
	string entity;
	if(method!="GET"&&method!="POST")//GET和POST之外的请求
    {
		entity="<htmL><title>501 Not Implemented</title><body bgcolor=ffffff>\n Not Implemented\n<p>Does not implement this method: "+method+"\n,<hr><em>HTTP Web Server </em>\n</body></html>\n";
		int ilen=entity.length();
		stringstream ss;
		ss<<ilen;
		string slen;
		ss>>slen;
		string tmp="Http/1.1 501 Not Implemented\r\nContent-type: text/html\r\nContent-Length: "+slen+"\r\n\r\n";
		string message=tmp+entity;
		putMessage(1,"NULL",sockfd,message);
        //char send_buf[1024];
        //sprintf(send_buf,"%s",message.c_str());
        //write(sockfd,send_buf,strlen(send_buf));
                
	}
	else
    {
		if(method=="GET")//GET请求不满足要求
        {
			entity="<html><title>GET 404 Not Found</title><body bgcolor=ffffff>\n Not Found\n<p>Couldn't find this file: "+url+"\n<hr><em>HTTP Web Sever</em>\n</body></html>\n";
		}
		else if(method=="POST")//POST请求不满足要求
        {
			entity="<html><title>POST 404 Not Found</title><body bgcolor=ffffff>\n Not Found\n<hr><em>HTTP Web Sever</em>\n</body></html>\n";
		}
		int ilen=entity.length();
		stringstream ss;
		ss<<ilen;
		string slen;
		ss>>slen;
		//string tmp="Http/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-Length:"+slen+"\r\n\r\n";
		string tmp="Http/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-Length: "+slen+"\r\n\r\n";
		string message=tmp+entity;
		putMessage(1,"NULL",sockfd,message);
		//char send_buf[1024];
        //sprintf(send_buf,"%s",message.c_str());
        //write(sockfd,send_buf,strlen(send_buf));
	}
}

void Get_dealer(string method,string url,int sockfd)
{
	printf("请求类型：Get\n");
	int len=url.length();
	string tmp="./src";
	if(url.find(".")==string::npos)//url中不存在‘.’
    {
		if(url[len-1]=='/'||url.length()==0)
        {
			tmp+=url+"index.html";
		}
		else tmp+=url+"./index.html";
	}
	else tmp+=url;
	cout<<"请求路径:"<<tmp<<'\n';
	int fd=open(tmp.c_str(),O_RDONLY);
	if(fd>=0)
    {
		int tlen=tmp.length();
    	struct stat stat_buf;
   		fstat(fd,&stat_buf);
		char outstring[1024];
        sprintf(outstring,"Http/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n",stat_buf.st_size);
		//sprintf(outstring,"Http/1.1 200 OK\r\ncontent-length: %d\n\r\n",stat_buf.st_size);
		//sprintf(outstring,"Http/1.1 200 OK\r\ncontent-length: %d\r\n\r\n",stat_buf.st_size);
		//Content-Type: text/html
		string message(outstring);
		//cout<<message<<'\n';
		putMessage(0,tmp,sockfd,message);
		//write(sockfd,outstring,strlen(outstring));
		//sendfile(sockfd,fd,0,stat_buf.st_size);
	}
	else
    {
		Error_dealer(method,url,sockfd);
	}
}

void Post_dealer(string name,string ID,int sockfd)
{
	printf("请求类型：Post\n");
	string entity="<html><title>POST Method</title><body bgcolor=ffffff>\nYour Name: "+name+"\nID: "+ID+"\n<hr><em>Http Web server</em>\n</body></html>\n";
	int ilen=entity.length();
	stringstream ss;
	ss<<ilen;
	string slen;
	ss>>slen;
	string tmp="HTTP/1.1 200 OK\r\ncontent-type: text/html\r\ncontent-length: "+slen+"\r\n\r\n";
	printf("生成消息成功");
	string message=tmp+entity;
	printf("准备放入消息%d",sockfd);
	putMessage(1,"NULL",sockfd,message);
	//char send_buf[1024];
    //sprintf(send_buf,"%s",message.c_str());
    //write(sockfd,send_buf,strlen(send_buf));
}

void Dealer(char* recv_buf,int connfd)
{
	string s_buf;
	bool status;
	status=true;//持久连接标志
	s_buf=string(recv_buf);
	//处理http请求报文
	cout<<s_buf<<'\n';
	while(s_buf.find("HTTP/1.1")!=string::npos||s_buf.find("HTTP/1.0")!=string::npos)//判断s_buf中有没有完整报文
    {
		int request_end_pos=0;//请求除主体外报文尾部         
		if((request_end_pos=s_buf.find("\r\n\r\n"))!=-1)//判断是否有请求体
        {
            string request="";//请求报文
			request_end_pos+=4;
			request=s_buf.substr(0,request_end_pos);//复制报文到request
			int head_end_pos=request.find("Content-Length");
			int entity_pos=request.length();//实体主体起始位置
			if(head_end_pos!=-1)//存在请求体
            {
				string num;
				head_end_pos+=15;
				while(request[head_end_pos]!='\r')
                {
					num+=request[head_end_pos++];
				}
				int entity_len=atoi(num.c_str());
				if((s_buf.length()-request.length())>=entity_len)//有主体
                {
					request+=s_buf.substr(request.length(),entity_len);
					request_end_pos+=entity_len;
                }
                else continue;
            }
            s_buf=s_buf[request_end_pos];//得到完整请求报文
            string method,url;
            request_end_pos=0;
            while(request[request_end_pos]!=' ')
            {
            	method+=request[request_end_pos++];
            }
            if(method!="GET"&&method!="POST")
            {
            	Error_dealer(method,url,connfd);
            	continue;
            }
            ++request_end_pos;
            while(request[request_end_pos]!=' ')
            {
			    url+=request[request_end_pos++];
			}
			++request_end_pos;//提取URL
			if(method=="GET")
            {
				Get_dealer(method,url,connfd);
			}
			else if(method=="POST")
            {                     
				if(url!="/Post_show")
                {
					Error_dealer(method,url,connfd);
					continue;
				}
				string entity=request.substr(entity_pos,request.length()-entity_pos);
				cout<<entity<<'\n';
				int namepos=entity.find("Name"),idpos=entity.find("ID");
				if(namepos==-1||idpos==-1||idpos<=namepos)
                {
					Error_dealer(method,url,connfd);
					continue;
				}
				string name,ID;                        
				name=entity.substr(namepos+5,idpos-namepos-5);
				ID=entity.substr(idpos+3);
				//现在切割出的字符串以------开头
				//int namepos1=name.find("------");
				//int IDpos1=ID.find("------");
				// cout<<"namePos1"<<namepos1<<" IDPos1"<<IDpos1<<'\n';
				// cout<<"Name:"<<name<<"\nID:"<<ID;
				// name=name.substr(0,namepos1);
				// ID=ID.substr(0,IDpos1);
				cout<<"Name:"<<name<<"\nID:"<<ID;
				Post_dealer(name,ID,connfd);
			}
		}
	}
}

void setIndexHtml(){
	
}

struct outPutTask{
	int type;//0为get带有index.html文件，1为post
	string file_path;
	int sockfd;
	string message;
};
//消费队列，消费掉主线程传入Connectionid，由消费者消费掉
const int Size=1000;
int Connections[Size];
sem_t full;
sem_t empty;
sem_t semMutex;
int semFill=0,semUse=0;
outPutTask opt[Size];
sem_t fullOut;
sem_t emptyOut;
sem_t semMutexOut;
int semFillOut=0,semUseOut=0;
pthread_t Thread_id[RunTask];
pthread_t ThreadOut_id;

void putTask(int value){
	Connections[semFill]=value;
	semFill=(semFill+1)%Size;
}

int getTask(){
	int tmp=Connections[semUse];
	semUse=(semUse+1)%Size;
	return tmp;
}

void putTaskOut(int type,string file_path,int sockfd,string message){
	opt[semFillOut].type=type,opt[semFillOut].file_path=file_path,opt[semFillOut].sockfd=sockfd,opt[semFillOut].message=message;
	semFillOut=(semFillOut+1)%Size;
}

outPutTask getTaskOut(){
	outPutTask tmp=opt[semUseOut];
	semUseOut=(semUseOut+1)%Size;
	return tmp;
}

void putMessage(int type,string file_path,int sockfd,string message){
	printf("进入函数%d",sockfd);
	sem_wait(&emptyOut);
	sem_wait(&semMutexOut);
	printf("写入响应");
	putTaskOut(type,file_path,sockfd,message);
	sem_post(&semMutexOut);
	sem_post(&fullOut);
}

/************************************************************************
函数名称：	void *client_process(void *arg)
函数功能：	线程函数,处理客户信息
函数参数：	已连接套接字
函数返回：	无
************************************************************************/
void *client_process(void *arg)
{
	int recv_len = 0;
	char recv_buf[3000] = "";	// 接收缓冲区
	while(true){
		//从数组中取出connection
		sem_wait(&full);
		sem_wait(&semMutex);
		int connfd=getTask();
		sem_post(&semMutex);
		sem_post(&empty);

		printf("处理线程:%d\n",pthread_self());

		while((recv_len = recv(connfd, recv_buf, sizeof(recv_buf), 0)) > 0)
		{
			printf("开始处理%d\n",connfd);
			Dealer(recv_buf,connfd);
			printf("结束处理");
			pthread_mutex_lock(&myLock);
			total++;
			pthread_mutex_unlock(&myLock);
			printf("\ntotal=%d\n\n",total);
			//send(connfd, recv_buf, recv_len, 0); // 给客户端回数据
		}
		printf("client closed!\n");
		close(connfd);//关闭已连接套接字
	}
}


void *output_process(void *arg){
	char send_buf[1024];
	while(true){
		sem_wait(&fullOut);
		sem_wait(&semMutexOut);
		outPutTask TmpOpt=getTaskOut();
		sem_post(&semMutexOut);
		sem_post(&emptyOut);
		// if(TmpOpt.type==0){
		// 	TmpOpt.message+="<html><title>Get the ReUp</title><body bgcolor=ffffff>\n";
		// 	TmpOpt.message += " GOOD! \n";
        // 	TmpOpt.message += "<hr><em>HTTP Web server</em>\n";
        // 	TmpOpt.message += "</body></html>\n";
		// }
		sprintf(send_buf,"%s",TmpOpt.message.c_str());
		write(TmpOpt.sockfd,send_buf,strlen(send_buf));
		//send(TmpOpt.sockfd,send_buf,1024,0);
		if(TmpOpt.type==0){
			int fd=open(TmpOpt.file_path.c_str(),O_RDONLY);
			if(fd>=0){
				int tlen=TmpOpt.file_path.length();
				struct stat stat_buf;
				fstat(fd,&stat_buf);
				sendfile(TmpOpt.sockfd,fd,0,stat_buf.st_size);
			}
		}
	}
}

void create_consumer_thread_pool(){
	sem_init(&empty,0,Size);
	sem_init(&full,0,0);
	sem_init(&semMutex,0,1);
	//创建50个线程去处理connection
	for(int i=0;i<RunTask;i++){
		pthread_create(&Thread_id[i], NULL, client_process, NULL);
	}
}
void create_output_thread(){
	sem_init(&emptyOut,0,Size);
	sem_init(&fullOut,0,0);
	sem_init(&semMutexOut,0,1);
	pthread_create(&ThreadOut_id, NULL, output_process, NULL);
}

//===============================================================
// 语法格式：	void main(void)
// 实现功能：	主函数，建立一个TCP并发服务器
// 入口参数：	无
// 出口参数：	无
//===============================================================
int main(int argc, char *argv[])
{
	int sockfd = 0;//套接字
	int connfd = 0;
	int err_log = 0;
	struct sockaddr_in my_addr;// 服务器地址结构体
	unsigned short port = 80;// 监听端口

	pthread_t thread_id;
	
	pthread_mutex_init(&myMutex, NULL);//初始化互斥锁，互斥锁默认是打开的

	printf("TCP Server Started at port %d!\n", port);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建TCP套接字
	if(sockfd < 0)
	{
		//返回一个socket标识符。小于0为创建错误
		perror("socket error");
		exit(-1);
	}
	
	bzero(&my_addr, sizeof(my_addr));// 初始化服务器地址
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	printf("Binding server to port %d\n", port);
	printf("Binding server to ip %s\n",my_addr.sin_addr.s_addr);//默认绑定本地ip,localhost

        int opt;
        int digit_optind = 0;
        int option_index = 0;
        char *string = "a:b:d";
        static struct option long_options[] =
        {  
            {"ip",required_argument,NULL,'r'},
            {"port",required_argument,NULL,'r'},
            { NULL,0,NULL,0},
        };
        int i=1;
	    //命令行解析工具
        while((opt =getopt_long_only(argc,argv,string,long_options,&option_index))!= -1)
        {  
            if(strcmp(argv[i],"--ip")==0)
            {
               //my_addr.sin_addr.s_addr=inet_addr(optarg);
               //printf("2 Binding server to ip %s\n",my_addr.sin_addr.s_addr);
            }
            if(strcmp(argv[i],"--port")==0)
            {
           
               port=atoi(optarg);
               my_addr.sin_port   = htons(port);
               printf("2 Binding server to port %s\n", optarg);
            }
            printf("opt = %c\t\t",opt);
            printf("optarg = %s\t\t",optarg);
            printf("argv[i] =%s\t\t",argv[i]);
            if(i==1)
            {
                i=optind;
            }
            printf("option_index = %d\n", option_index);
        }


	//将socket和ip_port进行绑定
	err_log = bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
	if(err_log != 0)
	{
		perror("bind");
		close(sockfd);		
		exit(-1);
	}
	
	//对socket进行监听
	err_log = listen(sockfd, 10);
	if( err_log != 0)
	{
		perror("listen");
		close(sockfd);		
		exit(-1);
	}
	
	create_consumer_thread_pool();//创建线程池
	create_output_thread();//创建输出线程

	//监听完毕，等待客户端访问
	printf("Waiting client...\n");
	while(1)
	{
		char cli_ip[INET_ADDRSTRLEN] = "";	       // 用于保存客户端IP地址
		struct sockaddr_in client_addr;		       // 用于保存客户端地址
		socklen_t cliaddr_len = sizeof(client_addr);   // 必须初始化
		//(感觉没什么意义的锁)上锁，在没有解锁之前，pthread_mutex_lock()会阻塞
		//pthread_mutex_lock(&mutex);	
		
		//阻塞进程，等待连接建立
		connfd = accept(sockfd, (struct sockaddr*)&client_addr, &cliaddr_len);   							
		if(connfd < 0)
		{
			perror("accept this time");
			continue;
		}	
		// 打印客户端的 ip 和端口
		//const char *inet_ntop(int af, const void *src, char *dst, socklen_t cat);
		inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);//将数值格式转化为点分十进制的ip地址格式
		//printf("inet_ntop:%s\n",cli_ip);
        //返回值：若成功则为指向结构的指针，若出错则为NULL
		printf("----------------------------------------------\n");
		printf("client ip=%s,port=%d\n", cli_ip,ntohs(client_addr.sin_port));
		//将connection放入队列中
		printf("准备获取信号量\n");
		sem_wait(&empty);
		sem_wait(&semMutex);
		printf("放入任务%d\n",connfd);
		putTask(connfd);
		sem_post(&semMutex);
		sem_post(&full);
	}
	
	close(sockfd);
	
	return 0;
}


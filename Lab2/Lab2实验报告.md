# 云计算Lab2实验报告

## 基本信息

- 实验题目：Your Own HTTP Server
- 实验人员
  - 201808010110 来泽宇
  - 201808010120 曾修平
  - 201808010127 林肖彤
  - 201878020227 潘振鹏
- 完成日期：2020/4/30

## 实验概要

### 1.1 实验概要

超文本传输协议（HTTP）是当今 Internet 上最常用的应用程序协议。像许多网络协议一样，HTTP 使用客户端-服务器模型。HTTP 客户端打开与 HTTP 服务器的网络连接，并发送HTTP 请求消息。HTTP 消息是简单的格式化数据块。所有 HTTP 消息分为两种：请求消息和响应消 息。请求消息请求来自 Web 服务器的操作。响应消息将请求的结果返回给客户端。请求和响应消息都具有相同的基本消息结构。

### 1.2 HTTP服务器功能说明

每个TCP连接只能同时发送一个请求，客户端等待响应，当客户端获得响应时，也许将TCP 连接重新用于新请求（或使用新的 TCP 连接）。这也是普通 HTTP 服务器支持的内容。

- 处理HTTP GET请求
- 处理HTTP POST请求
- 其他请求不做处理

### 1.3 使用主要技术

- c++ socket
- c++ thread

### 1.4 使用多线程提高并发性

在该实验中，使用了三种线程

- 主线程，用于进行初始化，绑定socket，创建其他线程。当其他线程创建完毕后，作为从socket中接收connection的线程，获取到connection后存入到任务队列中
- 工作线程：是一个线程池，里面的每个线程都可以处理一个connection。主动从任务队列中取出任务然后进行处理，处理之后放入结果队列中
- 输出线程：主动从结果队列中获取需要发送的结果，进行结果的返回

### 1.5 指定参数

-- port：指定服务监听的port端口

### 1.6 运行http服务器

创建套接字socket，并将该套接字绑定指定的port端口(默认80)，然后监听该套接字，等待conntection建立，每获取一个连接放入连接队列中，等待工作线程的处理

运行实例：./httpserver  --port 8081(默认端口为80)

### 1.7 请求处理大致流程

- 处理GET请求

  判断报文是否是GET请求，请求路径与HTML页面文件相对应，如果恢复200OK则返回index.html的完整内容，否则返回404 Not Found

- 处理Post

  判断报文是否是Post请求，判断URL是/Post_show，并且键为Name和ID，则回复200 OK和Name-ID对，否则回复404 Not Found

- 其他方法

  回复501 Not Implement

### 1.8 实验环境

- linux 版本

  Linux ubuntu 4.4.0-31-generic #50-Ubuntu SMP Wed Jul 13 00:07:12 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux

- cpu信息

  processor       : 0
  vendor_id       : GenuineIntel
  cpu family      : 6
  model           : 142
  model name      : Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz
  stepping        : 10
  microcode       : 0xe0
  cpu MHz         : 1992.003
  cache size      : 8192 KB
  physical id     : 0
  siblings        : 1
  core id         : 0
  cpu cores       : 1
  apicid          : 0
  initial apicid  : 0
  fpu             : yes
  fpu_exception   : yes
  cpuid level     : 22
  wp              : yes
  flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon nopl xtopology tsc_reliable nonstop_tsc eagerfpu pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch fsgsbase tsc_adjust bmi1 avx2 smep bmi2 invpcid mpx rdseed adx smap clflushopt xsaveopt xsavec arat
  bugs            :
  bogomips        : 3984.00
  clflush size    : 64
  cache_alignment : 64
  address sizes   : 43 bits physical, 48 bits virtual
  power management:

## 主要代码实现

- 主线程，负责绑定socket读出需要处理请求，将未处理的请求放入请求池

  ```c++
  	//监听完毕，等待客户端访问
  	printf("Waiting client...\n");
  	while(1)
  	{
  		char cli_ip[INET_ADDRSTRLEN] = "";// 用于保存客户端IP地址
  		struct sockaddr_in client_addr;// 用于保存客户端地址
  		socklen_t cliaddr_len = sizeof(client_addr);// 必须初始化
          
  		//阻塞进程，等待连接建立
  		connfd = accept(sockfd, (struct sockaddr*)&client_addr, &cliaddr_len);   							
  		if(connfd < 0)
  		{
  			perror("accept this time");
  			continue;
  		}	
  		// 打印客户端的 ip 和端口
  		inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);//将数值格式转化为点分十进制的ip地址格式
  		printf("----------------------------------------------\n");
  		printf("client ip=%s,port=%d\n", cli_ip,ntohs(client_addr.sin_port));
  		sem_wait(&empty);
  		sem_wait(&semMutex);
  		putTask(connfd);
  		sem_post(&semMutex);
  		sem_post(&full);
  	}
  	
  	close(sockfd);
  ```

- 工作线程，从请求池中读出未处理请求，产生响应放入响应池

  ```c++
  void *client_process(void *arg)
  {
  	int recv_len = 0;
  	char recv_buf[3000] = "";
  	while(true){
  		sem_wait(&full);
  		sem_wait(&semMutex);
  		int connfd=getTask();
  		sem_post(&semMutex);
  		sem_post(&empty);
          
  		{
  			recv_len = recv(connfd, recv_buf, sizeof(recv_buf), 0);
  			Dealer(recv_buf,connfd);
  			pthread_mutex_lock(&myLock);
  			total++;
  			pthread_mutex_unlock(&myLock);
  			printf("\ntotal=%d\n\n",total);
  		}
  	}
  }
  
  ```

- 输出线程，从响应池中取出响应写回客户端

  ```c++
  void *output_process(void *arg){
  	char send_buf[1024];
  	while(true){
  		sem_wait(&fullOut);
  		sem_wait(&semMutexOut);
  		outPutTask TmpOpt=getTaskOut();
  		sem_post(&semMutexOut);
  		sem_post(&emptyOut);
          
  		sprintf(send_buf,"%s",TmpOpt.message.c_str());
  		write(TmpOpt.sockfd,send_buf,strlen(send_buf));
          
  		if(TmpOpt.type==0){
  			int fd=open(TmpOpt.file_path.c_str(),O_RDONLY);
  			if(fd>=0){
  				int tlen=TmpOpt.file_path.length();
  				struct stat stat_buf;
  				fstat(fd,&stat_buf);
  				sendfile(TmpOpt.sockfd,fd,0,stat_buf.st_size);
  			}
  		}
  		close(TmpOpt.sockfd);
  	}
  }
  ```

## 性能测试

- **修改不同的线程数，1000个用户并发产生10000个请求(并发量远远大于服务器核数)**

  ![cloud-computing/1.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/new_1.png)
  
  ![cloud-computing/1.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/new_2.png)
  可以看出，在不同的工作线程下， 吞吐量的大小没有什么区别，仅存在测试上的误差，其原因主要是性能瓶颈存在于输出线程上，因为输出线程只有一个，并且输出线程承担了io操作，输出线程必须一个一个去处理这些输出请求，即使工作线程已经处理完了这些工作

  所以读入-处理-输出三层结构的缺点是在工作线程增加时不能很好的提升性能，但是我们可以再做另一个实验，让每个工作线程处理完后自己返回，而不是交付给输出线程

- **修改项目结构，每个工作线程处理完后自己返回**

  ![cloud-computing/2.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/new_3.png)
  
  ![cloud-computing/1.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/new_4.png)

    可以看出，当每个线程运后自己返回结果，涉及到每个线程运行完后读取index.html发生io阻塞的问题，当线程数量增大时，吞吐量有所上升且略小于交给输出线程处理的情况，吞吐量上升的原因是是因为当线程增多时，遇到io情况下可以将cpu让给其他线程执行，所以随着线程数增加吞吐量有所上升；这个结构相比于读入-处理-输出三层的吞吐量略小，是因为进程对于socket的io近乎于满载，那么如果自己处理响应，则还要等待io写入，如果交给输出线程处理，不需要等待io写入直接可以处理下一个请求，这样提升了请求处理的效率，但是实际上并没有提高太多的性能，因为向socket的写入实际上是性能瓶颈，并且几乎已经到达了写入速度极限

- 进行压力测试，选择性能表现较好的1000个线程

  ![cloud-computing/1.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/3.png)

  选择20000并发量进行100000次请求，很遗憾在接近90000次时发生了超时，检查日志文件发现

  ![cloud-computing/1.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/4.png)

  是因为打开的socket过多而造成了错误，所以是因为服务器自身的socket连接上限过小造成了发生超时错误

  ![cloud-computing/1.png at master · setsuna-Lzy/cloud-computing (github.com)](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab2/picture/5.png)

  在请求达到5w时截断，可以看到当并发量为20000的时候，可以达到7000+的吞吐率


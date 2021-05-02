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

## 性能测试

- 设置不同的cpu核数，测试可以承受的http请求压力

  ![](1.png)

  通过虚拟机中的设置修改cpu的核数，修改结果可以通过less /proc/cpuinfo观察到修改后的cpu核数。通过wrk工具测试30s内服务器可以处理的请求数，测试五组数据取平均值

  结果如上图折线图所示，当cpu核心数为1时，每秒的请求处理数最多，当核数增加后，每秒的请求数明显降低

  理论上来说，CPU核数越多，处理请求越快，每秒处理的请求数也会越多，但是实际测试中，cpu核数为1时性能确实最好的。通过分析发现这和我们的测试方式有关，我们只开启了单个用户线程串行访问，使得线程池只有一个tcp连接，也就只有一个工作线程进行处理，同时有一个输出线程输出结果，这两个也是并行的，单核cpu效果最好，多核cpu因为有着上下文切换的原因效果较差

- 设置压力测试的并发量，测试可以承受的http请求压力

  ![](2.png)
  
  为了验证上一个问题的猜想，控制cpu的核数为1，修改客户端的并发量
  
  同样是测试30s服务器处理请求的个数，分别测试客户端并发量1-5的情况，结果如上图所示
  
  当只有一个客户端时，服务器性能最好，随着客户端并行程度的提高，服务器性能有所下降
  
  因为当多用户并发时，每个用户建立一个tcp连接，有一个工作线程去处理这个tcp连接，对于单核cpu来说，多线程存在着上下文切换的开销，使得最终处理请求的个数变慢


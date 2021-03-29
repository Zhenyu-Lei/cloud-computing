# 云计算Lab1实验报告

## 0.组员及日期

2021/03/21

- 201808010110 来泽宇
- 201808010120 曾修平
- 201878020227 潘振鹏
- 201808010127 林肖彤

## 1. 实验概要

​	多线程编程是高性能编程的技术之一，实验1将针对数独求解问题比较多线程与单线程的性能差异、同一功能不同代码实现的性能差异以及多线程在不同硬件环境下的性能差异。

## 2.实验简述

### 2.1实验准备

​	从github仓库中clone Lab1的代码，其中包括了四种算法。阅读代码可以得出大致求解过程：①选择输入文件->②选择算法->③输入矩阵puzzle->④求解问题，将答案填入board中

### 2.2实验过程

**明确修改方向：**首先需要确定需要修改的方向，原来的问题是单线程的，现在要变成多线程求解，具体是申请n个工作线程，对于m个问题，每个线程求解一个问题，当处理完一个问题后，去申请下一个线程。最后处理完所有问题

**修改为多线程**

① 创建线程

```c
void Create()
{
    for (int i = 0; i < RunTask; i++)//根据一个#define RunTask的变量，创建RunTask个线程去
    {
        /*
		pthread_create是创建线程的函数，包括四个变量
		1 第一个参数是指向线程标识符的指针，唯一识别这个线程
		2 第二个参数用来设置线程的属性
		3 第三个是线程运行函数的地址
		4 第四个参数是函数的参数
	*/
	if (pthread_create(&th[i], NULL, Run,(void*)&Tdata[i])!= 0)
        {
            perror("pthread_create failed");
            exit(1);
        }
    }
}
```

② 销毁线程

```c
void End()
{
    //当所有问题解决完后，回收线程资源
    for (int i = 0; i < RunTask; i++)
        pthread_join(th[i], NULL);
}
```

③ 获取任务

```c
int getJob()
{
    int currentJobID = 0;
    pthread_mutex_lock(&mutex);//加锁，保护不会取出相同的任务
    if (Sid>=total)//如果任务已经被取完了，那么直接返回-1
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    currentJobID=Sid++;//获取新的任务
    if (currentJobID==total-1)//如果这个获取的任务是最后一个了，就标记所有问题都已经被求解完了
    {
        shutdown=true;
    }
    pthread_mutex_unlock(&mutex);
    return currentJobID;//返回去除的问题序号
}
```

④ 线程创建

```c
	Create();///创建线程去求解问题
    while (fgets(sudoku[total], sizeof sudoku, fp) != NULL)
    {
        if (strlen(sudoku[total]) >= N)
        {
            pthread_mutex_lock(&mutex);
            total++;//每读入一个矩阵，就将total+1，
            pthread_mutex_unlock(&mutex);
        }
    }
    End();//销毁所有线程
```

以上就是一些关于线程的工具函数，接下来就是使用上面的函数完成多线程的求解。写在Run函数中，是创建线程的执行函数

⑤ 线程执行

```c
void *Run(void *args)//求解一个矩阵
{
    bool (*solve)(int,int[],int[],int)=solve_sudoku_dancing_links;//使用dancing_links算法
    /**
    	在老师给出的算法中，board和spaces都是一个全局变量，这是因为全局变量不需要使用函数传递
    	但是在多线程的环境下，由于多个线程同时对board填入结果，所以要考虑同步安全的问题
    	方法1：board仍然是全局的，但是我们修改board的时候直接修改，而是通过一个函数修改，修改前后加锁，这样是对多线程共享资源		的通用处理方法，这样可以避免多个线程同时进行操作，但是效率大大下降，无法得到多线程的性能提升
    	方法2：不将资源共享，在此采用的就是这个方法，每个线程都有自己的board，每个线程单独求解，发挥多线程的性能
    */
    int board[N];
	int spaces[N];
	int nspaces;
    while (!shutdown)
    {
        int id=getJob();//只要所有问题没有全都被求解完，就不停的切换下一个问题
        if (id==-1)//如果无法获取新的任务了，就结束循环
            break;
        input(sudoku[id],board,spaces,nspaces);//将问题进行输入
        if (solve(0,board,spaces,nspaces))//求解问题
        {
            if (!solved(board))
                assert(0);
            for (int i=0;i<N;i++)//将答案记录到Answer中，由于多线程问题不停切换，所以离线处理
                Answer[id][i] = char('0' + board[i]);
        }
        else
        {
            printf("No: %d,无解\n", id);
        }
    }
}
```

⑥ 对dance函数进行修改：将所有的全局变量变为参数传入即可，其余不进行改变

### 2.3 实验结果

![](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab1/picture/result1.png)

可以输出结果和用时

![](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab1/picture/result2.png)

选用了四个测试文件test1,test10,test20,test1000，如果不输出结果(1000的结果太多了)，可以观察到每个测试文件的平均消耗时间

## 3.性能分析

性能分析选择个四方面，在不同的输入规模下的时间开销

设备信息

- 单线程的dance算法
- 多线程资源共享的dance算法
- 多线程资源独享的dance算法
- 单线程资源独享dance算法的不同线程数量

### 3.1 实验环境

Intel(R) Core(TM) id-8300H CPU @ 2.30GHz 2.30GHz 8G 64位

### 3.2数据对比 1

**首先我们比较单线程，多线程共享，多线程独享的情况(数据是处理n个数据的总时间)**

![](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab1/picture/chart1.png)

![](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab1/picture/chart1.png)

- 多线程共享：

  ```c
          pthread_mutex_lock(&mutex2);
          input(sudoku[id]);
          if (solve(0))
          {
              if (!solved())
                  assert(0);
              for (int i=0;i<N;i++)
                  Answer[id][i] = char('0' + board[i]);
          }
          else
          {
              printf("No: %d,无解\n", id);
          }
          pthread_mutex_unlock(&mutex2);
  ```

  在每次求解问题前后加锁，防止出现冲突，每个资源都被锁保护了起来

- 多线程独享：即为实验过程中给出的Run函数

**结果分析：**

**综合来看，多线程独享>单线程>多线程共享，具体来说：**

- **对于多线程共享，性能是最差的**，可以说是集合了所有的缺点。首先，因为是多线程的，所以增加了线程销毁、创建和切换的开销，但是由于资源是锁起来的，不能同时进行，任务仍然是串行求解，所以没有利用起来多线程并行的优点。
- 对于多线程独享和单线程来说，各有优劣。具体来说，**多线程具有最好的性能**，因为问题是并行求解的，最大发挥了CPU的性能，。但是**在数据量小于10的情况下，单线程却具有更好性能**，这是因为多线程需要有线程创建和销毁的开销，这种开销是一次性的，当数据量够大时，可以忽略不记，但是数据量小时却增加了运行时间

所以，当我们想具有最好的性能时，应该采用多线程去求解每个任务，这时任务的数据最好是互相独立的，不会产生同步问题。

### 3.3数据对比2

**我们可以选择不同数量的线程去求解问题，具体是修改头文件中定义的线程数，在test1000上进行测试**

![](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab1/picture/data2.png)

![](https://github.com/setsuna-Lzy/cloud-computing/blob/master/Lab1/picture/data2.png)

可以观察到，在10-30左右的性能是最好的，实验环境是8核cpu，我们想要达到的目标是将cpu的所有核心利用起来，使得每个核心都在运行线程，取得最好的性能。当线程数为1000时效果很差，虽然所有核心都在运行，但是1000个线程造成了大量的上下文切换，使得性能极度下降。

所以我们在选择线程数量的时候，尽量靠经cpu核数，使得cpu满载并且不会造成大量上下文切换

## 4.实验思考与心得

本次实验是将一个单线程任务修改为多线程任务，需要使用c语言pthread库。多线程的核心就是同步问题，在这个问题中，对于任务的分派，我们必须维护一个全局的任务id记录，这就需要加锁维护这个数据；对于全局的数组同步问题，我们避免了这一问题的发生，将每个资源独享，成勋运行中内存中同时存在着多个矩阵数组，但是他们都是互不干扰的。

这是第一次尝试将多线程真正应用到实际问题求解中，感受到了多线程的性能优势，虽然也感受到了维护一个多线程程序的困难，需要巧妙的设计数据和线程的管理。

最后是实验的教训，在刚开始，简单把任务分配给每个线程，但是资源是共享的，程序报错，因为大量线程同时修改一块数据，造成各种奇怪问题。收获了教训后，选择了加锁和线程独享两种方式都获得了答案。在最后，分享一个小tip，**锁一定要初始化，不然不生效QAQ**，被这个锁不生效的问题困扰了好久

```c
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;//锁的初始化
```












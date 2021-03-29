#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "sudoku.h"
#define RunTask 5 

char Answer[TaskNum][128];
char sudoku[TaskNum][128];

int total=0;
int Sid=0;
bool shutdown=false;

int Tdata[TaskNum];
pthread_t th[TaskNum];
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

int getJob()
{
    int currentJobID = 0;
    pthread_mutex_lock(&mutex);
    if (Sid>=total)
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    currentJobID=Sid++;
    if (currentJobID==total-1)
    {
        shutdown=true;
    }
    pthread_mutex_unlock(&mutex);
    return currentJobID;
}

void *Run(void *args)
{
    bool (*solve)(int,int[],int[],int)=solve_sudoku_dancing_links;
    int board[N];
	int spaces[N];
	int nspaces;
    while (!shutdown)
    {
        int id=getJob();
        if (id==-1)
            break;
        input(sudoku[id],board,spaces,nspaces);
        if (solve(0,board,spaces,nspaces))
        {
            if (!solved(board))
                assert(0);
            for (int i=0;i<N;i++)
                Answer[id][i] = char('0' + board[i]);
        }
        else
        {
            printf("No: %d,нч╫Б\n", id);
        }
    }
}

void Create()
{
    for (int i = 0; i < RunTask; i++)
    {
		if (pthread_create(&th[i], NULL, Run,(void*)&Tdata[i])!= 0)
        {
            perror("pthread_create failed");
            exit(1);
        }
    }
}

void End()
{
    for (int i = 0; i < RunTask; i++)
        pthread_join(th[i], NULL);
}

void result()
{
    for(int i=0;i<total; i++)
        printf("%s\n",Answer[i]);
}

int64_t getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[])
{
    init_neighbors();
    int64_t start = getTime();
    FILE *fp = fopen(argv[1], "r");
    Create();
    while (fgets(sudoku[total], sizeof sudoku, fp) != NULL)
    {
        if (strlen(sudoku[total]) >= N)
        {
            pthread_mutex_lock(&mutex);
            total++;
            pthread_mutex_unlock(&mutex);
        }
    }
    End();
    result();
    int64_t end = getTime();
    double second = (end - start) / 1000000.0;
    printf("%f sec %f ms each %d\n", second, 1000 * second / total, total);
    return 0;
}

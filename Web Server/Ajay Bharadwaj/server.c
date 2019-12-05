#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#define MESSAGE_SIZE 200

long double initTime;

struct connInfo
{
	int fileDesc;
	long double arriveTime;
	long double startTime;
	long double finishTime;
};

struct Node
{
	struct connInfo data;
	struct Node* next;
};

struct Queue
{
	int numElem;
	struct Node* front;
	struct Node* end;
};

struct threadPool
{
	struct Queue queue;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

	unsigned int numThreads;
	pthread_t* threads;
};
struct threadPool pool;

void queueInit(struct Queue*);
int queueEmpty(struct Queue*);
void queueEnqueue(struct Queue*, struct connInfo);
struct connInfo queueDequeue(struct Queue*);

void threadPoolInit(int);
void threadPoolEnqueue(struct connInfo);
struct connInfo threadPoolDequeue();

void reverse(char*, int);
int intToStr(int, char[], int);
void ftoa(long double, char*, int);

long double getTime();

void* handleConnection(void*);

void queueInit(struct Queue* queue)
{
	queue->numElem = 0;
	queue->front = NULL;
	queue->end = NULL;
}

int queueEmpty(struct Queue* queue)
{
	if (queue->numElem > 0)
	{
		return 0;
	}

	return 1;
}

void queueEnqueue(struct Queue* queue, struct connInfo elem)
{
	if (queueEmpty(queue))
	{
		queue->front = (struct Node*) malloc(sizeof(struct Node));
		queue->front->data = elem;
		queue->front->next = NULL;

		queue->end = queue->front;
		(queue->numElem)++;
		return;
	}

	queue->end->next = (struct Node*) malloc(sizeof(struct Node));
	queue->end->next->data = elem;
	queue->end->next->next = NULL;
	queue->end = queue->end->next;
	(queue->numElem)++;
}

struct connInfo queueDequeue(struct Queue* queue)
{
	(queue->numElem)--;

	struct connInfo elem = queue->front->data;

	struct Node* temp = queue->front;
	queue->front = queue->front->next;
	free(temp);

	if (queueEmpty(queue))
	{
		queue->end = queue->front;
	}

	return elem;
}

void threadPoolInit(int numThreads)
{
	queueInit(&pool.queue);

	pthread_mutex_init(&pool.mutex, NULL);
	pthread_cond_init(&pool.cond, NULL);

	pool.numThreads = numThreads;
	pool.threads = (pthread_t*) malloc(numThreads * sizeof(pthread_t));

	int* arg;
	for (int a = 0; a < numThreads; a++)
	{
		arg = (int*) malloc(sizeof(int));
		*arg = a;
		pthread_create(&pool.threads[a], NULL, handleConnection, arg);
	}
}

void threadPoolEnqueue(struct connInfo elem)
{
	pthread_mutex_lock(&pool.mutex);

	queueEnqueue(&pool.queue, elem);

	pthread_mutex_unlock(&pool.mutex);
	pthread_cond_signal(&pool.cond);
}

struct connInfo threadPoolDequeue()
{
	pthread_mutex_lock(&pool.mutex);

	while (queueEmpty(&pool.queue))
	{
		pthread_cond_wait(&pool.cond, &pool.mutex);
	}

	struct connInfo elem = queueDequeue(&pool.queue);

	pthread_mutex_unlock(&pool.mutex);

	return elem;
}

void reverse(char* str, int len) 
{ 
    int i = 0;
    int j = len - 1;
    int temp; 
    while (i < j)
    { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 

int intToStr(int x, char str[], int d) 
{ 
    int i = 0; 
    while (x)
    { 
        str[i++] = (x % 10) + '0'; 
        x /= 10; 
    } 
  
    while (i < d) 
    {
        str[i++] = '0'; 
    }
  
    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
} 
  
void ftoa(long double n, char* res, int afterpoint) 
{ 
    int ipart = (int) n; 
  
    long double fpart = n - (long double) ipart; 

    int i = intToStr(ipart, res, 0); 

    if (afterpoint != 0)
    { 
        res[i] = '.';
        fpart = fpart * pow(10, afterpoint); 
  
        intToStr((int) fpart, res + i + 1, afterpoint); 
    } 
}

long double getTime()
{
	struct timeval time;
	gettimeofday(&time, NULL);

	long double ans = (long double) time.tv_sec * 1000 + (long double) time.tv_usec / 1000;
	ans /= 1000;

	return ans;
}

void* handleConnection(void* arg)
{
	int threadId = *((int*) arg);

	while (1)
	{
		struct connInfo info = threadPoolDequeue();
		info.startTime = getTime() - initTime;
		int connfd = info.fileDesc;

		//RECEIVING THE NAME OF THE FILE
		char fileName[MESSAGE_SIZE];
		read(connfd, fileName, MESSAGE_SIZE);

		FILE* file = fopen(fileName, "r");

		//FILE DOES NOT EXIST
		char success = 1;
		if (!file)
		{
			success = 0;
			write(connfd, &success, 1);
			close(connfd);
			pthread_exit(NULL);
		}

		//SEND THE CONTENTS OF THE FILE
		write(connfd, &success, 1);
		char output[MESSAGE_SIZE];
		while (fgets(output, MESSAGE_SIZE, file))
		{
			write(connfd, output, sizeof(output));
		}

		//SEND STATISTICS REGARDING THE REQUEST
		char message[MESSAGE_SIZE];

		strcpy(message, "\n\nThread that handled the request: ");
		write(connfd, message, sizeof(message));
		intToStr(threadId, message, 0);
		write(connfd, message, sizeof(message));

		strcpy(message, "\nArrival time: ");
		write(connfd, message, sizeof(message));
		ftoa(info.arriveTime, message, 4);
		write(connfd, message, sizeof(message));

		strcpy(message, "\nStart time: ");
		write(connfd, message, sizeof(message));
		ftoa(info.startTime, message, 4);
		write(connfd, message, sizeof(message));

		info.finishTime = getTime() - initTime;
		strcpy(message, "\nFinish time: ");
		write(connfd, message, sizeof(message));
		ftoa(info.finishTime, message, 4);
		write(connfd, message, sizeof(message));

		fclose(file);
		close(connfd);	
	}
	
	pthread_exit(NULL);
}

int main()
{
	//SETTING THE PORT NUMBER
	int SERV_PORT;
	printf("Enter the port number: ");
	scanf("%d", &SERV_PORT);

	///CREATING A SOCKET
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);

	//BINDING THE SOCKET
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
	{
		printf("Error During Binding\n");
		exit(1);
	}

	int NUM_THREADS;
	printf("Enter the number of worker threads: ");
	scanf("%d", &NUM_THREADS);
	threadPoolInit(NUM_THREADS);

	struct sockaddr_in cliaddr;
	unsigned int clilen;
	int connfd;
	struct connInfo* elem;
	initTime = getTime();
	while (1)
	{
		//LISTENING FOR A CONNECTION
		if (listen(listenfd, 1) == -1)
		{
			printf("Error During Listening\n");
			exit(1);
		}
		printf("Listening\n");

		//ACCEPTING THE CONNECTION
		clilen = sizeof(cliaddr);
		bzero(&cliaddr, clilen);
		connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
		printf("Connected\n");

		elem = (struct connInfo*) malloc(sizeof(struct connInfo));
		elem->fileDesc = connfd;
		elem->arriveTime = getTime() - initTime;
		threadPoolEnqueue(*elem);
	}
}
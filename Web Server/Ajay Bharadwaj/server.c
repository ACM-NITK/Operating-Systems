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

struct connInfo
{
	int fileDesc;
	long double time;
};

void* handleConnection(void* arg)
{
	struct connInfo info = *((struct connInfo*) arg);
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
	info.time = getTime() - info.time;
	char timeTaken[MESSAGE_SIZE];
	ftoa(info.time, timeTaken, 4);
	success = '\n';
	write(connfd, &success, 1);
	write(connfd, timeTaken, sizeof(timeTaken));

	fclose(file);
	close(connfd);
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

	struct sockaddr_in cliaddr;
	unsigned int clilen;
	int connfd;
	struct connInfo* arg;
	pthread_t threadId;
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

		arg = (struct connInfo*) malloc(sizeof(struct connInfo));
		arg->fileDesc = connfd;
		arg->time = getTime();
		pthread_create(&threadId, NULL, handleConnection, arg);
	}
}
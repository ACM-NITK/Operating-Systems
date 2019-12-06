#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define MESSAGE_SIZE 200

int SERV_PORT;
char* connLoc;
char fileName[MESSAGE_SIZE];

int callStatus = 1;
pthread_mutex_t mutex;
pthread_cond_t cond;

void* makeConnectionConcur()
{
	//CREATING A SOCKET
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//CONNECTING TO THE SERVER
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, connLoc, &servaddr.sin_addr);
	if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
	{
		printf("Error during connecting\n");
		close(sockfd);
		pthread_exit(NULL);
	}
	printf("Connected\n");

	//SENDING THE FILE NAME TO THE SERVER
	write(sockfd, fileName, sizeof(fileName));

	char success;
	read(sockfd, &success, 1);

	//IF THE FILE DOES NOT EXISTS ON THE SERVER
	if (!success)
	{
		printf("File does not exist\nClosing the connection\n\n");
		close(sockfd);
		pthread_exit(NULL);
	}

	//OBTAINING THE FILE FROM THE SERVER ALONG WITH STATISTICS ABOUT THE REQUEST
	char input[MESSAGE_SIZE];
	while (read(sockfd, input, MESSAGE_SIZE))
	{
		printf("%s", input);
	}

	printf("\n\nClosing the connection\n\n");
	close(sockfd);
	pthread_exit(NULL);
}

void* makeConnectionSerial()
{
	pthread_mutex_lock(&mutex);
	callStatus = 0;
	pthread_mutex_unlock(&mutex);

	//CREATING A SOCKET
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//CONNECTING TO THE SERVER
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, connLoc, &servaddr.sin_addr);
	if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
	{
		printf("Error during connecting\n");
		close(sockfd);
		pthread_exit(NULL);
	}
	printf("Connected\n");

	//SENDING THE FILE NAME TO THE SERVER
	write(sockfd, fileName, sizeof(fileName));

	pthread_mutex_lock(&mutex);
	callStatus = 1;
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);

	char success;
	read(sockfd, &success, 1);

	//IF THE FILE DOES NOT EXISTS ON THE SERVER
	if (!success)
	{
		printf("File does not exist\nClosing the connection\n\n");
		close(sockfd);
		pthread_exit(NULL);
	}

	//OBTAINING THE FILE FROM THE SERVER ALONG WITH STATISTICS ABOUT THE REQUEST
	char input[MESSAGE_SIZE];
	while (read(sockfd, input, MESSAGE_SIZE))
	{
		printf("%s", input);
	}

	printf("\n\nClosing the connection\n\n");
	close(sockfd);
	pthread_exit(NULL);
}

int main(int argc, char** argv)
{
	//SETTING THE PORT NUMBER
	printf("Enter the port number: ");
	scanf("%d", &SERV_PORT);

	int NUM_THREADS;
	printf("Enter the number of worker threads: ");
	scanf("%d", &NUM_THREADS);

	char TYPE[MESSAGE_SIZE];
	printf("Enter '1' for Concurrent Requests and '2' for Serial Requests: ");
	scanf("%s", TYPE);

	printf("Enter the name of the file to be requested from the server: ");
	scanf("%s", fileName);

	connLoc = argv[1];

	pthread_t threadID[NUM_THREADS];
	char choice[MESSAGE_SIZE] = "Y";

	if (!strcmp(TYPE, "1"))
	{
		while (strcmp(choice, "n") && strcmp(choice, "N"))
		{
			for (int a = 0; a < NUM_THREADS; a++)
			{
				pthread_create(&threadID[a], NULL, makeConnectionConcur, NULL);
			}

			for (int a = 0; a < NUM_THREADS; a++)
			{
				pthread_join(threadID[a], NULL);
			}

			printf("Enter 'n' or 'N' if you wish to exit: ");
			scanf("%s", choice);
			printf("\n");
		}
	}

	else if (!strcmp(TYPE, "2"))
	{
		pthread_mutex_init(&mutex, NULL);
		pthread_cond_init(&cond, NULL);

		while (strcmp(choice, "n") && strcmp(choice, "N"))
		{
			for (int a = 0; a < NUM_THREADS; a++)
			{
				pthread_mutex_lock(&mutex);
				while (callStatus != 1)
				{
					pthread_cond_wait(&cond, &mutex);
				}
				pthread_mutex_unlock(&mutex);

				pthread_create(&threadID[a], NULL, makeConnectionSerial, NULL);
			}

			for (int a = 0; a < NUM_THREADS; a++)
			{
				pthread_join(threadID[a], NULL);
			}

			printf("Enter 'n' or 'N' if you wish to exit: ");
			scanf("%s", choice);
			printf("\n");
		}
	}

	else
	{
		printf("Incorrect Type\n");
	}
}
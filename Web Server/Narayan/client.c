#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <string.h>

#include <pthread.h>

int port_no;
char *file_name;

void *thread_worker(void *arg)
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        printf("Couldnt establish a socket\n");
        pthread_exit(NULL);
    }

    //address of the server to connect based on the port given (argv[1])
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);
    server_address.sin_addr.s_addr = INADDR_ANY;

    //connect to the server
    int connection_status = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1)
    {
        printf("Couldnt connect\n");
        pthread_exit(NULL);
    }
    else
    {
        printf("Connected to the server\n");
    }
    // send the file name (arg[2]) to the server
    int write_info = write(client_socket, file_name, sizeof(file_name));
    if (write_info == -1)
    {
        printf("Error while writing....Try again\n");
        pthread_exit(NULL);
    }
    else
    {
        printf("Wrote to the server\n");
    }

    //receive the file from the server
    char server_response[222];
    int receive_info = recv(client_socket, &server_response, sizeof(server_response), 0);

    if (receive_info == -1)
    {
        printf("Error while receiving....Try again\n");
        pthread_exit(NULL);
    }
    printf("The data sent by the server:%s\n", server_response);

    close(client_socket);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incompatible arguments\n");
        exit(1);
    }

    port_no = atoi(argv[1]);
    file_name = argv[2];
    char input;
    int thread_count;

    do
    {
        printf("Enter the number of threads:\n");
        scanf("%d", &thread_count);

        pthread_t thread[thread_count];
        for (int i = 0; i < thread_count; i++)
        {
            pthread_create(&thread[i], NULL, thread_worker, NULL);
        }

        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(thread[i], NULL);
        }

        printf("Continue(Y/N)?:");
        fflush(stdin);
        scanf("\n%c", &input);
    } while (input == 'Y' || input == 'y');
    return 0;
}
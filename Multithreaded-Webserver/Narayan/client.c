#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incompatible arguments\n");
        exit(1);
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        printf("Couldnt establish a socket\n");
    }

    //address of the server to connect based on the port given (argv[1])
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = INADDR_ANY;

    //connect to the server
    int connection_status = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1)
    {
        printf("Couldnt connect\n");
        exit(1);
    }
    else
    {
        printf("Connected to the server\n");
    }
    // send the file name (arg[2]) to the server
    int write_info = write(client_socket, argv[2], sizeof(argv[2]));
    if (write_info == -1)
    {
        printf("Error while writing....Try again\n");
        exit(1);
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
        exit(1);
    }
    printf("The data sent by the server:%s\n", server_response);

    close(client_socket);
    return 0;
}
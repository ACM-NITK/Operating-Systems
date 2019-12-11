#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<sys/wait.h>
#include<termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int server_socket;
struct sockaddr_in server_address;

void func()
{
    pid_t pid = 1;
    while (1)
    {
        struct sockaddr *addr;
        socklen_t addrlen;

        //connect to a new request
        int client_socket = accept(server_socket, addr, &addrlen);
        if (client_socket < 0)
        {
            printf("Connection establishment failed");
            continue;
        }
        else
        {
            printf("Connected\n");
        }
        //receive the file_name from the client
        char file_name[250];
        int receieve_status = recv(client_socket, file_name, sizeof(file_name), 0);
        if (receieve_status < 0)
        {
            printf("Couldnt receive the file name...Try again\n");
        }
        else
        {
            //run two concurrent functions, one(parent) for listening more requests and the other(child) for answering the request
            printf("Received the file name\n");
            pid = fork();
        }
        if (pid < 0)
        {
            printf("\nError during fork\n");
            break;
        }
        else if (pid == 0)
        {
            // child process
            FILE *fp = fopen(file_name, "r");
            if (!fp)
            {
                printf("File not found\n");
                char server_response[20] = "File not found\n";
                send(client_socket, server_response, sizeof(server_response), 0);
                exit(1);
            }

            fseek(fp, 0L, SEEK_END);
            int lSize = ftell(fp);
            rewind(fp);

            /* allocate memory for entire content */
            char *server_response = (char *)malloc(lSize);
            if (!server_response)
                fclose(fp), printf("memory alloc failed\n"), exit(1);

            /* copy the file into the buffer */
            if (fread(server_response, lSize, 1, fp) != 1)
                fclose(fp), free(server_response), printf("entire read failed\n"), exit(1);

            fclose(fp);

            //send the file content to the client
            int send_status = send(client_socket, server_response, int(strlen(server_response)), 0);
            if (send_status < 0)
            {
                printf("Sending failed...try again\n");
                exit(1);
            }
            else
            {
                printf("Sent %s\n", file_name);
            }
            close(client_socket);
            free(server_response);
            exit(0);
        }
        else
        {
            // parent process
	    signal(SIGCHLD,SIG_IGN); 
            continue;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incompatible arguments\n");
        exit(1);
    }

    //establish a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        printf("Couldnt establish a socket\n");
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = INADDR_ANY;

    //bind the socket with the address given
    int binding_status = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (binding_status < 0)
    {
        printf("Couldnt bind to the given server address, try again with a different port\n");
        exit(1);
    }

    //limit the server requests to 5 at a time
    int listen_status = listen(server_socket, 5);
    if (listen_status < 0)
    {
        printf("listen() failed\n");
        exit(1);
    }

    func();

    close(server_socket);
    return 0;
}

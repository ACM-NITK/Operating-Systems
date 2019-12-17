#define __TPOOL_H__

#include <stdbool.h>
#include <stddef.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

//for sockets
#include <sys/types.h>
#include <sys/socket.h>

//for different socket architectures
#include <netinet/in.h>

#include <pthread.h>

#include <string.h>

//for kbhit()
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, 4, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

struct work_list
{
    char *file_name;
    int client_socket;
    struct work_list *next;
};

struct tpool
{
    struct work_list *start, *end; //start, end of the unassigned-work-linked-list
    pthread_mutex_t mutex;
    pthread_cond_t none_working;    //if none of the threads are working on a specific client
    pthread_cond_t to_be_processed; //if there is any unassigned work
    int working_count;              //number of threads which are catering clients
    int thread_count;               //number of alive threads
    bool stop;                      //to be true when 'Q' is pressed, used for peacefully destroying all the thread-workers
};

typedef struct tpool tpool_t;
typedef struct work_list work_list_t;

tpool_t *info;

//acquires the mutex, pushes the new work
void push(char file[], int client_socket)
{
    work_list_t *curr_node = (work_list_t *)malloc(sizeof(work_list_t));
    curr_node->file_name = file;
    curr_node->client_socket = client_socket;
    curr_node->next = NULL;

    pthread_mutex_lock(&(info->mutex));
    if (info->start == NULL)
    {
        info->start = info->end = curr_node;
    }
    else
    {
        info->end->next = curr_node;
        info->end = curr_node;
    }
    if (info->end != NULL)
    {
        printf("Pushed %s succesfully\n", info->end->file_name);
        work_list_t *curr = info->start;
        printf("List is: ");
        while (curr != NULL)
        {
            printf("%s  -->  ", curr->file_name);
            curr = curr->next;
        }
        printf("\n");
    }
    else
    {
        printf("Wrong with push \n");
    }
    pthread_cond_broadcast(&(info->to_be_processed));
    pthread_mutex_unlock(&(info->mutex));
}

//before calling pop, we should have acquired the mutex and verified the queue is not empty
work_list_t *pop()
{
    //we already know that queue is not empty while calling this function
    work_list_t *return_node = info->start;
    info->start = info->start->next;
    printf("popped %s\n", return_node->file_name);
    if (info->start == NULL)
    {
        printf("list empty after popping\n");
    }
    else
    {
        work_list_t *curr = info->start;
        printf("List is : ");
        while (curr != NULL)
        {
            printf("%s  -->  ", curr->file_name);
            curr = curr->next;
        }
        printf("\n");
    }
    return return_node;
}

//this is the main work of our program: send the file to the specified client
//this is called by a thread_worker for whom the client is assigned to
void func(char *file_name, int client_socket)
{

    FILE *fp = fopen(file_name, "r");
    if (!fp)
    {
        printf("File not found\n");
        char server_response[20] = "File not found\n";
        send(client_socket, server_response, sizeof(server_response), 0);
        free(file_name);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    int lSize = ftell(fp);
    rewind(fp);

    /* allocate memory for entire content */
    char *server_response = (char *)malloc(lSize + 1);
    if (!server_response)
    {
        fclose(fp), printf("memory alloc failed\n");
        free(file_name);
        return;
    }

    /* copy the file into the buffer */
    if (fread(server_response, lSize, 1, fp) != 1)
    {
        fclose(fp), free(server_response), printf("entire read failed\n");
        free(file_name);
        return;
    }
    fclose(fp);
    //send the file content to the client
    int send_status = send(client_socket, server_response, lSize + 1, 0);
    if (send_status < 0)
    {
        printf("Sending failed...try again\n");
        free(file_name);
        return;
    }
    else
    {
        printf("Sent %s\n", file_name);
    }
    close(client_socket);
    free(file_name);
    free(server_response);
}

//this is the fuction which contains an infinite loop which each thread will be running on
//the thread workers sleep in function call pthread_cond_wait, until there is an unassigned work
static void *tpool_worker(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&(info->mutex));
        if (info->stop)
        {
            pthread_mutex_unlock(&(info->mutex));
            break;
        }
        if (info->start == NULL)
        {
            pthread_cond_wait(&(info->to_be_processed), &(info->mutex));
        }
        info->working_count++;

        //could happen on info->stop==true, or when the unassigned work was taken by some other thread

        if (info->start != NULL)
        {
            work_list_t *work = pop();
            pthread_mutex_unlock(&(info->mutex));
            func(work->file_name, work->client_socket);
            free(work);
            pthread_mutex_lock(&(info->mutex));
        }

        info->working_count--;
        if (!info->stop && info->working_count == 0 && info->start == NULL)
        {
            pthread_cond_signal(&(info->none_working));
        }
        pthread_mutex_unlock(&(info->mutex));
    }

    //after coming out of while loop, this thread is destroyed
    pthread_mutex_lock(&(info->mutex));
    info->thread_count--;
    if (info->thread_count == 0)
        pthread_cond_signal(&(info->none_working));
    pthread_mutex_unlock(&(info->mutex));
    return NULL;
}

//creates the specified number of threads, assigns each thread to tpool_worker
tpool_t *tpool_create(int num)
{
    pthread_t thread;

    info = (tpool_t *)malloc(sizeof(tpool_t));
    info->thread_count = num;

    pthread_mutex_init(&(info->mutex), NULL);
    pthread_cond_init(&(info->to_be_processed), NULL);
    pthread_cond_init(&(info->none_working), NULL);

    info->start = info->end = NULL;

    for (int i = 0; i < num; i++)
    {
        pthread_create(&thread, NULL, tpool_worker, info);
        pthread_detach(thread);
    }

    return info;
}
void tpool_wait()
{
    pthread_mutex_lock(&(info->mutex));
    if (info == NULL)
        return;
    while (1)
    {
        if ((!info->stop && info->working_count != 0) || (info->stop && info->thread_count != 0))
        {
            pthread_cond_wait(&(info->none_working), &(info->mutex));
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&(info->mutex));
}

/*
    1.gets the mutex
    2.destroys the unassigned-work-inked-list
    3.info->stop=true
    4destroys everything when thread-count is reduced to zero
*/
void tpool_destroy()
{
    pthread_mutex_lock(&(info->mutex));
    work_list_t *curr_node = info->start;
    while (curr_node != NULL)
    {
        work_list_t *temp = curr_node->next;
        free(curr_node);
        curr_node = temp;
    }
    info->stop = true;

    //so that the threads no more wait thinking some other requests are going to come
    pthread_cond_broadcast(&(info->to_be_processed));
    pthread_mutex_unlock(&(info->mutex));
    tpool_wait();
    pthread_mutex_destroy(&(info->mutex));
    pthread_cond_destroy(&(info->to_be_processed));
    pthread_cond_destroy(&(info->none_working));

    free(info);
}

int server_socket;
struct sockaddr_in server_address;

//the main thread executes this and adds work into the unassigned-work-linked-list
void driver()
{
    while (1)
    {
        if (kbhit())
        {
            char ch;
            scanf("%c", &ch);
            if (ch == 'q' || ch == 'Q')
            {
                printf("Destruction starts\n");
                tpool_destroy();
                printf("Destruction ends\n");
                exit(0);
            }
        }

        //connect to a new request
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0)
        {
            printf("Connection establishment failed");
            continue;
        }
        //receive the file_name from the client
        //to be freed after sending file(in func)
        char *file_name = (char *)malloc(250);
        int receieve_status = recv(client_socket, file_name, sizeof(file_name), 0);
        if (receieve_status < 0)
        {
            printf("Couldnt receive the file name...Try again\n");
        }
        else
        {
            //run two concurrent functions, one(parent) for listening more requests and the other(child) for answering the request
            push(file_name, client_socket);
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

    int thread_count;
    printf("Enter the number of threads in the thread pool:\n");
    scanf("%d", &thread_count);

    printf("Press Q to quit after listening to one more connection\n");
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
    info = tpool_create(thread_count);

    driver();

    close(server_socket);
    return 0;
}

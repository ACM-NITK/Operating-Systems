This folder contains server and client applications. Server sends the file requested by client to it.

The server contains a thread pool and all the workers are intially put to sleep.
Whenever there is a new request for file, this file_name is pushed into fifo unassigned-work-queue.This work is then taken up by some unassigned thread.

To compile the server code, run:

    g++ -pthread server.c -o server

To compile the client code, run:

    g++ client.c -o client

To run the server receiving requests from port X in your linux shell, run:

    ./server X

Eg:

    ./server 9002

To run the client sending requests to the server with port Y looking for a file file_name,run:

    ./client Y file_name

Eg:

    ./client 9002 abc.txt

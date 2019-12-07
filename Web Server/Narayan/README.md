This folder contains server and client applications. Server sends the file requested by client to it.
Currently, the server handles multiple requests by forking into seperate processes for handling requests and listening for more requests.

To run the server receiving requests from port X in your linux shell, run:

    ./server X
Eg:

    ./server 9002

To run the client sending requests to the server with port Y looking for a file file_name,run:

    ./client Y file_name

Eg:

    ./client 9002 abc.txt
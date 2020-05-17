# server-client-filesend

This program allows a client to request a file or a directory reading from a server

First navigate to the folder with ftserver.c and ftclient.py from the command line

Next compile the server by running:

gcc -o ftserver ftserver.c


Then run the server with port number as argument:

example: ftserver 50001


Next run the client with the following arguments:

python3 ftclient.py <server host> <server port> <command> <dataport>


EXAMPLES:

To request a directory reading:
ftclient.py flip1 50001 -g 50000

To request a file:
ftclient.py flip1 50001 -l file.txt 50000

NOTE: 
"flip1" is an Oregon State University specific host

The included file, "pride_and_prejudice.txt" is a free version of the novel by the same name
The file is large enough that the file cannot be expected to be sent in one receive call
This showcases that TCP/IP streams data through sockets and does not just send all data.

WARNING:
Using the same dataport multiple times in a short period of time will cause
the server to hang. 

Use a different dataport number EACH call to avoid hanging the server

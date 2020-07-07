# server-client-filesend

This program allows a client to request a file or a directory reading from a server

It works best if the client and server are ran from separate folders, and even different hosts, so that the data actually gets to travel.

Note this program has only been tested on Linux and MacOS

First navigate to the folder with ftserver.c in one command line window.

Next compile the server by running:

gcc -o ftserver ftserver.c


Then run the server with port number as argument:

example: ftserver 50001




Then navigate to the folder with ftclient.py in a separate command line window.

Next run the client with the following arguments:

python3 ftclient.py serverhost serverport command dataport


## EXAMPLES:

To request a directory reading:
python3 ftclient.py localhost 50001 -g 50000

To request a file:
python3 ftclient.py localhost 50001 -l file.txt 50000

The included file, "pride_and_prejudice.txt" is a free version of the novel by the same name.
The file is large enough that the file cannot be expected to be sent in one receive call.
This showcases that TCP/IP streams data through sockets in segments and does not just send all data.

### WARNING:
Using the same dataport multiple times in a short period of time will cause
the server to hang. 

Use a different dataport number EACH call to avoid hanging the server

################################################################################
## Author:        Story Caplain
## Filename:      chatserve.py
## Description:   This program hosts a connection on a specified port number.
##                It utilizes the socket API to communicate to a peer via TCP.
##                The program assumes the user has access to OSU's flip3
## Course name:   CS 372
## Last Modified: 03/11/2020
################################################################################

import sys
import socket
import time
import signal
import os

# Make sockets global for signal handling
LSOCKET = 0
DSOCKET = 0
BUFSIZE = 100000

################################################################################
# SOURCES:
# https://realpython.com/python-sockets/
# https://www.geeksforgeeks.org/socket-programming-python/
# https://stackoverflow.com/questions/2444459/python-sock-listen
# https://stackoverflow.com/questions/12454675/whats-the-return-value-of-socket-accept-in-python
# https://docs.python.org/3/library/internet.html
# https://stackoverflow.com/questions/1112343/how-do-i-capture-sigint-in-python
# https://stackoverflow.com/questions/25953626/receive-all-of-the-data-when-using-python-socket
# https://stackoverflow.com/questions/23459095/check-for-file-existence-in-python-3
# https://stackoverflow.com/questions/34883234/attempting-to-reconnect-after-a-connection-was-refused-python
################################################################################

################################################################################
## Function name: signal_handler
## parameters:    This function takes custom signal handlers
## returns:       Nothing, this function servers to catch the sigint to avoid
##                an ugly error message upon SIGINTing this py file
################################################################################
def signal_handler(signal, frame):
    print("\n\nYou have shut down the client, goodbye...\n")
    if(LSOCKET != 0):
        LSOCKET.close()
    if(DSOCKET != 0):
        DSOCKET.close()
    sys.exit(0)

################################################################################
## Function name: socket_setup 
## parameters:    This function takes in a port no. and host and opens a socket
## returns:       A socket ready for listening
################################################################################
def socket_setup(PORT, HOST):

    # got this try/except idea from stack overflow
    while True:

        try:
            # setup up a socket using ipv4 and TCP
            time.sleep(1)
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((HOST, PORT))
            print("\nConnected on port:",PORT)
            break

        except socket.error:
            print(PORT)
            # if connection failed try again
            print("\nConnection failed, retrying...\n")

    return s

################################################################################
## Function name: receive
## parameters:    Takes in the data socket and filename as an argument
##                This socket was returned by an accept call in main
## returns:       This function returns either -1, or 0
##                -1 means the communication failed
##                0 means that valid data was received
################################################################################
def receive(d, filename):

    # if a directory, print to stdout
    if(filename == None):
        # call recv on the connected to socket
        while True:
            data = d.recv(1024)
            if not data:
                break
            data = data.decode()
            print(data)
            
    # if a file, write to file
    else:
        # if file already exists write to a new file with the same name
        # but with a number appended
        i = 2
        while(os.path.exists(filename)):
            filename = filename + "_" + str(i)

        # open file for writing
        f = open(filename, "a+")

        # call recv on the connected to socket
        while True:
            data = d.recv(1024)
            if not data:
                break
            data = data.decode()
            f.write(data)

        f.close()

    return 0

################################################################################
## Function name: sending
## parameters:    Takes as params the listening socket and message to send
##                This socket was returned by a call to accept in main
## returns:       This function returns either a 1 or 0
##                -1 means that the send failed
##                0 means that a valid request was sent over
################################################################################
def sending(l, message):

    # add null chars to end of the message to make the c recv() easier
    nulls_to_add = BUFSIZE - len(message)
    for i in range(nulls_to_add):
        message += "\0"

    # send over the message
    # sendall continuosly calls send until all data is senr
    data = l.sendall(message.encode('utf-8'))

    # if sendall failed
    if(data != None):
        print("Send request failed in ftclient.py")
        return -1

    return 0

################################################################################
###################### END OF FUNCTIONS START OF MAIN ##########################
################################################################################

# start by making sure that the correct number of args has been passed in
l = len(sys.argv)
if(l > 6 or l < 5):
    print("Incorrect argument amount!")
    print("Usage 1:   ftclient <host> <listen_port> –l <data_port>")
    print("Example 1: ftclient flip3 30021 –l 30020\n")
    print("Usage 2:   ftclient <host> <listen_port> –g <filename> <data_port>")
    print("Example 2: ftclient flip3 30021 –g file.txt 30020\n")
    exit(1)

# also check that the command used is either -l or -g 
if((l == 5 and sys.argv[3] != "-l") or (l == 6 and sys.argv[3] != "-g")):
    print("Incorrect command argument!")
    print("supported command arguments are \"-l\" and \"-g\"\n")
    print("Usage 1:   ftclient <host> <listen_port> –l <data_port>")
    print("Example 1: ftclient flip3 30021 –l 30020\n")
    print("Usage 2:   ftclient <host> <listen_port> –g <filename> <data_port>")
    print("Example 2: ftclient flip3 30021 –g file.txt 30020\n")
    exit(1)

# set up signal handler
signal.signal(signal.SIGINT, signal_handler)

# store host argument
HOST = sys.argv[1]
    
# store listening port numbers as int
LPORT = int(sys.argv[2])

# determine requested command
CMD = sys.argv[3]

# make message when command is asking for a directory
if(len(sys.argv) == 5):
    DPORT = int(sys.argv[4])
    message = "dir " + sys.argv[4]

# make message when command is asking for a file
else:
    DPORT = int(sys.argv[5])
    message = "file " + sys.argv[5] + " " + sys.argv[4]
    print(message)

# setup the listening socket
# client sends requests over the listening socket
LSOCKET = socket_setup(LPORT, HOST)

# call sending function on c to send file/directory request
retval = sending(LSOCKET, message)
    
# a return value of -1 means the communication failed, break
if(retval == -1):
    print("\nConnection failure...")
    LSOCKET.close()
    time.sleep(1)
    exit(1)

# store filename or None if directory
filename = None
if(l == 6):
    filename = sys.argv[4]

# setup the data socket
# server sends files/directory readings over the data socket
DSOCKET = socket_setup(DPORT, HOST)

# receive the directory or file
retval = receive(DSOCKET, filename)
    
# a return value of -1 means the communication failed, break
if(retval == -1):
    print("\nConnection failure...")
    time.sleep(1)
    exit(1)
    














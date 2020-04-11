/*******************************************************************************
** Program Name: ftserver.c
** Author:       Story Caplain
** Last Updated: 03/02/20
** Description:  This program will act as a server for a file transfer program.
**               A client can request a file or a directory description
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#define SIZE 100000

/* made sockets global for sigint, error functions */
/* ecFD_1 is establishedConnectionFD */
int ecFD_1, ecFD_2;

/*******************************************************************************
** Sources:
** https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf
** Socket lectures from CS 344 at Oregon State University
** Socket programming assignment from CS 344 at OSU
** https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
** https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
** https://www.geeksforgeeks.org/c-program-delete-file/
** https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
** https://stackoverflow.com/questions/2029103/correct-way-to-read-a-text-file-into-a-buffer-in-c
** https://stackoverflow.com/questions/27205810/how-recv-function-works-when-looping
** https://www.codeproject.com/Questions/469837/How-to-check-whether-socket-connection-is-alive
** http://my.fit.edu/~vkepuska/ece3551/ADI_Speedway_Golden/Blackfin%20Speedway%20Manuals/LwIP/socket-api/setsockopt_exp.html
** https://stackoverflow.com/questions/5616092/non-blocking-call-for-reading-descriptor
*******************************************************************************/

/* Error function used for reporting issues */
void error(const char *msg) { 
  fprintf(stderr,"%s\n",msg); 
  close(ecFD_1);
  close(ecFD_2);
  exit(1);
} 

/* catches sigint and closes sockets */
void socket_cleanup(int);

/* allows for message sending between client and server */
void connector();

/* to avoid repetitive code, going to call this function for socket setyp */
int socketSetup(int);

/* used to send files across the data socket */
void filesend(char*, int);

/* main function */
/* much of this code is ripped straight from my CS 344 socket assignment */
int main(int argc, char *argv[]) {

  int listenSocketFD, portNumber, charsRead;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo;
 
  /* setup signal handling to close sockets when user sigints */
  /* modified from my smallsh project from CS 344 at OSU */
  struct sigaction SIGINT_action = {0};
  memset(&SIGINT_action,0,1);
  SIGINT_action.sa_handler = socket_cleanup;
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;
  sigaction(SIGINT, &SIGINT_action, NULL);

  /* Check usage & args */
  if(argc != 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } 

  /* Get the port number, convert to an integer from a string */
  portNumber = atoi(argv[1]); 

  /* setup the port */
  listenSocketFD = socketSetup(portNumber);
  if(listenSocketFD < 0) error("ERROR opening socket");

  /* Get the size of the address for the client that will connect */
  sizeOfClientInfo = sizeof(clientAddress); 

  /* loop until loop is killed manually */
  while(1) {

    /* hold the status of waitpid calls */
    int exit_status = -5;
  
    /* set spawnpid to -5 to start, bogus value */
    pid_t spawnpid = -5;
  
    /* fork it */
    spawnpid = fork();
  
    switch(spawnpid) {
  
      /* fork failed */
      case -1:
        error("Hull Breach!");
        break;
  
      /* child process, call connector function */
      case 0:

        /* Accept */
        ecFD_1 = accept(listenSocketFD,
          (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if(ecFD_1 < 0) error("ERROR accepting ecFD_1");
        else printf("\nListening socket connection accepted\n");

        connector();
        close(ecFD_2);
        printf("\nAll data successfully transmitted!\n");
        printf("\nAwaiting next client connection...\n\n\n");
        break; 

      /* parent process, wait for children to finish */
      default:
        //waitpid(spawnpid,&exit_status,0);
        waitpid(-1,&exit_status,0);
        break;
    }
  }
  return 0; 
}

/*******************************************************************************
** Function name:  connector
** preconditions:  Takes no args, is called to handle socket connection/comms
** postconditions: All requested data has been sent via sockets to client
*******************************************************************************/
void connector() {

  /* init variables */
  int dataSocketFD, charsRead = 0, charsWritten = 0, temp = 0, i = 0;

  char buffer[SIZE];
  memset(buffer,'\0',sizeof(buffer));
  char temp_buffer[SIZE];
  memset(temp_buffer,'\0',sizeof(temp_buffer));
  char file_buffer[SIZE];
  memset(file_buffer,'\0',sizeof(temp_buffer));

  char* filename;
  int dataport;

  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo;

  /* not sure if this is necessary, wait here until buffer has data */
  while(temp <= 0) {
    temp = recv(ecFD_1, &buffer, 1, MSG_PEEK);
    sleep(1);
  }

  /* recv the command and dataport from client */
  /* from stackoverflow, my version was uglier */
  while((temp = recv(ecFD_1, &buffer[charsRead], sizeof buffer-temp, 0)) > 0) {
    charsRead += temp;
    if(temp < 0) {
      error("initial recv returned -1");
    }
    else if(temp == 0 && charsRead > 0) {
      break;
    }
    if(charsRead == SIZE) break;
  }

  /* strtok the received command using space as delimeter */
  const char s[2] = " ";
  char* token;
  token = strtok(buffer, s);

  /* if user requested a directory reading the client sends over:
   * "dir <dataport>"
   * else if user requested a file the client sends over:
   * "file <dataport> <filename>" 
   */

  /* if user wants a directory */
  if(strncmp(token, "dir", 3)==0) {
    token = strtok(NULL, s);
    if(token != NULL) {
      dataport = atoi(token);
    }
    else {error("client sent over bad data");}

    /* setup the data socket */
    dataSocketFD = socketSetup(dataport);
    if(dataSocketFD < 0) error("ERROR opening data socket");
    else { 
      printf("\nData socket is now setup in ftserver!\n"); 
    }
    sizeOfClientInfo = sizeof(clientAddress);

    /* set nonblocking */
    int flags = fcntl(dataSocketFD, F_GETFL, 0);
    fcntl(dataSocketFD, F_SETFL, flags | O_NONBLOCK);

    /* Accept */
    ecFD_2 = -1;
    while(ecFD_2 == -1) {
      ecFD_2 = accept(dataSocketFD,
        (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    }

    /* reset buffers */
    memset(buffer,'\0',sizeof(buffer));
    memset(temp_buffer,'\0',sizeof(temp_buffer));

    /* create temporary filename */
    /* another concept from CS 344 */
    sprintf(temp_buffer, "caplains%d", getpid());

    /* setup directory variables */
    DIR* d;
    struct dirent* dir;
    d = opendir(".");

    /* open the temp file for writing and fill it with the dir contents */
    if(d) {
      FILE* fp = fopen(temp_buffer, "w");
      if(fp == NULL) {error("file creation failed");}

      /* loop thru dir, write it to temp file */
      while((dir = readdir(d)) != NULL) {
        if(strncmp(dir->d_name,"caplains",8)!=0) {
          fprintf(fp, "%s\n", dir->d_name);
        }
      }
      fclose(fp);

      /* call filesend with datasocket, filename, and filesize */
      struct stat st;
      stat(temp_buffer, &st);
      int filesize = st.st_size;

      filesend(temp_buffer, filesize);     
 
      /* close directory */
      closedir(d);
    }
    else{error("opendir failed in directory reading");}
  }

  /* if user wants a file */
  else if(strncmp(token, "file", 4)==0) {

    /* store data port number */
    token = strtok(NULL, s);
    if(token != NULL) {
      dataport = atoi(token);
    }
    else {error("client sent over bad dataport");}

    /* store filename */
    token = strtok(NULL, s);
    if(token != NULL) {
      filename = token;
    }
    else {error("client sent over bad filename");}

    /* setup the data socket */
    dataSocketFD = socketSetup(dataport);
    if(dataSocketFD < 0) error("ERROR opening data socket");

    sizeOfClientInfo = sizeof(clientAddress);

    /* set nonblocking */
    int flags = fcntl(dataSocketFD, F_GETFL, 0);
    fcntl(dataSocketFD, F_SETFL, flags | O_NONBLOCK);

    /* Accept */
    ecFD_2 = -1;
    while(ecFD_2 == -1) {
      ecFD_2 = accept(dataSocketFD,
        (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    }

    /* Accept 
    ecFD_2 = accept(dataSocketFD,
      (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    printf("\nData socket is now setup!\n");
    */

    /* reset buffers */
    memset(temp_buffer,'\0',sizeof(temp_buffer));

    /* setup directory/file variables */
    DIR* d;
    struct dirent* dir;
    d = opendir(".");
    sprintf(temp_buffer, "%s", filename);
    memset(buffer,'\0',sizeof(buffer));
    int found = 0;    

    /* open the temp file for writing and fill it with the dir contents */
    if(d) {
    
      /* loop thru dir and look for file */
      while((dir = readdir(d)) != NULL) {
        if(strcmp(temp_buffer, dir->d_name)==0) {
          found = 1;
        }
      }

      /* if the file is found in the current directory */
      if(found==1) {        
        for(i = 0; i < SIZE; i++) {
          file_buffer[i] = temp_buffer[i];
        }

        /* call filesend with datasocket, filename, and filesize */
        struct stat st;
        stat(temp_buffer, &st);
        int filesize = st.st_size;
        int check = 0; 
        
        filesend(file_buffer, filesize);
        printf("\nData sent over successfully!\n");
      }

      /* if file not found */
      else {
        int temp = 0;
        char notfound[14] = "file not found";
        temp = send(ecFD_2, notfound, sizeof(notfound), 0);
        if(temp==0){error("failed to send file not found");}
      }

      /* close directory */
      closedir(d);
    } 
  }
  /* if bad message was received */
  else {
    int temp = 0;
    char badmsg[11] = "bad message";
    temp = send(ecFD_2, badmsg, sizeof(badmsg), 0);
    if(temp==0){error("failed to send bad message warning");}
  }
}

/* Again this code is borrowed/modified from my CS 344 project */
/*******************************************************************************
** Function name:  socketSetup
** preconditions:  Takes in a port number supplied via command line or client
** postconditions: Returns an accepted socket ready to make send/recv calls
*******************************************************************************/
int socketSetup(int portNumber) {
  
  /* init variables */  
  int socketFD;
  socklen_t sizeOfClientInfo;
  struct sockaddr_in serverAddress;

  /* Set up the address struct for this process (the server) */
  /* Clear out the address struct */
  memset((char *)&serverAddress, '\0', sizeof(serverAddress)); 

  /* Create a network-capable socket */
  serverAddress.sin_family = AF_INET; 

  /* Store the port number */
  serverAddress.sin_port = htons(portNumber); 

  /* Any address is allowed for connection to this process */
  serverAddress.sin_addr.s_addr = INADDR_ANY; 

  /* Set up the socket */
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if(socketFD < 0) {
    error("ERROR opening socket");
  }

  /* this is straight from stackoverflow */
  int reuse = 1;
  if(setsockopt(socketFD,SOL_SOCKET,SO_REUSEADDR,
    (const char*)&reuse,sizeof(reuse))<0)
      error("setsockopt(SO_REUSEADDR) failed");
  if(setsockopt(socketFD,SOL_SOCKET,SO_REUSEPORT,
    (const char*)&reuse,sizeof(reuse))<0)
      error("setsockopt(SO_REUSEPORT) failed");

  /* Enable the socket to begin listening */
  if(bind(socketFD, 
     (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
     printf("\nPORTNO: %d",portNumber);
     error("ERROR on binding");
  }

  /* Flip the socket on - it can now receive up to 5 connections */
  listen(socketFD, 5); 

  printf("\nserver opened a socket on portnumber: %d\n", portNumber);

  /* return the port */
  return socketFD;
}

/*******************************************************************************
** Function name:  filesend
** preconditions:  Takes in a filename to send and its size
** postconditions: After completion file will have been sent to client
*******************************************************************************/
void filesend(char* filename, int filesize) {

  /* modified from stackoverflow, puts file contents into a buffer */
  char buffer[filesize + 1];
  memset(buffer,'\0',sizeof(buffer));

  FILE *fp = fopen(filename, "r");
  if(fp != NULL) {
    size_t newLen = fread(buffer, sizeof(char), filesize, fp);
    if(ferror(fp) != 0) {
      error("Error writing file to buffer");
    }
    else {
      buffer[newLen++] = '\0';
    }
    fclose(fp);
  }

  /* delete the tempfile */
  if(strncmp(filename,"caplains",8)==0) {
    if(remove(filename)!=0) {
      error("server failed to delete tempfile");
    }
  }

  /* init send() variables */
  int charsWritten = 0;
  int temp = 0;

  sleep(1);

  /* going to chunk the buffer */
  int chunksize = 1024;
  char chunk[chunksize];

  int index = 0;
  int i = 0;

  /* send the file over the socket to client */
  while(charsWritten < sizeof(buffer)) {

    memset(chunk,'\0',sizeof(chunk));

    for(i=0;i<chunksize;i++) {
      if((index + i) < filesize) {
        chunk[i] = buffer[index + i];
      }
    }
    index += chunksize;

    temp = send(ecFD_2, chunk, sizeof(chunk), 0);

    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("Failed to send file to client");
    }

    charsWritten += temp;
  }
}

/* modified from my CS 344 smallsh project */
/*******************************************************************************
** Function name:  socket_cleanup
** preconditions:  Takes in a caught SIGINT to handle socket closing
** postconditions: All sockets will be closed and file will exit without error
*******************************************************************************/
void socket_cleanup(int signo) {

  /* assert that we are dealing with a sigint */
  assert(signo == SIGINT);

  /* close sockets */
  if(ecFD_1 != 0) {close(ecFD_1);}
  if(ecFD_2 != 0) {close(ecFD_2);}

  exit(0);
}














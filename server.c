#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>

/*
    * sockets are bidirectional
    * read and write are essentially the same
    
    * TCP sends a connect request to ensure that the other device is ready for communication
    * connect goes from client to server

    * By default, sockets are blocking --> particularly the accept call
    SOLUTION1
    * get around this by making a parent and child process
    * communication with client takes place in the child process
    * create a child for every client that wishes to communicate
    * parent waits for incoming connections in the future
    SOLUTION2
    * create sockets as nonblocking right after making the sid!!! 
    * fcntl API makes socket nonblocking by editing the flags after creating the socket
    * this is part of the starter code ?? prof said??
    MONITOR ACTIVITY ON MULT. SOCKETS AT THE SAME TIME
    * select API

    * sending first the size of the message you are going to send 
    * make a buffer of that size
    * then do a loop to recieve the buffer
    
    * implement a nonblocking code --> week 7 lecture 1

*/

void checkError(int status)
{
   if (status < 0) {
      printf("socket error(%d): [%s]\n",getpid(),strerror(errno));
      exit(-1);
   }
}

//declaring functions that will perform the socket creation, connection, and communication
int createSocket(int portNum, int listenNum); //create it nonblocking
int connectClient(int sid);
void childProcess(int chatSocket);
void reapingChildren();



//implementation of the socket
int main(int argc, char* argv[]){
    int sid = createSocket(9000, 10);
    
    while(1)
     {
        //connect to a client 
        int chatSocket = connectClient(sid);
        pid_t child1 = fork();
        if (child1 == 0)                                                       
         {
            childProcess(chatSocket); //send and recv data between client and server
            close(chatSocket);
         }
        else 
         {
            printf("Parent produced child [%d]\n", child1);
            reapingChildren();
            close(chatSocket);
         }
     }
     close(sid);
     reapingChildren();
    return 0;
}

//definition of the functions 
int createSocket(int portNum, int listenNum) //this function will return SID
 {
    int sid = socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(portNum);
    addr.sin_addr.s_addr = INADDR_ANY;
    int status = bind(sid, (struct sockaddr*)&addr, sizeof(addr)); 
    checkError(status);
    status = listen(sid, listenNum); 
    checkError(status);
    return sid;
 }

int connectClient(int sid)
 {
    struct sockaddr_in client;
    socklen_t clientSize;
    int chatSocket = accept(sid, (struct sockaddr*)&client, &clientSize);
    checkError(chatSocket);
    return chatSocket;
 }

void childProcess(int chatSocket)
 {
    int sizeBuf;
    int rc = read(chatSocket, &sizeBuf, sizeof(sizeBuf));
    char* buf = malloc(sizeof(char) * sizeBuf);
    int rem = sizeBuf, rec = 0;
    while (rem > 0)
     {
        int rc = read(chatSocket, buf+ rec, rem);
        checkError(rc);
        rem -= rc;
        rec += rc;
     }
    printf("Server read [%s] from the client.\n", buf);
    free(buf);

    char* message = "Hello Shorty!";
    printf("Server wrote [%s] to the client.\n", message);
    int mesSize = strlen(message) + 1;
    int wc = write(chatSocket, &mesSize, sizeof(mesSize));
    rem = mesSize;
    int sent = 0;
    while(rem > 0)
     {
        int wc = write(chatSocket,message+sent,rem);
        rem -= wc;
        sent += wc;
     }
 }


void reapingChildren()
 {
    int status = 0;
    pid_t deadChild;
    do
    { 
      deadChild = waitpid(0, &status, WNOHANG); //if the first parameter = 0, you are waiting for any child 
      checkError(deadChild);                    // return immediately if no child existed
      if (deadChild > 0) printf("Reaped child [%d]\n", deadChild);
    }while(deadChild > 0);
 }
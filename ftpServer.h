#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>c read file into buffer
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

#ifndef FTPSERVER_H
#define FTPSERVER_H

//from Beej's Guide. Helps to clean up 'zombie' child processes
void sigchld_handler(int s);

//validate arguments
void validateArguments(int argc, char *argv[]);

//open a new socket
int openSocket(char portNum[6]);	//old: int startServer(char portNum[6]);

//run main server loop, call receiveCommand once connection is established
int runFTP(int controlListener);//old: int runServer(int listen_socket, int control_socket);

//attempts to receive command from the connection
void receiveCommand(int *controlConnection);//old: void receiveCommand(int *control);

//checks if the file with the indicated filename exists
bool fileExists(char *filename[]);

//opens data connection with client and sends a directory listing
void sendDirectory(char dataPort[6]);//old: void listDirectory(char dataPort[6], int *control);

//opens data connection with client and sends a file
void sendFile(int *controlConnection, char dataPort[6], char fileName[128]);


#endif // FTPSERVER_H
/* 	Program: 		ftpServer
**	By:				Cody J Dhein
**	For:			CS372 Project 2
**	Description:	A basic FTP server that is able to send a directory listing or text files to a client.
**					All parameters are passed by the client. This program only needs to be provided with a
**					single argument in the form of the port number for the control connection.
**	Sources:		Much help and guidance from Beej's guide
**						http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
**					Reference used for other functions
*						http://en.cppreference.com/w/c
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>


int startServer(char portNum[6]);
int runServer(int listen_socket, int control_socket);
void receiveCommand(int *control);
void listDirectory(char dataPort[6], int *control);
void sendFile(char dataPort[6], int *control, char fileName[128]);
void sigchld_handler(int s);


//from Beej's Guide to clean up 'zombie' child processes
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}



/******************************************************************************************
*	Name: startServer(char portNum[6])
*	
*	Description: Starts the FTP server's control connection on the provided port number. If
*			there are any issues during set up the program will exit and an error message will
*			be printed to the console. This should be called first.
*				I
*
*	Input: 
*			char portNum[6] - character array representing the port number to start the 
*				server on.
*	Output:
*			Returned - a socket that is listening for connections
******************************************************************************************/
int startServer(char portNum[6]){
	int serverSocket;
	struct addrinfo hints, *results, *attempt;	
	struct sigaction sa;
	int yes=1;
	int status_check;	
	
	//initialize hints
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	//get potential 
	if((status_check = getaddrinfo(NULL,portNum, &hints, &results)) != 0){ //0 is success
		fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(status_check));
		return 1;
	}
	
	//check results and bind to first possible
	for(attempt = results; attempt != NULL; attempt = attempt->ai_next){
		
			//allocate socket descriptor
		if((serverSocket = socket(attempt->ai_family, attempt->ai_socktype, attempt->ai_protocol)) == -1){
			perror("server: socket");
			continue;
		}
		
			//establish socket options
		if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			perror("setsockopt");
			exit(1);
		}
			//attempt to bind socket to IP/PORT
		if(bind(serverSocket, attempt->ai_addr, attempt->ai_addrlen) == -1) {
			close(serverSocket);
			perror("server: bind");
			continue;
		}
		
		break;
	}
	
	freeaddrinfo(results); //done with this
	
	if(attempt == NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	
	//start listening for connections
	if(listen(serverSocket, 10) != 0){
		perror("listen");
		exit(1);
	}
	
	//reap zombies
	sa.sa_handler = sigchld_handler; 
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) != 0){
		perror("sigaction");
		exit(1);
	}
	
	printf("ftpServer is waiting for connections on port %s\n", portNum);	
	return serverSocket;

}


/******************************************************************************************
*	Name: runServer(int listen_socket)
*	
*	Description: Takes a socket file descriptor that is listening for connections and runs
*			a while loop that accepts an incoming connection attempt. Once a client is 
*			connected it calls the receiveCommand function to receive the client's command.
*				I
*
*	Input: 
*			int listen_socket - a file descriptor already bound to a port and listening
*						for connecitons.
*	Output:
*			control_socket is passed by reference to receiveCommand
******************************************************************************************/
int runServer(int listen_socket){
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
	int control_socket;
	//main listen loop
	while(1) {
		sin_size = sizeof client_addr;
		control_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &sin_size);
		
		if(control_socket == -1){
			
			perror("accept");
			continue;
		}
		
		printf("Server: client connected and control connection established\n");
		
		if(!fork()){
			close(listen_socket);
			
			receiveCommand(&control_socket);

			close(control_socket);
			exit(0);
		}
		close(control_socket);
				
	}
	return 0;
}


/******************************************************************************************
*	Name: receiveCommand(int *control)
*	
*	Description: Takes a socket file descriptor by reference that has a client connected to it.
*			It then receives input from the client in the form of a string containing the 
*			following: a port to open the data connection on, a command (-l or -g) and
*			a filename if -g was passed as command. The input is parsed and assigned to 
*			variables that are then checked for validity. If an invalid command was received
*			the client is informed of this on the control socket. If the input was good, 
*			sendFile or listDirectory are called to handle the rest of the request.
*				I
*
*	Input: 
*			int *control - a file descriptor with a connected client
*			
*			Accepts from client a string consisting of <dataport> <command> <filename>
*			Valid commands are -l for listDirectory and -g for sendFile
*	Output:
*			The data port, control socket and potentially filename are sent to the applicable
*				methods to carry out the request.
******************************************************************************************/
void receiveCommand(int *control){
	const char error[] = "Invalid command passed. Expected -l or -g <filename>";
	const char accept[]= "accept\n";
	char fromClient[512];
	char dataPort[6];
	char command[2];
	char filename[128];
	
	memset(&fromClient, 0, sizeof fromClient);
	memset(&dataPort, 0, sizeof dataPort);
	memset(&command, 0, sizeof command);
	memset(&filename, 0, sizeof filename);

	printf("Server: receiving command line\n");	
	recv(*control, &fromClient, sizeof fromClient, 0);
	char * temp;
	temp = strtok(fromClient," \n");
	strcpy(dataPort, temp);
	temp = strtok(NULL, "  \n");
	strcpy(command, temp);
	
	if(strncmp(command,"-g",2) == 0){
		temp = strtok(NULL, " \n");
		strcpy(filename, temp);		

		sendFile(dataPort, control, filename);
		
	}else if(strncmp(command,"-l",2) == 0){
	
		listDirectory(dataPort, control);
	}else{
		//send error message
		send(*control, &error, sizeof error, 0);
	}	
}

/******************************************************************************************
*	Name: listDirectory(char dataPort, int *control)
*	
*	Description: Opens a new socket file descriptor for the data connection on the port 
*			provided and then notifies the client that it is ready to accept a connection.
*			Once connection is accepted, the directory is opened and the names of all the 
*			files are concatenated onto the dirList variable. Once this is complete it is
*			sent to the client over the data connection, which is then terminated.
*				I
*
*	Input: 
*			int *control - socket with client connected. Used for control transmissions.
*			char dataPort - the port the data connection will be established on 
*
*	Output:
*			A listing of the directory's files is sent to the client, or an error string is sent.
******************************************************************************************/
void listDirectory(char dataPort[6], int *control){
	const char acceptString[]= "accept\n";
	char dirList[512];
	/***************************************
			Establish Data Connection
	****************************************/	
	int data_listener = startServer(dataPort);
	int data_socket;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
	
	printf("Server: attempting to establish data connection on port %s\n", dataPort);
	
	//send accept message to notify client to try connection
	send(*control, &acceptString, sizeof acceptString, 0);		
	
	//main listen loop
	while(1) {
		sin_size = sizeof client_addr;
		data_socket = accept(data_listener, (struct sockaddr *)&client_addr, &sin_size);

		if(data_socket == -1){
			perror("accept");
			continue;
		}
			
		printf("Server: Data connection established. Sending directory ...\n");

		if(!fork()){
			close(data_listener);

		/***************************************
				Directory to Client
		****************************************/				
			
			DIR *dp; //directory pointer
			struct dirent *ep;

			memset(&dirList, 0, sizeof dirList);
			
			dp = opendir("./"); //open directory
			
			/*build string of directory listing*/
			if(dp != NULL){
				strcat(dirList, "Directory Listing:\n\n"); 
				while (ep = readdir(dp)){
					strcat(dirList, ep->d_name);
					strcat(dirList, "\n");
				}
				strcat(dirList, "\n\nEnd of directory listing.\n");	
				(void) closedir (dp);
			}else{
				perror("Couldn't open directory");
			}
			
			//send directory
			if(send(data_socket, dirList, sizeof dirList, 0) == -1)
				perror("send");
			
			printf("Server: Directory sent. Closing data connection.");
		}
		
		//close data connection
		close(data_socket);
		close(data_listener);	

		return;			
	}	
}


/******************************************************************************************
*	Name: sendFile(char dataPort, int *control, char fileName)
*	
*	Description: First checks to see if the filename provided is valid and if the file can
*			be opened. If so, it opens a new socket file descriptor for the data connection 
*			on the port provided and then notifies the client that it is ready to accept a
*			connection. Once connection is accepted, a buffer is created and the file is read
*			into it. This buffer is then passed to the send() function in a loop that checks 
*			the number of bytes sent vs the total number. Once they match a success string
*			is printed to the console and the data connection is closed.
*				I
*	Input: 
*			int *control - socket with client connected. Used for control transmissions.
*			char dataPort - the port the data connection will be established on 
*			char fileName - the name of the file the client is requesting
*	Output:
*			The file will be sent to the client on the data connection, or an error message
*			will be sent on the control connection.
******************************************************************************************/
// http://www.cplusplus.com/reference/cstdio/fread/
void sendFile(char dataPort[6], int *control, char fileName[128]){
	const char acceptString[]= "accept\n";
	const char noFile[] = "Server: Error, file not found, or invalid file name provided.";
		// Open File to confirm if filename is valid
			FILE *fp;	
			fp = fopen(fileName, "r");
			if (fp == NULL){
				printf("Can't open %s, or file not found.\n", fileName);
				send(*control, &noFile, sizeof noFile, 0);
				return;
			}	
	
	/***************************************
			Establish Data Connection
	****************************************/	
	
	//open socket for data connection and accept
	int data_listener = startServer(dataPort);
	int data_socket;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
	
	printf("Server: attempting to establish data connection on port %s\n", dataPort);
	
	//send accept message to notify client to try connection
	send(*control, &acceptString, sizeof acceptString, 0);		
	
	//main listen loop
	while(1) {

		sin_size = sizeof client_addr;
		data_socket = accept(data_listener, (struct sockaddr *)&client_addr, &sin_size);

		if(data_socket == -1){
			perror("accept");
			continue;
		}
			
		printf("Server: data connection established. Sending file...\n");

		if(!fork()){
			close(data_listener);

			/***************************************
						File to Client
			****************************************/
			
		//Allocate buffer and read file in
			long fileSize;
			char * buffer;
			
			size_t result;
			//get file size
			fseek(fp, 0, SEEK_END);
			fileSize = ftell(fp);
			rewind(fp);
			
			//allocate buffer
			buffer = (char*) malloc (sizeof(char)*fileSize);
		
		//read file into buffer
			result = fread(buffer, 1, fileSize, fp);
			if(result != fileSize){
				fprintf(stderr, "Error reading file into buffer.");
				exit(1);		
			}
			
			int bytesSent = 0;
			int len = strlen(buffer);			
			int n;				
			while(bytesSent < len){	//send file
				n = send(data_socket, buffer + bytesSent, sizeof buffer,0);
				if(bytesSent == -1){
					break;
				}
				bytesSent += n;
			}									
			printf("Server: %s transferred to client. %d bytes sent. Closing data connection\n", fileName, bytesSent);
		}

		//closing data connection
		close(data_socket);
		close(data_listener);	
		return;			

	}	
}



int main(int argc, char *argv[]){
	char portNum[6];
	int listen_socket, control_socket;

	
	/* validate provided arguments before going any further	*/
	if (argc != 2) {
		fprintf(stderr, "\nExpected usage: ftpServer PORT\n\n");
		exit(1);		
	}
	
	if(*argv[1] > 65535){
		fprintf(stderr, "\nInvalid PORT provided.\n\n");
		exit(1);				
	}
	
	/* copy args into variables */
	strcpy(portNum, argv[1]);


	listen_socket = startServer(portNum);
	runServer(listen_socket);//, control_socket);
	
	return 0;
}


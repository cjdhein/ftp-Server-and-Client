/* 	Program: 	ftpServer
**	By:			Cody J Dhein
**	For:		CS372 Project 2
**	Sources:	Much help and guidance from Beej's guide
**					http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
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

void sigchld_handler(int s);
int startServer(char portNum[6]);
int runServer(int listen_socket, int control_socket);
void receiveCommand(int *control);
bool fileExists(char *filename[], int filenameSize);
void listDirectory(char dataPort[6], int *control);
void sendFile(char dataPort[6], char fileName[128]);


//from Beej's Guide to clean up 'zombie' child processes
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}



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
	
	freeaddrinfo(results);
	
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

int runServer(int listen_socket, int control_socket){
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
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
		
		int filenameSize = strlen(filename);
		//send accept message
		send(*control, &accept, sizeof accept, 0);
		
		printf("Command received: %s\nDataport: %s\nFilename: %s\nFilenameSize: %d", command, dataPort, filename, filenameSize);
		
		sendFile(dataPort, filename);
		
	}else if(strncmp(command,"-l",2) == 0){
		printf("Server: sending accept\n");	
		
	
		
		printf("Command received: %s\nDataport: %s\n", command, dataPort);
		
		listDirectory(dataPort, control);
	}else{
		//send error message
		send(*control, &error, sizeof error, 0);
	}	
}

void listDirectory(char dataPort[6], int *control){
	DIR *dp;
	struct dirent *ep;
	char dirList[512];
	memset(&dirList, 0, sizeof dirList);
	const char acceptString[]= "accept\n";
	
	dp = opendir("./");
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
	printf("\nattempting send\n");
	/* TODO: Actually send the listing*/
	
	//open socket for data connection and accept
	int data_listener = startServer(dataPort);
	int data_socket;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
	
	printf("Server: attempting to establish data connection on port %s\n", dataPort);
	
	//send accept message
	send(*control, &acceptString, sizeof acceptString, 0);		
	
	//main listen loop
	while(1) {
		printf("danger\n");
		sin_size = sizeof client_addr;
		printf("danger\n");
		data_socket = accept(data_listener, (struct sockaddr *)&client_addr, &sin_size);
		printf("danger\n");
		if(data_socket == -1){
			perror("accept");
			continue;
		}
		
		printf("Server: data connection established. Sending data...\n");

		if(!fork()){
			close(data_listener);

		if(send(data_socket, dirList, sizeof dirList, 0) != 0)
			perror("send");
		
			close(data_socket);
			exit(0);
		}
		printf("closing datasocket\n");
		close(data_socket);
		printf("closing datalistener\n");
		close(data_listener);	
		return;
	}	
}

void sendFile(char dataPort[6], char fileName[128]){

	FILE *fp;
	
	
	printf("\nInside sendFile\n");
	
	fp = fopen(fileName, "r");
	
	if (fp == NULL){
		printf("Can't open file %s", fileName);
		exit(1);
	}
	
	printf("\nattempting send\n");
	/* TODO: Actually send the listing*/
	
	//open socket for data connection and accept
	int data_listener = startServer(dataPort);
	int data_socket;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
	
	printf("Server: attempting to establish data connection on port %s\n", dataPort);
	//main listen loop
	while(1) {
		printf("danger\n");
		sin_size = sizeof client_addr;
		printf("danger\n");
		data_socket = accept(data_listener, (struct sockaddr *)&client_addr, &sin_size);
		printf("danger\n");
		if(data_socket == -1){
			perror("accept");
			continue;
		}
		
		printf("Server: data connection established. Sending data...\n");
		if(!fork()){
			close(data_listener);
			printf("What's going on?\n");
		// if(send(data_socket, dirList, sizeof dirList, 0) != 0)
			// perror("send");
		
			close(data_socket);
			exit(0);
		}
		close(data_socket);
		close(data_listener);			
		return;
	}	

}

bool fileExists(char *filename[], int filenameSize){
	
	/* TODO: check listing for provided filename*/
	
	return true;
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
	runServer(listen_socket, control_socket);
	
	
}


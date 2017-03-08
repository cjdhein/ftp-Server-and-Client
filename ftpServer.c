#include "ftpServer.h"


void validateArguments(int argc, char *argv[]){
	if (argc != 2) {
		fprintf(stderr, "\nInvalid number of arguments provided. Expected usage: ftpServer PORT\n\n");
		exit(1);		
	}
	
	if(*argv[1] > 65535){
		fprintf(stderr, "\nInvalid PORT provided.\n\n");
		exit(1);				
	}	
}

int openSocket(char portNum[6]){
	int newSocket;
	struct addrinfo hints, *results, *attempt;
	struct sigaction sa;
	int statusCheck;
	
	//initialize hints
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	//setup structures for attempt
	if((statusCheck = getaddrinfo(NULL, portNum, &hints, %results)) != 0){ //0 is success
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(statusCheck));
		exit(1);
	}
	
	//loop through results to attempt bind. failure at any step jumps to next result
	for(attempt = results; attempt != NULL; attempt = attempt->ai_next){
		
		//assign socket descriptor
		if((newSocket = socket(attempt->ai_family, attempt->ai_socktype, attempt->ai_protocol)) == -1){
			perror("server: socket");
			continue;
		}
		
		//establish socket's options
		if(setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int)) == -1){
			perror("server: setsockopt");
			continue;
		}
		
		//attempt to bind socket
		if(bind(newSocket, attempt->ai_addr, attempt->ai_addrlen) == -1){
			close(newSocket);
			perror("server: bind");
			continue;
		}
	
		break;
	}
	
	freeaddrinfo(results); //not needed anymore
	
	//check if socket bound successfully
	if(attempt == NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	
	//start listening for connections
	if(listen(newSocket, 10) != 0){
		perror("listen");
		exit(1);
	}
	
	//clean up possible zombies
	sa.sa_handler = sigchld_handler; 
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) != 0){
		perror("sigaction");
		exit(1);
	}
	
	return newSocket;
}

void runFTP(int controlListener){
	int controlSocket;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;
	
	//Loop to keep server listening for connections even after client disconnect
	while(1){
		sin_size = sizeof client_addr;
		
		//accept connection and assign to controlSocket
		controlSocket = accept(controlListener, (struct sockaddr *)&client_addr, &sin_size);
		
		if(controlSocket == -1){
			perror("accept");
			continue;
		}
		
		printf("Server: client connected and control connection established\n");
		
		
		//copied from Beej's guide
		if(!fork()){
			close(controlListener); //child process does not need the listener
			
			//call receiveCommand (this handles the subsequent calls to send a file or directory list)
			receiveCommand(&controlSocket);
			
			//close control socket as this client has finished with server
			close(controlSocket);
		}
		
		close(controlSocket); //parent has no need for this
	}
}

void receiveCommand(int *controlConnection){
	
	//used to send to client to inform of command acceptance or refusal. Only refuse is printed.
	const char accept[] = "accept\n";
	const char refuse[] = "Unknown command received. Expected '-l' or '-g <filename>'\n";
	
	char clientRequest[512]; //will hold entire client request
	char dataPort[6]; //will hold dataPort from client
	char command[2]; //will hold 2 digit client command ('-l' or '-g')
	char filename[128]; //will hold filename if client sends -g
	
	//set character strings to 0
	memset(&clientRequest, 0, sizeof clientRequest);
	memset(&dataPort, 0, sizeof dataPort);
	memset(&command, 0, sizeof command);
	memset(&filename, 0, sizeof filename);	
	
	printf("Server: receiving command line\n");	
	//receive from client and store into clientRequest
	recv(*controlConnection, &clientRequest, sizeof clientRequest, 0);
	
	/*parse out elements from clientRequest and assign to applicable variables
		filename will not be read unless -g is received*/
	char *temp;
	temp = strtok(clientRequest," \n");
	strcpy(dataPort, temp);
	temp = strtok(NULL, "  \n");
	strcpy(command, temp);
		
	
	//branch based on command type received.
	if(strncmp(command,"-g",2) == 0) {//received getFile command
		//read filename from request
		temp = strtok(NULL, " \n");
		strcpy(filename, temp);
		
		if(strncmp(filename, "",1) == 0){ //no filename provided
			send(*controlConnection, &refuse, sizeof refuse, 0);
		}
		
		//notify client command was accepted
		send(*controlConnection, &accept, sizeof accept, 0);
		
		//call sendFile to handle the rest
		sendFile(controlConnection, dataPort, filename);
		
	}else if(strncmp(command, "-l", 2) == 0){ //received getDirectory command
		strcpy(filename,""); //set to empty as not needed
		
		//notify client command was accepted
		send(*controlConnection, &accept, sizeof accept, 0);		
		
		sendDirectory(dataPort);
	}else{ //invalid command received
		send(*controlConnection, &refuse, sizeof refuse, 0);
	}
}


//	directory opening / listen sourced from: 
//	https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Simple-Directory-Lister.html
void sendDirectory(char dataPort[6]){
	
	char dirList[512]; //to hold directory listing
	memset(&dirList, 0, sizeof dirList);
	
	/***************************************
		Open Directory and Read into dirList
	****************************************/
	DIR *dp; //directory pointer
	struct dirent *ep;
	
	dp = opendir("./"); //open directory this file is in
	if(dp != NULL){
		strcat(dirList, "Directory Listing: \n");
		while(ep = readdir(dp)){	//print each file's name and go to new line
			strcat(dirList, ep->d_name);
			strcat(dirList, "\n");
		}
		strcat(dirList, "\nEnd of directory listing.\n");
		(void) closedir (dp);
	}else{
		perror("Couldn't open directory");
	}
	
	/***************************************
		Open data connection and send dirList
	****************************************/
	int dataListener = openSocket(dataPort);
	int dataConnection;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;
	
	bool dataSent = false;
	
	//accept incoming connection
	while(!dataSent){
		sin_size = sizeof client_addr;
		dataConnection = accept(dataListener, (struct sockaddr *)&client_addr, &sin_size);
		
		if(dataConnection ==-1){
			perror("accept");
			continue;
		}		
		if(!fork()){
			close(dataListener);
			
			if(send(dataConnection, dirList, sizeof dirList, 0) != 0){
				perror("send");
			}
			dataSent = true;
			close(dataConnection);
		}
	}
	
	close(dataConnection);
	close(dataListener);
	return;

}

// http://www.cplusplus.com/reference/cstdio/fread/
void sendFile(int *controlConnection, char dataPort[6], char fileName[128]){
	char noFile[] = "File not found / invalid file name.";
	bool fileSent = false;

	/***************************************
					Open File 
	****************************************/
	FILE *fp;	
	
	fp = fopen(fileName, "r");
	
	if (fp == NULL){
		printf("Can't open file %s", fileName);
		send(controlConnection, &noFile, sizeof noFile, 0);
		exit(1);
	}

	/***************************************
		Allocate buffer and read file in
	****************************************/	
	long fileSize;
	char * buffer;
	size_t result;

	//allocate buffer
	buffer = (char*) malloc (sizeof(char)*fileSize);
	if(buffer == NULL){
		fprintf(stderr, "Error allocating buffer.");
		exit(1);
	}
	
	//read file into buffer
	result = fread(buffer, 1, fileSize, fp);
	if(result != fileSize){
		fprintf(stderr, "Error reading file into buffer.");
		exit(1);		
	}
	
	/*****************************************
		Open data connection, and send file
	*****************************************/
	//open socket for data connection and accept
	int dataListener = startServer(dataPort);
	int dataConnection;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;	
	
	printf("Server: attempting to establish data connection on port %s\n", dataPort);
	//main listen loop
	while(!fileSent) {
		sin_size = sizeof client_addr;
		dataConnection = accept(dataListener, (struct sockaddr *)&client_addr, &sin_size);

		if(dataConnection == -1){
			perror("accept");
			continue;
		}
		
		printf("Server: data connection established. Sending data...\n");
		if(!fork()){
			close(dataListener);
			
			//send the buffer to client
			int bytesSent = 0;
			int len = strlen(buffer);			
			int bytesLeft = len;
			int n;

			
			while(byteSent < len){
				n = send(dataConnection, buffer, sizeof buffer,0);
				if(bytesSent == -1){
					break;
				}
				bytesSent += n;
				bytesLeft -= n;
			}
			
			close(dataConnection);
			exit(0);
		}
		close(dataConnection);
		close(dataListener);			
		return;
	}	

}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}
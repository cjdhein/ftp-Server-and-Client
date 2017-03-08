#include "ftpServer.h"

int main(int argc, char *argv[]){
	char portNum[6];
	int listen_socket, control_socket;

	validateArguments(argc, argv);
	strcpy(portNum, argv[1]);

	runServer(startServer(portNum));	
}


# Rudimentary FTP Server & Client

By: Cody J Dhein
____________________________________________________________________________________________________________
The apps function by creating a control connection to accept commands from the client and if valid, create a data connection to send the data over to the server.

They are only able to send/recv a directory listing or a simple .txt file. 

Note that both programs can only function in the directory from which they are executed, so it makes most sense to have the client
be run from a different directory if they are being run from the same host.

Different hosts are usable, all you need to do is provide the host name to the client when connecting. It should work
provided there is open communication between the hosts and no proxy or firewall issues.


Before running, each must be compiled as follows:


**********
## The Server
**********
The server is written in C and can be compiled via command line as follows:

"g++ ftpServer.c -o ftpServer"

This will create a program titled ftpServer that can then be run.


**********
## The Client
**********
The client is written in Java and can be compiled via command line as follows:

"javac ftpClient.java"

This will create a Java class file that can be executed.


**********************
# How To Use The Program
**********************

## 1) Start the Server
____________________________________________________________________________________________________________
The first thing that needs to be done after compiling the programs is to start the ftp server.
This can be done via the command line by entering:

"ftpServer <port number>" - where 'ftpServer' is the name of the executable you compiled, and 'port number'
is the port that you want to start the ftp control connection.

If it is successful (e.g. nothing is already running on that port) you will be greeted with a message as follows:

"ftpServer is waiting for connections on port 19910"

As it indicates, the server is now waiting for client connections.



## 2) Run the Client
____________________________________________________________________________________________________________
Once the server is up and running we can then run the client. The client is set up to function
as a simple one shot executable. You load it up right at the command line with the proper
arguments and fire away. It will either give you what you want or let you know what went wrong.
The format for running it via command line is as follows:

"java ftpClient <host name> <control port> <data port> <command> <filename>"
|_______
		|"host name" 	- the name of the host that the server is running on. (e.g. "flip1.engr.oregonstate.edu")
		|
		|"control port" 	- the port number that the server is listening for connections on
		|
		|"data port" 	- the port that will be used for the data connection
		|
		|"command" 		- the command the client is sending. This can either be: 
		|			"-l" to request a directory listing from the server
		|			"-g" to request a file from the server. If "-g" is used a filename must be provided as well
		|
		|"filename" - the name of the file being requested. This is only used if the "-g" command was sent.
		 -----------------------------------------------------------------------------------------------------------
		 
If valid arguments were provided you will receive, depending on the command, either a screen printout of the directory
of the server, or the indicated text file.

If a file exists with the name you are requesting you will be prompted to overwrite it with a simple (y/n) prompt.
Answering no will terminate the program without overwriting.

/* 	Program:		ftpClient.java 	
**	By:				Cody J Dhein
**	For:			CS372 Project 2
**	Description:	A basic FTP client that can be used to request text files from the FTP server.
**					All parameters are passed via command line when running the client.
**						e.g. "java ftpClient <hostname> <controlport> <dataport> <command> <*filename>"
**						*filename is only needed if command is "-g"
**	Sources:		Guidance on usage from Java Tutorial "All About Sockets"
**						https://docs.oracle.com/javase/tutorial/networking/sockets/
**					Guidance on converting inputstream to a file
**						https://www.mkyong.com/java/how-to-convert-inputstream-to-file-in-java/
*/	
	/******************************************************************************************
	*	Name: requestList(PrintWriter sendCommand, BufferedReader ctrlReader, 
	*						String hostName, int dataPort, String command)
	*	
	*	Description: Initiates a request for directory listing from the server by 
	*			sending the desired command string and desired data port. The function
	*			then waits for the server's response to the request on the control 
	*			connection. 
	*			- If the server accepts, it sends 'accept' and a data connection is 
	*				established on the provided dataPort. The resulting data is requested 
	*				from the server and then printed to the screen. 
	*			- If the server refuses, it sends the reason which is then printed on
	*				screen before the program closes.
	*
	*	Input: 
	*			PrintWriter sendCommand - set up to write to the already established
	*						control connection. Responsible for sending the dataPort and
	*						command to the server.
	*					
	*			BufferedReader ctrlReader - set up to read from the already established
	*						control connection. Responsible for reading the server's response
	*						to the sent request.
	*
	*			String hostName - the host name of the server that the client is connected to.
	*						Used to establish the data connection.
	*
	*			int dataPort - a 0 to 6 digit integer representing the port to establish
	*						a data connection on. This should already have been validated
	*						to be a valid port number (between 1 and 65535)
	*
	*			String command - the command representing what is being requested from the
	*						server. This will be "-l" in this function
	*
	*	Output:
	*			Printed to screen - the directory from the server if successful or an error 
	*						string from the server if not.
	******************************************************************************************/	
	
	/******************************************************************************************
	*	Name: requestFile(PrintWriter sendCommand, BufferedReader ctrlReader, 
	*						String hostName, int dataPort, String command, String filename)
	*	
	*	Description: Initiates a request for a specified file from the server by 
	*			sending the desired command string, desired data port, and desired filename. 
	*			The function then waits for the server's response to the request on the control 
	*			connection. 
	*			- If the server accepts, it sends 'accept' and a data connection is 
	*				established on the provided dataPort. The file is then transmitted and saved
	*				to the working directory. If it already exists, confirmation for overwrite
	*				is requested from the user.
	*			- If the server refuses, it sends the reason which is then printed on
	*				screen before the program closes.
	*
	*	Input: 
	*			PrintWriter sendCommand - set up to write to the already established
	*						control connection. Responsible for sending the dataPort and
	*						command to the server.
	*					
	*			BufferedReader ctrlReader - set up to read from the already established
	*						control connection. Responsible for reading the server's response
	*						to the sent request.
	*
	*			String hostName - the host name of the server that the client is connected to.
	*						Used to establish the data connection.
	*
	*			int dataPort - a 0 to 6 digit integer representing the port to establish
	*						a data connection on. This should already have been validated
	*						to be a valid port number (between 1 and 65535)
	*
	*			String command - the command representing what is being requested from the
	*						server. This will be "-g" in this function.
	*
	*			String filename - the name of the file being requested from the server. 
	*	Output:
	*			Printed to screen - a string indicating if the file was successfuly received.
	*			Saved to directory - the file that was requested.
	******************************************************************************************/	
	
	/******************************************************************************************
	*	Name: validateArgs(String[] args)
	*	
	*	Description: Validates that the correct number and type of arguments were provided. 
	*				I
	*
	*	Input: 
	*			String[] args - array of the arguments passed to the program. Parsed to confirm
	*					their type and value are valid.
	*	Output:
	*			Returned - array of either 4 or 5 validated arguments. (5 if command is to get file)
	******************************************************************************************/	
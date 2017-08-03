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
**					General referencing
**						https://docs.oracle.com/javase/7/docs/api/
*/	


import java.net.*;
import java.io.*;
import java.util.*;


public class ftpClient{
	
	public static void main (String[] args) throws IOException{
		String hostName, command, filename;
		String[] validatedArgs;
		int ctrlPort, dataPort;
		boolean gettingFile = false;
		
		validatedArgs = validateArgs(args);
		
		hostName = validatedArgs[0];
		ctrlPort = Integer.parseInt(validatedArgs[1]);
		dataPort = Integer.parseInt(validatedArgs[2]);
		command = validatedArgs[3];
		
		if(validatedArgs.length > 4){
			filename = validatedArgs[4];
			gettingFile = true;
		}else{
			filename = "";
		}
		
		System.out.println("Attempting to connect to " + hostName + " on port " + ctrlPort + ". . .");
		try{	
			Socket controlSocket = new Socket(hostName, ctrlPort);
			System.out.println("Connected on port " + ctrlPort);	
			PrintWriter sendCommand = new PrintWriter(controlSocket.getOutputStream(), true);
			InputStreamReader ctrlIsr = new InputStreamReader(controlSocket.getInputStream());
			BufferedReader ctrlReader = new BufferedReader(ctrlIsr);
			
			if(gettingFile){
				requestFile(sendCommand, ctrlReader, hostName, dataPort, command, filename);
			}else{
				requestList(sendCommand, ctrlReader, hostName, dataPort, command);				
			}
						
		} catch(UnknownHostException e){
			System.err.println("Unable to locate host: " + hostName);
			System.exit(1);
		} catch(IOException e){
			System.err.println("Unable to locate host: " + hostName);
			System.exit(1);			
		}
		
		
	}

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
	public static void requestList(PrintWriter sendCommand, BufferedReader ctrlReader, String hostName, int dataPort, String command){
		String serverResponse;

		try{
			//send command
			sendCommand.println(dataPort + " " + command);			

			System.out.println("Waiting for server response");;
			while((serverResponse = ctrlReader.readLine()) != null){
				
				//trim whitespace from message
				serverResponse = serverResponse.trim();
				
				if(serverResponse.equals("accept")){
					Socket dataSocket = new Socket(hostName, dataPort);

					InputStreamReader dataIsr = new InputStreamReader(dataSocket.getInputStream());
					BufferedReader dataReader = new BufferedReader(dataIsr);			

					char[] serverOutput = new char[512];
					dataReader.read(serverOutput, 0, 512);
					String fileListing = new String(serverOutput);
					System.out.println(fileListing);
					
					dataSocket.close();
					return;
				}else{
					System.out.println(serverResponse);
					System.exit(1);
				}
			}
			
		} catch(IOException e){
			System.out.println("Exception caught trying to read from server.");			
			System.out.println(e.getMessage());
		}
		System.out.println("serverResponse = null");
	}

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
	public static void requestFile(PrintWriter sendCommand, BufferedReader ctrlReader, String hostName, int dataPort, String command, String filename){
		String serverResponse;

		try{
			System.out.println("Sending command");
			//send command to server
			sendCommand.println(dataPort + " " + command + " " + filename);			
			while((serverResponse = ctrlReader.readLine()) != null){
				//trim whitespace from message
				serverResponse = serverResponse.trim();
				
				if(serverResponse.equals("accept")){ //server accepted and will open data connection

					Socket dataSocket = new Socket(hostName, dataPort);
					InputStreamReader dataIsr = new InputStreamReader(dataSocket.getInputStream());
					BufferedReader dataReader = new BufferedReader(dataIsr);						

				//Guidance from: http://www.mkyong.com/java/how-to-construct-a-file-path-in-java/
					String currDirectory = System.getProperty("user.dir"); 
					String fullPath = currDirectory + File.separator + filename;
					
					File newFile = new File(fullPath);
					if(newFile.exists()){
						Scanner s = new Scanner(System.in);
						System.out.println("File already exists in directory. Overwrite? (Y/N)");
						String answer = s.nextLine();
						if(!(answer.equals("Y") || answer.equals("y"))){
							System.out.println("File will not be overwritten. Exiting program.");
							System.exit(1);
						}else{
							newFile.delete();
							newFile.createNewFile();
						}
					}else{
						newFile.createNewFile();
					}
					
					String fromServer = "";
					String status;
					while((status = dataReader.readLine()) != null){
						fromServer += status;
					}
					//System.out.println(fromServer);
					//writing utilities
					FileWriter fw = new FileWriter(newFile);
					BufferedWriter bw = new BufferedWriter(fw);
					PrintWriter pw = new PrintWriter(bw, true);
					pw.write(fromServer);
					
					System.out.println("File '" + filename +"' successfully received.");
					
					//close utilities
					if(bw != null)
						bw.close();
					if(fw != null)
						fw.close();
					dataIsr.close();
					dataReader.close();
					dataSocket.close();		
				

					dataSocket.close();
					return;
				}else{
					System.out.println(serverResponse);
					System.exit(1);
				}
			}
			
		} catch(IOException e){
			System.out.println("Exception caught trying to read from server.");			
			System.out.println(e.getMessage());
		}
		System.out.println("serverResponse = null");
	}	
	

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
	public static String[] validateArgs(String[] args){
	
	
	//validate ports
	try{
		int testHostPort, testDataPort;
		
		/* check arguments for validity */
		// check number of args
		int numArgs = args.length;
		if(numArgs < 4 || numArgs > 5){
			System.err.println("Invalid number of arguments provided.");
			System.err.println("Expected usage: java ftpClient <server host> <server port> <data port> <command> <file name> (if command = -g)");
			System.exit(1);					
		}
		
		//set hostName
		String testHostName = args[0];
		
		//validate command, if -g, set filename
		String testCommand = args[3];
		String testFile = "";
		switch (testCommand){
			case "-l": 
				if(numArgs == 5){
					System.err.println("Invalid number of arguments provided.");
					System.err.println("filename should not be provided with the -l command");
					System.exit(1);													
				}
				break;
			case "-g":
				if(numArgs != 5){
					System.err.println("Invalid number of arguments provided.");
					System.err.println("-g command requested, but no filename provided.");
					System.exit(1);									
				}
				testFile = args[4];
				break;
			default:
					System.err.println("Invalid command provided. Must be -l or -g <filename>");
					System.exit(1);				
		}
			
		testHostPort = Integer.parseInt(args[1]);
		testDataPort = Integer.parseInt(args[2]);
		if(testHostPort < 1 || testHostPort > 65535){
			System.err.println("Invalid server port. Must be valid port number");
			System.exit(1);			
		}
		if(testDataPort < 1 || testDataPort > 65535){
			System.err.println("Invalid data port. Must be valid port number");
			System.exit(1);			
		}
		if(testDataPort == testHostPort){
			System.err.println("Host and data ports cannot be the same.");
			System.exit(1);							
		}
		
		//return array of validated arguments
		
		if(numArgs == 5){
			String[] longArg = {testHostName, args[1], args[2], testCommand, testFile};
			return longArg;
		}else{
			String[] shortArg = {testHostName, args[1], args[2], testCommand};
			return shortArg;
		}		
		
	} catch(NumberFormatException e){
		System.err.println("Invalid argument provided.");
		System.err.println("Expected usage: java ftpClient <server host> <server port> <data port> <command> <file name> (if command = -g)");
		System.exit(1);	
		}

	return null;
		
	}
}
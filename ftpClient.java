/* 	Program:	ftpClient.java 	
**	By:			Cody J Dhein
**	For:		CS372 Project 2
**	Sources:	Guidance on usage from Java Tutorial "All About Sockets"
**					https://docs.oracle.com/javase/tutorial/networking/sockets/
**				
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
			
			PrintWriter sendCommand = new PrintWriter(controlSocket.getOutputStream(), true);
			InputStreamReader ctrlIsr = new InputStreamReader(controlSocket.getInputStream());
			BufferedReader ctrlReader = new BufferedReader(ctrlIsr);
			
			if(gettingFile){
				System.out.println("calling requestFile");
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
	
	public static Socket establishDataConnection(String hostName, int dataPort){
		Socket dataSocket;
		
		try{
			//System.in.read();
			dataSocket = new Socket(hostName, dataPort);
			return dataSocket;
		} catch(UnknownHostException e){
			System.err.println("Unable to locate host: " + hostName);
			System.exit(1);
		} catch(IOException e){
			System.err.println("Unable to locatey host: " + hostName);
			System.err.println(e.getMessage());
			System.exit(1);			
		}
		return null;
	}
	
	public static String getData(Socket dataSocket, boolean gettingFile){
		try{
			InputStreamReader dataIsr = new InputStreamReader(dataSocket.getInputStream());
			BufferedReader dataReader = new BufferedReader(dataIsr);			
			if(!gettingFile){ //accept file listing and print to user
				char[] serverOutput = new char[512];
				dataReader.read(serverOutput, 0, 512);
				String fileListing = new String(serverOutput);
				return fileListing;
			}		
		}catch(IOException e){
				System.out.println("Exception caught trying to read from server.");			
				System.out.println(e.getMessage());
			}
		return "Shouldn't be here\n";
	}
	
	public static void requestList(PrintWriter sendCommand, BufferedReader ctrlReader, String hostName, int dataPort, String command){
		String serverResponse;
	
		//send command
		sendCommand.println(dataPort + " " + command);
		
		//get response
		try{
			//send command
			sendCommand.println(dataPort + " " + command);			

/* 			int x;
			for(int i = 0; i < 10; i++){
				x = ctrlReader.read();
				System.out.println(x + " ");				
			} */
			while((serverResponse = ctrlReader.readLine()) != null){
				
				//trim whitespace from message
				serverResponse = serverResponse.trim();
				
				if(serverResponse.equals("accept")){
					//TODO: Set up data connection
				//System.out.println("accepted. calling establishDataConnection");
					Socket dataSocket = establishDataConnection(hostName, dataPort);
					serverResponse = getData(dataSocket, false);
					System.out.println(serverResponse);
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
	
	public static void requestFile(PrintWriter sendCommand, BufferedReader ctrlReader, String hostName, int dataPort, String command, String filename){
		String serverResponse;
		System.out.println("Sending command");
		//send command to server
		sendCommand.println(dataPort + " " + command + " " + filename);
		
		//wait for server accept handshake
		try{
			while((serverResponse = ctrlReader.readLine()) != null){
				//trim whitespace from message
				serverResponse = serverResponse.trim();
				
				if(serverResponse.equals("accept")){ //server accepted and will open data connection
					System.out.println("Sending command");
					Socket dataSocket = establishDataConnection(hostName, dataPort);
					
				}
			}
			
		} catch(IOException e){
			System.out.println("Exception caught trying to read from server.");			
			System.out.println(e.getMessage());
		}
		//getData();
	}	
	
	
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
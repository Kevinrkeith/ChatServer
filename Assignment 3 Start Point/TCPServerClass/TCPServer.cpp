#include "TCPServer.h"

//--------------------------------------------------------------------------------
// Default Constructor
TCPServer::TCPServer()
{
	errorState = ERR_NONE;
	ipAddress = "127.0.0.1";
	port = 54000;
	FD_ZERO(&masterSockets);
}

//--------------------------------------------------------------------------------
// Paramaterized constructor allows server to run on any ip address and port
TCPServer::TCPServer(string inIPAddress, int inPort)
{
	errorState = ERR_NONE;
	ipAddress = inIPAddress;
	port = inPort;
	FD_ZERO(&masterSockets);
}
//--------------------------------------------------------------------------------
// Deconstructor cleans up socket resourses
TCPServer::~TCPServer()
{
	ShutDown();
}

//--------------------------------------------------------------------------------
//	Cleanup Winsock resouces
void TCPServer::ShutDown()
{
	for (int i = 0; i < masterSockets.fd_count; i++)	// loop through all the sockets in the master set
	{
		SOCKET socket = masterSockets.fd_array[i];		// grab the socket from the array
		closesocket(socket);							// close the socket
		FD_CLR(socket, &masterSockets);					// remove the socket from the master set
	}

	WSACleanup();										// clean up Winsock
}

//--------------------------------------------------------------------------------
// Initialized Winsock and calls the CreateSocket function if successful
bool TCPServer::Initialize()
{
	 int result;

	 result = WSAStartup(winSockVerion, &winSockData);		// startup winsock and ensure we are good to continue
	 if (result == 0)										// all good, create and bind the socket
	 {
		 if(verbose)
			cout << "Server status ..." << endl << ">>> WSA Start Up" << endl;

		 CreateSocket();
		 if (errorState == ERR_NONE)						// if we have no errors creating and binding return true, we're ready to go
			 return true;
	 }
	 else
	 {
		 errorState = ERR_STARTUP;							// set startup error code 
		 errorCode = result;
		 false;
	 }
}

//--------------------------------------------------------------------------------
// Creates the server listeing socket and calls BindSocket if successful
void TCPServer::CreateSocket()
{
	// Create the socket the server will listen to for applications wishing to connect to the server

	listeningSocket = socket(AF_INET, SOCK_STREAM, 0);  // create a socket which uses an IPv4 address, stream type socket with no set protocol (socket will figure it out)
	if (listeningSocket != INVALID_SOCKET)
	{
		if (verbose)
			cout << ">>> Listening socket configured" << endl;

		BindSocket();
	}
	else
	{
		errorState = ERR_LISTENING;
	}
}

//--------------------------------------------------------------------------------
// Binds the socket to the IP/port
void TCPServer::BindSocket()
{
	// Bind the socket to an IP address and port
	// In order to bind we need to fill in the socket address stucture prior to calling the bind function

	int result;
	sockaddr_in severAddrInfo;								// socket address variable
	int serverAddrSize = sizeof(sockaddr_in);				// get the size of the structure as it is required for the bind call
	severAddrInfo.sin_family = AF_INET;						// set to IPv4
	severAddrInfo.sin_port = htons(port);					// choose port number - note, stay away from widely accepted port numbers
	inet_pton(AF_INET, ipAddress.c_str(), &severAddrInfo.sin_addr);	// convert the IP info to binary form and return via sin.addr member

	result = bind(listeningSocket, (sockaddr*)&severAddrInfo, serverAddrSize);   // bind the listening socket with the specified IP info
	if (result != SOCKET_ERROR)								// check for successful binding of socket and address info
	{
		if(verbose)
			cout << ">>> Socket binding complete" << endl;
	}
	else
	{
		errorState == ERR_BIND;
	}
}

//--------------------------------------------------------------------------------
// Set the socket to listen for connection requests.  If we receive one call
// AcceptClient to accept the request
void TCPServer::Listen()
{
	// Set the socket to listen for connection requests 
	int result;

	result = listen(listeningSocket, SOMAXCONN);			// sets the socket to listen and sets the maximum size of pending request queue
	if (result != SOCKET_ERROR)
	{
		if( verbose )
			cout << ">>> Listening socket active: IP: " << ipAddress << " Port: " << port << endl << ">>> Accepting connections" << endl;

		AcceptClient();
	}
	else
	{
		errorState = ERR_LISTEN;
	}
}

//--------------------------------------------------------------------------------
// Accepts a client connection request and adds it to the master socket list so 
// messages will be caught by the select function
void TCPServer::AcceptClient()
{
	SOCKET clientSocket = INVALID_SOCKET;			// create a client socket so the server can receive the client that's attempting to connect.  
	sockaddr_in clientAddrInfo;						// required if we want some client machine connection info
	int clientAddrSize = sizeof(clientAddrInfo);	// get the size of the structure as it is required for the accept call

	clientSocket = accept(listeningSocket, (sockaddr*)&clientAddrInfo, &clientAddrSize);	// accept the client.  If we don't care about client connecting info set the last 2 parameters to NULL
	if (clientSocket != INVALID_SOCKET)
	{
		FD_SET(clientSocket, &masterSockets);					// add new client to the mastersockets  
		ServerMessage(clientSocket, SVR_CLIENT_CONNECTION);		// post message to the server indicating a client connection
	}
	else														//  client connection error
	{
		errorState = ERR_CLIENT_CONNECTION;
	}
}
//--------------------------------------------------------------------------------
// Receives all messages via the "select" function and processes them accordingly
void TCPServer::Receive()
{
	int messageCount;								// variable to track the number of message requiring processing
	timeval timeout;								// timeout value for the select statement to block on (optional)
	fd_set tempSockets;								// temporary list of sockets that we will copy to the master list to 
													// the select statement will clear whatever list it provided
													// so you MUST make a copy

	FD_ZERO(&tempSockets);							// clear the temporary socket list
	timeout.tv_sec = 1;								// setting the timeout for the select to 1 second

	FD_SET(listeningSocket, &masterSockets);		// add our listening socket into the master socket list

	if (verbose)
		cout << ">>> Server actively receiving and sending messages..." << endl;

	while (true)							// continuously looping and checking the select statement for sockets trying to connect or send messages
	{
		tempSockets = masterSockets;		// make a copy of the master socket list to send to the select statement

		messageCount = select(FD_SETSIZE, &tempSockets, NULL, NULL, &timeout);		// select statement replaces the passed in FD_SET (tempSockets) with all requests/messages which
																					// have been recieved relating to the sockets in tempSockets
		if (messageCount != SOCKET_ERROR)
		{
			for (int i = 0; i < messageCount; i++)					// loop through all new socket messages
			{
				SOCKET currentSocket = tempSockets.fd_array[i];		// grab the socket which requires servicing from the list

				if (currentSocket == listeningSocket)				// if it's the listening socket we need to accept a new client connection
				{
					AcceptClient();
				}
				else												// otherwise a new message was received from an existing connected socket
				{
					if (FD_ISSET(currentSocket, &masterSockets))	// checking to see if the socket is in our list of master sockets
					{
						ReceiveMessage(currentSocket);						// Receive message and process
					}
				}
			}
		}
		else
		{
			cout << "Select Failed: " << WSAGetLastError() << endl;
		}
	}
}

//--------------------------------------------------------------------------------
// Recieves the message from the client socket and determines how to process it
void TCPServer::ReceiveMessage(SOCKET currentSocket)
{
	const int BUFFER_SIZE = 512;									// message size
	char buf[BUFFER_SIZE];											// buffer to hold the message info

	ZeroMemory(buf, BUFFER_SIZE);									// clear buffer prior to receiveing data 
	int bytesReceived = recv(currentSocket, buf, BUFFER_SIZE, 0);	// get the message from the socket

	if (bytesReceived > 0)									// process the results of the recv function
	{
		ProcessIncomingMessage(currentSocket, buf, bytesReceived);
	}
	else if (bytesReceived == 0)							// if the recv function returns with 0 bytes it's a client disconnection
	{
		ServerMessage(currentSocket, SVR_DISCONNECT);		// post disconnect message to server console
		closesocket(currentSocket);							// close the socket that the client was using
		FD_CLR(currentSocket, &masterSockets);				// clear the socket from the master sockets 
	}
	else													// SOCKET - ERROR of some type
	{
		if (WSAGetLastError() == WSAECONNRESET)				// disconnect error - generally from a client app shutdown or other disconnection
		{
			ServerMessage(currentSocket, SVR_DISCONNECT);	// post disconnect message to server console
			closesocket(currentSocket);						// close the socket that the client was using
			FD_CLR(currentSocket, &masterSockets);			// clear the socket from the master sockets 
		}
		else												// other error, output the message, may want to close the socket and remove from the master set
		{
			errorCode = ERR_SOCKET;
			ErrorMessage();
		}
	}
}

//--------------------------------------------------------------------------------
//	send a message to the client
void TCPServer::Send(SOCKET clientSocket, char buf[], int numBytes)
{

	int sent = send(clientSocket, buf, numBytes, 0);						// send the info we received back to the client
	if (sent != SOCKET_ERROR)
	{
		if (verbose)
		{
			char *serverMessage = new char[strlen(buf)];					// required for the server message because changing buf will cause all but first client to recieve a message without the \r\n

			strncpy_s(serverMessage, numBytes, buf, numBytes - 2);			// adjusting the string variable so it does not include \r\n
			cout << ">>> Sent[" << numBytes << "] -"; 						// we received some bytes so output them to the console
			ServerMessage(clientSocket, SVR_SENT, serverMessage);

			delete serverMessage;											// return memory from requested by the new operator
		}
	}
	else
	{
		cout << "Problem sending message to client. Err#" << WSAGetLastError() << endl;
	}
}

//--------------------------------------------------------------------------------
// Posts server messages to the console
void TCPServer::ServerMessage(SOCKET socket, eServerMessages messageType, string message)
{
	ClientIPPort clientInfo;
	GetClientConnectionDetails(socket, &clientInfo);

	switch (messageType)
	{
	case SVR_CLIENT_CONNECTION:
		cout << ">>> Client connection accepted - IP: " << clientInfo.clientIP << " Port: " << clientInfo.port << endl;
		break;
	case SVR_RECEIVE:
		cout << " IP:" << clientInfo.clientIP << " Port:" << clientInfo.port << " Msg: " << message << endl;
		break;
	case SVR_SENT:
		cout << " IP:" << clientInfo.clientIP << " Port:" << clientInfo.port << " Msg: " << message << endl;
		break;
	case SVR_DISCONNECT:
		cout << ">>> Client disconnected - IP: " << clientInfo.clientIP << " Port: " << clientInfo.port << endl;
		break;
	}
}

//--------------------------------------------------------------------------------
// Gets the IP and port of a client socket so the info can be posted to the server
// console.
void TCPServer::GetClientConnectionDetails(SOCKET socket, ClientIPPort *clientInfo)
{
	sockaddr_in socketAddrInfo;						// required if we want some client machine connection info
	int socketAddrSize = sizeof(socketAddrInfo);	// get the size of the structure as it is required for the accept call

	if (getpeername(socket, (sockaddr*)&socketAddrInfo, &socketAddrSize) != SOCKET_ERROR)			// get the socket address info from the socket #
	{
		inet_ntop(AF_INET, &socketAddrInfo.sin_addr, clientInfo->clientIP, sizeof(clientInfo->clientIP));	// retrieve the ip address portion
		clientInfo->port = ntohs(socketAddrInfo.sin_port);													// retrieve the port address portion and convert to correct format
	}
	else
	{
		errorState = ERR_SOCKET;
		ErrorMessage();
	}
}

//--------------------------------------------------------------------------------
// Error message helper function, prints out error based on obhects errorState.  
// errorState is set in various functions used to initialize Winsock
void TCPServer::ErrorMessage()
{
	if (errorState != ERR_NONE)
	{
		switch (errorState)
		{
			case ERR_STARTUP:
				cout << "Error starting up windows socket: " << errorCode << endl << endl;
				break;
			case ERR_LISTENING:
				cout << "Error creating socket: " << WSAGetLastError() << endl << endl;
				break;
			case ERR_BIND:
				cout << "Error binding socket: " << WSAGetLastError() << endl << endl;
				break;
			case ERR_LISTEN:
				cout << "Listen socket failure: " << WSAGetLastError() << endl << endl;
				break;
			case ERR_CLIENT_CONNECTION:
				cout << "Client connection failure: " << WSAGetLastError() << endl << endl;
				break;
		}
	}
}

//--------------------------------------------------------------------------------
// Process non connect/disconnect messages.  Command messages will be executed and
// simple messages will be echoed back to the client
void TCPServer::ProcessIncomingMessage(SOCKET currentSocket, string message, int bytesReceived)
{

	int lineFeedPos = message.find("\r\n");

	if (message != "\r\n")				// don't process solo linefeed messages
	{
		if (lineFeedPos != -1)			// linefeed characters are present in the message, strip them out
			message.erase(lineFeedPos, 2);

		if (verbose)
		{
			cout << endl << ">>> Recd[" << bytesReceived << "] -";
			ServerMessage(currentSocket, SVR_RECEIVE, message);
		}

		
	}
}

//--------------------------------------------------------------------------------
// Sends client welcome message and request a user name
void  TCPServer::SendClientMessage(SOCKET clientSocket, eClientMessages messageType)
{
	string serverMessage = "";
	if (clientSocket != INVALID_SOCKET) {
		if (messageType == CL_WELCOME) {
			serverMessage= ":SVC displays a list of server commands";
			serverMessage += "\r\n";
			serverMessage= ":UNL displays a list of Connected Users";
			serverMessage += "\r\n";
			serverMessage= ":UCN sets the chat users name";
			serverMessage += "\r\n";
			serverMessage= ":UNM user1;user2; message send the message to the users in the list";
			serverMessage += "\r\n";
			serverMessage= "Welcome to Kevin's Chat Server";
			serverMessage += "\r\n";
			serverMessage= "Please enter your username using the :UCN<user name> command";
			serverMessage += "\r\n";
		}
		if (serverMessage.length() > 0) {
			Send(clientSocket, &serverMessage[0], serverMessage.length());
		}
	}
}

//--------------------------------------------------------------------------------
// Stores client user names
void  TCPServer::SetClientUserName(SOCKET clientSocket, string chatUserName)
{
	
}

// --------------------------------------------------------------------------------
// Checks if client has a user name 
bool TCPServer::ClientHasChatUserName(SOCKET clientSocket)
{
	return false;
}

//--------------------------------------------------------------------------------
// Generates a list of connected users and sends them back to the requesting
// client
void  TCPServer::SendChatUserNames(SOCKET clientSocket)
{

}

//--------------------------------------------------------------------------------
// Sends the chat message to the designated client by user name
void  TCPServer::SendChatUserNamesMessage(SOCKET clientSocket, string commandMessage)
{
	
}


//--------------------------------------------------------------------------------
// Sends the recieved message to all connected sockets/users other than the
// listening socket and the message originating socket
void  TCPServer::ChatIt(SOCKET clientSocket, string sendMessage, int numBytes)
{

}


//--------------------------------------------------------------------------------
// returns the chat user name based on the socket
string TCPServer::GetClientUserName(SOCKET clientSocket)
{
	

	return "No Chat User Name found.  :(";
}

//--------------------------------------------------------------------------------
// Checks usernames and returns the socket associated with the user 
SOCKET TCPServer::GetUserNameSocket(string userName)
{


	return (SOCKET)-1;
}

//--------------------------------------------------------------------------------
// Remove the client from our list of clients 
void TCPServer::RemoveConnectedClientInfo(SOCKET socket)
{
	
}

//--------------------------------------------------------------------------------
// Display server commands 
void TCPServer::SendServerCommands(SOCKET clientSocket)
{

}

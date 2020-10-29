#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

//-------------------------------------------------------------------------------
// structure to hold client data
struct ClientIPPort
{
	char clientIP[16];
	int port;
};

//-------------------------------------------------------------------------------
// Class Definition
class TCPServer
{
	private:
		enum eErrorState{ERR_NONE, ERR_STARTUP, ERR_LISTENING, ERR_BIND, ERR_LISTEN, ERR_CLIENT_CONNECTION, ERR_SOCKET};
		enum eServerMessages { SVR_CLIENT_CONNECTION, SVR_RECEIVE, SVR_SENT, SVR_DISCONNECT };
		enum eClientMessages {CL_WELCOME};

		bool verbose = true;
		bool echo = true;
		string ipAddress;
		int port;

		eErrorState errorState;
		int  errorCode;
		WORD winSockVerion = MAKEWORD(2, 2);	// set/use most recent version of windows sockets which is 2.2.  Not function call to startup requires it in a WORD data type
		WSADATA winSockData;					// struct which will hold required winsock data

		SOCKET listeningSocket = INVALID_SOCKET;
		fd_set masterSockets;

		//TODO: Data structure to hold client names

		void CreateSocket();
		void BindSocket();
		void AcceptClient();
		void ReceiveMessage(SOCKET currentSocket);
		void ProcessIncomingMessage(SOCKET currentSocket, string message, int bytesReceived);
		void ChatIt(SOCKET clientSocket, string sendMessage, int numBytes);
		void Send(SOCKET clientSocket, char buf[], int numBytes);
		void ServerMessage(SOCKET socket, eServerMessages messageType, string message = "");
		void GetClientConnectionDetails(SOCKET socket, ClientIPPort *clientInfo);


		//implement each of these functions
		void  SendClientMessage(SOCKET clientSocket, eClientMessages messageType);
		void  SetClientUserName(SOCKET currentSocket, string chatUserName);
		string GetClientUserName(SOCKET clientSocket);
		bool ClientHasChatUserName(SOCKET clientSocket);
		void SendChatUserNames(SOCKET clientSocket);
		void SendChatUserNamesMessage(SOCKET clientSocket, string sendMessage);
		SOCKET GetUserNameSocket(string userName);
		void RemoveConnectedClientInfo(SOCKET socket);			
		void SendServerCommands(SOCKET socket);

	public:
		TCPServer();
		TCPServer(string inIPAddress, int inPort);
		~TCPServer();
		bool Initialize();
		void Listen();
		void ShutDown();
		void Receive();
		void ErrorMessage();
};
#include "TCPServer.h"
#include <iostream>

using namespace std;

void main()
{
	TCPServer myServer;				// instantiate a TCPServer object

	if (myServer.Initialize())		// initialize the server object, if successful listen for clients and receive messages
	{
		myServer.Listen();
		myServer.Receive();
	}
	else							// error occurred during initialization, display error message
	{
		myServer.ErrorMessage();
	}

	cout << endl << endl;
	system("pause");
}
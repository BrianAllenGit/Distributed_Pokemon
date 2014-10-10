
/* Game Server.cpp
When running this program, begin by starting up the Game Server. 
This program sets up a connection with Clients. Once a connection is established, You
can receive commands from clients to enter into the game. When receive a message,
the server displays the message you receieved, processes the keystroke , and then sends it back to all clients.
*/

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "User32.lib")

#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>
#include <conio.h>
#include "sqlite3.h" 
#include <string.h>
#include <iostream>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */
#define KEYDOWN(name, key) (name[key] & 0x80) 
#define	QLEN		   5	/* maximum connection queue length	*/
#define	STKSIZE		16536
#define	LINELEN		 128
#define	BUFSIZE		4096
#define	WSVERS		MAKEWORD(2, 0)
u_short	portbase = 0;		/* port base, for test servers		*/
SOCKET	msock, ssock;		/* master & slave server sockets	*/

int	TCPechod(SOCKET);
void	errexit(const char *, ...);
SOCKET	passiveTCP(const char *, int);
SOCKET connectsock(const char *, const char *, const char *);
SOCKET connectTCP(const char *, const char *);
SOCKET passivesock(const char *, const char *, int);
SOCKET passiveTCP(const char *, int);
void broadcast();
void newUser(char user[20], char password[20], char ip[20], struct Users, SOCKET);
void validation(char user[20], char password[20], char ip[20], struct Users, SOCKET);
void doesUserExist(char user[20], char password[20], char ip[20], struct Users, SOCKET);
static int callback(void *data, int argc, char **argv, char **azColName);
void checkFileExist();
void getCred(struct Users, SOCKET);


//Structure to hold all of the current users.
struct Users {
	char *address;
	char *username;
	SOCKET sock;
};

static HWND hWnd = FindWindow(NULL, L"POKEMON RED");// Grabs the pokemon window
static Users currentUsers[50];//50 possible users
static int numConnected = 0;// used to switch between users
using namespace std;
sqlite3 *db;//pointer to db
char *zErrMsg = 0;//initialize 
int rc;
/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int _tmain(int argc, char *argv[])
{
	char	*service = "echo";	/* service name or port number	*/
	struct	sockaddr_in fsin;	/* the address of a client	*/
	int	alen;			/* length of client's address	*/
	WSADATA	wsadata;
	SetConsoleTitle(L"Game Server"); // Sets game title.
	checkFileExist();
	if (WSAStartup(WSVERS, &wsadata) != 0) // Initializes winsock2
	{
		errexit("WSAStartup failed\n"); // Fails if two of these programs are open
	}
	system("ipconfig"); // Posts IP to screen
	printf("\n\nAbove is a list of all of this server's interfaces and IP addresses");
	printf("\n\nWaiting for a connection...\n");
	msock = passiveTCP(service, QLEN); // Prepares master socket
	_beginthread((void(*)(void *))broadcast, STKSIZE, 0); // Starts the broadcast thread
	while (1)
	{
		alen = sizeof(fsin);
		ssock = accept(msock, (struct sockaddr *)&fsin, &alen); //If a socket connection occurs

		if (ssock == INVALID_SOCKET)
		{
			errexit("accept: error number\n", GetLastError());
		}
		currentUsers[numConnected].sock = ssock;// Save the socket number
		if (_beginthread((void(*)(void *))TCPechod, STKSIZE, (void *)ssock) < 0) //Start a thread that listens to that socket
		{
			errexit("_beginthread: %s\n", strerror(errno));
		}
		numConnected++;
	}
	return 1;	/* not reached */
}
/*--------------------------------------------------
*Each thread will listen for any incomming messages,
*send out the message to every other client, and then
*perform the action that the client requested.
*---------------------------------------------------
*/
int TCPechod(SOCKET fd)
{
	SOCKADDR_IN clientAddr;
	IN_ADDR clientIn;
	int nClientAddrLen = sizeof(clientAddr);
	char	buf[BUFSIZE], response[BUFSIZE], userName[20], password[20], ip[20];
	int	clientData, current, outchars;
	getpeername(fd, (struct sockaddr*)&clientAddr, &nClientAddrLen); //Finds IP address of peers
	memcpy(&clientIn, &clientAddr.sin_addr.s_addr, 4); // Copies into memory
	for (int k = 0; k < 50; k++) //Used to connect the global variable to this thread
	{
		if (currentUsers[k].sock == fd)
		{
			currentUsers[k].address = inet_ntoa(clientIn); current = k; break; // Stores IP address
		}
	}
	printf("Client connected!\tIP address: %s\tSocket: %d\n", currentUsers[current].address, currentUsers[current].sock);
	send(fd, "hello", 6, 0); //Validation message. If the validation fails, the thread will end.
	getCred(currentUsers[current], fd);

	for (int i = 0; i < 20; i++)
	{
		if (currentUsers[current].username[i] != '\n')
			userName[i] = currentUsers[current].username[i];
		else
		{
			userName[i] = '\0';
			break;
		}
	}
	clientData = recv(currentUsers[current].sock, buf, sizeof buf, 0); //message received wait.
	while (clientData != SOCKET_ERROR && clientData > 0) // While there is data coming in...
	{
		buf[clientData] = '\0';
		strcpy(response, "User ");   // Prepare the outgoing message
		strcat(response, userName);
		strcat(response, " entered ");
		strcat(response, buf);
		outchars = strlen(response);
		for (int k = 0; k < 51; k++) // Sends the message to every socket that isnt the one that sent the message
		{
			if (currentUsers[k].sock != fd)
				send(currentUsers[k].sock, response, outchars, 0);
		}
		SetForegroundWindow(hWnd); // Sets the keyboard focus to the game
		for (int i = 0; i < sizeof(buf); i++) // Used to prevent against uppercase / lowercase problems
		{
			buf[i] = tolower(buf[i]);
		}
		//The next section determines what the client typed, and then presses the appropriate button
		//in order to match what they sent.
		if (buf[0] == 'u' && buf[1] == 'p')
		{
			keybd_event(VkKeyScan('w'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('w'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 'd' && buf[1] == 'o' && buf[2] == 'w' && buf[3] == 'n')
		{
			keybd_event(VkKeyScan('s'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('s'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 'l' && buf[1] == 'e' && buf[2] == 'f' && buf[3] == 't')
		{
			keybd_event(VkKeyScan('a'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('a'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 'r' && buf[1] == 'i' && buf[2] == 'g' && buf[3] == 'h' && buf[4] == 't')
		{
			keybd_event(VkKeyScan('d'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('d'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 's' && buf[1] == 't' && buf[2] == 'a' && buf[3] == 'r' && buf[4] == 't')
		{
			keybd_event(VkKeyScan('l'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('l'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 's' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'e' && buf[4] == 'c' && buf[5] == 't')
		{
			keybd_event(VkKeyScan('k'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('k'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 'a')
		{
			keybd_event(VkKeyScan('z'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('z'), 0, KEYEVENTF_KEYUP, 0);
		}
		if (buf[0] == 'b')
		{
			keybd_event(VkKeyScan('x'), 0, 0, 0);
			Sleep(50);
			keybd_event(VkKeyScan('x'), 0, KEYEVENTF_KEYUP, 0);
		}
		fprintf(stderr, response); // Prints the data to the server

		clientData = recv(fd, buf, sizeof buf, 0); // waits for more data
	}
	if (clientData == SOCKET_ERROR) //If a client disconnects...
	{
		if (GetLastError() == 10054)
		{
			fprintf(stderr, "Client %s on Socket %d disconnected.\n", userName, fd); // Print who left
		}
		else
		{
			fprintf(stderr, "echo recv error: %d\n", GetLastError()); // If something else happened, print that error
		}
	}
	sqlite3_close(db);//close db
	closesocket(fd); // close the socket
	return 0;
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

void broadcast()
{/*
	char	buf[BUFSIZE];
	int outchars;
	while (fgets(buf, sizeof(buf), stdin)) //Sends out messages to all clients.
	{
		buf[LINELEN] = '\0';		
		outchars = strlen(buf);
		for (int k = 0; k < 51; k++)
		{
			send(currentUsers[k].sock, buf, outchars, 0);
		}
	}*/
}
static int callback(void *data, int argc, char **argv, char **azColName){
	int i;
	fprintf(stderr, "%s: ", (const char*)data);//print data
	for (i = 0; i<argc; i++){// loop through all arguments (line in db)
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");// print information in column name with column name or if no information present print "NULL"
	}
	printf("\n");//line return
	return 0;
}
void getCred(struct Users Temp, SOCKET sock){
	char userName[20], password[20], ip[20], input[50];
	char	buf[BUFSIZE]; 
	int	clientData;
	

	clientData = recv(sock, buf, sizeof buf, 0); //message received wait.
	buf[clientData] = '\0';
	strcpy_s(userName, buf);
	clientData = recv(sock, buf, sizeof buf, 0);
	buf[clientData] = '\0';
	strcpy_s(password, buf);
	strcpy_s(ip, Temp.address);
	clientData = recv(sock, buf, sizeof buf, 0);
	buf[clientData] = '\0';
	strcpy_s(input, buf);
	for (int k = 0; k < 50; k++) //Used to connect the global variable to this thread
	{
		if (currentUsers[k].sock == Temp.sock)
		{
			currentUsers[k].username = userName; Temp.username = userName; // Stores IP address
		}
	}
	switch (input[0]){
	case 'y': case'Y':
		doesUserExist(userName, password, ip, Temp, sock);
		break;
	case 'n': case 'N':
		validation(userName, password, ip, Temp, sock);
		break;
	default:
		printf("I have no idea what the fuck you're talking about\n");
		send(sock, "1" , 2, 0);
		getCred(Temp, sock);
		break;
	};
}
void doesUserExist(char user[20], char password[20], char ip[20], struct Users Temp, SOCKET sock){
	char str[150];
	strcpy_s(str, "SELECT ID from USERS WHERE USERNAME = '");
	strcat_s(str, user);
	strcat_s(str, "'");

	string itemname;
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, str, -1, &stmt, NULL);

	sqlite3_bind_text(stmt, 1, itemname.c_str(), -1, SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) == SQLITE_ROW)//if result found
	{
		int id = sqlite3_column_int(stmt, 0);
		//std::cout << "Found id=" << id << std::endl;
		printf("User exists. Try another username.\n");
		send(sock, "2", 2, 0);
		getCred(Temp, sock);
	}
	else{
		newUser(user, password, ip, Temp, sock);
	}
}
void validation(char user[20], char password[20], char ip[20], struct Users Temp, SOCKET sock){
	//char sql[] = "SELECT ID from USERS WHERE USERNAME = 'bkah' AND PASSWORD = 'sandiego22'";    
	char str[150];
	strcpy_s(str, "SELECT ID from USERS WHERE USERNAME = '");
	strcat_s(str, user);
	strcat_s(str, "' AND PASSWORD = '");
	strcat_s(str, password);
	strcat_s(str, "'");


	string itemname;
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, str, -1, &stmt, NULL);

	sqlite3_bind_text(stmt, 1, itemname.c_str(), -1, SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) == SQLITE_ROW)//if result found
	{
		int id = sqlite3_column_int(stmt, 0);
		//std::cout << "Found id=" << id << std::endl;
		printf("Connecting to server...\n");
		send(sock, "0", 2, 0);
		//WHERE SERVER CONNECTION WILL GO
	}
	else{
		printf("not found :(\n");
		send(sock, "3", 2, 0);
		sqlite3_finalize(stmt);
		getCred(Temp, sock);
	}
}
void newUser(char user[20], char password[20], char ip[20], struct Users Temp, SOCKET sock){
	char str[150];

	/*strcpy(str, "INSERT INTO USERS (ID,USERNAME,PASSWORD,IP) "  \ //works when hard coding ID
	"VALUES (1, '");*/
	strcpy_s(str, "INSERT INTO USERS (USERNAME,PASSWORD,IP) "  \
		"VALUES ('");
	strcat_s(str, user);
	strcat_s(str, "', '");
	strcat_s(str, password);
	strcat_s(str, "', '");
	strcat_s(str, ip);
	strcat_s(str, "');");


	rc = sqlite3_exec(db, str, callback, 0, &zErrMsg);
	(rc != SQLITE_OK) ? fprintf(stderr, "SQL error: %s\n", zErrMsg) : fprintf(stdout, "Records created successfully\n");
	send(sock, "4", 2, 0);
	getCred(Temp, sock);
}
void checkFileExist(){
	FILE *file;
	if (!(file = fopen("database.db", "r"))){
		printf("does not exist\n");
		/*OPEN DB CONNECTION */
		rc = sqlite3_open("database.db", &db);//open connection
		(rc != SQLITE_OK) ? fprintf(stderr, "SQL error: %s\n", zErrMsg) : fprintf(stdout, "Connected!\n");

		char sqlcmd[] = "CREATE TABLE USERS("  \
			"ID INTEGER PRIMARY KEY   AUTOINCREMENT," \
			"USERNAME        TEXT    NOT NULL," \
			"PASSWORD        TEXT    NOT NULL," \
			"IP           TEXT    NOT NULL);";

		/* Execute SQL statement */
		rc = sqlite3_exec(db, sqlcmd, callback, 0, &zErrMsg);
		(rc != SQLITE_OK) ? fprintf(stderr, "SQL error: %s\n", zErrMsg) : fprintf(stdout, "Table created successfully\n");
	}
	else {
		//printf("DB File exists\n");
		rc = sqlite3_open("database.db", &db);//open connection
		(rc != SQLITE_OK) ? fprintf(stderr, "SQL error: %s\n", zErrMsg) : fprintf(stdout, "\n");
	}
}
SOCKET connectsock(const char *host, const char *service, const char *transport)
{
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/


	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	/* Map service name to port number */
	if (pse = getservbyname(service, transport))
		sin.sin_port = pse->s_port;
	else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
		errexit("can't get \"%s\" service entry\n", service);

	/* Map host name to IP address, allowing for dotted decimal */
	if (phe = gethostbyname(host))
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
		errexit("can't get \"%s\" host entry\n", host);

	/* Map protocol name to protocol number */
	if ((ppe = getprotobyname(transport)) == 0)
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
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) ==
		SOCKET_ERROR)
		errexit("can't connect to %s.%s: %d\n", host, service,
		GetLastError());
	return s;
}
SOCKET connectTCP(const char *host, const char *service)
{
	return connectsock(host, service, "tcp");
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
	if (pse = getservbyname(service, transport))
		sin.sin_port = htons(ntohs((u_short)pse->s_port)
		+ portbase);
	else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
		errexit("can't get \"%s\" service entry\n", service);

	/* Map protocol name to protocol number */
	if ((ppe = getprotobyname(transport)) == 0)
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

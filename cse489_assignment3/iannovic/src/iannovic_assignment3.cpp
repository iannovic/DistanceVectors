/**
 * @iannovic_assignment3
 * @author  Ian Novickis <iannovic@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <errno.h>
#include <netdb.h>
#include "../include/global.h"

#include "../include/global.h"
#include "../include/logger.h"

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

int initListen();

/* INPUT PARAMETERS */

int number_of_servers;		//read this value in from the text file
int number_of_neighbors;	//read this value in from the text file
int timeout	;				//read from arg associated with -i

int port_number;			//read this value in from the text file

int main(int argc, char **argv)
{

	/*Init. Logger*/
        cse4589_init_log();

	/* Clear LOGFILE and DUMPFILE */
        fclose(fopen(LOGFILE, "w"));
        fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/
	
    /* DO VALIDATION OF CONSOLE INPUT AND MOVE ARGUMENTS TO VARIABLES APPROPRIATELY */
    cout << "beginning validation of console args... " << endl;
	if (argc != 4)
	{
			cout << argc << " is an illegal # of params" << endl;
			cout << "there must be 4 args" << endl;
			return -1;
	}

	if (strcmp(argv[1],"-i") == 0
			&& strcmp(argv[3],"-t") == 0)
	{
			/* argv[2] is interval and argv[4] is topology filepath */
	}
	else if (strcmp(argv[1],"-t") == 0
				&& strcmp(argv[3],"-i") == 0)
	{
			/* argv[4] is interval and argv[2] is topology filepath */
	}
	else
	{
			cout << "invalid arguments." << endl;
			return -1;
	}
	cout << "ending validation of console args... " << endl;


	/* READ TOPOLOGY FILE AND POPULATE PARAMETER VARIABLES*/

	/* INITIALIZE UDP LISTENING SOCKET WITH PARAMETER VARIABLES ACQUIRED FROM TOPOLOGY TEXT FILE*/
	if (initListen() == -1)
	{
		cout << "failed to open listening socket!" << endl;
		return -1;
	}
	return 0;
}

int initListen()
{
	struct addrinfo hints, *response;

	int fd = 0;
	memset(&hints,0, sizeof hints);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_UDP;

	/*
		get the address info
	*/
	if (getaddrinfo(NULL,port_number,&hints,&response) != 0)
	{
		cout << "failed to get addr info: " << strerror(errno) << endl;
		return -1;
	}
	fd = socket(AF_INET,SOCK_DGRAM,0);
	if (fd == -1)
	{
		cout << "failed to open a socket: " << strerror(errno) << endl;
		return -1;
	}
	if (bind(fd,response->ai_addr,response->ai_addrlen) != 0)
	{
		cout << "failed to bind socket to port: " << strerror(errno) << endl;
		return -1;
	}
	if (listen(fd,10) != 0)
	{
		cout << "failed to listen on socket" << strerror(errno) << endl;
		return -1;
	}

	//init the first node of the ip list to be the server
	listening_socket = new struct node;
	listening_socket->fd = fd;
	listening_socket->addr = response->ai_addr;
	listening_socket->next = NULL;
	listening_socket->port = port_number;
	cout << "listening port number is " << listening_socket->port << endl;
	return fd;
}

int main(int argc, char **argv)
{
     /*         VALIDATE ALL OF THE ARGUMENTS		 */


	port_number = argv[2];


	/*
	 * initiate the listening socket, both server and client
	 */
	if (initListen() == -1)
	{
		cout << "failed to open listening socket!" << endl;
		return -1;
	}

	/*
	 * populate listening_socket->address field with the external IP
	 */
	if (-1 == getExternalIp())
	{
		cout << "failed to get external ip" << endl;
		return -1;
	}
	cout << ".-*^*-._.-*^*-._.-*^*-._.-*^*-._WELCOME!.-*^*-._.-*^*-._.-*^*-._.-*^*-._" << endl;
	// add some validation to make sure port is a number
	while (true)
	{
		fd_set rfds;
		if (-1 == createRfds(&rfds))
		{
			cout << "failed to create the rfds" << endl;
			return -1;
		}
		FD_SET(0,&rfds); 	//read on stdin to see when it has input

		/********************************************************************************
		 * BEGIN THE SELECT() LOOP!
		 ********************************************************************************/
		if (select(1024,&rfds,NULL,NULL,NULL) == -1)
		{
			cout << "failed to select: " << strerror(errno) << endl;
			return -1;
		}
		//cout << "found input" << endl;
		/*
		 * this will always be the listening socket, both client and server
		 */
		if (FD_ISSET(listening_socket->fd,&rfds))
		{
			if (-1 == blockAndAccept())
			{
				cout << "failed to accept" << endl;
			}
			else
			{
				cout << "accepted a new connection!" << endl;
			}
		}

		struct node* head = open_connections_head;
		/**************************************************
		 * CHECK INPUT ON EACH SOCKET CONNECTION
		 **************************************************/
		while (head != NULL)
		{
			if (FD_ISSET(head->fd,&rfds))
			{
				//cout << "found input on socket: " << head->fd << endl;
				int bufLength = 256;
				char buf[256] = {0};
				ssize_t bytesReceived;
				bytesReceived = recv(head->fd,&buf,256,0);
				if (bytesReceived == -1)
				{
					cout << "failed to receive the message " << buf << endl;
				}
				/***************************************************
				 * CURRENT HEAD CLOSED CONNECTION
				 ***************************************************/
				else if (bytesReceived == 0)
				{
					cout << "the process at " << head->hostname << " has closed their socket" << endl;
					cout << "Closer's Address: " << head->address << endl;
					cout << "Closer's Port: " << head->port << endl;

					closeSocketAndDeleteNode(head);

					if (!isClient)
					{
						updateAndSendValidList();
					}
				}
				/*************************************************
				 * CHECK FOR MESSAGES
				 * each "message" has a "prefix" token which determines how to process the message
				 * if update -> we parse the message into a new valid list of connections from the server
				 * if message-> we assume that the message was sent to us
				 * if port -> we assume that the peer is sending its listening port number
				 *
				 *************************************************/
				else
				{
					cout << buf << endl;
					int maxTokens = 32;
					int tokenCount = 0;
					char *tokens[maxTokens];
					//cout << "initialized buffer" << endl;
					if (-1 == tokenizeBufferedMessage(buf,(char**)&tokens,maxTokens,&tokenCount))
					{
						cout << "failed to tokenize the buffer" << endl;
						return -1;
					}
					else if (tokenCount < 1)
					{
						cout << "too little tokens from the message..." << endl;
						//return -1;
					}
					//cout << "determining what function to run" << endl;
					std::string command = tokens[0];
					/**************************************************************
					 * CHECK TO SEE IF WE NEED TO UPDATE THE VALID LIST OF CONNECTIONS
					 **************************************************************/
					if (command.compare("update") == 0)
					{
						if (tokenCount < 3)
						{
							cout << "not enough tokens to run this command" << endl;
						}
						else if (!isClient)
						{
							cout << "server should not run this command" << endl;
						}
						//cout << "now building list" << endl;
						else if (1 == buildUpdatedValidList((char**)&tokens))
						{
							cout << "failed to build the valid connections list from server" << endl;
						}
						else
						{
							cout << "Valid connections have been updated by the server..." << endl;
							printValidList();
						}
					}
					/******************************************************************************
					 * CHECK TO SEE IF SOMEONE SENT THER LISTENING PORT NUMBER TO THIS PROCESS
					 ******************************************************************************/
					else if (command.compare("port") == 0)
					{
						cout << "starting to set port" << endl;
						if (tokenCount < 2)
						{
							cout << "failed to set port, not enough tokens" << endl;
						}
						else if (!isClient)
						{
							if (isContained(head->address,tokens[1],open_connections_head) == -1)
							{
								cout << "DANGER!! the peer trying to connect has already connected to you, closing the connection" << endl;
								/*
								 * write an errorr message to the client
								 */
								std::string errorMessage = "message Your connection was refused, you are already connected to me on another port/socket!";
								size_t bufLength = strlen(errorMessage.c_str());
								if (-1 == write(head->fd,errorMessage.c_str(),bufLength))
								{
									cout << "failed to send: " << strerror(errno) << endl;
								}
								closeSocketAndDeleteNode(head);
								if (-1 == close(head->fd))
								{
									cout << "failed to close the socket" << strerror(errno) << endl;
								}
							}
							else
							{
								/*
								 * move the value of tokens[1] into the head->port. we only do this now because we had to validate that it did not exist
								 */
								head->port = tokens[1];
								if (updateAndSendValidList() == -1)
								{
									/*
									 * we only updateClientList IF we are the server
									 * otherwise assume that its relevant to the open connections list only
									 */
									cout << "failed to update all clients valid list" << endl;
								}
								else
								{
									printOpenList();
								}
							}
						}
						else if (isClient && isContained(head->address,tokens[1],valid_connections_head) == 0)
						{
							cout << "DANGER!! The peer on this socket is not a validated host. closing the connection immedietely." << endl;
									/*
							 * write an errorr message to the client
							 */
							std::string errorMessage = "message Your connection was refused, you are not registered to the server";
							size_t bufLength = strlen(errorMessage.c_str());
							if (-1 == write(head->fd,errorMessage.c_str(),bufLength))
							{
								cout << "failed to send: " << strerror(errno) << endl;
							}
							closeSocketAndDeleteNode(head);
							if (-1 == close(head->fd))
							{
								cout << "failed to close the socket" << strerror(errno) << endl;
							}
						}
						else
						{
							/*
							 * this will happen if everythign else was successfull and we are the isClient == true
							 */
							head->port = tokens[1];
							printOpenList();
						}
					}
					/**************************************************************************
					 * CHECK TO SEE IF SOMEONE SENT A MESSAGE TO THIS PROCESS
					 **************************************************************************/
					else if (command.compare("message") == 0)
					{
						cout << "Message received from " << head->hostname << endl;
						cout <<	"Sender's IP: " 	<< head->address	<< endl;
						cout << "Sender's Port: " 	<< head->port		<< endl;
						cout << "Message: ";
						for (int i = 1; i < tokenCount; i ++)
						{
							cout << tokens[i] << " ";
						}
						cout << endl;
					}
				}
			}
			head = head->next;
		}
		/****************************************************************************
		 * BEGIN CHECKING FOR SHELL COMMANDS
		 ****************************************************************************/
		if (FD_ISSET(0,&rfds))
		{
			//cout << "reading from stdin" << endl;
			std::string line;
			std::getline(std::cin,line);
			int maxTokensRead = 32;
			string arg[maxTokensRead];
			int argc = 0;
			stringstream ssin(line);
			while (ssin.good())
			{
				ssin >> arg[argc];
				//cout << arg[argc] << endl;
				argc++;
				if (argc >= maxTokensRead)
				{
					cout << "cannot have that many arguments" << endl;
					break;
				}
			}

			if (arg[0].compare("myport") == 0)
			{
				printPort();
			}
			/*************************************************************************
			 * CREATOR COMMAND
			 *************************************************************************/
			else if (arg[0].compare("creator") == 0)
			{
					printCreator();
			}
			/*************************************************************************
			 * MYIP COMMAND
			 *************************************************************************/
			else if (arg[0].compare("myip") == 0)
			{
					printExternalIp();
			}
			/*************************************************************************
			 * HELP COMMAND
			 *************************************************************************/
			else if (arg[0].compare("help") == 0)
			{
					printHelp();
			}
			/*************************************************************************
			 * SEND COMMAND
			 *************************************************************************/
			else if (arg[0].compare("send") == 0)
			{
				struct node* retNode = NULL;
				if (argc < 3)
				{
					cout << "not enough arguments to run this command" << endl;
				}
				else if (!isClient)
				{
					cout << "server is not permitted to send messages" << endl;
				}
				else if (arg[1].compare("1") == 0)
				{
					cout << "you are not allowed to send a message to the server." << endl;
				}
				else if (-1 == getNodeById(&retNode,atoi(arg[1].c_str())))
				{
					cout << "invalid connection ID" << endl;
				}
				else
				{
					std::string prefix = "message ";
					size_t buflen = strlen(prefix.c_str());
					size_t max_buflen = 100 + buflen;

					/*
					 * start i at 2 because it ignores the first 2 arguments (send, connectionID)
					 */
					for (int i = 2; i < argc; i ++)
					{
						buflen = buflen + strlen(arg[i].c_str()) + 1;
					}

					//create buffer
					char buf[buflen];
					strcpy(buf,prefix.c_str());
					for (int i = 2; i < argc; i ++)
					{
						strcat(buf,arg[i].c_str());
						strcat(buf," ");
					}

					if (buflen >= max_buflen)
					{
						cout << "message cannot be greater than 100 characters, please shorten the message!" << endl;
					}
					else if (-1 == write(retNode->fd,&buf,buflen))
					{
						cout << "failed to send: " << strerror(errno) << endl;
					}
				}
			}
			/************************************************************************
			 * REGISTER COMMAND
			 ************************************************************************/
			else if (arg[0].compare("register") == 0)
			{
				if (argc != 3)
				{
					cout << "invalid number of arguments" << endl;
				}
				else if (!isClient)
				{
					cout << "you are the server, silly. You can't register." << endl;
				}
				else if (-1 == connectTo(arg[1],arg[2],0))
				{
					cout << "failed to register" << endl;
				}
			}
			/************************************************************************
			 * CONNECT COMMAND
			 ************************************************************************/
			else if (arg[0].compare("connect") == 0)
			{
				if (argc != 3)
				{
					cout << "invalid number of arguments" << endl;
				}
				else if (!isClient)
				{
					cout << "you are the server, silly. You can't register." << endl;
				}

				/*
				 * check to see if the connection is already in open connections list
				 */
				else if (isContained(arg[1],arg[2],open_connections_head) == -1)
				{
					cout << "connection with this address and port already exists!" << endl;
				}
				else if (-1 ==connectTo(arg[1],arg[2],1))
				{
					cout << "failed to connect" << endl;
				}
				else
				{
					printOpenList();
				}
			}
			/******************************************************************
			 * TERMINATE COMMAND
			 *****************************************************************/
			else if (arg[0].compare("terminate") == 0)
			{
				struct node* retNode = NULL;
				int id = atoi(arg[1].c_str());
				if (argc != 2)
				{
					cout << "when using terminate you need 2 arguments, second is the connectID. use list command to find connectionID's" << endl;
				}
				else if (arg[1].compare("1") == 0)
				{
					cout << "now terminating connection with server and all clients (cannot connect to peers while not connnected to server" << endl;
					struct node *head = open_connections_head;
					valid_connections_head = NULL;
					while (head != NULL)
					{
						struct node *deletedNode = head;
						head = head->next;
						if (-1 == closeSocketAndDeleteNode(deletedNode))
						{
							cout << "Failed to delete and close socket connection" << endl;
						}
					}

				}
				else if (getNodeById(&retNode,id) == -1)
				{
					cout << "failed to get the connection from the list by that ID, wrong ID perhaps?" << endl;
				}
				else if (closeSocketAndDeleteNode(retNode) == -1)
				{
					cout << "Failed to closeSocketAndDeleteNode() ..." << endl;
				}
				else
				{
					cout << "successfully terminated the socket connection!" << endl;
				}
			}
			/*****************************************************************************
			 * LIST COMMAND (open connections)
			 *****************************************************************************/
			else if (arg[0].compare("list") == 0)
			{
				printOpenList();
				cout << endl;
			}
			/*****************************************************************************
			 * PRINTS THE VALID LIST (added this for my own use)
			 *****************************************************************************/
			else if (arg[0].compare("valid") == 0)
			{
				printValidList();
				cout << endl;
			}
			/*****************************************************************************
			 * EXIT BYE QUIT COMMAND
			 *****************************************************************************/
			else if ((arg[0].compare("exit") == 0) ||(arg[0].compare("bye") == 0) || (arg[0].compare("quit") == 0))
			{
				struct node *head = open_connections_head;
				while (head != NULL)
				{
					struct node *deletedNode = head;
					head = head->next;
					if (-1 == closeSocketAndDeleteNode(deletedNode))
					{
						cout << "Failed to delete and close socket connection" << endl;
					}
				}

				if (-1 == closeSocketAndDeleteNode(listening_socket))
				{
					cout << "Failed to close listening socket on port " << port_number << endl;
					return -1;
				}
				else
				{
					cout << "Successfully closed the listening socket on port " << port_number	<< endl;
				}
				cout << ".-*^*-._.-*^*-._.-*^*-._.-*^*-._GOODBYE!.-*^*-._.-*^*-._.-*^*-._.-*^*-._" << endl;
				return -1;
			}
			/*******************************************************************
			 * IF INPUT IS UNRECOGNIZED...
			 *******************************************************************/
			else
			{
					cout << "invalid command, type 'help' if you need help." << endl;
			}
		}
		/***********************************************************************************************
		 * END OF STDIN CHECKING
		 ***********************************************************************************************/
	}
	return 0;
}




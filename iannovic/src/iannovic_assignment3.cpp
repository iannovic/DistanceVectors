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


/* ENDOF INPUT PARAMETERS */

int listening_fd;			// populated by initListen();

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
	listening_fd = initListen();
	if (listening_fd == -1)
	{
		cout << "failed to open listening socket!" << endl;
		return -1;
	}

	/* RUN THE SELECT LOOP FOREVER. EVENTS SHOULD OCCUR WHEN:
	 * 	a. A DATAGRAM IS RECEIVED.
	 * 		--> Determine which server it came from, update forwarding table.
	 * 		-->	Reset timeSinceUpdate variable for neighbor.
	 *  b. CONSOLE COMMANDS ARE RECEIVED
	 *  	--> Carry out command.
	 * 	c. TIMEOUT OF SELECT '
	 * 		--> Send out forwarding table to neighbors.
	 * 		-->	Increment timeSinceUpdate variable for each neighbor.
	 */

	fd_set rfds;
	FD_SET(listening_fd,&rfds);
	FD_SET(0,&rfds);

	if (select(1024,&rfds,NULL,NULL,NULL) == -1)
	{
		cout << "failed to select: " << strerror(errno) << endl;
		return -1;
	}
	if (FD_ISSET(listening_fd,&rfds))
	{
		cout << "received a datagram" << endl;
		/*	RECEIVED DATAGRAM.	*/
	}
	if (FD_ISSET(0,&rfds))
	{
		cout << "received console command" << endl;
		/* RECEIVED CONSOLE COMMAND */

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

		if (arg[0].compare("help") == 0)
		{
			cout << "do you need help?" << endl;
		}
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
	return fd;
}

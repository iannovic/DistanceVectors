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
 * General Publicj License for more details at
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
#include <fstream>
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

/* FORMAT OF ONE DATAGRAM ENTITY */
struct entry {
	uint32_t server_address;
	uint16_t server_port;
	uint16_t buffer;
	uint16_t server_id;
	uint16_t cost;			//cost to reach the specified server
} entry;

/* META STRUCTURE OF DATAGRAM */
struct header {
	uint16_t fields_count;			//size of the entry array;
	uint16_t server_port;			//port of sender
	uint32_t server_address;		//address of sender
	struct entry entries[5];		//there will be at most five entries in the array since only five servers
};

/* DATAGRAM IS A MEDIUM OF THIS STRUCTURE*/
struct forwarding_table {
	uint16_t intervals_since_update;	//intervals since last received update
	uint16_t server_id;					//id of the server with this table
	uint16_t number_of_servers;			//read this value in from the text file
	uint16_t number_of_neighbors;		//read this value in from the text file
	struct entry entries[5];			//list of entries in the file
};

int initListen();
int readTopologyFile(char * filepath, struct forwarding_table *ftable);
struct forwarding_table* initForwardingTable();					//DEFINE THIS


int main(int argc, char **argv)
{

	/*Init. Logger*/
        cse4589_init_log();

	/* Clear LOGFILE and DUMPFILE */
        fclose(fopen(LOGFILE, "w"));
        fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/

    struct forwarding_table *ftable = initForwardingTable();
    int timeout	;				//read from arg associated with -i
    int pathIndex = 0;

    int listeningFd;			// populated by initListen();

    /* DO VALIDATION OF CONSOLE INPUT AND MOVE ARGUMENTS TO VARIABLES APPROPRIATELY */
    cout << "beginning validation of console args... " << endl;
	if (argc != 5)
	{
			cout << argc << " is an illegal # of params" << endl;
			cout << "there must be 4 args" << endl;
			return -1;
	}

	if (strcmp(argv[1],"-i") == 0
			&& strcmp(argv[3],"-t") == 0)
	{
		timeout = atoi(argv[2]);
		pathIndex = 4;
			/* argv[2] is interval and argv[4] is topology filepath */
	}
	else if (strcmp(argv[1],"-t") == 0
				&& strcmp(argv[3],"-i") == 0)
	{
		timeout = atoi(argv[4]);
		pathIndex = 2;
			/* argv[4] is interval and argv[2] is topology filepath */
	}
	else
	{
			cout << "invalid arguments." << endl;
			return -1;
	}
	cout << "ending validation of console args... " << endl;


	/* READ TOPOLOGY FILE AND POPULATE PARAMETER VARIABLES*/
	cout << "reading topology file" << endl;
	if (-1 == readTopologyFile(argv[pathIndex],ftable))
	{
		cout << "failed to read topology file!" << endl;
		return -1;
	}

	cout << "successfully read the topology file" << endl;
	return 0;

	/* INITIALIZE UDP LISTENING SOCKET WITH PARAMETER VARIABLES ACQUIRED FROM TOPOLOGY TEXT FILE*/
	listeningFd = initListen();
	if (listeningFd == -1)
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
	FD_SET(listeningFd,&rfds);
	FD_SET(0,&rfds);

	if (select(1024,&rfds,NULL,NULL,NULL) == -1)
	{
		cout << "failed to select: " << strerror(errno) << endl;
		return -1;
	}
	if (FD_ISSET(listeningFd,&rfds))
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

int readTopologyFile(char * filepath, struct forwarding_table *ftable)
{
	/*
	 * Topology file:
	 *
	 * 0				server count
	 * 1				server neighbors
	 * 2 thru n			server id and corresponding ip and port
	 * n + 1 thru m 	my server id, neighbor server id, cost
	 *
	 * create forwarding table structure and populate it for this process
	 *
	 */

	/* READ IN THE TOPOLOGY FILE*/

	ifstream stream;
	stream.open(filepath,ios::in);
	int buffSize = 256;				//size, duh.

	char buff[buffSize];		//char buffer to save token values;
	memset(&buff,buffSize,0);
	char c;							//last read char from the file
	int buffIndex = 0;					//furthest index of a relevant char in the buffer
	int tokenIndex = 0;				//number of tokens read on the current line number
	int lineNumber = 0;				//how many lines have been read from the file
	int entryIndex = 0;				//index into the entry structure to determine which to populate

	int currentNeighborId = 0;		//reference to the last updated neighbor to update the cost

	cout << "starting to read" << endl;
	while (!stream.eof())
	{
		stream.get(c);
		cout << c << endl;
		if (c == ' ' || c == '\n')
		{
			//cout << "buffer contains " << buff << endl;
			if (lineNumber == 0)		//this line corresponds to the number of servers in the network
			{
				ftable->number_of_servers = atoi(buff);
				cout << "number of servers is " << ftable->number_of_servers << endl;
				buffIndex = 0;
			}
			else if (lineNumber == 1)	//this line corresponds to the number of neighbors of this process
			{
				ftable->number_of_neighbors = atoi(buff);
				cout << "number of neighbors is " << ftable->number_of_servers << endl;
			}
			else if (lineNumber <= 1 + ftable->number_of_servers)	//populate forwarding table with values
			{
				buff[buffIndex] = 0;
				switch (tokenIndex)
				{
					case 0:
							ftable->entries[entryIndex].server_id = atoi(buff);				//server-id in buffer
							break;
					case 1:
							ftable->entries[entryIndex].server_address = atoi(buff);		//ip address in buffer
							break;
					case 2:
							ftable->entries[entryIndex].server_port = atoi(buff);			//port in buffer
							cout << ftable->entries[entryIndex].server_id << " "
									<< ftable->entries[entryIndex].server_address << " "
									<< ftable->entries[entryIndex].server_port << " " << endl;
							entryIndex++;							//increment entryIndex because we populated each variable other than costs of this entry
							break;
				}
			}
			else if (lineNumber <= 1 + ftable->number_of_neighbors)
			{
				switch (tokenIndex)
				{
					case 0:	ftable->server_id = atoi(buff);				//server-id in buffer
						break;
					case 1:	currentNeighborId = atoi(buff);				//neighbor-id in buffer (used in case 2 of this switch)
						break;
					case 2: 											//cost in buffer
							for (int i = 0; i < 5; i++)
							{
								if (ftable->entries[i].server_id == currentNeighborId)
								{
									ftable->entries[i].cost = atoi(buff);
								}
							}
						break;
				}
			}

			/* SEPERATE IF BLOCK ON PURPOSE. DIFFERENT INDEX UPDATES BASED ON WHETHER THE LINE IS NEW OR IT IS JUST A SPACE*/
			if (c == '\n')
			{
				tokenIndex = 0;
				lineNumber++;
			}
			else
			{
				tokenIndex++;
			}
			buffIndex = 0;
			memset(&buff,buffSize,0);
		}
		else
		{
			/* MOVE CHAR INTO BUFFER AT INDEX */
			buff[buffIndex] = c;
			buffIndex++;
		}

	}

	cout << "reading top file complete!" << endl;
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

	/*UNCOMMENT THIS
	if (getaddrinfo(NULL,port_number_c,&hints,&response) != 0)
	{
		cout << "failed to get addr info: " << strerror(errno) << endl;
		return -1;
	}
	*/
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

struct forwarding_table* initForwardingTable()
{
	struct forwarding_table *ret = new forwarding_table;

	ret->intervals_since_update = 0;
	ret->number_of_neighbors = 0;
	ret->number_of_servers = 0;
	ret->server_id = 0;

	return ret;
}

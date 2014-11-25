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
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>u
#include <sys/fcntl.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <errno.h>
#include <netdb.h>
#include <fstream>
#include <time.h>
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
};

/* META STRUCTURE OF DATAGRAM */
struct header {
	uint16_t fields_count;			//size of the entry array;
	uint16_t server_port;			//port of sender
	uint32_t server_address;		//address of sender
	struct entry entries[5];		//there will be at most five entries in the array since only five servers
};

/* SIMILAR TO DATAGRAM ENTRY BUT FOR FORWARDING TABLE ONLY CHANGES THE ADDRESS STRUCTURE */
struct f_entry {
	uint16_t server_port;
	uint16_t buffer;
	uint16_t server_id;
	uint16_t cost;			//cost to reach the specified server
	uint16_t next_hop_id;				//id of the next hop server in the path to reach the final destination of server_id
	bool is_neighbor = false;
	bool is_disabled = false;
	struct sockaddr_in sa;				//socket address structure to store the address
};

/* DATAGRAM IS A MEDIUM OF THIS STRUCTURE*/
struct forwarding_table {
	uint16_t intervals_since_update;	//intervals since last received update
	time_t last_packet_rcvd;			//time of the last packet received
	uint16_t server_id;					//id of the server with this table
	uint16_t number_of_servers;			//read this value in from the text file
	uint16_t number_of_neighbors;		//read this value in from the text file
	struct f_entry entries[5];			//list of entries in the file
};

int initListen(char * portNumber);
int readTopologyFile(char * filepath, struct forwarding_table *ftable);
struct forwarding_table* initForwardingTable();					//DEFINE THIS
void printForwardingTable(struct forwarding_table *ftable);
int createDatagram(struct forwarding_table *ftable, struct header * ret,char * buffer);	//forwarding table to dgram
int createTable(struct header * dgram, struct forwarding_table **ftable);				//dgram to ftable
int serializeDatagram(struct header * dgram, char * buf);								//dgram to byte order buffer
int deserializeDatagram(struct header * dgram, char * buf);								// byte order buffer to dgram
int updateNeighbors (struct forwarding_table * ftable);
int sendTable(int serverId, struct forwarding_table * ftable);
int insertIntoRouting(struct forwarding_table *ftable, struct forwarding_table **rtable);
int updateForwardingTable(struct forwarding_table * routing_table, struct forwarding_table **rtable);
int updateTableEntry(struct forwarding_table *base,struct forwarding_table *comparee,int id);
int resetRoutingTableCosts(struct forwarding_table *routing_table);
bool isDisabled(struct forwarding_table * routing_table, int serverId);
int disable(struct forwarding_table *routing_table,struct forwarding_table **rtable, int server_id)
int main(int argc, char **argv)
{

	/*Init. Logger*/
        cse4589_init_log();

	/* Clear LOGFILE and DUMPFILE */
        fclose(fopen(LOGFILE, "w"));
        fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/
    int timeout	;				//read from arg associated with -i
    int pathIndex = 0;
    int numPacketsReceivedSinceLastPackets = 0;

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



	/* CREATE ROUTING TABLE TO STORE ALL FORWARDING TABLES FROM ALL SERVERS*/
	struct forwarding_table *ftable = initForwardingTable();
	struct forwarding_table *routing_table = initForwardingTable();
	struct forwarding_table *rtable[5];
	rtable[0] = ftable;
	for (int i = 1; i < 5; i ++)
	{
		rtable[i] = NULL;
	}

	/* READ TOPOLOGY FILE AND POPULATE PARAMETER VARIABLES*/
	cout << "reading topology file" << endl;
	if (-1 == readTopologyFile(argv[pathIndex],ftable))
	{
		cout << "failed to read topology file!" << endl;
		return -1;
	}

	cout << "successfully read the topology file" << endl;

	/* DETERMINE LOCAL PORT NUMBER FROM TOPOLOGY RESULTS	*/
    char portNumber[5];
    for (int i = 0; i < 5; i ++)
    {
    	if (ftable->entries[i].server_id == ftable->server_id)
    	{
    		sprintf(portNumber,"%d",ftable->entries[i].server_port);
    		cout << "process port number is " << portNumber << endl;
    	}
    }

    cout << "your initial forwarding table for this process is " << endl;
    printForwardingTable(rtable[0]);
	/* INITIALIZE UDP LISTENING SOCKET WITH PARAMETER VARIABLES ACQUIRED FROM TOPOLOGY TEXT FILE*/
	int listeningFd = initListen(portNumber);
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

	while (true)
	{
		struct timeval timer;
		timer.tv_sec = timeout;
		timer.tv_usec = 0;
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(listeningFd,&rfds);
		FD_SET(0,&rfds);

		int ret = select(1024,&rfds,NULL,NULL,&timer);
		if (ret == -1)
		{
			cout << "failed to select: " << strerror(errno) << endl;
			return -1;
		}
		else if (ret == 0)
		{
			cout << "Timeout, sending update" << endl;
			if (-1 == updateNeighbors(routing_table))
			{
				cout << "failed to update neighbors" << endl;
				return -1;
			}
		}
		if (FD_ISSET(listeningFd,&rfds))
		{
			/*	RECEIVED DATAGRAM.	*/
			cout << "received a datagram" << endl;
			numPacketsReceivedSinceLastPackets++;
			char buffer[sizeof(header)];
			int numBytes = recv(listeningFd,buffer,sizeof(header),0);
			if (numBytes == -1)
			{
				cout << "failed to recv dgram " << strerror(errno) << endl;
			}

			struct header dgram;
			deserializeDatagram(&dgram,buffer);

			struct forwarding_table *ft;
			if (-1 == createTable(&dgram,&ft))
			{
				cout << "failed to create table from datagram" << endl;
			}
			ft->last_packet_rcvd = time(NULL);
			//printForwardingTable(ft);
			insertIntoRouting(ft,rtable);
			updateForwardingTable(routing_table, rtable);
			printForwardingTable(routing_table);
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

			if (arg[0].compare("update") == 0)
			{
				cout << "update" << endl;
			}
			else if (arg[0].compare("step") == 0)
			{
				cout << "step" << endl;
				if (-1 == updateNeighbors(rtable[0]))
				{
					cout << "failed to update neighbors" << endl;
					return -1;
				}
				else cse4589_print_and_log("%s:SUCCESS",arg[0]);
			}
			else if (arg[0].compare("packets") == 0)
			{
				cout << "received " << numPacketsReceivedSinceLastPackets << " packets" << endl;
				numPacketsReceivedSinceLastPackets = 0;
				cse4589_print_and_log("%s:SUCCESS",arg[0]);
			}
			else if (arg[0].compare("display") == 0)
			{
				cout << "update" << endl;
			}
			else if (arg[0].compare("disable") == 0)
			{
				cout << "update" << endl;
			}
			else if (arg[0].compare("crash") == 0)
			{
				cout << "update" << endl;
			}
			else if (arg[0].compare("dump") == 0)
			{
				cout << "update" << endl;
			}
			else if (arg[0].compare("academic_integrity") == 0)
			{
				cout << "update" << endl;
			}
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
	int buffIndex = 0;				//furthest index of a relevant char in the buffer
	int tokenIndex = 0;				//number of tokens read on the current line number
	int lineNumber = 0;				//how many lines have been read from the file
	int entryIndex = 0;				//index into the entry structure to determine which to populate

	int currentNeighborId = 0;		//reference to the last updated neighbor to update the cost

	cout << "starting to read" << endl;
	while (!stream.eof())
	{
		stream.get(c);
		if (c == ' ' || c == '\n')
		{
			buff[buffIndex] = 0;
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
				switch (tokenIndex)
				{
					case 0:
							ftable->entries[entryIndex].server_id = atoi(buff);				//server-id in buffer
							break;
					case 1:

							if (1 != inet_pton(AF_INET,buff,&(ftable->entries[entryIndex].sa.sin_addr)))
							{
								cout << "failed to convert the buffer into an address structure" << endl;
							}
							/* REFERENCE THIS FOR PRINTING IP ADDRESS
							ip = inet_ntoa(ftable->entries[entryIndex].sa.sin_addr);
							cout << ip << endl;
							*/

							break;
					case 2:
							ftable->entries[entryIndex].server_port = atoi(buff);			//port in buffer
							cout << ftable->entries[entryIndex].server_id << " " <<  ftable->entries[entryIndex].server_port << " " << endl;
							entryIndex++;							//increment entryIndex because we populated each variable other than costs of this entry
							break;
				}
			}
			else if (lineNumber <= 1 + ftable->number_of_neighbors + ftable->number_of_servers)
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
									ftable->entries[i].is_neighbor = true;
									ftable->entries[i].next_hop_id = ftable->server_id;
									//cout << "server id " << currentNeighborId << "has cost " << 	ftable->entries[i].cost << endl;
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
	/* BEFORE WE RETURN, WE SET THE VALUE OF ENTRY CONTAINING THE PROCESS' INFO TO HAVE A SELF LINK COST OF 0*/
	for(int i = 0; i < 5; i++)
	{
		if (ftable->server_id == ftable->entries[i].server_id)
		{
			ftable->entries[i].cost = 0;
		}
		ftable->entries[i].next_hop_id = ftable->server_id;
	}

	cout << "reading top file complete!" << endl;
	return 0;
}

int initListen(char * portNumber)
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

	if (getaddrinfo(NULL,portNumber,&hints,&response) != 0)
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

struct forwarding_table* initForwardingTable()
{
	struct forwarding_table *ret = new forwarding_table;

	ret->intervals_since_update = 0;
	ret->number_of_neighbors = 0;
	ret->number_of_servers = 0;
	ret->server_id = 0;
	for (int i = 0; i < 5; i ++)
	{
		ret->entries[i].buffer = 0;
		ret->entries[i].server_id = 0;
		ret->entries[i].server_port = 0;
		ret->entries[i].cost = 65535;		//initialize this to be the max unsigned 16bit int value should be 65k
	}
	return ret;
}

void printForwardingTable(struct forwarding_table *ftable)
{
	cout << "servers: " << ftable->number_of_servers << endl;
	cout << "neighbors: " << ftable->number_of_neighbors << endl;
	cout << "server id: " << ftable->server_id << endl;
	for (int i = 0; i < 5; i++)
	{
		cout << "id: " << ftable->entries[i].server_id << " cost: " << ftable->entries[i].cost
				<< " addr: " << inet_ntoa(ftable->entries[i].sa.sin_addr) << " port: " << ftable->entries[i].server_port << endl;
	}
}

/* this function returns the non serialized and serialized byte order version of the datagram */
int createDatagram(struct forwarding_table *ftable, struct header * ret, char * buffer)
{
	ret->fields_count = ftable->number_of_servers;

	/* DETERMINE PORT AND ADDR OF SENDER */
	for (int i = 0; i < 5; i ++)
	{
		if (ftable->server_id == ftable->entries[i].server_id)
		{
			ret->server_port = ftable->entries[i].server_port;
			ret->server_address = ftable->entries[i].sa.sin_addr.s_addr;
		}
	}
	/* POPULATE EACH ENTRY OF THE DGRAM*/
	for (int i = 0; i < 5; i ++)
	{
		ret->entries[i].buffer = 0;
		ret->entries[i].cost = ftable->entries[i].cost;
		ret->entries[i].server_address = ftable->entries[i].sa.sin_addr.s_addr;
		ret->entries[i].server_id = ftable->entries[i].server_id;
		ret->entries[i].server_port = ftable->entries[i].server_port;
	}

	return serializeDatagram(ret,buffer);			//number of relevant bytes in the buffer
}

int serializeDatagram(struct header * dgram, char * buf)
{

	int bufferIndex = 0;
	uint16_t shortBuf;
	uint32_t longBuf;

	shortBuf = htons(dgram->fields_count);
	memcpy(buf + bufferIndex,&shortBuf,2);
	bufferIndex += 2;

	shortBuf = htons(dgram->server_port);
	memcpy(buf + bufferIndex,&shortBuf,2);
	bufferIndex += 2;

	longBuf =  htonl(dgram->server_address);
	memcpy(buf + bufferIndex,&longBuf,4);
	bufferIndex += 4;

	for (int i = 0; i < 5; i ++)
	{
		shortBuf = htons(dgram->entries[i].server_port);
		memcpy(buf + bufferIndex,&shortBuf,2);
		bufferIndex += 2;

		shortBuf = htons(dgram->entries[i].buffer);
		memcpy(buf + bufferIndex,&shortBuf,2);
		bufferIndex += 2;

		shortBuf = htons(dgram->entries[i].server_id);
		memcpy(buf + bufferIndex,&shortBuf,2);
		bufferIndex += 2;

		shortBuf = htons(dgram->entries[i].cost);
		memcpy(buf + bufferIndex,&shortBuf,2);
		bufferIndex += 2;

		longBuf = htonl(dgram->entries[i].server_address);
		memcpy(buf + bufferIndex,&longBuf,4);
		bufferIndex += 4;

	}							// = 12 chars * 5 = 60 + 8 = 68 bytes
	return bufferIndex;			//number of relevant bytes in the buffer
}

int deserializeDatagram(struct header * dgram, char * buf)
{

	int bufferIndex = 0;
	uint16_t shortBuf;
	uint32_t longBuf;

	uint16_t shortResultBuf;
	uint32_t longResultBuf;

	memcpy(&shortBuf,buf + bufferIndex,2);
	shortResultBuf = ntohs(shortBuf);
	dgram->fields_count = shortResultBuf;
	bufferIndex += 2;

	memcpy(&shortBuf,buf + bufferIndex,2);
	shortResultBuf = ntohs(shortBuf);
	dgram->server_port = shortResultBuf;
	bufferIndex += 2;

	memcpy(&longBuf,buf + bufferIndex,4);
	longResultBuf = ntohl(longBuf);
	dgram->server_address = longResultBuf;
	bufferIndex += 4;

	for (int i = 0; i < 5; i ++)
	{
		memcpy(&shortBuf,buf + bufferIndex,2);
		shortResultBuf = ntohs(shortBuf);
		dgram->entries[i].server_port = shortResultBuf;
		bufferIndex += 2;

		memcpy(&shortBuf,buf + bufferIndex,2);
		shortResultBuf = ntohs(shortBuf);
		dgram->entries[i].buffer = shortResultBuf;
		bufferIndex += 2;

		memcpy(&shortBuf,buf + bufferIndex,2);
		shortResultBuf = ntohs(shortBuf);
		dgram->entries[i].server_id = shortResultBuf;
		bufferIndex += 2;

		memcpy(&shortBuf,buf + bufferIndex,2);
		shortResultBuf = ntohs(shortBuf);
		dgram->entries[i].cost = shortResultBuf;
		bufferIndex += 2;

		memcpy(&longBuf,buf + bufferIndex,4);
		longResultBuf = ntohl(longBuf);
		dgram->entries[i].server_address = longResultBuf;
		bufferIndex += 4;

	}							// = 12 chars * 5 = 60 + 8 = 68 bytes
	return bufferIndex;			//number of relevant bytes in the buffer
}

int createTable(struct header * dgram, struct forwarding_table ** ftable)
{
	struct forwarding_table *ret;
	ret = initForwardingTable();

	ret->number_of_servers = dgram->fields_count;

	for (int i = 0; i < 5; i ++)
	{
		if (dgram->entries[i].cost == 0)
		{
			ret->server_id = dgram->entries[i].server_id;
		}
		if (dgram->entries[i].cost != 0 && dgram->entries[i].cost != 65535)
		{
			ret->number_of_neighbors++;
		}
		ret->entries[i].buffer = dgram->entries[i].buffer;
		ret->entries[i].cost = dgram->entries[i].cost;
		ret->entries[i].server_id = dgram->entries[i].server_id;
		ret->entries[i].server_port = dgram->entries[i].server_port;
		ret->entries[i].sa.sin_addr.s_addr = dgram->entries[i].server_address;
	}
	*ftable = ret;
	return 0;
}

int updateNeighbors(struct forwarding_table *ftable)
{
	for (int i = 0; i < 5; i++)
	{
		if (ftable->entries[i].cost != 0 && ftable->entries[i].cost != 65535)
		{
			if (-1 == sendTable(ftable->entries[i].server_id, ftable))
			{
				cout << "failed to send forwarding table to id " << ftable->entries[i].server_id << endl;
				return -1;
			}
		}
	}
	return 0;
}

int sendTable(int serverId, struct forwarding_table * ftable)
{
	struct addrinfo hints, *response;
	int fd = 0;
	memset(&hints,0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_UDP;

    char portNumber[5];
    char ip[INET_ADDRSTRLEN];
    for (int i = 0; i < 5; i ++)
    {
    	if (serverId == ftable->entries[i].server_id)
    	{
    		sprintf(portNumber,"%d",ftable->entries[i].server_port);
    		inet_ntop(AF_INET,&(ftable->entries[i].sa.sin_addr.s_addr),ip,INET_ADDRSTRLEN);
    	}
    }
    //cout << "sending to ip: " << ip << endl;
	/*
		get the address info
	*/

	if (getaddrinfo(ip,portNumber,&hints,&response) != 0)
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
	struct header dgram;
	char buf[128];
	int numBytesInBuf = createDatagram(ftable,&dgram,buf);
	if (-1 == numBytesInBuf)
	{
		cout << "failed to create datagram!" << endl;
		return -1;
	}

	int bytes_sent = sendto(fd,buf,numBytesInBuf,0,response->ai_addr,response->ai_addrlen);
	if (-1 == bytes_sent)
	{
		cout << "failed to send" << strerror(errno) <<endl;
	}
	return 0;
}

int insertIntoRouting(struct forwarding_table *ftable, struct forwarding_table **rtable)
{
	/* Check to see if the forwarding table being passed in is from a link that is disabled */
	for (int i = 0; i < 5; i ++)
	{
		if (rtable[0]->entries[i].server_id == ftable->server_id
				&& rtable[0]->entries[i].is_disabled)
		{
			cout << "Link has been disabled on this process to server id "
					<< ftable->server_id << "will not insert the table" << endl;
			return 0;
		}
	}

	int i = 0;
	while (rtable[i] != NULL && i < 5)
	{
		if (rtable[i]->server_id == ftable->server_id)
		{
			rtable[i] = ftable;			//replace the existing forwarding table with  the more up to date one
			cout << "replacing existing ftable in rtable " << endl;
			return 0;
		}
		i++;
	}

	if (i < 5)		//rtable is not full. insert into table at the first null pointer
	{
		rtable[i] = ftable;
		cout << "adding new entry to rtable " << endl;
	}
	cout << "successfully updated routing table" << endl;
	return 0;
}
int updateForwardingTable(struct forwarding_table * routing_table, struct forwarding_table **rtable)
{
	/*determining new costs of forwarding table*/
	cout << "determining new costs of forwarding table " << endl;
	/* Before we proceed, we make each cost the maximum cost*/
	resetRoutingTableCosts(routing_table);

	/* SET the initial values of the routing table to be the base server's*/
	routing_table->number_of_neighbors = rtable[0]->number_of_neighbors;
	routing_table->number_of_servers = rtable[0]->number_of_servers;
	routing_table->server_id = rtable[0]->server_id;

	for (int i = 0; i < 5; i ++)
	{
		routing_table->entries[i].cost = rtable[0]->entries[i].cost;
		routing_table->entries[i].next_hop_id = rtable[0]->entries[i].next_hop_id;
		routing_table->entries[i].server_port = rtable[0]->entries[i].server_port;
		routing_table->entries[i].sa.sin_addr.s_addr = rtable[0]->entries[i].sa.sin_addr.s_addr;
		routing_table->entries[i].server_id = rtable[0]->entries[i].server_id;
		routing_table->entries[i].is_neighbor= rtable[0]->entries[i].is_neighbor;
	}

	for (int i = 0; i < 5; i ++)
	{
		/*
		 * Determine shortest path to each node from this server node
		 */
		if (rtable[i] != NULL && !isDisabled(routing_table, rtable[i]->server_id))
		{
			if (-1 == updateTableEntry(routing_table,rtable[i],routing_table->entries[i].server_id))
			{
				cout << "failed to update Table Entry" << endl;
				return -1;
			}
		}
	}
	return 0;
}

int updateTableEntry(struct forwarding_table *base,struct forwarding_table *comparee,int id)
{
	/* DETERMINE COST TO COMPAREE */
	int costToComparee = 65535;
	for (int i = 0; i < 5; i ++)
	{
		if (base->entries[i].server_id == comparee->server_id)
		{
			costToComparee = base->entries[i].cost;
		}
	}
	cout << "cost to comparee is" << costToComparee << endl;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			if (base->entries[i].server_id == comparee->entries[j].server_id)
			{
				//cout << "i is now " << i << endl;
				if (comparee->entries[j].cost + costToComparee < base->entries[i].cost)
				{
					cout << "updated the cost of a link in a forwarding table for id " << comparee->server_id << endl;
					base->entries[i].cost = comparee->entries[j].cost + costToComparee;
					base->entries[i].next_hop_id = comparee->server_id;
				}
			}
		}
	}
	return 0;
}
int resetRoutingTableCosts(struct forwarding_table *routing_table)
{
	for (int i = 0; i < 5; i ++)
	{
		routing_table->entries[i].cost = 65535;
	}
	return 0;
}
int disable(struct forwarding_table *routing_table, struct forwarding_table **rtable, int server_id)
{
	if (routing_table->server_id == server_id)
	{
		cout << "error cannot disable yourself" << endl;
		return -1;
	}
	for (int i = 0; i < 5; i ++)
	{
		if (routing_table->entries[i].server_id == server_id)
		{
			if (routing_table->entries[i].is_neighbor)
			{
				routing_table->entries[i].is_disabled = true;
			}
		}
	}

	/* FORCE UPDATE OF ROUTING TABLE*/
	if (-1 == updateForwardingTable(routing_table,rtable))
	{
		cout << "failed to update routing table" << endl;
		return -1;
	}
	return 0;
}

bool isDisabled(struct forwarding_table * routing_table, int serverId)
{
	for (int i = 0; i < 5; i ++)
	{
		if (routing_table->entries[i].server_id == serverId && routing_table->entries[i].is_disabled)
		{
			return true;
		}
	}
	return false;
}

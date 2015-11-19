/*
 * ServerRouter.cpp
 *
 *  Created on: Nov 9, 2015
 *      Author: sarang
 */

#include <iostream>
#include <string>
#include <fstream>
#include <utility>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>

#include "../inc/ServerRouter.h"
#include "../inc/Constants.h"

ServerRouter::ServerRouter()
{
	numPacket = 0;
	serverId = 0;
	portNumber = 0;
	numServers = 0;
	numNeighbors = 0;
	routingUpdateInterval = 0;
	updatePacket = 0;
	updatePacketLen = 0;
	FD_ZERO(&masterFdSet);
	serSocketFd = 0;
	maxFd = 0;
}

ServerRouter::~ServerRouter()
{
	free(updatePacket);
}

command_code ServerRouter::commandInterpretor(std::string command)
{
	std::transform(command.begin(), command.end(), command.begin(), tolower);
	if (command == "display")
		return DISPLAY;
	if(command == "crash")
		return CRASH;
	if(command == "disable")
		return DISABLE;
	if(command == "update" )
		return UPDATE;
	if(command == "packets" )
		return PACKETS;
	if(command == "step" )
		return STEP;
	return UNKNOWN;
}
int ServerRouter::serverRouterInitialize(std::string fileName, int timeInterval)
{
	topologyFileName.assign(fileName);
	routingUpdateInterval = timeInterval;

	struct ifaddrs *ifaddr, *tmp;

	// for getting the self IP address , loop through all the interfaces and choose the appropriate one
	if (getifaddrs(&ifaddr) == -1)
	{
		std::cout << "Failed to get self address" << std::endl;
		return 1;
	}
	tmp = ifaddr;
	while (tmp)
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET&& (!INTERFACE_NAME1.compare(tmp->ifa_name)||!INTERFACE_NAME2.compare(tmp->ifa_name)))
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *) tmp->ifa_addr;
			serverIp = inet_ntoa(pAddr->sin_addr);
			break;
		}
		tmp = tmp->ifa_next;
	}
	freeifaddrs(ifaddr);

	//read the topology file and update the data structure for storing the the topology file information

	std::ifstream topologyFile;
	topologyFile.open(topologyFileName.c_str());
	if(!topologyFile.eof())
	{
		std::string ip;
		unsigned short port,serverid,neighborid,cost;
		topologyFile>>numServers;
		topologyFile>>numNeighbors;

		for(int i=0;i<numServers;i++)
		{
			topologyFile>>serverid>>ip>>port;
			if(!serverIp.compare(ip))
			{
				portNumber = port;
				serverId = serverid;
			}
			ServerInfo temp;
			temp.servIp = ip;
			temp.port = port;
			temp.nextId = serverid;
			temp.nextIp = ip;
			temp.cost = std::numeric_limits<unsigned short>::max();
			serverTable.insert(std::make_pair(serverid,temp));
		}

		serverTable[serverId].cost = 0;		//update the self cost to be zero

		for(int i=0;i<numNeighbors;i++)
		{
			topologyFile>>serverid>>neighborid>>cost;
			serverTable[neighborid].cost =cost;
			NeighborInfo temp;
			temp.port = serverTable[neighborid].port;
			temp.servIp = serverTable[neighborid].servIp;
			temp.packetRecvd = 0;
			temp.packetSent = 0;
			neighborList.insert(std::make_pair(neighborid,temp));
		}
	}
	topologyFile.close();
	distanceVectorInit();
	updatePacketInit();

	//bind the socket to address and port for server
	serSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
	maxFd = serSocketFd;
	if (serSocketFd < 0)
		return 1;
	struct sockaddr_in serAddress;
	memset(&serAddress, 0, sizeof(serAddress));
	serAddress.sin_family = AF_INET;
	serAddress.sin_addr.s_addr = INADDR_ANY;
	serAddress.sin_port = htons(portNumber);
	if (bind(serSocketFd, (struct sockaddr *) &serAddress,sizeof(serAddress)) == -1)
	{
		close(serSocketFd);
		return 1;
	}
	return 0;
}

int ServerRouter::distanceVectorInit()
{
	distanceVector.resize( numServers, std::vector<unsigned short>( numServers ,std::numeric_limits<unsigned short>::max() ) );
	for(int i=0;i<numServers;i++)
	{
		if(serverId == i+1)
			distanceVector[i][i] = 0;
		else
			distanceVector[i][i] = serverTable[i+1].cost;
	}

	return 0;
}


void ServerRouter::displayRoutingTable()
{
	std::cout<<std::setw(6)<<"FRM ID"<<std::setw(17)<<"FROM IP"<<std::setw(7)<<"TO ID"<<std::setw(17)<<"TO IP"<<std::setw(7)<<"NXT ID"<<std::setw(17)<<"NXT IP"<<std::setw(7)<<"COST"<<std::endl;
	for(unsigned int i=0;i<numServers;i++)
	{
		if(serverTable[i+1].cost == std::numeric_limits<unsigned short>::max())
		{
			std::cout<<std::setw(6)<<serverId<<std::setw(17)<<serverIp<<std::setw(7)<<i+1<<std::setw(17)<<serverTable[i+1].nextIp<<std::setw(7)<<serverTable[i+1].nextId<<std::setw(17)<<serverTable[i+1].nextIp<<std::setw(7)<<"inf"<<std::endl;
		}
		else
		{
			std::cout<<std::setw(6)<<serverId<<std::setw(17)<<serverIp<<std::setw(7)<<i+1<<std::setw(17)<<serverTable[i+1].nextIp<<std::setw(7)<<serverTable[i+1].nextId<<std::setw(17)<<serverTable[i+1].nextIp<<std::setw(7)<<serverTable[i+1].cost<<std::endl;
		}
	}
}

int ServerRouter::updatePacketInit()
{
	updatePacketLen = sizeof(ServerRouterPacket) + numServers*sizeof(ServerRouterInfo);
	updatePacket = (ServerRouterPacket*)malloc(updatePacketLen);
	updatePacket->numFields = numServers;
	updatePacket->serverPort = portNumber;
	struct in_addr *temp;
	inet_aton(serverIp.c_str(),temp);
	updatePacket->serverIP = temp->s_addr;
	for(int i = 0;i<numServers;i++)
	{
		updatePacket->List[i].linkCost= serverTable[i+1].cost;
		updatePacket->List[i].serverId = i+1;
		inet_aton(serverTable[i+1].servIp.c_str(),temp);
		updatePacket->List[i].serverIp = temp->s_addr;
		updatePacket->List[i].serverPort = serverTable[i+1].port;
	}
	return 0;
}

int ServerRouter::updatePacketRefresh()
{
	for(int i = 0;i<numServers;i++)
	{
		updatePacket->List[i].linkCost= serverTable[i+1].cost;
	}
	return 0;
}

int ServerRouter::sendRoutingUpdatePacket(std::string IP, unsigned short port)
{
	struct sockaddr_in servAddr;

	//Construct the server sockaddr_ structure
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	inet_pton(AF_INET, IP.c_str(), &servAddr.sin_addr.s_addr);
	servAddr.sin_port = htons(port);

	if (sendto(serSocketFd, updatePacket, updatePacketLen, 0,
			(struct sockaddr *) &servAddr, sizeof(servAddr))
			!= updatePacketLen) {
		std::cout << "Mismatch in number of bytes sent"<<std::endl;
		return 1;
	}
	return 0;
}

int ServerRouter::sendRoutingUpdatePacketToAll()
{
	for(std::map<unsigned short,NeighborInfo>::iterator it = neighborList.begin();it!=neighborList.end();++it)
	{
#ifdef DEBUG
		std::cout<<"Sending update packet to:"<<std::endl;
		std::cout<<it->second.servIp<<" "<<it->second.port<<std::endl;
#endif
		if(sendRoutingUpdatePacket(it->second.servIp,it->second.port) != 0)
			return 1;
		else
			it->second.packetSent++;
	}
	return 0;
}

int ServerRouter::recvProcessUpdatePacket()
{
	struct sockaddr_in recvAddr;
	socklen_t addrlen = sizeof(recvAddr);
	ServerRouterPacket *recvdPacket;
	recvdPacket = (ServerRouterPacket*)malloc(updatePacketLen);
	if(updatePacketLen != recvfrom(serSocketFd, recvdPacket, updatePacketLen, 0, (struct sockaddr *)&recvAddr, &addrlen))
	{
		std::cout<<"Wrong length of packet"<<std::endl;
		return 1;
	}
	numPacket++;
	std::string fromIp;
	int fromId = 0;
	int ip = recvdPacket->serverIP;
	char * temp;
	temp = inet_ntoa(recvAddr.sin_addr);
	fromIp = temp;
	for(int i=0;i<numServers;i++)
	{
		if(fromIp == serverTable[i+1].servIp)
		{
			fromId = i+1;
			break;
		}
	}
	if(neighborList.find(fromId) == neighborList.end())
		return 0;										//not processing the packet from anyone which is not a neighbor

	neighborList[fromId].packetRecvd++;  //increment the no. of packet received from a particular neighbor

/**************************************************START OF DISTRIBUTED BELLMAN FORD ALGORITHM****************************************************************/
	for(int i=0;i<numServers;i++)
	{

		if(recvdPacket->List[i].serverId != serverId && distanceVector[fromId-1][fromId-1] != std::numeric_limits<unsigned short>::max()
				&& recvdPacket->List[i].serverId != fromId)  //do not fill the self distance, distance via itself, and distance via non neighbors. Also don't update cost for reverse route
		{
			if(serverTable[fromId].cost == std::numeric_limits<unsigned short>::max()||
					recvdPacket->List[i].linkCost == std::numeric_limits<unsigned short>::max())  // handle cases if new cost or shortest cost is infinity
				distanceVector[recvdPacket->List[i].serverId-1][fromId-1] = std::numeric_limits<unsigned short>::max();
			else
				distanceVector[recvdPacket->List[i].serverId-1][fromId-1] = serverTable[fromId].cost + recvdPacket->List[i].linkCost;
		}

	}
	//the distance vector has been updated according to the incoming packet
	updateRoutingTable();		//it updates the routing table by the minimum value in the Distance Vector's row and the next hop

	updatePacketRefresh();		//the Packets to be sent are updated with the change in value if any
	std::cout<<"Update Packet from Id : "<<fromId<<" IP : "<<fromIp<<std::endl;

#ifdef DEBUG		//set this flag from Constants.h to get the packet and DV output at console

	std::cout<<"The received packet"<<std::endl;
	for(int i=0;i<numServers;i++)
	{
		std::cout<<recvdPacket->List[i].serverId<<" "<<recvdPacket->List[i].linkCost<<std::endl;
	}
	std::cout<<"Distance Vector :"<<std::endl;
	displayDV();
	displayRoutingTable();
	/******************************************END OF DISTRIBUTED BELLMAN FORD ALGORITHM************************************************************8*/
#endif
	free(recvdPacket);
	return 0;

}

void ServerRouter::displayDV()
{
	for (int i = 0; i < numServers; i++)
	{
		for (int j = 0; j < numServers; j++)
		{
			if(j+1 == serverId || i+1 == serverId || neighborList.find(j+1) == neighborList.end())
			{
				std::cout<<std::setw(5)<<"N.A";
			}
			else
			{
				if(distanceVector[i][j] == std::numeric_limits<unsigned short>::max())
					std::cout<<std::setw(5)<<"inf";
				else
					std::cout<<std::setw(5)<< distanceVector[i][j];
			}
		}
		std::cout << std::endl;
	}
}
int ServerRouter::updateRoutingTable()
{
	for(int i=0;i<numServers;i++)
	{
		serverTable[i+1].cost = minOfRowInDV(i);
		serverTable[i+1].nextId = minIndexOfRowInDV(i)+1;
		serverTable[i+1].nextIp = serverTable[serverTable[i+1].nextId].servIp;
	}
	return 0;
}

unsigned short ServerRouter::minOfRowInDV(int row)
{
	unsigned short minVal = distanceVector[row][0];
	for(int i=0;i<numServers;i++)
	{
		if(minVal>distanceVector[row][i])
			minVal = distanceVector[row][i];
	}
	return minVal;
}

unsigned short ServerRouter::minIndexOfRowInDV(int row)
{
	unsigned short minVal = distanceVector[row][0];
	unsigned short minIndex = 0;
	for(int i=0;i<numServers;i++)
	{
		if(minVal>distanceVector[row][i])
		{
			minVal = distanceVector[row][i];
			minIndex = i;
		}
	}
	return minIndex;
}
int ServerRouter::updateCost(unsigned short server1, unsigned short server2, unsigned short cost)
{
	if(serverId != server1 && serverId != server2)
		return 1;
	unsigned short otherId = 0;
	if(serverId == server1)
		otherId = server2;
	else
		otherId = server1;
	if(neighborList.find(otherId) == neighborList.end())
		return 1;
	if(cost == std::numeric_limits<unsigned short>::max())
	{
		for(int i=0;i<numServers;i++)		//remove all costs via this server ID
		{
			distanceVector[i][otherId-1] = cost;
		}
	}
	else
	{
		distanceVector[otherId-1][otherId-1] = cost;
	}
	updateRoutingTable();
	updatePacketRefresh();
	return 0;
}
int ServerRouter::serverRun()
{
	std::string command;
	struct timeval tv;
	tv.tv_sec = routingUpdateInterval;
	tv.tv_usec = 0;
	int retval = 0;
	FD_SET(serSocketFd, &masterFdSet);
	FD_SET(fileno(stdin), &masterFdSet);
	while (true)
	{
		activeFdSet = masterFdSet;
		retval = select(maxFd + 1, &activeFdSet, NULL, NULL, &tv);
		if (retval == -1)
		{
			std::cout<<"Error in SELECT"<<std::endl;
		}
		if (retval == 0)
		{
			tv.tv_sec = routingUpdateInterval;
			tv.tv_usec = 0;
			if(0 != sendRoutingUpdatePacketToAll())
				std::cout<<"Failed to send the update packet"<<std::endl;
		}
		else
		{
			if (FD_ISSET(fileno(stdin), &activeFdSet))
			{
				unsigned short server = 0, serverId1 = 0, serverId2 = 0, newCost = 0;
				std::string cost;
				std::cin >> command;
				switch (commandInterpretor(command))
				{

				case DISPLAY:
					displayRoutingTable();
					break;

				case CRASH:
					close(serSocketFd);
					break;

				case UPDATE:
					std::cin >> serverId1 >> serverId2 >> cost;
					if(cost == "inf")
						newCost = std::numeric_limits<unsigned short>::max();
					else
						newCost = atoi(cost.c_str());
					if(updateCost(serverId1, serverId2, newCost)!= 0)
						std::cout<<"None of the server id is the neighbor or none of the id is of the current server"<<std::endl;
					else
						std::cout<<"UPDATE : SUCCESS"<<std::endl;
					break;

				case PACKETS:
					std::cout<<"Number of packets received since last call : "<<numPacket<<std::endl;
					std::cout<<" PACKET : SUCCESS"<<std::endl;
					numPacket = 0;
					break;

				case DISABLE:
					std::cin >> server;
					if(neighborList.find(server) == neighborList.end())
						std::cout<<"ERROR : Given server id is not a not a neighbor"<<std::endl;
					else
					{
						updateCost(serverId, server, std::numeric_limits<unsigned short>::max());
						neighborList.erase(server);
						std::cout<<"SUCCESS : DISABLE"<<std::endl;
					}
					break;
				case STEP:
					if(0 != sendRoutingUpdatePacketToAll())
						std::cout<<"ERROR : Failed to send the update packet"<<std::endl;
					else
						std::cout<<"STEP : SUCCESS"<<std::endl;
					break;
				default:
					std::cout << "ERROR : Wrong command entered" << std::endl;
					break;
				}
				if (commandInterpretor(command) == CRASH)
					break;
				FD_CLR(fileno(stdin), &activeFdSet);
			}
			if (FD_ISSET(serSocketFd, &activeFdSet)) //we have got a routing update
			{
				if(0 != recvProcessUpdatePacket())
				{
					std::cout<<"Failed to receive update packet"<<std::endl;
				}
				std::map<unsigned short,NeighborInfo>::iterator it = neighborList.begin();
				while(it != neighborList.end())
				{
					std::map<unsigned short,NeighborInfo>::iterator toErase;
					bool shouldErase = false;
					if(it->second.packetSent - it->second.packetRecvd > 3)
					{
						updateCost(serverId, it->first, std::numeric_limits<unsigned short>::max());
						toErase = it;
						shouldErase = true;
					}
					++it;
					if(shouldErase)
					{
						neighborList.erase(toErase);
#ifdef DEBUG
						std::cout<<"Erasing : "<< toErase->first<<" "<<toErase->second.servIp<<std::endl;
#endif
					}
				}

				FD_CLR(serSocketFd, &activeFdSet);
			}
		}
	}
	return 0;
}

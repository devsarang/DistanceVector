/*
 * ServerRouter.h
 *
 *  Created on: Nov 9, 2015
 *      Author: sarang
 */

#ifndef SERVERROUTER_H_
#define SERVERROUTER_H_
#include <vector>
#include <map>

#include "../inc/Constants.h"
#include "../inc/RouterPacket.h"

class ServerRouter
{
	int numPacket;
	ServerRouterPacket *updatePacket;
	int updatePacketLen;
	unsigned short serverId;
	unsigned short portNumber;
	std::string serverIp;
	std::string topologyFileName;
	int routingUpdateInterval;
	unsigned short numNeighbors;
	unsigned short numServers;
	typedef struct
	{
		std::string servIp;
		unsigned short port;
		unsigned short nextId;
		std::string nextIp;
	    unsigned short cost;
	}ServerInfo;
	std::map<unsigned short,ServerInfo> serverTable;
	typedef struct
		{
			std::string servIp;
			unsigned short port;
			int packetRecvd;
			int packetSent;
		}NeighborInfo;
	std::map<unsigned short,NeighborInfo >neighborList;
	std::vector<std::vector<unsigned short> > distanceVector;
	int serSocketFd, maxFd;
	std::vector<int>crashList;
	fd_set activeFdSet, masterFdSet;
public:
	ServerRouter();
	virtual ~ServerRouter();
	int serverRouterInitialize(std::string, int);
	command_code commandInterpretor(std::string);
	int updatePacketInit();
	int distanceVectorInit();
	void displayRoutingTable();
	int sendRoutingUpdatePacketToAll();
	int sendRoutingUpdatePacket(std::string, unsigned short);
	int recvProcessUpdatePacket();
	int updateRoutingTable();
	void displayDV();
	int updatePacketRefresh();
	unsigned short minOfRowInDV(int row);
	unsigned short minIndexOfRowInDV(int row);
	int updateCost(unsigned short serverId1, unsigned short serverId2, unsigned short newCost);
	int serverRun();
};


#endif /* SERVERROUTER_H_ */

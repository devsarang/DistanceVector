#ifndef ROUTERPACKET_H_
#define ROUTERPACKET_H_

typedef struct
{

	unsigned int serverIp;
	unsigned short serverPort;
	unsigned short serverId;
	unsigned short linkCost;			//automatic padding of 2 bytes will be added
}ServerRouterInfo;

typedef struct
{
	unsigned short numFields;
	unsigned short serverPort;
	unsigned int serverIP;
	ServerRouterInfo List[];
}ServerRouterPacket;


#endif

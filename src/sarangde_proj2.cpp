/*
 * sarangde_proj2.cpp
 *
 *  Created on: Nov 9, 2015
 *      Author: sarang dev
 *
 *      The main file which contains the main function and also creates the objects of the server class.
 */
#include "../inc/RouterPacket.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>


#include "../inc/Constants.h"
#include "../inc/ServerRouter.h"

int main(int argc, char *argv[])
{
	int c;
	std::string filename;
	int interval;
	while ((c = getopt(argc, argv, ":t:i:")) != EOF)
	{
		switch (c)
		{
		case 't':
			filename = optarg;
			break;
		case 'i':
			interval = atof(optarg);
			break;
		case ':':
			std::cout<<"Missing Options"<<std::endl;
			exit(1);
			break;
	    case '?':
		    std::cout<<"unknown argument"<<std::endl;
		    break;
		}
	}

	ServerRouter *serverRouter = new ServerRouter();
	serverRouter->serverRouterInitialize(filename,interval);
	serverRouter->serverRun();
	return 0;
}


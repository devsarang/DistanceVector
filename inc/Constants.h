#ifndef CONSTANTS_H_
#define CONSTANTS_H_
#include <string>

enum command_code {
	UNKNOWN = -1, UPDATE, PACKETS, DISPLAY, DISABLE, CRASH, STEP
};

const std::string INTERFACE_NAME1="eth0";
const std::string INTERFACE_NAME2="em2";
const int MAX_COST=100;
#endif

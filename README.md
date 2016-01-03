

The program runs in two modes :
1.DEBUG
2.NON DEBUG

To run the program in Debug mode, the debug definition macro must be uncommented.

//#define DEBUG in Constants.h

In debug mode, all the packets and distance vector is displayed in console after receive of each packet.

To compile the program, use make. i
make in the program's root directory will make the executable named server.

To run the program :
./server -i 30 -t topology.txt

where i takes timeinterval and t takes topology file name. Also the order of t and i does't matter.

All the commands given in the project description is implemented as needed.


// A hello world program in C++

#include "stdio.h"
#include <iostream>
#include <math.h>
#include <time.h>
#include <chrono>

#include "../include/dataSource.h"
//#include "dataSourceTestConfig.h"

using namespace std;
//using namespace std::chrono;

int main(int argc, char* argv[])
{
  //time_t timer;
  //int startTime = time(&timer);
  //int lastTime = time(&timer);
  
  char IP[16];//(Note: only allowing for v4 addresses here)
  int server = 1;
  int port = 9984;
  int maxLoops = 1000;
  int loop = 0;

  if (argc == 5)
  {
    server = atoi(argv[1]);
    port = atoi(argv[2]);
    strcpy(IP,argv[3]);
	maxLoops = atoi(argv[4]);
  }
  
  cout << "\n\n--- Data Source Socket Library Test Program ---\n  arg count  " << argc  << "  server " << server << 
	  " port " << port << " maxLoops " << maxLoops << " IP " << IP << " \n\n";
  
  dataSource *dataSrc = new dataSource(server,port,IP);
  long int c = 0;
  //long int ms, lastMs;
  while (loop < maxLoops)//FIX FIX FIX, get time.h included
  {
    //crudest possible timer
    //if (c % 100000000 == 0) 
    //{
		  dataSrc->tick();
		  //lastMs = ms;
		  cout << "ticking!!  loop = " << loop << "\n";
	  //}
	  loop++;
    //}
    //c++;
  }
  
  delete dataSrc;
  
 
  return 0;
}

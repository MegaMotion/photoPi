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
  char IP[128];
  int server = 1;
  int port = 9985;
  int sleepMS = 10;
  int maxTick = 500;//for testing, safety valve so we don't get stuck in endless loops.

  //Process command line arguments.
  if (argc > 1)
  {
#ifdef windows_OS
    strcpy_s(IP,argv[1]);
#else
    strcpy(IP,argv[1]);
#endif
  }
  
  if (argc > 2)
    port = atoi(argv[2]);

  //if (argc > 3) ...

  
  cout << "\n\n--- Data Source Socket Library Test Program ---\n  arg count  " << argc  << "  server " << server << 
	  " port " << port <<  " IP " << IP << " \n\n";
  
  dataSource *dataSrc = new dataSource(server,port,IP);
  while ( dataSrc->mCurrentTick < maxTick ) //!(dataSrc->mFinished) )
  {
	dataSrc->tick();
#ifdef windows_OS
    Sleep(sleepMS);
#else		  
    timespec time1, time2;
    time1.tv_sec = 0;
    time1.tv_nsec = sleepMS * 1000000L;//Ten times one million nanoseconds, or 10 milliseconds.
    nanosleep(&time1, &time2);//Time one is requested, Time two is for leftovers if interrupted.
#endif		  
  }
  
  delete dataSrc;
  return 0;
}

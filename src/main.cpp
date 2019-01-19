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
  char IP[16];//(Note: only allowing for v4 addresses here)
  int server = 1;
  int port = 9985;
  int sleep_MS = 10;

  if (argc == 2)
  {
    //server = atoi(argv[1]);
    //port = atoi(argv[2]);
#ifdef windows_OS
    strcpy_s(IP,argv[1]);
#else
    strcpy(IP,argv[1]);
#endif
    //maxLoops = atoi(argv[4]);
  }
  
  cout << "\n\n--- Data Source Socket Library Test Program ---\n  arg count  " << argc  << "  server " << server << 
	  " port " << port <<  " IP " << IP << " \n\n";
  
  dataSource *dataSrc = new dataSource(server,port,IP);
  long int c = 0;

  //FIX, get time.h or chrono in here, so we don't have to run a while loop at top speed while we're waiting.
  while ( !(dataSrc->mFinished) )
  {
		  dataSrc->tick();
#ifdef windows_OS
    Sleep(sleep_MS);
#else		  
    timespec time1, time2;
    time1.tv_sec = 0;
    time1.tv_nsec = sleep_MS * 1000000L;//Ten times one million nanoseconds, or 10 milliseconds.
    nanosleep(&time1, &time2);//Time one is requested, Time two is for leftovers if interrupted.
#endif		  
		  //cout << "ticking!!  loop = " << loop << "\n";
  }
  
  delete dataSrc;
  return 0;
}

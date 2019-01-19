// A hello world program in C++

#include "stdio.h"
#include <iostream>
#include <math.h>
#include <time.h>
#include <chrono>

#include "dataSource.h"
#include "dataSourceTestConfig.h"

using namespace std;
//using namespace std::chrono;

int main(int argc, char* argv[])
{
  //time_t timer;
  //int startTime = time(&timer);
  //int lastTime = time(&timer);
  
  char IP_file[36];//(Note: only allowing for v4 addresses here)
  char outputName[256];
  int server = 0;
  int port = 9985;
  int maxLoops = 1000;
  int loop = 0;
  int sleep_MS = 10;

  if (argc == 2)
  {
    //server = atoi(argv[1]);
    //port = atoi(argv[2]);
#ifdef windows_OS
    strcpy_s(IP_file,argv[1]);
#else
	strcpy(IP, argv[3]);
#endif
	//maxLoops = atoi(argv[4]);
  }
  
  cout << "\n\n--- Data Source Socket Library Test Program ---\n  arg count  " << argc  << "  server " << server << 
	  " port " << port << " maxLoops " << maxLoops << " IP " << IP_file << " \n\n";
  
  dataSource *dataSrc = new dataSource(server,port, IP_file);
  while (!dataSrc->mFinished)// (loop < maxLoops)//(!dataSrc->mFinished)// //FIX FIX FIX, get time.h included
  {
#ifdef windows_OS
	  Sleep(sleep_MS);
#else
	  timespec time1, time2;
	  time1.tv_sec = 0;
	  time1.tv_nsec = sleep_MS * 1000000L;
	  nanosleep(&time1, &time2);
#endif
	  dataSrc->tick();
	  //cout << "ticking!!  loop = " << loop << "\n";
  }
  
  delete dataSrc; 
  return 0;
}

// A hello world program in C++

#include "stdio.h"
#include <iostream>
#include <math.h>
#include <time.h>

#include "dataSource.h"
#include "dataSourceTestConfig.h"

using namespace std;

int main(int argc, char* argv[])
{
  //time_t timer;
  //int startTime = time(&timer);
  //int lastTime = time(&timer);
  
  char IP[16];//(Note: only allowing for v4 addresses here)
  int server,port;
  long int c;
  
  if (argc == 4)
  {
    server = atoi(argv[1]);
    port = atoi(argv[2]);
    strcpy(IP,argv[3]);
  }
  
  cout << "\n\n--- Data Source Socket Library Test Program ---\n  argc  " << argc  << "  server " << server << " port " << port << " IP " << IP << " \n\n";
  
  //sprintf(IP,"%s",argv[3]);
  cout << " IP: " << IP << "\n";
  
  dataSource *dataSrc = new dataSource(server,port,IP);
  c = 0;
  
  //while (lastTime - startTime < 30)// (int c=0;c<80;c++)
  while (c < 10000000000)//FIX FIX FIX, get time.h included
  {
    //if (time(&timer) > lastTime )
    if (c % 100000000 == 0) //crude but simple
    {
      //lastTime = time(&timer);
      dataSrc->tick();
    }
    c++;
  }
  
  delete dataSrc;
  
 
  return 0;
}

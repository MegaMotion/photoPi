//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

/////// Comment this out for linux builds.   //////////////
#define windows_OS 
///////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#ifdef windows_OS
#include "winsock2.h"
#else 
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#define OPCODE_BASE		1

using namespace std;

/// Base class for various kinds of data sources, first one being worldDataSource, for terrain, sky, weather and map information.
class dataSource 
{
   public:
	   char mSourceIP[36];

	   string mStrFilename;
	   string mStrOptions;
	   string mStrEncoding;


	   unsigned int mPort;

	   unsigned int mCurrentTick;
	   unsigned int mLastSendTick;//Last time we sent a packet.
	   unsigned int mLastSendTimeMS;//Last time we sent a packet.
	   unsigned int mTickInterval;
	   unsigned int mStartDelay;
	   unsigned int mPacketCount;
	   unsigned int mMaxPackets;
	   unsigned int mPacketSize;

	   bool mReadyForRequests;//flag to user class (eg terrainPager) that we can start adding requests.

#ifdef windows_OS
	   SOCKET mListenSockfd;
	   SOCKET mWorkSockfd;
	   WSADATA wsaData;
#else
	   int mListenSockfd;
	   int mWorkSockfd;
#endif
	   
	   bool mServer;
	   bool mListening;
	   bool mAlternating;
	   bool mConnectionEstablished;
	   bool mFinished;

	   char *mReturnBuffer;
	   char *mSendBuffer;
	   char *mStringBuffer;

	   short mSendControls;
	   short mReturnByteCounter;
	   short mSendByteCounter;

	   dataSource(bool listening, int port, char *IP);
	   ~dataSource();

	   void tick();
	   
	   void openListenSocket();
	   void connectListenSocket();
	   void listenForPacket();
	   void readPacket();
	   void clearReturnPacket();

	   void connectSendSocket();
	   void sendPacket();
	   void clearSendPacket();
	   
	   void trySockets();
	   void disconnectSockets();	  

	   void writeShort(short);
	   void writeInt(int);
	   void writeFloat(float);
	   void writeDouble(double);
	   void writeString(const char *content);
	   //void writePointer(void *);//Someday? Using boost?

	   short readShort();
	   int readInt();
	   float readFloat();
	   double readDouble();
	   char *readString();
	   //void *readPointer();
	   
	   void clearString();

	   void addBaseRequest();
	   void handleBaseRequest();

	   void addPhotoRequest();
	   void handlePhotoRequest();
};

#endif // _DATASOURCE_H_

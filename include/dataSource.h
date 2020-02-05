//-----------------------------------------------------------------------------
// Copyright (c) 2020 Chris Calef
//-----------------------------------------------------------------------------

#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

/////// Comment this out for linux builds.   //////////////
#define windows_OS 
///////////////////////////////////////////////////////////

//Needed in standalone version
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")

#define OPCODE_BASE    1
#define OPCODE_TEST    2

//controlDataSource
#define OPCODE_QUIT 10

//photoPi project
#define OPCODE_PHOTO  51

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


enum serverConnectStage {
	NoServerSocket,
	ServerSocketCreated,
	ServerSocketBound,
	ServerSocketListening,
	ServerSocketAccepted,
	PacketReceived,
	PacketRead
};

enum clientConnectStage {
	NoClientSocket,
	ClientSocketCreated,
	ClientSocketConnected,
	PacketSent
};

class dataSource
{
public:
	char mSourceIP[128];
	unsigned int mPort;

	//PhotoPi properties
	std::string mStrFilename;
	std::string mStrOptions;
	std::string mStrEncoding;
	
	unsigned int mCurrentTick;
	unsigned int mLastSendTick;//Last time we sent a packet.
	unsigned int mLastSendTimeMS;//Last time we sent a packet.
	unsigned int mTickInterval;
	unsigned int mStartDelayMS;
	unsigned int mPacketCount;
	unsigned int mMaxPackets;
	unsigned int mPacketSize;

	serverConnectStage mServerStage;
	clientConnectStage mClientStage;

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

	bool mDebugToConsole;
	bool mDebugToFile;

	char* mReturnBuffer;
	char* mSendBuffer;
	char* mStringBuffer;

	short mSendControls;
	short mReturnByteCounter;
	short mSendByteCounter;

	FILE* mDebugLog;

	   dataSource(bool listening, bool alternating, int port, char *IP);
	~dataSource();

	void tick();

	void createListenSocket();
	void bindListenSocket();
	void connectListenSocket();
	void listenForConnection();
	void readPacket();
	void clearReturnPacket();
	void allocateBuffers();

	void connectSendSocket();
	void sendPacket();
	void clearSendPacket();

	void disconnectSockets();

	void writeShort(short);
	void writeInt(int);
	void writeFloat(float);
	void writeDouble(double);
	void writeString(const char*);
	//void writeBlob(void *);

	short readShort();
	int readInt();
	float readFloat();
	double readDouble();
	char* readString();
	//void *readBlob();

	void clearString();

	void addBaseRequest();
	void handleBaseRequest();

	void addTestRequest();
	void handleTestRequest();

	void addPhotoRequest(const char* imgName);
	void handlePhotoRequest();
};

#endif // _DATASOURCE_H_

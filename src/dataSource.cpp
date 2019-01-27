//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#include "../include/dataSource.h"

using namespace std;


#define OPCODE_BASE    1
#define OPCODE_PHOTO  51

//#include "console/consoleTypes.h"//Torque specific, should do #ifdef TORQUE or something

dataSource::dataSource(bool server,int port, char *IP)
{
	mPort = port;
	sprintf(mSourceIP,IP);
	mPacketSize = 1024;
	mCurrentTick = 0;
	mLastSendTick = 0;
	mLastSendTimeMS = 0;
	mTickInterval = 1;//For skipping ticks, in case they are getting called faster than desired.
	mStartDelayMS = 1000;
	mPacketCount = 0;
	mMaxPackets = 20;
	mSendControls = 0;
	mReturnByteCounter = 0;
	mSendByteCounter = 0;
		
	mListenSockfd = 0;
	mWorkSockfd = 0;
	
	mReturnBuffer = NULL;
	mSendBuffer = NULL;
	mStringBuffer = NULL;
	mReadyForRequests = false;	
	mAlternating = false;

	mDebugToConsole = true;
	mDebugToFile = false;
	
	mServerStage = NoServerSocket;
	mClientStage = NoClientSocket;
	
	mServer = false;
	mListening = false;	
	if (server)
	{
	  mServer = true;
	  mListening = true;
	}

#ifdef windows_OS
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR) {
		cout << "Error initialising WSA.\n";		
	}
#endif

	if (mDebugToFile)
	{
	  mDebugLog = fopen("/home/pi/photoPi/log.txt","a");
	  fprintf(mDebugLog,"\n\n--------------------------- PhotoPi Debug Log\n\n");
	}


	//Now, break for some number of milliseconds, to let system get on wifi network, etc. ... ?
#ifdef windows_OS
	Sleep(mStartDelayMS);
#else		  
	timespec time1, time2;
	time1.tv_sec = 0;
	time1.tv_nsec = mStartDelayMS * 1000000L;//Ten times one million nanoseconds, or 10 milliseconds.
	nanosleep(&time1, &time2);//Time one is requested, Time two is for leftovers if interrupted.
#endif
	
}

dataSource::~dataSource()
{
	disconnectSockets();
  if (mDebugToFile)
    fclose(mDebugLog);
}

////////////////////////////////////////////////////////////////////////////////

void dataSource::tick()
{	
  if (mServer)
	{ 
    switch (mServerStage)
		{
    case NoServerSocket:
	createListenSocket(); break;
      case ServerSocketCreated:
	bindListenSocket(); break;
      case ServerSocketBound:
	connectListenSocket(); break;
      case ServerSocketListening:
	listenForConnection(); break;
      case ServerSocketAccepted:
	receivePacket(); break;
      case PacketReceived:
	readPacket(); break;
      case PacketRead:
	mServerStage = ServerSocketListening;
	break;
    }
  }
  else
  {
    switch (mClientStage)
    {
      case NoClientSocket:
	break;
      case ClientSocketCreated:
	break;
      case ClientSocketConnected:
	break;
      case PacketSent:
	break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::createListenSocket()
{
	mListenSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	if (mListenSockfd < 0)
	{
	  if (mDebugToConsole)
	    cout << "ERROR in createListenSocket. Error: " << errno << " " << strerror(errno) << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"ERROR in createListenSocket. Error: %d %s\n",errno,strerror(errno));
	} else {
	  if (mDebugToConsole)
	    cout << "SUCCESS in createListenSocket\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"SUCCESS in createListenSocket\n");
	  mServerStage = serverConnectStage::ServerSocketCreated;
	}
	

#ifdef windows_OS
	u_long iMode=1;
	//ioctlsocket(mListenSockfd,O_NONBLOCK,&iMode);//Make it a non-blocking socket.
#else
	int flags;
	flags = fcntl(mListenSockfd,F_GETFL,0);
	if (flags != -1)
	  fcntl(mListenSockfd, F_SETFL, flags | O_NONBLOCK);
	
	bool bOptVal = true;
	if (setsockopt(mListenSockfd,SOL_SOCKET,SO_REUSEADDR,(char *) &bOptVal,sizeof(bool)) == -1) 
	  cout << "FAILED to set socket option ReuseAddress\n";			
#endif
}

void dataSource::bindListenSocket()
{  
	struct sockaddr_in source_addr;
	
#ifdef windows_OS
	ZeroMemory((char *) &source_addr, sizeof(source_addr));
#else
	bzero((char *)&source_addr, sizeof(source_addr));
#endif

    source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = inet_addr( mSourceIP );
    source_addr.sin_port = htons(mPort);

	if (bind(mListenSockfd, (struct sockaddr *) &source_addr,sizeof(source_addr)) < 0) 
	{
	  if (mDebugToConsole)
	    cout << "ERROR in bindListenSocket. Error: " <<  errno <<  " " << strerror(errno) << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"ERROR in bindListenSocket. Error: %d  %s\n\n",errno,strerror(errno));
	}
	else
	{
	  if (mDebugToConsole)
	    cout << "SUCCESS in bindListenSocket " << mListenSockfd << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"SUCCESS in bindListenSocket %d \n",mListenSockfd);
	  mServerStage = ServerSocketBound;
	}

}

void dataSource::connectListenSocket()
{
	int n;
	n = listen(mListenSockfd,10);
	if (n == -1) //SOCKET_ERROR)
	{
	  if (mDebugToConsole)
	    cout << "ERROR in connectListenSocket!  errno " << errno <<  "  " << strerror(errno) << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"ERROR in connectListenSocket. Error: %d  %s\n\n",errno,strerror(errno));
	} else {
	  if (mDebugToConsole)
	    cout << "SUCCESS in connectListenSocket\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"SUCCESS in connectListenSocket \n");
	  allocateBuffers();
	  mServerStage = ServerSocketListening;
	}
}

void dataSource::listenForConnection()
{
	mPacketCount = 0;
	  mWorkSockfd = accept(mListenSockfd,NULL,NULL);
	if (mWorkSockfd == -1)
	{
	  if (mDebugToConsole && (errno != 11))//11 = resource unavailable, ie waiting for connection.
	    cout << "ERROR in listenForConnection. Error " << errno << "   " << strerror(errno) << "\n";
	  if (mDebugToFile && (errno != 11))
	    fprintf(mDebugLog,"ERROR in listenForConnection. Error: %d  %s\n\n",errno,strerror(errno));
	} else {
	  if (mDebugToConsole)
	    cout << "SUCCESS in listenForConnection.  workSock: " << mWorkSockfd  << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"SUCCESS in listenForConnection. workSock: %d \n\n",mWorkSockfd);
	  mServerStage = ServerSocketAccepted;
	}
	}
	
void dataSource::receivePacket()
{
	int n = recv(mWorkSockfd,mReturnBuffer,mPacketSize,0);
	if (n<0) {
	  if (mDebugToConsole)
	    cout << "ERROR in receivePacket.  Error: " << errno  << "  " << strerror(errno)  << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"ERROR in receivePacket. Error: %d  %s\n\n",errno,strerror(errno));
	} else {
	  if (mDebugToConsole)
	    cout << "SUCCESS in receivePacket. Size = : " << n << "\n";
	  if (mDebugToFile)
	    fprintf(mDebugLog,"SUCCESS in receivePacket. Size: %d \n\n",n);
	  mServerStage = PacketReceived;
	}
}

void dataSource::readPacket()
{
	short opcode,controlCount;//,packetCount;
	mDebugLog = fopen("/home/pi/photoPi/log.txt","a");	
	controlCount = readShort();
	for (short i=0;i<controlCount;i++)
	{		
		opcode = readShort();

		if (mDebugToConsole)
		cout << "Reading packet, opcode = " << opcode << "\n";
		if (mDebugToFile)
		  fprintf(mDebugLog,"Reading packet, opcode = %d \n\n",opcode);
		if (opcode == OPCODE_BASE)
		{
		  handleBaseRequest();		        		
		   }
		else if (opcode == OPCODE_PHOTO)
		{
		  handlePhotoRequest();
		
		}		
		// else if (opcode==22) { // send us some number of packets after this one
		//	packetCount = readShort();
		//	if ((packetCount>0)&&(packetCount<=mMaxPackets))
		//		mPacketCount = packetCount;
		//}
	}
	clearReturnPacket();
	mServerStage = PacketRead;
}

void dataSource::allocateBuffers()
{
  if (!mReturnBuffer) {
    mReturnBuffer = new char[mPacketSize];
    memset((void *)(mReturnBuffer),0,mPacketSize);
  }
  if (!mSendBuffer) {
    mSendBuffer = new char[mPacketSize];
    memset((void *)(mSendBuffer),0,mPacketSize);
  }
  if (!mStringBuffer) { 
    mStringBuffer = new char[mPacketSize];
    memset((void *)(mStringBuffer),0,mPacketSize);
  }
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

void dataSource::connectSendSocket()
{
	struct sockaddr_in source_addr;
	
	mReturnBuffer = new char[mPacketSize];	
	mSendBuffer = new char[mPacketSize];		
	mStringBuffer = new char[mPacketSize];
	memset((void *)(mReturnBuffer),0,mPacketSize);
	memset((void *)(mSendBuffer),0,mPacketSize);
	memset((void *)(mStringBuffer),0,mPacketSize);		

	mReadyForRequests = true;

	addBaseRequest();
	addPhotoRequest("test01.jpg");

	mWorkSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	if (mWorkSockfd < 0) {
	  cout << "ERROR opening send socket\n";
		return;
	}

	//if (windows)
	//u_long iMode=1;
	//ioctlsocket(mWorkSockfd,O_NONBLOCK,&iMode);//Make it a non-blocking socket.
	//endif
	

#ifdef windows_OS
	ZeroMemory((char *)&source_addr, sizeof(source_addr));
#else
	bzero((char *)&source_addr, sizeof(source_addr));
#endif

    source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = inet_addr( mSourceIP );
    source_addr.sin_port = htons(mPort);
	int connectCode = connect(mWorkSockfd,(struct sockaddr *) &source_addr,sizeof(source_addr));
	if (connectCode < 0) 
	{
	  cout << "dataSource: ERROR connecting send socket, error: " << errno  << " mWorkdSockfd " << mWorkSockfd  << "\n";
		return;
	} else {		
	  cout << "dataSource: SUCCESS connecting send socket:" << connectCode << " mWorkdSockfd " << mWorkSockfd << "\n";
	}
}

void dataSource::sendPacket()
{
  
  memset((void *)(mStringBuffer),0,mPacketSize);	
  memcpy((void*)mStringBuffer,reinterpret_cast<void*>(&mSendControls),sizeof(short));
  memcpy((void*)&mStringBuffer[sizeof(short)],(void*)mSendBuffer,mPacketSize-sizeof(short));
  cout << " SENDING - " <<  mSendByteCounter  << " bytes! \n";
  int n = send(mWorkSockfd,mStringBuffer,mPacketSize,0);	
  if (n < 0)
	  cout << "ERROR sending packet!  errno = " << errno << "\n";

  mLastSendTick = mCurrentTick;
  cout << " clearing send packet!   currentTick = " << mCurrentTick << " \n";
  clearSendPacket();
}

void dataSource::clearSendPacket()
{	
	memset((void *)(mSendBuffer),0,mPacketSize);
	memset((void *)(mStringBuffer),0,mPacketSize);	

	mSendControls = 0;
	mSendByteCounter = 0;
}

void dataSource::clearReturnPacket()
{
	memset((void *)(mReturnBuffer),0,mPacketSize);	
	memset((void *)(mStringBuffer),0,mPacketSize);	

	mReturnByteCounter = 0;	
}


/////////////////////////////////////////////////////////////////////////////////


void dataSource::disconnectSockets()
{
#ifdef windows_OS
  closesocket(mListenSockfd);
  closesocket(mWorkSockfd);
#else
  close(mListenSockfd);
  close(mWorkSockfd);
#endif  
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::writeShort(short value)
{
	memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(short));
	mSendByteCounter += sizeof(short);	
}

void dataSource::writeInt(int value)
{
	memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(int));
	mSendByteCounter += sizeof(int);	
}

void dataSource::writeFloat(float value)
{
	memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(float));
	mSendByteCounter += sizeof(float);	
}

void dataSource::writeDouble(double value)
{
	memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(double));
	mSendByteCounter += sizeof(double);	
}

void dataSource::writeString(const char *content)
{
	int length = strlen(content);
	writeInt(length);
	strncpy(&mSendBuffer[mSendByteCounter],content,length);
	mSendByteCounter += length;
}

//void dataSource::writePointer(void *pointer) //Maybe, someday? using boost, shared pointer or shared memory? 
//{  //Or can it be done in a more brute force way with global scale pointers? 
//}

//////////////////////////////////////////////

short dataSource::readShort()
{
	char bytes[sizeof(short)];
	for (int i=0;i<sizeof(short);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
	mReturnByteCounter+=sizeof(short);
	short *ptr = reinterpret_cast<short*>(bytes);
	return *ptr;	
}

int dataSource::readInt()
{
	char bytes[sizeof(int)];
	for (int i=0;i<sizeof(int);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
	mReturnByteCounter+=sizeof(int);
	int *ptr = reinterpret_cast<int*>(bytes);
	return *ptr;
}

float dataSource::readFloat()
{
	char bytes[sizeof(float)];
	for (int i=0;i<sizeof(float);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
	mReturnByteCounter+=sizeof(float);
	float *ptr = reinterpret_cast<float*>(bytes);
	return *ptr;
}

double dataSource::readDouble()
{
	char bytes[sizeof(double)];
	for (int i=0;i<sizeof(double);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
	mReturnByteCounter+=sizeof(double);
	double *ptr = reinterpret_cast<double*>(bytes);
	return *ptr;
}

char *dataSource::readString()
{
	int length = readInt();
	strncpy(mStringBuffer,&mReturnBuffer[mReturnByteCounter],length);
	mReturnByteCounter += length;
	//cout << "READING STRING: " << mStringBuffer << "\n";
	return mStringBuffer;
}

void dataSource::clearString()
{
	if (mStringBuffer!=NULL)	
		memset((void *)(mStringBuffer),0,mPacketSize);
}

//void dataSource::readPointer()
//{
//}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::addBaseRequest()
{	
  short opcode = OPCODE_BASE;
  mSendControls++;//(Increment this every time you add a control.)
	writeShort(opcode);
	writeInt(mCurrentTick);
	//For a baseRequest, do nothing but send a tick value to make sure there's a connection.
}

void dataSource::handleBaseRequest()
{	
	int tick = readInt();				
  if (mDebugToConsole)
    cout << "dataSource clientTick = " << tick << ", my tick " << mCurrentTick;
  if (mDebugToFile)
    fprintf(mDebugLog,"dataSource clientTick %d, my tick %d\n\n",tick,mCurrentTick);
}

void dataSource::addPhotoRequest(const char *cmdArgs)
{
  short opcode = OPCODE_PHOTO;
  mSendControls++;//(Increment this every time you add a control.)
	writeShort(opcode);
  writeString(cmdArgs);
}

void dataSource::handlePhotoRequest()
{
  char imgName[512],command[512];
  readString();
  if (strlen(mStringBuffer)>0 )
    {
      sprintf(command,"%s %s","raspistill ",mStringBuffer);		   
      system(command);
      if (mDebugToConsole)
	cout  << "Executing command: [" << command << "]\n";
      if (mDebugToFile)
	  fprintf(mDebugLog,"Executing command: %s \n\n",command);
      
      mWorkSockfd = -1;
}
}


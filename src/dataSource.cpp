//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#include "dataSource.h"

using namespace std;

//#include "console/consoleTypes.h"//Torque specific, should do #ifdef TORQUE or something

dataSource::dataSource(bool server,int port)
{
	mPacketSize = 1024;
	mSocketTimeout = 0;
	mPort = port;//9984
	mCurrentTick = 0;
	mLastSendTick = 0;
	mLastSendTimeMS = 0;
	mTickInterval = 1;//45;
	mTalkInterval = 20;
	mStartDelay = 3;//50
	mPacketCount = 0;
	mMaxPackets = 20;
	mSendControls = 0;
	mReturnByteCounter = 0;
	mSendByteCounter = 0;
	sprintf(mSourceIP,"127.0.0.1");//(IP argument)
	mListenSockfd = 0;//INVALID_SOCKET;
	mWorkSockfd = 0;//INVALID_SOCKET;
	mReturnBuffer = NULL;
	mSendBuffer = NULL;
	mStringBuffer = NULL;
	mReadyForRequests = false;	
	mAlternating = true;
	mConnectionEstablished = false;
	if (server)
	{
	  mServer = true;
	  mListening = true;
	} else {
	  mServer = false;
	  mListening = false;
	}
	//printf("New data source object!!! Source IP: %s listening %d\n",mSourceIP,server);
}

dataSource::~dataSource()
{
	disconnectSockets();
}

////////////////////////////////////////////////////////////////////////////////

void dataSource::tick()
{	
  cout << "datasource tick " << mCurrentTick << "\n";
  
    if ((mCurrentTick++ % mTickInterval == 0)&&
		(mCurrentTick > mStartDelay)) 
	{ 
		if (mConnectionEstablished == false)
		{
			  cout << " trying sockets!! " ;
			trySockets();
		} else {
			if (mListening) {
			  cout << " listening for packet! " ;
				listenForPacket();
				if (mAlternating) {
					mListening = false;
					addBaseRequest();
					if (mServer) tick();
				}
			} else {
			  cout << " sending packet! " ;				
				sendPacket();
				if (mAlternating) {
					mListening = true;
					if (!mServer) tick();
				} else 
					addBaseRequest();
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::openListenSocket()
{
	struct sockaddr_in source_addr;
	//int n;

	//Con::printf("connecting listen socket\n");
	mListenSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	if (mListenSockfd < 0) {
	  //Con::printf("ERROR opening listen socket \n");
		return;
	} else {
	  //Con::printf("SUCCESS opening listen socket \n");
	}
	
	bool bOptVal = true;
	//// lose the pesky "Address already in use" error message  - only for listen socket?
	if (setsockopt(mListenSockfd,SOL_SOCKET,SO_REUSEADDR,(char *) &bOptVal,sizeof(bool)) == -1) {
	  //Con::printf("FAILED to set socket options\n");
		return;
	} 

#ifdef windows_OS
	u_long iMode=1;
	//ioctlsocket(mListenSockfd,O_NONBLOCK,&iMode);//Make it a non-blocking socket.
#else
	int flags;
	flags = fcntl(mListenSockfd,F_GETFL,0);
	if (flags != -1)
	  fcntl(mListenSockfd, F_SETFL, flags | O_NONBLOCK);
#endif //Unix equivalent?? Or do it in setsockopt?
	
#ifdef windows_OS
	ZeroMemory((char *) &source_addr, sizeof(source_addr));
#else
	bzero((char *)&source_addr, sizeof(source_addr));
#endif

    source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = inet_addr( mSourceIP );
    source_addr.sin_port = htons(mPort);

    if (bind(mListenSockfd, (struct sockaddr *) &source_addr,
    		sizeof(source_addr)) < 0) 
	  printf("ERROR on binding mListenSockfd\n");
    else printf("SUCCESS binding mListenSockfd\n");

}

void dataSource::connectListenSocket()
{
	int n;
	
	n = listen(mListenSockfd,10);
	if (n == -1) //SOCKET_ERROR)
	{
	  //Con::printf(" listen socket error!\n");
		return;
	}
	
	mWorkSockfd = accept(mListenSockfd,NULL,NULL);
	
	if (mWorkSockfd == 0) //INVALID_SOCKET)
	{
	  //Con::printf(".");
		return;
	} else {
	  //Con::printf("\nlisten accept succeeded!\n");
		mReturnBuffer = new char[mPacketSize];
		memset((void *)(mReturnBuffer),0,mPacketSize);	
		mReturnBuffer = new char[mPacketSize];
		memset((void *)(mSendBuffer),0,mPacketSize);
		mStringBuffer = new char[mPacketSize];
		memset((void *)(mStringBuffer),0,mPacketSize);	
	}
}

void dataSource::listenForPacket()
{
  //Con::printf("dataSource listenForPacket");
	mPacketCount = 0;
	cout << " receiving packet? ";
	int n = recv(mWorkSockfd,mReturnBuffer,mPacketSize,0);
	if (n<=0) {
	  printf(".");
		return;
	}

	cout << " reading packet! ";
	readPacket();
}

	//for (int j=0;j<mPacketCount;j++)
	//{
	//	n = recv(mWorkSockfd,mReturnBuffer,mPacketSize,0);
	//	if (n<=0) j--;
	//	else {
	//		readPacket();
	//	}
	//}


void dataSource::readPacket()
{
	short opcode,controlCount;//,packetCount;

	controlCount = readShort();
	for (short i=0;i<controlCount;i++)
	{		
		opcode = readShort();
		if (opcode==1) {   ////  keep contact, but no request /////////////////////////
			int tick = readInt();				
			//if (mServer) Con::printf("dataSource clientTick = %d, my tick %d",tick,mCurrentTick);
			//else Con::printf("dataSource serverTick = %d, my tick %d",tick,mCurrentTick);
		}// else if (opcode==22) { // send us some number of packets after this one
		//	packetCount = readShort();
		//	if ((packetCount>0)&&(packetCount<=mMaxPackets))
		//		mPacketCount = packetCount;
		//}
	}
	
	clearReturnPacket();
}

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

	mWorkSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	if (mWorkSockfd < 0) {
	  //Con::printf("ERROR opening send socket\n");
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
	  //Con::printf("dataSource: ERROR connecting send socket: %d\n",connectCode);
		return;
	} else {		
	  //Con::printf("dataSource: SUCCESS connecting send socket: %d\n",connectCode);
	}
}

void dataSource::sendPacket()
{
  cout << " setting up packet \n" ;
  memset((void *)(mStringBuffer),0,mPacketSize);	
  memcpy((void*)mStringBuffer,reinterpret_cast<void*>(&mSendControls),sizeof(short));
  memcpy((void*)&mStringBuffer[sizeof(short)],(void*)mSendBuffer,mPacketSize-sizeof(short));
  cout << " SENDING!! \n";
  send(mWorkSockfd,mStringBuffer,mPacketSize,0);	
  mLastSendTick = mCurrentTick;
  cout << " clearing send packet! \n";
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

void dataSource::trySockets()
{
  if (mServer)
    {
      cout << " server ... " ;
      if (mListenSockfd == 0) { //INVALID_SOCKET) {
	cout << " openListenSocket ... \n\n" ;
	    openListenSocket();
      } else if (mWorkSockfd ==  0) { //INVALID_SOCKET) {
	cout << " connectListenSocket ...  \n\n" ;
	connectListenSocket();
      } else {
	cout << " ListenForPacket ...  \n\n" ;
	listenForPacket();
	mConnectionEstablished = true;
      }
    } else {
      cout << " client ... " ;
    if (mWorkSockfd ==  0) { //INVALID_SOCKET) {
      cout << " connectSendSocket()... \n\n";      
      connectSendSocket();
    } else {
      cout << " sendPacket()... \n\n";   
      sendPacket();
      mConnectionEstablished = true;
    }
  }
}

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

void dataSource::writeString(char *content)
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
	short opcode = 1;//base request
	mSendControls++;//Increment this every time you add a control.
	writeShort(opcode);
	writeInt(mCurrentTick);//For a baseRequest, do nothing but send a tick value to make sure there's a connection.
}

void dataSource::handleBaseRequest()
{	
	int tick = readInt();				
	//if (mServer) Con::printf("dataSource clientTick = %d, my tick %d",tick,mCurrentTick);
	//else Con::printf("dataSource serverTick = %d, my tick %d",tick,mCurrentTick);
}

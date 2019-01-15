//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#include "dataSource.h"

using namespace std;

//#include "console/consoleTypes.h"//Torque specific, should do #ifdef TORQUE or something

dataSource::dataSource(bool server,int port, char *IP)
{
	mPacketSize = 1024;
	mSocketTimeout = 0;
	mPort = port;//9984
	mCurrentTick = 0;
	mLastSendTick = 0;
	mLastSendTimeMS = 0;
	mTickInterval = 1;//45;
	mTalkInterval = 20;
	mStartDelay = 0;//50
	mPacketCount = 0;
	mMaxPackets = 20;
	mSendControls = 0;
	mReturnByteCounter = 0;
	mSendByteCounter = 0;
	//sprintf(mSourceIP,"192.68.1.32");//(IP argument)
	sprintf(mSourceIP,IP);
	//sprintf(mSourceIP,"10.0.0.242");
	mListenSockfd = 0;//INVALID_SOCKET;
	mWorkSockfd = 0;//INVALID_SOCKET;
	mReturnBuffer = NULL;
	mSendBuffer = NULL;
	mStringBuffer = NULL;
	mReadyForRequests = false;	
	mAlternating = false;
	mConnectionEstablished = false;
	mFinished = false;
	if (server)
	{
	  mServer = true;
	  mListening = true;
	} else {
	  mServer = false;
	  mListening = false;
	}

#ifdef windows_OS
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR) {
		cout << "Error initialising WSA.\n";		
	}
#endif

	//printf("New data source object!!! Source IP: %s listening %d\n",mSourceIP,server);
}

dataSource::~dataSource()
{
	disconnectSockets();
}

////////////////////////////////////////////////////////////////////////////////

void dataSource::tick()
{	
  //cout << "datasource tick " << mCurrentTick << "\n";
  
    if ( (mCurrentTick++ % mTickInterval == 0)&& //(!mFinished) && 
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
			  //cout << " sending packet! " ;				
				sendPacket();
				if (mAlternating) {
					mListening = true;
					if (!mServer) tick();
				} else 
					addBaseRequest();
			}
		}
	}
	//cout << "ending tick " << mCurrentTick << "\n";
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::openListenSocket()
{
	struct sockaddr_in source_addr;
	//int n;

	mListenSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	if (mListenSockfd < 0) {
	  cout << "ERROR opening listen socket \n";
		return;
	} else {
	  cout << "SUCCESS opening listen socket \n";
	}
	
	bool bOptVal = true;
	//// lose the pesky "Address already in use" error message  - only for listen socket?
	//if (setsockopt(mListenSockfd,SOL_SOCKET,SO_REUSEADDR,(char *) &bOptVal,sizeof(bool)) == -1) {
	//  cout << "FAILED to set socket options\n";
	//	return;
	//} 

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

	if (bind(mListenSockfd, (struct sockaddr *) &source_addr,sizeof(source_addr)) < 0) 
	  cout << "ERROR on binding mListenSockfd\n";
	else cout << "SUCCESS binding mListenSockfd\n";

}

void dataSource::connectListenSocket()
{
	int n;
	
	n = listen(mListenSockfd,10);
	if (n == -1) //SOCKET_ERROR)
	{
		int tmp = errno;
	  cout << " listen socket error!  errno " << errno <<  "  " << strerror(tmp) << "\n";
		return;
	}
	
	//cout << "Accepting work socket...\n";
	mWorkSockfd = accept(mListenSockfd,NULL,NULL);
	
	if (mWorkSockfd == -1) //INVALID_SOCKET)
	{
	  cout << "waiting... worksock " << mWorkSockfd  << "\n";
		return;
	} else {
	  cout << "\nlisten accept succeeded mPacketSize: " << mPacketSize << "\n";
	  
		mReturnBuffer = new char[mPacketSize];
		memset((void *)(mReturnBuffer),0,mPacketSize);	
	  
	  	mSendBuffer = new char[mPacketSize];
		memset((void *)(mSendBuffer),0,mPacketSize);
	  
		mStringBuffer = new char[mPacketSize];
		memset((void *)(mStringBuffer),0,mPacketSize);	

	}
}

void dataSource::listenForPacket()
{
	mPacketCount = 0;
	if (mWorkSockfd == -1)
	  mWorkSockfd = accept(mListenSockfd,NULL,NULL);

	if (mWorkSockfd == -1) {
	  //cout << ",\n";
		return;
	}
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
	
	//cout << "Calling recv, workSock " << mWorkSockfd << "\n";
	int n = recv(mWorkSockfd,mReturnBuffer,mPacketSize,0);
	//int n = read(mWorkSockfd,mReturnBuffer,mPacketSize);
	
	//cout << " receiving packet? n=" << n << "\n";
	if (n<0) {
	  cout << "ERROR : " << errno  << "  " << strerror(errno)  << "\n";
		return;
	}

	//cout << " reading packet! ";
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
		  }
	}
		// else if (opcode==22) { // send us some number of packets after this one
		//	packetCount = readShort();
		//	if ((packetCount>0)&&(packetCount<=mMaxPackets))
		//		mPacketCount = packetCount;
		//}
	
	
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
	addPhotoRequest("test01.jpg");
	mFinished = true;//New plan, just kill the whole process after this request goes out.

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
  //cout << " SENDING - " <<  mSendByteCounter  << " bytes! \n";
  int n = send(mWorkSockfd,mStringBuffer,mPacketSize,0);	
  if (n < 0)
	  cout << "ERROR sending packet!  errno = " << errno << "\n";

  mLastSendTick = mCurrentTick;
  //cout << " clearing send packet!   currentTick = " << mCurrentTick << " \n";
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
	writeInt(mCurrentTick);
	//For a baseRequest, do nothing but send a tick value to make sure there's a connection.
}

void dataSource::handleBaseRequest()
{	
	int tick = readInt();				
	if (mServer) cout << "dataSource clientTick = " << tick << ", my tick " << mCurrentTick;
	else cout << "dataSource serverTick = " << tick << ", my tick " << mCurrentTick;
}

void dataSource::addPhotoRequest(const char *imgName)
{
	short opcode = 15;//photo request, arbitrary but putting it above the ones I've already used for terrain server, etc.
	mSendControls++;//Increment this every time you add a control.
	writeShort(opcode);
	writeString(imgName);
}

void dataSource::handlePhotoRequest()
{
	//Empty on this side, fires off on pi side.
}
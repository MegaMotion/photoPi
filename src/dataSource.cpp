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

	mDebugToConsole = false;
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
	  mDebugLog = fopen("log.txt","a");
	  fprintf(mDebugLog,"\n\n--------------------------- PhotoPi Debug Log\n\n");
	}

	string strLine;

	string strOutput;
	string strTime;
	string strIso;
	string strImageEffect;
	string strColorEffect;
	string strExposureMode;
	string strAwbMode;
	string strShutterSpeed;
	string strFlickerAvoid;
	string strDrc;
	string strImgWidth;
	string strImgHeight;
	string strJpgQuality;

	//string strOptions;

	char line[72];
	char label[36];
	char value[36];

	mStrOptions = "";

	//First, open config text file, load values for raspistill.
	FILE *fp = fopen("config.txt", "r");//TESING: overrode 
	while (fgets(line, 72, fp))
	{
		line[strcspn(line, "\n")] = 0;//Strip off the carriage return.

									  //Bail here if this is a comment or the line doesn't have an equals sign in it.
		if ((line[0] == '#') || !(strstr(line, "=")))
			continue;

		//Convert it to std::string.
		strLine = line;
		cout << "LINE: " << line << "\n";
		sprintf(label, strLine.substr(0, strLine.find('=')).c_str());
		sprintf(value, strLine.substr(strLine.find('=') + 1).c_str());
		if (strlen(value) == 0)
			continue;


		if (!strcmp(label, "OUTPUT"))
		{
			mStrFilename = value;
			mStrFilename.erase(mStrFilename.find("."));// Strip file extension, I hope?
			strOutput = " -o "; strOutput.append(value); mStrOptions.append(strOutput);
		}
		else if (!strcmp(label, "TIME"))
		{
			strTime = " -t "; strTime.append(value); mStrOptions.append(strTime);
		}
		else if (!strcmp(label, "ISO"))
		{
			strIso = " -ISO "; strIso.append(value); mStrOptions.append(strIso);
		}
		else if (!strcmp(label, "IMAGE_EFFECT"))
		{
			strImageEffect = " -ifx "; strImageEffect.append(value); mStrOptions.append(strImageEffect);
		}
		else if (!strcmp(label, "COLOR_EFECT"))
		{
			strColorEffect = " -cfx "; strColorEffect.append(value); mStrOptions.append(strColorEffect);
		}
		else if (!strcmp(label, "EXPOSURE_MODE"))
		{
			strExposureMode = " -ex "; strExposureMode.append(value); mStrOptions.append(strExposureMode);
		}
		else if (!strcmp(label, "AWB_MODE"))
		{
			strAwbMode = " -awb "; strAwbMode.append(value); mStrOptions.append(strAwbMode);
		}
		else if (!strcmp(label, "SHUTTER_SPEED"))
		{
			strShutterSpeed = " -ss "; strShutterSpeed.append(value); mStrOptions.append(strShutterSpeed);
		}
		else if (!strcmp(label, "FLICKER_AVOID"))
		{
			strFlickerAvoid = " -fli "; strFlickerAvoid.append(value); mStrOptions.append(strFlickerAvoid);
		}
		else if (!strcmp(label, "DRC"))
		{
			strDrc = " -drc "; strDrc.append(value); mStrOptions.append(strDrc);
		}
		else if (!strcmp(label, "IMAGE_WIDTH"))
		{
			strImgWidth = " -w "; strImgWidth.append(value); mStrOptions.append(strImgWidth);
		}
		else if (!strcmp(label, "IMAGE_HEIGHT"))
		{
			strImgHeight = " -h "; strImgHeight.append(value); mStrOptions.append(strImgHeight);
		}
		else if (!strcmp(label, "JPG_QUALITY"))
		{
			strJpgQuality = " -q "; strJpgQuality.append(value); mStrOptions.append(strJpgQuality);
		}
		else if (!strcmp(label, "ENCODING"))
		{
			mStrEncoding = " -e "; mStrEncoding.append(value); mStrOptions.append(mStrEncoding);
			mStrFilename += "."; 
			mStrFilename += value;
		}
	}
	fclose(fp);

	cout << "OPTIONS: " << mStrOptions.c_str() << "\n";
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
  //cout << "datasource tick " << mCurrentTick << "\n";
  
	
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
		  connectSendSocket(); break;
	  case ClientSocketCreated:
		  break;
	  case ClientSocketConnected:
		  break;
	  case PacketSent:
		  mCurrentTick = 501;//FIX: cheap hack to get us out of main loop in main.cpp
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

void dataSource::connectSendSocket()
{
	struct sockaddr_in source_addr;
	FILE *fp,*getPicsFp;

	mReturnBuffer = new char[mPacketSize];	
	mSendBuffer = new char[mPacketSize];		
	mStringBuffer = new char[mPacketSize];
	memset((void *)(mReturnBuffer),0,mPacketSize);
	memset((void *)(mSendBuffer),0,mPacketSize);
	memset((void *)(mStringBuffer),0,mPacketSize);		

	mReadyForRequests = true;

	//Next, we are going to open up our text file, cameraIPs.txt, and run through a list of IPs instead of just one.
	fp = fopen(mSourceIP, "r");//TESING: overrode 
	if (fp == NULL)
	{
		cout << "ERROR: can't open IPs file: " << mSourceIP << "\n";
		return;
	}
	else cout << "opened IPs file: " << mSourceIP << "\n";

	getPicsFp = fopen("getPics.sh", "w");

	char IP[16],IP_stripped[16];
	string filename,getPicString,getPicFile="";
	
	fgets(IP, 16, fp);
	IP[strcspn(IP, "\n")] = 0;//Strip off the carriage return.
	while (strlen(IP) > 0)
	{
		//APPLICATION SPECIFIC MODIFICAITON:
		addPhotoRequest();

		mWorkSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (mWorkSockfd < 0) {
			cout << "ERROR opening send socket\n";
			return;
		}

#ifdef windows_OS
		ZeroMemory((char *)&source_addr, sizeof(source_addr));
#else
		bzero((char *)&source_addr, sizeof(source_addr));
#endif

		source_addr.sin_family = AF_INET;
		//source_addr.sin_addr.s_addr = inet_addr( mSourceIP );
		source_addr.sin_addr.s_addr = inet_addr(IP);
		source_addr.sin_port = htons(mPort);
	
		int connectCode = connect(mWorkSockfd, (struct sockaddr *) &source_addr, sizeof(source_addr));
		if (connectCode < 0)
		{
			cout << "dataSource: ERROR connecting send socket, error: " << errno << " mWorkdSockfd " << mWorkSockfd << "\n";
			return;
		}
		else {
			cout << "dataSource: SUCCESS connecting send socket:" << connectCode << " mWorkdSockfd " << mWorkSockfd << "\n";
			sendPacket();
		}

		//write a line for getPics.sh
		getPicString = "scp pi@";
		getPicString.append(IP); //getPicString.erase(getPicString.find("\n"));//remove newline if present
		getPicString += ":~/photoPi/img/";
		getPicString.append(mStrFilename.c_str());
		getPicString += " ./img/";
		getPicString.append(IP); //getPicString.erase(getPicString.find("\n"));//remove newline if present
		getPicString += ".";
		getPicString.append(mStrFilename.c_str());
		getPicString += "\n";
		cout << getPicString.c_str();
		fprintf(getPicsFp, "%s", getPicString.c_str());

		//Clear and load the next IP from the file.
		strcpy(IP, "");//Unnecessary?
		fgets(IP, 16, fp);
		IP[strcspn(IP, "\n")] = 0;//Strip off the carriage return.
	}


	fclose(fp);
	fclose(getPicsFp);

	mClientStage = PacketSent;
	
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

void dataSource::addPhotoRequest()
{
  short opcode = OPCODE_PHOTO;
  mSendControls++;//(Increment this every time you add a control.)
	writeShort(opcode);
	writeString(mStrOptions.c_str());
}

void dataSource::handlePhotoRequest()
{
	//Empty on this side, fires off on pi side.
}
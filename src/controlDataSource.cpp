//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#include "sim/dataSource/controlDataSource.h"

#include "console/consoleTypes.h"//Torque specific, should do #ifdef TORQUE or something



controlDataSource::controlDataSource(bool listening, bool alternating, int port, char *IP) : dataSource(listening, alternating, port, IP)
{

}

controlDataSource::~controlDataSource()
{

}

////////////////////////////////////////////////////////////////////////////////
void controlDataSource::tick()
{
	if (mListening)
	{
		//Con::printf("controlDataSource tick %d server stage %d", mCurrentTick, mServerStage);
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
			receivePacket(); //this now calls readPacket
			break;
			//case PacketReceived:
			//    readPacket();
			//    break;
			//case PacketRead:
			//    mServerStage = ServerSocketListening;
			//    break;
		}
	}
	else
	{
		//Con::printf("controlDataSource tick %d client stage %d", mCurrentTick, mClientStage);
		switch (mClientStage)
		{
		case NoClientSocket:
			connectSendSocket(); break;
		case ClientSocketCreated:
			break;
		case ClientSocketConnected:
			addBaseRequest();
			addTestRequest();
			sendPacket();
			break;
		}
	}
	mCurrentTick++;
}

void controlDataSource::addQuitRequest()
{
	short opcode = OPCODE_QUIT;
	mSendControls++;//(Increment this every time you add a control.)
	writeShort(opcode);
	//For a baseRequest, do nothing but send a tick value to make sure there's a connection.
}

void controlDataSource::handleQuitRequest()
{

	if (mDebugToConsole)
		std::cout << "handleQuitRequest \n";
	if (mDebugToFile)
		fprintf(mDebugLog, "handleQuitRequest\n\n");
}

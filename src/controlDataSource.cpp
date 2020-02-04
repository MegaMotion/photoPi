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

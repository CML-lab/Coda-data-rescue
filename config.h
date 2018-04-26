// *********************************
//  Change these parameters only:

//define the trial table (to match back the file name for consistency)
#define TRIALFILE "../seqlearn/test.txt"  

//define config file
#define CONFIGFILE "../seqlearn/codaconfig.txt"


//calibration offsets - these are the default for the Coda system!
#define CALxOFFSET -0.152f
#define CALyOFFSET 0.034f


//
//**********************************


#define SERVER 0



#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <istream>
#include <fstream>
#include <sstream>
#include <memory.h>
#include <time.h>
#include <windows.h>
#include <math.h>
#include <map>

// RTNet C++ includes
#include "codaRTNetProtocolCPP/RTNetClient.h"
#include "codaRTNetProtocolCPP/DeviceOptionsAlignment.h"
#include "codaRTNetProtocolCPP/DeviceOptionsCodaMode.h"
#include "codaRTNetProtocolCPP/DeviceOptionsCodaPacketMode.h"
#include "codaRTNetProtocolCPP/DeviceInfoAlignment.h"
#include "codaRTNetProtocolCPP/DeviceInfoUnitCoordSystem.h"
#include "codaRTNetProtocolCPP/PacketDecode3DResult.h"
#include "codaRTNetProtocolCPP/PacketDecode3DResultExt.h"
#include "codaRTNetProtocolCPP/PacketDecodeADC16.h"
#include "codaRTNetProtocol/codartprotocol_gs16aio.h"
#include "codaRTNetProtocol/codartprotocol_ni6221.h"
#include "codaRTNetProtocol/codartprotocol_cx1.h"




struct CODASYSCONFIG
{
	codaRTNet::RTNetClient cl;  //client connection object
	codaRTNet::AutoDiscover discover;
	codaRTNet::DataStream stream;
	FILE* monitorfile;
	
	DWORD configchoice;
	BOOL bExtSync;

	int ncameras;

	//device ID flags
	bool bGS16AIOenabled;
	bool bNI6221enabled;

    int MonitorPeriod; //ms
	static const int UDPbufferSize = 100000;

	DWORD cx1mode;
    DWORD cx1decim;
	int MaxMarkers;
	WORD MaxMarkerInUse;

	int MonitorRate;  //effective sampling rate during the task
	float AcqRate;		//true data samplng rate in the final saved dataset
    DWORD MaxSamples;
    float AcqTimeMax;
	DWORD PacketSize;
	DWORD PacketTransmitSize;

    // acquisition counters & timers:
	int MonitorSample;
    DWORD SampleNum;
    DWORD PrevSampleNum;
    float SampleTime;
    DWORD SyncStopSampleNum[1000];

	float cx1TickTime;		//sample tick period for cx1 data (should be same as AcqRate)

	clock_t timeStart;
    clock_t timeStop;

};

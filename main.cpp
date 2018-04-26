/*
  This code is to rescue data that remains on the Codamotion server if the program crashes 
  before data can be downloaded from the buffer.

*/

//#include "TrackCodamotion.h"
#include "config.h"



void print_devicestatusarray_errors(const codaRTNet::DeviceStatusArray& array);
DWORD getfirstdeverror(const codaRTNet::DeviceStatusArray& arr);


int main()
{

	freopen( "transferlog.txt", "w", stderr); 

	int ch;
	

	DWORD iServer = SERVER;

	CODASYSCONFIG CodaSysConfig;


	std::cout << "Re-establishing connection to server..." << std::endl;
	std::cerr << "Attempting to re-establish connection to server..." << std::endl;

	//*** Connect to Coda Server ***


		// attempt auto-discover server
		CodaSysConfig.cl.doAutoDiscoverServer(CodaSysConfig.discover);

		// need at least one server to connect to
		if(CodaSysConfig.discover.dwNumServers == 0)
		{
			std::cerr << "ERROR: no RTNet Servers found using auto-discovery." << std::endl << "   Start the RTNetServer on the Codamotion host computer." << std::endl;
			return(1);
		}

		DWORD ip1,ip2,ip3,ip4;

		
		// list all servers
		std::cerr << "Found " << CodaSysConfig.discover.dwNumServers << "Codamotion Realtime Network servers:" << std::endl;
		for(DWORD iserver = 0; iserver < CodaSysConfig.discover.dwNumServers; iserver++)
		{
			ip1 = (CodaSysConfig.discover.server[iserver].ip & 0xFF000000) >> 24;
			ip2 = (CodaSysConfig.discover.server[iserver].ip & 0x00FF0000) >> 16;
			ip3 = (CodaSysConfig.discover.server[iserver].ip & 0x0000FF00) >>  8;
			ip4 = (CodaSysConfig.discover.server[iserver].ip & 0x000000FF);

			/*
			std::cerr << "   [" << iserver << "]: " << CodaSysConfig.discover.server[iserver].hostname 
				      << " IP: " << ip1 << "." << ip2 << "." << ip3 << "." << ip4 
					  << " port: " << CodaSysConfig.discover.server[iserver].port << std::endl;
			*/
			fprintf(stderr, "[%u]: %s IP: %u.%u.%u.%u port: %u\n",
			  iserver,
			  CodaSysConfig.discover.server[iserver].hostname,
			  (CodaSysConfig.discover.server[iserver].ip & 0xFF000000) >> 24,
			  (CodaSysConfig.discover.server[iserver].ip & 0x00FF0000) >> 16,
			  (CodaSysConfig.discover.server[iserver].ip & 0x0000FF00) >>  8,
			  (CodaSysConfig.discover.server[iserver].ip & 0x000000FF)      ,
			  CodaSysConfig.discover.server[iserver].port);
		}
		std::cerr << "   USING: server " << SERVER << std::endl;



	// connect to selected server
		try
		{
			CodaSysConfig.cl.connect(CodaSysConfig.discover.server[iServer].ip, CodaSysConfig.discover.server[iServer].port);
		}
		catch(codaRTNet::DeviceStatusArray& array)
		{
			print_devicestatusarray_errors(array);
			return 1;
		}
	
		std::cout << "   Connected to server." << std::endl;
		std::cerr << "   connection successful." << std::endl;

	// get hardware config list
	codaRTNet::HWConfigEnum configs;
	CodaSysConfig.cl.enumerateHWConfig(configs);
	
	// print config names
	std::cerr << "Found " << configs.dwNumConfig << " hardware configurations:" << std::endl;
	for(DWORD iconfig = 0; iconfig < configs.dwNumConfig; iconfig++)
		//std::cerr << "   [" << iconfig << "] address: " << configs.config[iconfig].dwAddressHandle << " name: " << configs.config[iconfig].strName << std::endl; 
		fwprintf(stderr, L"  [%u] address: %08X  name: %s\n", 
			iconfig, configs.config[iconfig].dwAddressHandle, configs.config[iconfig].strName);
	
	// must have at least one config set up
	if(configs.dwNumConfig == 0)
	{
		std::cerr << "ERROR: no hardware configurations are available" << std::endl;
		return(2);
	}



	//*** configuration setup ***

	//open the configuration file to get all necessary parameters
	std::ifstream cfile(CONFIGFILE);
	if (!cfile)
	{
		std::cerr << "Cannot open config file." << std::endl;
		return(3);
	}

	char tmpline[100] = "";
	char configname[50] = "";
	char tmpname[50];


	//read the name of the hardware configuration to use
	cfile.getline(tmpline,sizeof(tmpline),'\n');  //get the first line of the file, which is the name of the configuration
	sscanf(tmpline,"%s",&configname);
	std::string t(configname);
	//std::cerr << "Hardware config name: " << t << std::endl;
	std::wstring cfname(t.begin(), t.end()); 
	//fwprintf(stderr, L" Hardware config name: %s\n", cfname);
	std::wcerr << " Hardware config requested: " << cfname << std::endl;

	CodaSysConfig.configchoice = 100;

	for (int iconfig = 0; iconfig < configs.dwNumConfig; iconfig++)
	{
		std::wcerr << " Config: " << configs.config[iconfig].strName << " : " << cfname.compare(configs.config[iconfig].strName) << std::endl;
		if(cfname.compare(configs.config[iconfig].strName)==0) //zero means they are equal
			CodaSysConfig.configchoice = DWORD(iconfig);
	}

	if(CodaSysConfig.configchoice < 0 || CodaSysConfig.configchoice > configs.dwNumConfig)
	{
		std::cerr << "Unrecognized or Invalid Hardware Configuration specified in config file!" << std::endl;
		return(4);
	}
	else
	{	
		//std::cerr << "Using: " << configs.config[CodaSysConfig.configchoice].strName << std::endl;
		fwprintf(stderr, L" USING: %s\n", configs.config[CodaSysConfig.configchoice].strName);
	}

	std::cout << "   Hardware configuration selected." << std::endl;

	// get enabled devices for selected hardware config
	CODANET_HWCONFIG_DEVICEENABLE devices;
	CodaSysConfig.cl.getDeviceEnable(configs.config[CodaSysConfig.configchoice].dwAddressHandle, devices);

	// print device IDs
	char strDevice[][20] = {"", "cx1", "AMTIserial", "GS16AIO", "Video direct", "Video remote", "Kistler", "AMTIanalog", "DI-720", "NI6221","","","","",""};
	std::cerr << "Found " << devices.dwNumOptions << " available Device types: " << std::endl;
	for(DWORD idevice = 0; idevice < devices.dwNumOptions; idevice++)
	{
		WORD deviceID = (DWORD)devices.option[idevice].wDevice;
		std::cerr << deviceID << "   ";

		//std::cerr << "   ID[" << idevice << "]: " << deviceID << strDevice[deviceID] << " (" << (devices.option[idevice].wEnable ? "enabled" : "disabled") << ")" << std::endl;
		fprintf(stderr, "  ID[%u]: %04X %10s [%s]\n", idevice, deviceID, strDevice[deviceID], devices.option[idevice].wEnable ? "enabled" : "disabled");
		if(deviceID == DEVICEID_GS16AIO && devices.option[idevice].wEnable)
			CodaSysConfig.bGS16AIOenabled = TRUE;
		if(deviceID == DEVICEID_NI6221 && devices.option[idevice].wEnable)
			CodaSysConfig.bNI6221enabled = TRUE;
	}


	//Info from alignment data, if any: may not correspond to configured system
	codaRTNet::DeviceInfoAlignment info;
	CodaSysConfig.cl.getDeviceInfo(info);
	std::cerr << "DeviceInfoAlignment.dev.NumUnits: " << info.dev.dwNumUnits << std::endl;
	//Info about configured system:
	codaRTNet::DeviceInfoUnitCoordSystem devinfo;
	CodaSysConfig.cl.getDeviceInfo(devinfo);
	std::cerr << "DeviceInfoUnitCoordSystem.dev.NumUnits: " << devinfo.dev.dwNumUnits << std::endl;


	//pull out alignment info line from config file, although we will not use it
	cfile.getline(tmpline,sizeof(tmpline),'\n');  //get the next line of the config file

	//pull out monitor rate line, although we won't use it
	cfile.getline(tmpline,sizeof(tmpline),'\n');  //get the next line of the config file


	//*** Set up data stream ***


    // create data stream (if not attempting a restart)
    // New to V1.7: New RTNet SDK function has new parameter to increase UDP buffer size, 
    // so can download more samples (packets) from acquisition buffers with one request.
	CodaSysConfig.cl.createDataStreamBuffer(CodaSysConfig.stream, 7000, CodaSysConfig.UDPbufferSize); //createDataStreamBuffer(DataStream& stream, ::WORD port, int bufferSize)
	


    //*** Specify whether to use External sync ***
	char dosync;

	cfile.getline(tmpline,sizeof(tmpline),'\n');  //get the next line of the config file
	sscanf(tmpline,"%c", &dosync);

    CodaSysConfig.bExtSync = 0;
	if(dosync == 'Y' || dosync == 'y')
	{
      CodaSysConfig.bExtSync = TRUE;
	  std::cerr << " Using external sync." << std::endl;
	}
	else
		std::cerr << " Not using external sync." << std::endl;


	//*** Set sampling rate, Marker IDs, and related parameters ***

	int samprate;
	CodaSysConfig.cx1mode = CODANET_CODA_MODE_100;
    CodaSysConfig.cx1decim = 1;

	cfile.getline(tmpline,sizeof(tmpline),'\n');  //get the next line of the config file
	sscanf(tmpline,"%d %u %u", &samprate, &(CodaSysConfig.cx1decim), &(CodaSysConfig.MaxMarkerInUse)); //mode is the sampling rate, i.e. one of: (100, 200, 400, 800) Hz
	
	// choose mode and determine maximum number of markers
    switch(samprate)
    {
      default:
      case 100:
        CodaSysConfig.cx1mode = CODANET_CODA_MODE_100;
        CodaSysConfig.MaxMarkers = 56;
        break;
      case 200:
        CodaSysConfig.cx1mode = CODANET_CODA_MODE_200;
        CodaSysConfig.MaxMarkers = 28;
        break;
      case 400:
        CodaSysConfig.cx1mode = CODANET_CODA_MODE_400;
        CodaSysConfig.MaxMarkers = 12;
        break;
      case 800:
        CodaSysConfig.cx1mode = CODANET_CODA_MODE_800;
        CodaSysConfig.MaxMarkers = 6;
        break;
    }

	// request max Marker ID in use (limits datafile output; does not affect cx1 mode)
    //CodaSysConfig.MaxMarkerInUse = CodaSysConfig.MaxMarkers-1;
    
	std::cerr << "There are at most " << CodaSysConfig.MaxMarkerInUse << " markers requested for use." << std::endl;

	if(CodaSysConfig.MaxMarkerInUse <= 0)
		CodaSysConfig.MaxMarkerInUse = 1;
	else if(CodaSysConfig.MaxMarkerInUse > CodaSysConfig.MaxMarkers)
	{
		std::cerr << "  There are too many markers specified for requested sampling rate! "; 
		if (CodaSysConfig.MaxMarkerInUse > 56) //we are in any mode and more than 56 markers are in use; need to limit the total number of markers.
		{
			std::cerr <<  "Number of markers limited to 56. " << CodaSysConfig.MaxMarkerInUse - 56 << " markers not recorded." << std::endl;
			CodaSysConfig.MaxMarkerInUse = 56;
			CodaSysConfig.cx1mode = CODANET_CODA_MODE_100;
		}
		else if (CodaSysConfig.MaxMarkerInUse > 28) //we are in a mode greater than or equal to 200 and there are more than 28 markers in use; set to 100 Hz
		{
			std::cerr <<  "Sampling rate reduced to 100 Hz." << std::endl;
			CodaSysConfig.cx1mode = CODANET_CODA_MODE_100;
			CodaSysConfig.MaxMarkers = 56;
		}
		else if (CodaSysConfig.MaxMarkerInUse > 12) //we are in a mode greater than or equal to 400 and there are more than 12 markers in use; set to 200 Hz
		{
			std::cerr <<  "Sampling rate reduced to 200 Hz." << std::endl;
			CodaSysConfig.cx1mode = CODANET_CODA_MODE_200;
			CodaSysConfig.MaxMarkers = 28;
		}
		else if (CodaSysConfig.MaxMarkerInUse > 6) //we are in mode 800 and there are more than 6 markers in use; set to 400 Hz
		{
			std::cerr <<  "Sampling rate reduced to 400 Hz." << std::endl;
			CodaSysConfig.cx1mode = CODANET_CODA_MODE_400;
			CodaSysConfig.MaxMarkers = 12;
		}
	}


      if(CodaSysConfig.cx1decim < 1 || CodaSysConfig.cx1decim > 9)
         CodaSysConfig.cx1decim = 1;


    // calculate & show acquisition frame-rate
    CodaSysConfig.AcqRate = (float)CodaSysConfig.cx1mode / (float)CodaSysConfig.cx1decim;
	/*
	std::cerr << "Mode: " << CodaSysConfig.cx1mode << ", decimation: " << CodaSysConfig.cx1decim << std::endl 
			  << "  Markers: " << CodaSysConfig.MaxMarkerInUse << "out of " << CodaSysConfig.MaxMarkers << " in use." << std::endl
			  << "  Acquisition rate: " << CodaSysConfig.AcqRate << " Hz." << std::endl;
	*/
	fprintf(stderr, "Mode:%d Decimation:%d AcqRate:%0.1fHz\n", CodaSysConfig.cx1mode, CodaSysConfig.cx1decim, CodaSysConfig.AcqRate);

    //-- set CodaMode (internal sync)
    //codaRTNet::DeviceOptionsCodaMode codaMode1(CodaSysConfig.cx1mode, CodaSysConfig.cx1decim, CodaSysConfig.bExtSync);  // dwRateMode, dwDecimation, dwExternalSync
    //CodaSysConfig.cl.setDeviceOptions(codaMode1);

    // get the sample tick period for cx1 data
    // (should be same as AcqRate)
    CodaSysConfig.cx1TickTime = CodaSysConfig.cl.getDeviceTickSeconds(DEVICEID_CX1);

    // request acquisition time limit:
    CodaSysConfig.MaxSamples = CODANET_ACQ_UNLIMITED;
    CodaSysConfig.AcqTimeMax = 0.0F;
	
	cfile.getline(tmpline,sizeof(tmpline),'\n');  //get the next line of the config file
	sscanf(tmpline,"%f", &CodaSysConfig.AcqTimeMax); //maximum acquisition duration, in seconds
	
    if(CodaSysConfig.AcqTimeMax > 0.0f)
      CodaSysConfig.MaxSamples = (int)(CodaSysConfig.AcqTimeMax * CodaSysConfig.AcqRate);


    // limit acquisition to maxTicks samples of CX1 data, or not:
    if(CodaSysConfig.MaxSamples == 0)
       CodaSysConfig.MaxSamples = CODANET_ACQ_UNLIMITED;
	//CodaSysConfig.cl.setAcqMaxTicks(DEVICEID_CX1, CodaSysConfig.MaxSamples);
	
	std::cerr << "Max acquisition time (s): " << CodaSysConfig.AcqTimeMax << std::endl;


	//set packet mode
	//codaRTNet::DeviceOptionsCodaPacketMode codaPacketMode1(CODANET_CODAPACKETMODE_COMBINED_COORD);
	//CodaSysConfig.cl.setDeviceOptions(codaPacketMode1);
	CodaSysConfig.PacketSize = 0;
	CodaSysConfig.PacketTransmitSize = 0;


	//initialize the number of CX1 cameras to be 1; we will correct this later in the data acquisition phase
	CodaSysConfig.ncameras = 1;



	//close configuration file
	cfile.close();

	
	std::cout << "Stopping acquisition..." << std::endl;

	//stop acquisition
	CodaSysConfig.cl.stopAcq(); 

	std::cerr << "Acquisition stopped." << std::endl;

	
	std::cout << "Transferring buffered data..." << std::endl;


	//now that we've reinitialized all our variables, we can shutdown the system nicely and ask for the data in the buffer

	//ShutDownCoda(CODASYSCONFIG *CodaSysConfig,tm* ltm)
	
	std::cerr << "Attempting to transfer buffered data." << std::endl;

    // Download data buffers: -------------------------------------------------------------------
	DWORD NumSamplesCX1;
	try
	{
		NumSamplesCX1 = CodaSysConfig.cl.getAcqBufferNumPackets(DEVICEID_CX1);
		std::cerr << "Downloading " << NumSamplesCX1 << " samples from acq.buffer." << std::endl;
	}
	catch(codaRTNet::DeviceStatusArray& array)
		{
			print_devicestatusarray_errors(array);
			return 2;
		}
	
		std::cerr << "   Buffer packet inquiry successful." << std::endl;

    DWORD NumSamplesADC = 0;
    //if(CodaSysConfig.bGS16AIOenabled)
	//	NumSamplesADC = CodaSysConfig.cl.getAcqBufferNumPackets(DEVICEID_GS16AIO);

    // create datafile name:
    char datafile[200] = "";
	
	std::string savfile;
	savfile.assign(TRIALFILE);
	savfile.replace(savfile.rfind("."),4,"_datarescue");

	time_t current_time = time(0);
	tm* ltm = localtime(&current_time); //use the same date and time as the DataWriter-written event file
	std::stringstream ss1, ss2, ss3;
	
	ss1 << std::setw(4) << std::setfill('0') << ltm->tm_year + 1900;
	ss1 << std::setw(2) << std::setfill('0') << ltm->tm_mon + 1;
	ss1 << std::setw(2) << std::setfill('0') << ltm->tm_mday;
	
	ss2 << std::setw(2) << std::setfill('0') << ltm->tm_hour;
	ss2 << std::setw(2) << std::setfill('0') << ltm->tm_min;
	ss2 << std::setw(2) << std::setfill('0') << ltm->tm_sec;

	ss3 << savfile.c_str() << "_" << ss1.str() << ss2.str() << ".coda";

    // open output datafile:
    FILE* fp = fopen(ss3.str().c_str(), "wt");
	std::cerr << "Opened data file " << ss3.str().c_str() << " to write out... " << std::endl;
	if(fp)
	{
		fprintf(fp, "Codamotion Marker data\n");
		fprintf(fp, "NumMarkers:\t%d\n", CodaSysConfig.MaxMarkerInUse);
		fprintf(fp, "NumSamples:\t%d\n", NumSamplesCX1);
		fprintf(fp, "AcqMode_Hz:\t%u\n", CodaSysConfig.cx1mode);
		fprintf(fp, "AcqTime_s:\t%0.3f\n", CodaSysConfig.AcqTimeMax);
		if(CodaSysConfig.bExtSync)
		  fprintf(fp, "External_Sync:\t1\n");
		else
		  fprintf(fp, "AcqRate_Hz:\t%0.3f\n", CodaSysConfig.AcqRate);
		fprintf(fp, "NumSamplesADC:\t%d\n", NumSamplesADC);
		fprintf(fp, "--\n");  //print END OF HEADER indicator

		// column headers:
		fprintf(fp, "Sample\tTime");
		for(int m = 0; m < CodaSysConfig.MaxMarkerInUse; m++)
		  fprintf(fp, "\tM%dValid\tM%dX\tM%dY\tM%dZ", m+1, m+1, m+1, m+1);  //Marker ID is 1-based in datafile
		fprintf(fp, "\n");
		fprintf(fp, "-----\n");  //print START OF DATA FILE indicator
	}
    DWORD iStop = 0;

    // time the cx1 data download:
    CodaSysConfig.timeStart = clock();

    //-- request all samples:
    //-- CX1 packets have DataSize=452 and trasmitsize=468 bytes if not acquiring multi-coda data
    // add 1 for safety, if UDP buffer is N * n

	//DWORD NumSamplesPerRequest = CodaSysConfig.UDPbufferSize / (469);  //!!Need to change this if multi-coda data is acquired: packets will be larger
    DWORD sample = 0;

	DWORD NumSamplesPerRequest = 0;
	int Nsampiter = 0;

    while(sample < NumSamplesCX1)
    {
		if(NumSamplesPerRequest <= 0)
		{
			NumSamplesPerRequest = 1;
			Nsampiter++;
		}
		else if (Nsampiter < 10)  //have a warmup period to gain confidence in estimating the packet size
		{
			NumSamplesPerRequest = 1;
			Nsampiter++;
		}
		else
			NumSamplesPerRequest = CodaSysConfig.UDPbufferSize / (CodaSysConfig.PacketTransmitSize + 20); //we will insert a large safety margin here to make sure we don't lose anything if the packets happen to vary in size!


      //-- request a bunch of samples:
	  // clrequestAcqBufferPacketAll(WORD device, DWORD startSample, DWORD endsample) 
      DWORD lastsample = sample + NumSamplesPerRequest;
      if(lastsample >= NumSamplesCX1)
        lastsample = NumSamplesCX1 - 1;
      CodaSysConfig.cl.requestAcqBufferPacketAll(DEVICEID_CX1, sample, lastsample);
//TEMP:
      fprintf(fp, "request(%d,%d)\n", sample, lastsample);  

      //-- download the bunch of samples: 
      while(sample <= lastsample)
      {
		  codaRTNet::RTNetworkPacket packet;
      DWORD receiveTimeout = 1000; //us
	    if(CodaSysConfig.stream.receivePacket(packet, receiveTimeout) == CODANET_STREAMTIMEOUT)
         fprintf(fp, "\ntimeout:%u ", receiveTimeout);

        else
        {
			// update packetsize:
			DWORD psize = packet.getDataSize();
			DWORD tsize = packet.transmitSize();
  			if (CodaSysConfig.PacketSize < psize)
				CodaSysConfig.PacketSize = psize;
			if (CodaSysConfig.PacketTransmitSize < tsize)
				CodaSysConfig.PacketTransmitSize = tsize;
//          fprintf(fp, "packet.DataSize: %u  packet.transmitSize: %u \n", psize, tsize);

			    // check result
			    if(packet.verifyCheckSum())
			    {
				    // decode objects
				    codaRTNet::PacketDecode3DResultExt decode3D;	// 3D measurements (CX1)
				    // decode & print/store results
				    if(decode3D.decode(packet))
				    {
						// get the sample number:
						CodaSysConfig.SampleNum = decode3D.getTick();
						//CodaSysConfig.SampleTime = CodaSysConfig.SampleNum * CodaSysConfig.cx1TickTime;
						CodaSysConfig.SampleTime = float(CodaSysConfig.SampleNum) * CodaSysConfig.cx1TickTime;

						// insert a blank line if external sync paused at this sampleNum:
						if(CodaSysConfig.bExtSync && (iStop < 1000) && (CodaSysConfig.SyncStopSampleNum[iStop] == CodaSysConfig.SampleNum))
						{
							fprintf(fp, "\n");
							iStop++;
						}
              
						// write data to file:
						fprintf(fp, "%u\t%0.5f", (CodaSysConfig.SampleNum + 1), CodaSysConfig.SampleTime);

						// get number of marker data in packet:
					    DWORD NumMarkers = decode3D.getNumMarkers();
						DWORD MaxMarker = (NumMarkers < CodaSysConfig.MaxMarkerInUse)? NumMarkers : CodaSysConfig.MaxMarkerInUse;

						BYTE valid[56];  //max possible Markers is 56

					    for(DWORD marker = 0; marker < MaxMarker; marker++)
					    {
						    float* pos = decode3D.getPosition(marker);
							valid[marker] = (decode3D.getValid(marker))? 1:0;
						    BYTE* intensity = decode3D.getIntensity(marker);
							// write data to file:
							fprintf(fp, "\t%u\t%0.5f\t%0.5f\t%0.5f", valid[marker], pos[0]/1000.0f+CALxOFFSET, pos[1]/1000.0f+CALyOFFSET, pos[2]/1000.0f);
					    }
						// datfile newline:
						fprintf(fp, "\n");
				    }
			    }
			    else
			    {
					fprintf(stderr, "\r%d: checksum failed.", sample);
			    }
			}
//#endif
			//-- increment sample index:
			sample++;
		}
    }
    CodaSysConfig.timeStop = clock();

	std::cerr << std::endl << "Coda Max Transmit Packet Size: " << CodaSysConfig.PacketTransmitSize << std::endl;

    float ElapsedTime = (float)(CodaSysConfig.timeStop - CodaSysConfig.timeStart)/CLOCKS_PER_SEC;
	fprintf(stderr, "Total CODA File Download Time: %0.3fs (%d of %d samples acquired)\n", ElapsedTime, sample,NumSamplesCX1);  
    //fprintf(stderr, "Total CODA Rescue File Download: %0.1f samples\n", (float)NumSamplesCX1);  

    // close datafile:
    fclose(fp);

	std::cout << "Rescue complete." << std::endl << "Shutting down CODA system." << std::endl;
	
	// shutdown system before exiting:
	CodaSysConfig.cl.stopSystem();
	std::cerr << "Completed. CODA system shut down!" << std::endl;


	freopen( "CON", "w", stderr );

	return 0;
}





void print_devicestatusarray_errors(const codaRTNet::DeviceStatusArray& array)
{
	for (DWORD idev = 0; idev < array.numstatus; idev++)
	{
		if (array.status[idev].error)
		{
			fprintf(stderr, "DEVICE %u SUBSYSTEM %u ERROR: %u\n", 
				(DWORD)array.status[idev].deviceID, 
				(DWORD)array.status[idev].subsystemID,
				(DWORD)array.status[idev].error);
		}
	}
}

DWORD getfirstdeverror(const codaRTNet::DeviceStatusArray& arr)
{
  DWORD error = 0;
	for (DWORD idev = 0; idev < arr.numstatus; idev++)
	{
		if(arr.status[idev].error)
		  error = (DWORD)arr.status[idev].error;
	}
  return error;
}
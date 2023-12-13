
//- C++
#include <iostream>
#include <string>
#include <time.h>
#include <sys/time.h>


//- LIMA
#include <lima/HwInterface.h>
#include <lima/CtControl.h>
#include <lima/CtAcquisition.h>
#include <lima/CtVideo.h>
#include <lima/CtImage.h>
#include <lima/CtSaving.h>
#include <EigerInterface.h>
#include <EigerCamera.h>

//--------------------------------------------------------------------------------------
//waiting the status of acquisition
//--------------------------------------------------------------------------------------
void wait_while_status_is_standby(lima::CtControl& ct, lima::Eiger::Interface& hw)
{
	lima::CtControl::Status status;
	lima::HwInterface::StatusType state;

	bool is_already_displayed = false;
	while (1)
	{

		// let's take a look at the status of control & the status of the plugin
		lima::CtControl::Status ctStatus;
		ct.getStatus(ctStatus);

		switch (ctStatus.AcquisitionStatus)
		{
			case lima::AcqReady:
			{
				if (!is_already_displayed)
				{
					std::cout << "Waiting for Request ..." << std::endl;
					is_already_displayed = true;
				}
			}
				break;

			case lima::AcqRunning:
			{
				std::cout << "Acquisition is Running ..." << std::endl;
				return;
			}
				break;

			case lima::AcqConfig:
			{
				std::cout << "--> Detector is Calibrating...\n" << std::endl;
				return;
			}
				break;

			default:
			{
				std::cout << "--> Acquisition is in Fault\n" << std::endl;
				return;
			}
				break;
		}
	}
}
//--------------------------------------------------------------------------------------
//waiting the status of acquisition
//--------------------------------------------------------------------------------------
void wait_while_status_is_running(lima::CtControl& ct, lima::Eiger::Interface& hw)
{
	lima::CtControl::Status status;
	lima::HwInterface::StatusType state;

	bool is_already_displayed = false;
	while (1)
	{

		// let's take a look at the status of control & the status of the plugin
		lima::CtControl::Status ctStatus;
		ct.getStatus(ctStatus);

		switch (ctStatus.AcquisitionStatus)
		{
			case lima::AcqReady:
			{
				std::cout << "Waiting for Request ...\n" << std::endl;
				return;
			}
				break;

			case lima::AcqRunning:
			{
				if (!is_already_displayed)
				{
					std::cout << "Acquisition is Running ..." << std::endl;
					is_already_displayed = true;
				}
			}
				break;

			case lima::AcqConfig:
			{
				std::cout << "--> Detector is Calibrating...\n" << std::endl;
				return;
			}
				break;

			default:
			{
				std::cout << "--> Acquisition is in Fault\n" << std::endl;
				return;
			}
				break;
		}
	}
}

//--------------------------------------------------------------------------------------
// start a snap
//--------------------------------------------------------------------------------------
void do_snap(lima::CtControl& ct, lima::Eiger::Interface& hw)
{
	//- prepare acqusition
	std::cout << "prepareAcq" << std::endl;
	ct.prepareAcq();

	//- start acqusition
	std::cout << "startAcq" << std::endl;
	ct.startAcq();

	//- waiting while state is standby ...	
	wait_while_status_is_standby(ct, hw);

	clock_t start = clock();
	//- waiting while state is running ...
	wait_while_status_is_running(ct, hw);

	clock_t end = clock();
	double millis = (end - start) / 1000;
	std::cout << "Elapsed time  = " << millis << " (ms)" << std::endl;
	std::cout << "============================================\n" << std::endl;
}

//--------------------------------------------------------------------------------------
//test main:
//- 1st argument is the IP adress of camera
//- 2nd argument is the exposure_time (in milli seconds)
//- 3rd argument is the latency_time (in milli seconds)
//- 4rd argument is the latency_time (in milli seconds)
//- 5th argument is the nb. of frames to acquire
//- 6th argument is the nb. of snap to do
//- 7th argument is the Bpp
//- 8th argument is the packet size of the socket transport layer
//- 9th argument is the interpacket delay of the socket transport layer
//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	std::cout << "============================================" << std::endl;
	std::cout << "Usage :./ds_TestLimaEiger ip exp lat expacc nbframes nbsnap bpp packet interpacket" << std::endl << std::endl;

	try
	{
		std::string ip_adress			= "172.16.3.137";	//default value
		double exposure_time			= 500;				//default value is 1000 ms
		double latency_time				= 0.0;				//default value is 0 ms
		double acc_max_exposure_time	= 100;				//default value is 100 ms
		int nb_frames					= 10;				//default value is 10
		int nb_snap						= 1;                //default value
		unsigned bpp					= 8;
		int packet_size					= 1500;             //default value
		int interpacket_delay			= 0;				//default value  

        struct timeval start_time;
	    struct timeval end_time;     

		//read args of main 
		switch (argc)
		{
			case 2:
			{
				ip_adress = argv[1];
			}
				break;

			case 3:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;
			}
				break;

			case 4:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;
			}
				break;

			case 5:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;

				std::istringstream argAccExposure(argv[4]);
				argAccExposure >> acc_max_exposure_time;
			}
				break;

			case 6:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;

				std::istringstream argAccExposure(argv[4]);
				argAccExposure >> acc_max_exposure_time;

				std::istringstream argNbSnap(argv[5]);
				argNbSnap >> nb_snap;
			}
				break;

			case 7:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;

				std::istringstream argAccExposure(argv[4]);
				argAccExposure >> acc_max_exposure_time;

				std::istringstream argNbFrames(argv[5]);
				argNbFrames >> nb_frames;

				std::istringstream argNbSnap(argv[6]);
				argNbSnap >> nb_snap;

			}
				break;

			case 8:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;

				std::istringstream argAccExposure(argv[4]);
				argAccExposure >> acc_max_exposure_time;

				std::istringstream argNbFrames(argv[5]);
				argNbFrames >> nb_frames;

				std::istringstream argNbSnap(argv[6]);
				argNbSnap >> nb_snap;

				std::istringstream argBpp(argv[7]);
				argBpp >> bpp;

			}
				break;

			case 9:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;

				std::istringstream argAccExposure(argv[4]);
				argAccExposure >> acc_max_exposure_time;

				std::istringstream argNbFrames(argv[5]);
				argNbFrames >> nb_frames;

				std::istringstream argNbSnap(argv[6]);
				argNbSnap >> nb_snap;

				std::istringstream argBpp(argv[7]);
				argBpp >> bpp;

				std::istringstream argPacketSize(argv[8]);
				argPacketSize >> packet_size;
			}
				break;

			case 10:
			{
				ip_adress = argv[1];

				std::istringstream argExposure(argv[2]);
				argExposure >> exposure_time;

				std::istringstream argLatency(argv[3]);
				argLatency >> latency_time;

				std::istringstream argAccExposure(argv[4]);
				argAccExposure >> acc_max_exposure_time;

				std::istringstream argNbFrames(argv[5]);
				argNbFrames >> nb_frames;

				std::istringstream argNbSnap(argv[6]);
				argNbSnap >> nb_snap;

				std::istringstream argBpp(argv[7]);
				argBpp >> bpp;

				std::istringstream argPacketSize(argv[8]);
				argPacketSize >> packet_size;

				std::istringstream argInterPacketDelay(argv[9]);
				argInterPacketDelay >> interpacket_delay;

			}
				break;
		}

        //- test gettimeofday
        std::cout << "test gettimeofday" << std::endl;
        gettimeofday(&start_time, NULL);
		//        
		std::cout << "============================================" << std::endl;
		std::cout << "ip_adress              = " << ip_adress << std::endl;
		std::cout << "exposure_time          = " << exposure_time << std::endl;
		std::cout << "latency_time           = " << latency_time << std::endl;
		std::cout << "acc_max_exposure_time  = " << acc_max_exposure_time << std::endl;
		std::cout << "nb_frames              = " << nb_frames << std::endl;
		std::cout << "nb_snap                = " << nb_snap << std::endl;
		std::cout << "bpp                    = " << bpp << std::endl;
		std::cout << "packet_size            = " << packet_size << std::endl;
		std::cout << "interpacket_delay      = " << interpacket_delay << std::endl;
		std::cout << "============================================" << std::endl;

        gettimeofday(&end_time, NULL);
        std::cout   << "test gettimeofday : Elapsed time : " 
			        << 1e3 * (end_time.tv_sec - start_time.tv_sec) + 1e-3 * (end_time.tv_usec - start_time.tv_usec) << " ms" << std::endl;


		//initialize Eiger::Camera objects & Lima Objects
		std::cout << "Create Camera Object" << std::endl;
        gettimeofday(&start_time, NULL);
		lima::Eiger::Camera myCamera(ip_adress);
        gettimeofday(&end_time, NULL);
        std::cout   << "lima::Eiger::Camera : Elapsed time : " 
			        << 1e3 * (end_time.tv_sec - start_time.tv_sec) + 1e-3 * (end_time.tv_usec - start_time.tv_usec) << " ms" << std::endl;

	    std::cout << "============================================" << std::endl;

		std::cout << "Create Interface Object" << std::endl;
		lima::Eiger::Interface myInterface(myCamera);

		std::cout << "Create CtControl Object" << std::endl;
		lima::CtControl myControl(&myInterface);

		std::cout << "============================================" << std::endl;
		lima::HwDetInfoCtrlObj *hw_det_info;
		myInterface.getHwCtrlObj(hw_det_info);

		switch (bpp)
		{
			case 8:
				//- Set imageType = Bpp8
				std::cout << "Set imageType \t= " << bpp << std::endl;
				hw_det_info->setCurrImageType(lima::Bpp8);
				break;

			case 12:
				//- Set imageType = Bpp12
				std::cout << "Set imageType \t= " << bpp << std::endl;
				hw_det_info->setCurrImageType(lima::Bpp12);
				break;

			case 16:
				//- Set imageType = Bpp16
				std::cout << "Set imageType \t= " << bpp << std::endl;
				hw_det_info->setCurrImageType(lima::Bpp16);
				break;
			case 32:
				//- Set imageType = Bpp32
				std::cout << "Set imageType \t= " << bpp << std::endl;
				hw_det_info->setCurrImageType(lima::Bpp32);
				break;

			default:
				//- Error
				std::cout << "Don't Set imageType !" << std::endl;
				break;				
		}

		//- Set Roi = (0,0,MaxWidth,MaxHeight)
		lima::Size size;
		hw_det_info->getMaxImageSize(size);
		lima::Roi myRoi(0, 0, size.getWidth(), size.getHeight());
		std::cout << "Set Roi \t= (" << 0 << "," << 0 << "," << size.getWidth() << "," << size.getHeight() << ")" << std::endl;
		myControl.image()->setRoi(myRoi);

		//- Set Bin = (1,1)
		std::cout << "Set Bin \t= (1,1)" << std::endl;
		lima::Bin myBin(1, 1);
		myControl.image()->setBin(myBin);

		//- Set exposure
		std::cout << "Set exposure \t= " << exposure_time << " (ms)" << std::endl;
		myControl.acquisition()->setAcqExpoTime(exposure_time / 1000.0); //convert exposure_time to sec
		myControl.video()->setExposure(exposure_time / 1000.0); //convert exposure_time to sec

		//- Set latency
		std::cout << "Set latency \t= " << latency_time << " (ms)" << std::endl;
		myControl.acquisition()->setLatencyTime(latency_time / 1000.0);  //convert latency_time to sec

		std::cout << "Set Acc exposure= " << acc_max_exposure_time << " (ms)" << std::endl;
		myControl.acquisition()->setAccMaxExpoTime(acc_max_exposure_time / 1000.0); //convert exposure_time to sec
		
		//- Set nbFrames = 1
		std::cout << "Set nbFrames \t= " << nb_frames << std::endl;
		myControl.acquisition()->setAcqNbFrames(nb_frames);

		std::cout << "\n" << std::endl;

        //==========================================================================================================================
        //- Device : Eiger::read_nbTriggers
        /*std::cout << "Device : Eiger::read_nbTriggers ..." << std::endl;
        int nb_triggers = 0;
        gettimeofday(&start_time, NULL);
        lima::TrigMode trig_mode;
        myCamera.getTrigMode(trig_mode); // unusefull but written like this in the device
        myCamera.getNbTriggers(nb_triggers);
        gettimeofday(&end_time, NULL);
        std::cout   << "Device : Eiger::read_nbTriggers : Elapsed time : " 
			        << 1e3 * (end_time.tv_sec - start_time.tv_sec) + 1e-3 * (end_time.tv_usec - start_time.tv_usec) << " ms" << std::endl;
*/
        //==========================================================================================================================
        //- Device : Eiger::read_managedMode
        std::cout << "Device : Eiger::read_managedMode ..." << std::endl;
        gettimeofday(&start_time, NULL);
        lima::CtSaving::ManagedMode mode;
        myControl.saving()->getManagedMode(mode) ;
        gettimeofday(&end_time, NULL);
        std::cout   << "Device : Eiger::read_managedMode : Elapsed time : " 
			        << 1e3 * (end_time.tv_sec - start_time.tv_sec) + 1e-3 * (end_time.tv_usec - start_time.tv_usec) << " ms" << std::endl;

		//start nb_snap acquisition of 1 frame
		for (unsigned i = 0; i < nb_snap; i++)
		{
			//- Tests of AcqMode:
			std::cout << "============================================" << std::endl;
			std::cout << "-	 	AcqMode Tests		  -" << std::endl;
			std::cout << "============================================" << std::endl;
			lima::AcqMode an_acq_mode;
			myControl.acquisition()->getAcqMode(an_acq_mode);
			std::cout << "Setting AcqMode to SINGLE" << std::endl;
			myControl.acquisition()->setAcqMode(lima::Single);
			//			std::cout << "Resetting Status(False)..." << std::endl;
			//			myControl.resetStatus(false);

			std::cout << "Snapping ..." << std::endl;
			do_snap(myControl, myInterface);

			std::cout << "Setting AcqMode to ACCUMULATION" << std::endl;
			myControl.acquisition()->setAcqMode(lima::Accumulation);
			//			std::cout << "Resetting Status(False)" << std::endl;
			//			myControl.resetStatus(false);
			std::cout << "Snapping ..." << std::endl;
			do_snap(myControl, myInterface);
		}
	}
	catch (lima::Exception e)
	{
		std::cerr << "LIMA Exception : " << e << std::endl;
	}

	return 0;
}
//--------------------------------------------------------------------------------------

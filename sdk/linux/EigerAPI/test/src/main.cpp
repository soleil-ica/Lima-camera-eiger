//============================================================================
// Name        : main.cpp
// Author      : WB
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include "EigerDefines.h"
#include <EigerAdapter.h>
#include <ResourceFactory.h>
#include <Resource.h>
#include <FileImage_Nxs.h>
#include <RESTfulClient.h>

#include <string>

void SaveTGA(std::string filename, void* data, int w, int h, int d);

int main(int, char *[])
{

	std::cout << "[BEGIN]" << std::endl;

	std::string ip      = "172.19.27.26";
	std::string version = "0.8.1";

	//============================================================================  
	// try
	// {
	// 	eigerapi::EigerAdapter eiger_wrapper(ip);

	// 	std::cout << "========================================" << std::endl;
	// 	eiger_wrapper.downloadAcquiredFile("here");
	// 	std::cout << "========================================" << std::endl;

	// 	std::cout << "========================================" << std::endl;
	// 	eiger_wrapper.arm();
	// 	std::cout << "========================================" << std::endl;
	// 	std::cout << eiger_wrapper.getTriggerMode() << std::endl;
	// 	std::cout << "========================================" << std::endl;
	// 	eiger_wrapper.setTriggerMode(eigerapi::ETRIGMODE_EXPO);		
	// }
	// catch ( eigerapi::EigerException &e )
	// {
	// 	std::cout << e.what();
	// }


/*
	std::cout << std::endl << "= ResourceFactory test ========================================" << std::endl;
	// Check all resources url
	eigerapi::ResourceFactory* pResFactory = new eigerapi::ResourceFactory(ip, version);
	std::list<std::string> resources;
	pResFactory->getResourceList(resources);


	std::list<std::string>::const_iterator iter = resources.begin();
	while (resources.end() != iter)
	{
		eigerapi::Resource* res = pResFactory->getResource(*iter);
		//std::cout << *iter << '\t' << res->getURL() << std::endl;
		delete res;
		++iter;
	}

	delete pResFactory;
*/	
	

	// std::cout << std::endl << "= RESTfulClient::get_file() test ========================================" << std::endl;	
	// eigerapi::RESTfulClient client;

	//client.get_file("http://172.19.27.26/")


	std::cout << std::endl << "= CFileImage_Nxs test ========================================" << std::endl;
	eigerapi::CFileImage_Nxs H5File;

	//std::string name = "/home/guest/SOLEIL/HDF5/series_269_data_000000.h5";
	std::string name = "/home/guest/SOLEIL/HDF5/test_lz4.h5";
	//std::string name = "/home/guest/SOLEIL/HDF5/temp.h5";
	
	std::cout << "open: " << name << std::endl;
	long nimages = H5File.openFile(name);

	std::cout << "openfile returned nbImages: " << nimages << std::endl;

	if (-1 != nimages)
	{
		int w, h, d;
		H5File.GetSize(w, h, d);
		std::cout << "width:    " << w << std::endl;
		std::cout << "heigth:   " << h << std::endl;
		std::cout << "depth:    " << d << std::endl;

		std::cout << std::endl << "Extracting images ... " << std::endl;
		void* src;
		long long int ext = 0;
		while (NULL != (src = H5File.getNextImage()))
		{
			std::cout << std::to_string(ext) << "\t";
			std::cout << std::flush;

			if (ext < 10) // only write out the 10 first images
			{
				std::string fname = "/home/guest/SOLEIL/HDF5/eiger.";
				fname = fname + std::to_string(ext) + std::string(".tga");

				SaveTGA(fname, src, w, h, d);
			}

			ext++;
		}
		std::cout << std::endl << "Extracting images end " << std::endl;

		H5File.closeFile();
	}
	else
	{
		std::cout << "openfile: failed" << std::endl;
	}

	std::cout << "[END]" << std::endl;

	return 0;
}

void SaveTGA(std::string fileName, void* data, int w, int h, int d)
{
	FILE* file;
	file = fopen(fileName.c_str(), "wb");

	// output TGA header
	putc(0, file);
	putc(0, file);
	putc(2, file);                         /* uncompressed RGB */
	putc(0, file);
	putc(0, file);
	putc(0, file);
	putc(0, file);
	putc(0, file);
	putc(0, file);
	putc(0, file);           /* X origin */
	putc(0, file);
	putc(0, file);           /* y origin */
	putc((w & 0x00FF), file);
	putc((w & 0xFF00) / 256, file);
	putc((h & 0x00FF), file);
	putc((h & 0xFF00) / 256, file);
	//putc(d * 8, file);     /* depth bits */
	putc(24, file);          /* depth bits */
	putc(0, file);
	

	int max=0;
	for (int i=0; i < w*h; i++)
	{
		unsigned short int value_16bit = (((unsigned short int*)data)[i]);
		if (value_16bit>max) max=value_16bit;
	}

	// output data
	//fwrite(data, w*h, d, file);
	for (int i=0; i < w*h; i++)
	{
		char value_8bit = (255 * (((unsigned short int*)data)[i])) / max;
		putc(value_8bit, file);
		putc(value_8bit, file);
		putc(value_8bit, file);
	}

	fclose(file);
}

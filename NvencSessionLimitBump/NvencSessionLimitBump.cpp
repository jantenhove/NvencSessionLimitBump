
/*
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

Other licenses may apply (see other header files)
*/

#include <iostream>
#include <iostream>
#include <cstdlib>
#include "cuda.h"
#include <vector>
#include "NvEncoderCuda.h"
#include "NvCodecUtils.h"
#include "argagg/argagg.hpp"

int EnableEncodeSessions(int maxSessions, int gpuId)
{
	//When using D3D and a driver patch from: https://github.com/keylase/nvidia-patch/tree/master/win/win10_x64  the max sessions
	//stay at 2. When using CUDA before D3D encoding, we can enable the other sessions (also for D3D11), so we need to create
	//an x number of simultaneous sessions on CUDA.
	int numSessions = 0;

	CUdevice cuDevice = 0;
	ck(cuDeviceGet(&cuDevice, gpuId));
	CUcontext cuContext = NULL;
	ck(cuCtxCreate(&cuContext, 0, cuDevice));
	std::vector<NvEncoderCuda*> encs;
	//create maxSessions encoders simultaneously. Store the references for cleanup later on
	for (auto cnt = 0; cnt < maxSessions; cnt++)
	{
		try
		{
			auto* enc = new NvEncoderCuda(cuContext, 1920, 1080, NV_ENC_BUFFER_FORMAT_YV12_PL);
			NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
			NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
			initializeParams.encodeConfig = &encodeConfig;
			enc->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_DEFAULT_GUID);
			enc->CreateEncoder(&initializeParams);
			encs.push_back(enc);

			numSessions++;
		}
		catch (NVENCException& e)
		{
			break; //max sessions limit reached
		}

	}

	//and release all
	for (auto enc : encs)
	{
		std::vector<NvEncOutputFrame> packets;
		enc->EndEncode(packets);
		enc->DestroyEncoder();
		delete enc;
	}
	ck(cuCtxDestroy(cuContext));

	return  numSessions;

}


int main(int argc, char **argv)
{
	using argagg::parser_results;
	using argagg::parser;
	using std::cerr;
	using std::cout;
	using std::endl;
	using std::ofstream;
	using std::ostream;
	using std::ostringstream;
	using std::string;

	parser argparser{ {
	 {
	   "help", {"-h", "--help"},
	   "Print help and exit", 0},
	 {
	   "gpu", {"-g", "--gpu"},
	   "The GPU id to bump the limit on (default: all)", 1},
	 {
	   "sessions", {"-s", "--sessions"},
	   "The number of encoding sessions to unlock (default: 32)", 1},
   } };


	// Define our usage text.
	ostringstream usage;
	usage
		<< argv[0] << endl
		<< endl
		<< "Usage: " << argv[0] << " [-g] [-s]. See -h for more info." << endl
		<< endl;


	// Use our argument parser to... parse the command line arguments. If there
	// are any problems then just spit out the usage and help text and exit.
	argagg::parser_results args;
	try {
		args = argparser.parse(argc, argv);
	}
	catch (const std::exception& e) {
		argagg::fmt_ostream fmt(cerr);
		fmt << usage.str() << argparser << endl
			<< "Encountered exception while parsing arguments: " << e.what()
			<< endl;
		return EXIT_FAILURE;
	}

	// If the help flag was specified then spit out the usage and help text and
	// exit.
	if (args["help"]) {
		argagg::fmt_ostream fmt(cerr);
		fmt << usage.str() << argparser;
		return EXIT_SUCCESS;
	}

	//get device count to iterate through all CUDA devices
	ck(cuInit(0));
	int deviceCount = 0;
	ck(cuDeviceGetCount(&deviceCount));

	//get the params with a default option
	int gpu = args["gpu"].as<int>(-1);
	int maxSessions = args["sessions"].as<int>(32);;

	cout << "NVENC Session limit bump for Direct3D." << endl;
	cout << "You need to have the patch from https://github.com/keylase/nvidia-patch applied on your system before running this program." << endl << endl;

	//loop through all CUDA devices an check if we need to enable the encoding sessions
	for (int cnt = 0; cnt < deviceCount; cnt++)
	{
		if (gpu < 0 || gpu == cnt)
		{
			cout << "Trying to increase the session limit on GPU: " << cnt << " to " << maxSessions << "..." << endl;
			int limitResult = EnableEncodeSessions(maxSessions, cnt);
			cout << "Session limit on GPU: " << cnt << " is: " << limitResult <<
				". Bump " << (limitResult == maxSessions ? "succeeded." : "did not succeed.") << endl << endl;
		}
	}
	return EXIT_SUCCESS;
}

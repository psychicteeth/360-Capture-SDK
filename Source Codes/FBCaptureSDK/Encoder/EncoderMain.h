#pragma once

#include "LibRTMP.h"
#include "AudioCapture.h"
#include "NVEncoder.h"
#include "AMDEncoder.h"
#include "FLVmuxer.h"
#include "MP4muxer.h"
#include "Log.h"
#include "Common.h"
#include <sstream>
#include <shlobj.h>
#include <KnownFolders.h>
#include <ctime>
#include <stdio.h>

using namespace FBCapture::Video;
using namespace FBCapture::Audio;
using namespace FBCapture::Mux;

namespace FBCapture {
	namespace Common {

		class EncoderMain {

		public:
			EncoderMain();
			~EncoderMain();

		private:
			// Class instances for capture and encode
			NVEncoder*  nvEncoder = nullptr;
			AMDEncoder* amdEncoder = nullptr;
			FLVMuxer * flvMuxer = nullptr;
			MP4Muxer * mp4Muxer = nullptr;
			AudioCapture* audioCapture = nullptr;
			AudioEncoder* audioEncoder = nullptr;
			LibRTMP* rtmp = nullptr;

			ID3D11Device* device_ = nullptr;

			bool isLive = false;  // Check if it's Live or VOD mode
			bool silenceMode = false;
			atomic<bool> updateInputName = true;  // Set true when need to update Unix time for unique live video and audio file names
			atomic<bool> stopEncProcess = false;  // Stop encoding process when any critical encoding process is failed
			atomic<bool> isMuxed = false; // Set to be true after muxing
			atomic<bool> stopEncodingSession = false; // Set to be true after muxing

																					// Strings for folder
			wstring liveFolder = {};
			wstring videoH264 = {};
			wstring prevVideoH264 = {};
			string audioWAV = {};
			string prevAudioWAV = {};
			string audioAAC = {};
			string prevAudioAAC = {};

			const string wavExtension = ".wav";
			const string aacExtension = ".aac";
			const wstring h264Extension = L".h264";
			const wstring flvExtension = L".flv";
			const wstring mp4Extension = L".mp4";

			time_t unixTime = 0;
			bool amdDevice = false;  // When it's amd
			bool nvidiaDevice = false;  // When it's nvidia
			bool initiatedUnixTime = false;
			bool needEncodingSessionInit = false;
			bool movingToMuxingStage = false;

		public:
			FBCAPTURE_STATUS initEncoderComponents();
			FBCAPTURE_STATUS checkGraphicsCardCapability();
			FBCAPTURE_STATUS initSessionANDdriverCapabilityCheck();
			vector<wstring> splitString(wstring& str);
			FBCAPTURE_STATUS startEncoding(const void* texturePtr, const TCHAR* fullSavePath, bool isLive, int bitrate, int fps, bool needFlipping);
			FBCAPTURE_STATUS audioEncoding(bool useVRAudioEndpoint, bool enabledAudioCapture, bool enabledMicCapture, VRDeviceType vrDevice, LPCWSTR useMicIMMDeviceId);
			FBCAPTURE_STATUS stopEncoding(bool forceStop);
			FBCAPTURE_STATUS muxingData(PROJECTIONTYPE projectionType, STEREO_MODE stereMode, bool is360, int fps);
			FBCAPTURE_STATUS saveScreenShot(const void* texturePtr, const TCHAR* fullSavePath, bool is360);
			FBCAPTURE_STATUS startLiveStream(const TCHAR* streamUrl);
			GRAPHICS_CARD checkGPUManufacturer();
			void stopLiveStream();
			void resetResources();
			void setGraphicsDeviceD3D11(ID3D11Device* device);
			wstring generateLiveVideoFileWString();
		};
	}
}
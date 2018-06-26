﻿/****************************************************************************************************************

Filename	:	EncoderMain.cpp
Content		:	Encoder implementation for creating h264 format video with DX11 and Unity RenderTexture
Created		:	December 13, 2016
Authors		:	Homin Lee

Copyright	:

****************************************************************************************************************/
#include "EncoderMain.h"

using namespace FBCapture::Video;
using namespace FBCapture::Audio;
using namespace FBCapture::Mux;
using namespace FBCapture::Common;

namespace FBCapture {
  namespace Common {


    // Capture SDK Version Update String
    const string sdkVersion = "2.0";

    EncoderMain::EncoderMain()
    {
      DEBUG_LOG_VAR("Capture SDK Version: ", sdkVersion);
    }

    EncoderMain::~EncoderMain() {
      if (flvMuxer) {
        delete flvMuxer;
        flvMuxer = nullptr;
      }

      if (mp4Muxer) {
        delete mp4Muxer;
        mp4Muxer = nullptr;
      }

      if (audioEncoder) {
        delete audioEncoder;
        audioEncoder = nullptr;
      }

      if (audioCapture) {
        delete audioCapture;
        audioCapture = nullptr;
      }

      if (nvEncoder) {
        delete nvEncoder;
        nvEncoder = nullptr;
      }

      if (amdEncoder) {
        delete amdEncoder;
        amdEncoder = nullptr;
      }

      if (rtmp) {
        delete rtmp;
        rtmp = nullptr;
      }

      RELEASE_LOG();
    }

    vector<wstring> EncoderMain::splitString(wstring& str) {
      vector<wstring> stringArr;
      wstringstream stringStream(str); // Turn the string into a stream.
      wstring tok;

      while (getline(stringStream, tok, L'.')) {
        stringArr.push_back(tok);
      }

      return stringArr;
    }

    FBCAPTURE_STATUS EncoderMain::initEncoderComponents() {

      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

			if (this->nvidiaDevice && this->nvEncoder == nullptr) {
				this->nvEncoder = new NVEncoder(this->device_);
			}

			if (this->amdDevice && this->amdEncoder == nullptr) {
				this->amdEncoder = new AMDEncoder(this->device_);
			}

      if (this->flvMuxer == nullptr) {
				this->flvMuxer = new FLVMuxer();
      }

      if (this->mp4Muxer == nullptr) {
				this->mp4Muxer = new MP4Muxer();
      }

      if (this->audioCapture == nullptr) {
				this->audioCapture = new AudioCapture();
      }

      if (this->audioEncoder == nullptr) {
				this->audioEncoder = new AudioEncoder();
      }

      if (this->rtmp == nullptr) {
				this->rtmp = new LibRTMP();
      }

      // Using "%AppData%" folder for saving sliced live video files
      LPWSTR wszPath = nullptr;
      HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &wszPath);
      if (SUCCEEDED(hr)) {
				this->liveFolder = wszPath;
				this->liveFolder += L"\\FBEncoder\\";
        if (!CreateDirectory(this->liveFolder.c_str(), nullptr)) {
          if (ERROR_ALREADY_EXISTS == GetLastError()) {
            DEBUG_LOG("Output folder already existed");
          }
        }
      }
      else {
				this->liveFolder.clear();  // Just use root folder when it failed to create folder
      }
		
      return status;
    }

		FBCAPTURE_STATUS EncoderMain::checkGraphicsCardCapability()	{
			FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

			const int nvidiaVenderID = 4318;  // NVIDIA Vendor ID: 0x10DE
			const int amdVenderID1 = 4098;	// AMD Vendor ID: 0x1002
			const int amdVenderID2 = 4130;	// AMD Vendor ID: 0x1022

			IDXGIAdapter1 * adapter;
			std::vector <IDXGIAdapter1*> adapters;
			IDXGIFactory1* factory = nullptr;
			DXGI_ADAPTER_DESC1 adapterDescription;

			if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory))) {
				DEBUG_ERROR("Failed to create DXGI factory object");
				return FBCAPTURE_STATUS_DXGI_CREATING_FAILED;
			}

			for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
				adapter->GetDesc1(&adapterDescription);

				// Getting graphics card information in use
				wstring wDeviceName(adapterDescription.Description);
				string sDeviceName(wDeviceName.begin(), wDeviceName.end());
				DEBUG_LOG_VAR("Graphics Card Info: ", sDeviceName);

				if (adapterDescription.VendorId == nvidiaVenderID) {
					this->nvidiaDevice = true;
				}
				else if (adapterDescription.VendorId == amdVenderID1 || adapterDescription.VendorId == amdVenderID2) {
					this->amdDevice = true;
				}
			}

			if (factory) {
				factory->Release();
			}

			if (!this->nvidiaDevice && !this->amdDevice) {
				DEBUG_ERROR("Unsupported graphics card. The SDK supports only nVidia and AMD GPUs");
				return FBCAPTURE_STATUS_UNSUPPORTED_GRAPHICS_CARD;
			}

			return status;
		}

		GRAPHICS_CARD EncoderMain::checkGPUManufacturer() {
			if (this->amdDevice) {
				return GRAPHICS_CARD::AMD;
			} else if (this->nvidiaDevice) {
				return GRAPHICS_CARD::NVIDIA;
			} else {
				return GRAPHICS_CARD::UNSUPPORTED_DEVICE;
			}
		}


		FBCAPTURE_STATUS EncoderMain::initSessionANDdriverCapabilityCheck()	{
			FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

			if (this->nvidiaDevice && this->nvEncoder) {
				status = this->nvEncoder->initNVEncodingSession();
				if(status != FBCAPTURE_STATUS_OK)	{
					return status;
				}
			} else if (this->amdDevice && this->amdEncoder)	{
				status = this->amdEncoder->initAMDEncodingSession();
				if (status != FBCAPTURE_STATUS_OK) {
					return status;
				}
			}

			this->needEncodingSessionInit = false;

			return status;
		}

    wstring EncoderMain::generateLiveVideoFileWString() {
      wstring liveVideoFile = this->prevVideoH264;
      liveVideoFile.erase(this->prevVideoH264.length() - this->h264Extension.length(), this->h264Extension.length());
      liveVideoFile += this->flvExtension;
      return liveVideoFile;
    }

	int EncoderMain::getQueueLength()
	{
		if (this->nvidiaDevice && this->nvEncoder) {
			return this->nvEncoder->getQueueLength();
		}
		if (this->amdDevice && this->amdEncoder) {
			return 0;
		}
	}

	void EncoderMain::drainQueue()
	{
		if (this->nvidiaDevice && this->nvEncoder) {
			this->nvEncoder->checkQueue();
		}
	}

    FBCAPTURE_STATUS EncoderMain::startEncoding(const void* texturePtr, const TCHAR* fullSavePath, bool isLive, int bitrate, int fps, bool needFlipping) {
      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

      if (this->stopEncProcess)
        return status;

      this->isLive = isLive;

			if(needEncodingSessionInit)	{
				status = initSessionANDdriverCapabilityCheck();
				if(status != FBCAPTURE_STATUS_OK)	{
					return status;
				}
			}
			
			if (!this->initiatedUnixTime) {
				this->initiatedUnixTime = true;
				this->unixTime = std::time(nullptr);  // Using unix time for live video files' identity
			}

      if (!this->isLive && this->updateInputName) {  // VOD mode
        this->updateInputName = false;
        this->videoH264 = fullSavePath;
        if (this->videoH264.find(this->mp4Extension) != string::npos) {  // Convert file name to h264 when input name is mp4 format
          this->videoH264.replace(this->videoH264.find(this->mp4Extension), this->mp4Extension.length(), this->h264Extension);
        }
      }
      else if (this->isLive) {  // Live mode
        this->videoH264 = this->liveFolder + to_wstring(this->unixTime) + this->h264Extension;
      }

      // Encoding with render texture
      if (texturePtr && this->nvidiaDevice) {
        status = this->nvEncoder->encodeMain(texturePtr, this->videoH264, bitrate, fps, needFlipping);
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }
      else if (texturePtr && this->amdDevice) {
        status = this->amdEncoder->encodeMain(texturePtr, this->videoH264, bitrate, fps, needFlipping);
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }
      else {
        DEBUG_LOG("It's invalid texture pointer: null");
      }

      return status;
    }

    FBCAPTURE_STATUS EncoderMain::audioEncoding(bool useVRAudioEndpoint, bool enabledAudioCapture, bool enabledMicCapture, 
																								VRDeviceType vrDevice, LPCWSTR useMicIMMDeviceId) {
      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

      if (this->stopEncProcess || this->videoH264.empty()) {
        return status;
      }

      if (this->audioCapture->needToInitializeDevices) {
        status = this->audioCapture->initializeDevices(useVRAudioEndpoint, vrDevice, useMicIMMDeviceId);
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }

      if (this->audioCapture->needToOpenCaptureFile) {
        if (!this->isLive) {  // VOD mode
          wstring videoPath = this->videoH264;
          videoPath.erase(this->videoH264.length() - this->h264Extension.length(), this->h264Extension.length());

          wstring_convert<std::codecvt_utf8<wchar_t>> stringTypeConversion;
          string audioPath = stringTypeConversion.to_bytes(videoPath);

          this->audioWAV = audioPath + this->wavExtension;
          this->audioAAC = audioPath + this->aacExtension;
        } else {  // Live mode
          wstring_convert<std::codecvt_utf8<wchar_t>> stringTypeConversion;
          string folderPath = stringTypeConversion.to_bytes(this->liveFolder);
          this->audioWAV = folderPath + to_string(this->unixTime) + this->wavExtension;
          this->audioAAC = folderPath + to_string(this->unixTime) + this->aacExtension;
        }

        status = this->audioCapture->openCaptureFile(this->audioWAV);
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }

      if (!this->audioCapture->needToCloseCaptureFile) {
        this->audioCapture->continueAudioCapture(enabledAudioCapture, enabledMicCapture);
      } else {
        status = this->audioCapture->closeCaptureFile();
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }

      return status;
    }

    FBCAPTURE_STATUS EncoderMain::stopEncoding(bool forceStop) {
      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

      if (this->stopEncProcess)  // Only we can flush buffers after encoding starts
        return status;

      this->updateInputName = true;

      if (this->updateInputName) {  // Update unix time for next frame video and audio
        this->unixTime = std::time(nullptr);
        this->updateInputName = !this->updateInputName;
      }

      // Store previous file name to use in muxing
      this->prevAudioWAV = this->audioWAV;
      this->prevAudioAAC = this->audioAAC;
      this->prevVideoH264 = this->videoH264;

      // Update file names for next slice
      if (this->isLive) {
        wstring_convert<std::codecvt_utf8<wchar_t>> stringTypeConversion;
        string folderPath = stringTypeConversion.to_bytes(this->liveFolder);
        this->audioWAV = folderPath + to_string(this->unixTime) + this->wavExtension;
        this->audioAAC = folderPath + to_string(this->unixTime) + this->aacExtension;
        this->videoH264 = this->liveFolder + to_wstring(this->unixTime) + this->h264Extension;
      }

      // Flushing inputs
      if (this->nvidiaDevice) {
        status = this->nvEncoder->flushInputTextures();
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }
      else if (this->amdDevice) {
        status = this->amdEncoder->flushInputTextures();
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }
      }

			this->needEncodingSessionInit = true;
			this->movingToMuxingStage = true;

      if (!this->audioCapture->needToInitializeDevices) {
        this->audioCapture->needToCloseCaptureFile = true;
        this->stopEncodingSession = forceStop;
        if (this->stopEncodingSession) {
          this->audioCapture->closeCaptureFile();
        }
      }

      return status;
    }

    FBCAPTURE_STATUS EncoderMain::muxingData(PROJECTIONTYPE projectionType, STEREO_MODE stereMode, bool is360, int fps) {
      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;
      if (!this->movingToMuxingStage || this->stopEncProcess)
        return status;

      while (true) {
        if (this->stopEncProcess)
          break;

        Sleep(1);

        if (this->audioCapture->needToCloseCaptureFile)
          continue;

        // Transcoding wav to aac
        status = this->audioEncoder->continueAudioTranscoding(this->prevAudioWAV, this->prevAudioAAC);
        if (status != FBCAPTURE_STATUS_OK) {
          this->stopEncProcess = true;
          return status;
        }

        // Muxing
        if (this->isLive) {
          status = this->flvMuxer->muxingMedia(this->prevVideoH264, this->prevAudioAAC);
          if (status != FBCAPTURE_STATUS_OK) {
            this->stopEncProcess = true;
            return status;
          }
        }
        else {
          status = this->mp4Muxer->muxingMedia(this->prevVideoH264, this->prevAudioAAC, projectionType, stereMode, is360, fps);
          if (status != FBCAPTURE_STATUS_OK) {
            this->stopEncProcess = true;
            return status;
          }
        }

        // Clean up audio resouces in same thread
        if (this->stopEncodingSession) {
          this->stopEncodingSession = false; // Reset for next encoding session
          if (this->audioCapture) {
            this->audioCapture->closeDevices();
          }
          if (this->audioEncoder) {
            this->audioEncoder->shutdown();
          }
        }

        break;
      }
			this->movingToMuxingStage = false;
      this->isMuxed = true;
      this->stopEncProcess = false;

      return status;
    }

    FBCAPTURE_STATUS EncoderMain::saveScreenShot(const void* texturePtr, const TCHAR* fullSavePath, bool is360) {
      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;
					
			initEncoderComponents();			

      if (_tcslen(fullSavePath) == 0) {  //No input file path for screenshot
        DEBUG_ERROR("Didn't put screenshot file name");
        return FBCAPTURE_STATUS_NO_INPUT_FILE;
      }

      if (texturePtr && this->nvidiaDevice) {
        status = this->nvEncoder->saveScreenShot(texturePtr, fullSavePath, is360);
        if (status != FBCAPTURE_STATUS_OK) {
          return status;
        }
      } else if (texturePtr && this->amdDevice) {
        status = this->amdEncoder->saveScreenShot(texturePtr, fullSavePath, is360);
        if (status != FBCAPTURE_STATUS_OK) {
          return status;
        }
      }
      else if (texturePtr == nullptr) {
        DEBUG_ERROR("Invalid render texture pointer(null)");
        return FBCAPTURE_STATUS_INVALID_TEXTURE_POINTER;
      }

      DEBUG_LOG("Screenshot has successfully finished");

      return status;
    }

    FBCAPTURE_STATUS EncoderMain::startLiveStream(const TCHAR* streamUrl) {
      FBCAPTURE_STATUS status = FBCAPTURE_STATUS_OK;

      if (!this->isMuxed || this->stopEncProcess || !this->isLive)
        return status;

      if (this->rtmp && this->isLive) {
        wstring liveVideoFile = generateLiveVideoFileWString();
        if (!liveVideoFile.empty() && (status = this->rtmp->connectRTMPWithFlv(streamUrl, liveVideoFile.c_str())) != FBCAPTURE_STATUS_OK) {
          return status;
        }
      }
      else {
        DEBUG_LOG("Need to enable live streaming option in Unity script");
      }

      return status;
    }

    void EncoderMain::stopLiveStream() {
      if (this->rtmp) {
        this->rtmp->close();
      }

      // Remove any potential lingering files due to failures
      remove(this->audioWAV.c_str());
      remove(this->audioAAC.c_str());
      _wremove(this->videoH264.c_str());
      remove(this->prevAudioWAV.c_str());
      remove(this->prevAudioAAC.c_str());
      _wremove(this->prevVideoH264.c_str());
      if (!this->prevVideoH264.empty()) {
        wstring liveVideoFile = generateLiveVideoFileWString();
        _wremove(liveVideoFile.c_str());
      }
    }

    void EncoderMain::resetResources() {
			this->initiatedUnixTime = false;
      this->stopEncProcess = false;
      this->isLive = false;
      this->unixTime = 0;
      this->updateInputName = true;
      this->isMuxed = false;
      this->stopEncodingSession = false;
			this->needEncodingSessionInit = true;
			this->movingToMuxingStage = false;
    }

    void EncoderMain::setGraphicsDeviceD3D11(ID3D11Device* device) {
        this->device_ = (ID3D11Device*)device;
    }
  }
}

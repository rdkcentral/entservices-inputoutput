/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include "HdmiCecSource.h"


#include "ccec/Connection.hpp"
#include "ccec/CECFrame.hpp"
#include "ccec/MessageEncoder.hpp"
#include "host.hpp"
#include "ccec/host/RDK.hpp"

#include "dsMgr.h"
#include "dsDisplay.h"
#include "videoOutputPort.hpp"
#include "manager.hpp"
#include "websocket/URL.h"

#include "UtilsIarm.h"
#include "UtilsJsonRpc.h"
#include "UtilssyncPersistFile.h"
#include "UtilsSearchRDKProfile.h"

#define HDMICECSOURCE_METHOD_SEND_KEY_PRESS         "sendKeyPressEvent"
#define HDMICEC_EVENT_ON_DEVICES_CHANGED "onDevicesChanged"
#define HDMICEC_EVENT_ON_HDMI_HOT_PLUG "onHdmiHotPlug"
#define HDMICEC_EVENT_ON_STANDBY_MSG_RECEIVED "standbyMessageReceived"
#define DEV_TYPE_TUNER 1
#define HDMI_HOT_PLUG_EVENT_CONNECTED 0
#define ABORT_REASON_ID 4

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 8

enum {
	HDMICECSOURCE_EVENT_DEVICE_ADDED=0,
	HDMICECSOURCE_EVENT_DEVICE_REMOVED,
	HDMICECSOURCE_EVENT_DEVICE_INFO_UPDATED,
    HDMICECSOURCE_EVENT_ACTIVE_SOURCE_STATUS_UPDATED,
};

static const char *eventString[] = {
	"onDeviceAdded",
	"onDeviceRemoved",
	"onDeviceInfoUpdated",
        "onActiveSourceStatusUpdated"
};

#define CEC_SETTING_ENABLED_FILE "/opt/persistent/ds/cecData_2.json"
#define CEC_SETTING_ENABLED "cecEnabled"
#define CEC_SETTING_OTP_ENABLED "cecOTPEnabled"
#define CEC_SETTING_OSD_NAME "cecOSDName"
#define CEC_SETTING_VENDOR_ID "cecVendorId"

static std::vector<uint8_t> defaultVendorId = {0x00,0x19,0xFB};
static VendorID lgVendorId = {0x00,0xE0,0x91};
static PhysicalAddress physical_addr = {0x0F,0x0F,0x0F,0x0F};
static LogicalAddress logicalAddress = 0xF;
static int32_t powerState = 1;
static PowerStatus tvPowerState = 1;
static bool isDeviceActiveSource = false;
static bool isLGTvConnected = false;

using namespace WPEFramework;


namespace WPEFramework
{
    namespace {

        static Plugin::Metadata<Plugin::HdmiCecSource> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin
    {
        SERVICE_REGISTRATION(HdmiCecSource, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        HdmiCecSource* HdmiCecSource::_instance = nullptr;
        static int libcecInitStatus = 0;

        HdmiCecSource::HdmiCecSource()
        : PluginHost::JSONRPC(),
        {
            LOGWARN("ctor");
        }

        HdmiCecSource::~HdmiCecSource()
        {
            LOGWARN("dtor");
        }

        const string HdmiCecSource::Initialize(PluginHost::IShell *service)
        {
           LOGWARN("Initlaizing CEC_2");
           uint32_t res = Core::ERROR_GENERAL;

           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return (std::string("Not supported"));
           }

           string msg;
           HdmiCecSource::_instance = this;
           _hdmiCecSource = _service->Root<Exchange::IHdmiCecSource>(_connectionId, 5000, _T("HdmiCecSourceImplementation"));

           if(nullptr != _hdmiCecSource)
            {
                _hdmiCecSource->Configure(service);
                _hdmiCecSource->Register(&_hdmiCecSourceNotification);
                msg = "HdmiCecSource plugin is available";
                LOGINFO("HdmiCecSource plugin is available. Successfully activated HdmiCecSource Plugin");
            }
            else
            {
                msg = "HdmiCecSource plugin is not available";
                LOGINFO("HdmiCecSource plugin is not available. Failed to activate HdmiCecSource Plugin");
                return msg;
            }

           // On success return empty, to indicate there is no error text.
           return msg;
        }


        void HdmiCecSource::Deinitialize(PluginHost::IShell* /* service */)
        {
           LOGWARN("Deinitialize CEC_2");
           if(_powerManagerPlugin)
           {
               _powerManagerPlugin.Reset();
           }
           _registeredEventHandlers = false;

           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return ;
           }

           if(true == getEnabled())
           {
               HdmiCecSource::_hdmiCecSource->SetEnabled(false,false);
           }
           isDeviceActiveSource = false;
           HdmiCecSource::_instance->sendActiveSourceEvent();
           HdmiCecSource::_instance = nullptr;

           DeinitializeIARM();
        }

    void  HdmiCecSource::sendDeviceUpdateInfo(const int logicalAddress)
	{
		JsonObject params;
		params["logicalAddress"] = JsonValue(logicalAddress);
		LOGINFO("Device info updated notification send: for logical address:%d\r\n", logicalAddress);
		sendNotify(eventString[HDMICECSOURCE_EVENT_DEVICE_INFO_UPDATED], params);
	}

    void HdmiCecSource::sendActiveSourceEvent()
       {
           JsonObject params;
           params["status"] = isDeviceActiveSource;
           LOGWARN(" sendActiveSourceEvent isDeviceActiveSource: %d ",isDeviceActiveSource);
           sendNotify(eventString[HDMICECSOURCE_EVENT_ACTIVE_SOURCE_STATUS_UPDATED], params);
       }

       void HdmiCecSource::SendKeyPressMsgEvent(const int logicalAddress,const int keyCode)
       {
           JsonObject params;
           params["logicalAddress"] = JsonValue(logicalAddress);
           params["keyCode"] = JsonValue(keyCode);
           sendNotify(HDMICEC_EVENT_ON_KEYPRESS_MSG_RECEIVED, params);
       }
       void HdmiCecSource::SendKeyReleaseMsgEvent(const int logicalAddress)
       {
           JsonObject params;
           params["logicalAddress"] = JsonValue(logicalAddress);
           sendNotify(HDMICEC_EVENT_ON_KEYRELEASE_MSG_RECEIVED, params);
       }

    void HdmiCecSource::SendStandbyMsgEvent(const int logicalAddress)
       {
           JsonObject params;
           params["logicalAddress"] = JsonValue(logicalAddress);
           sendNotify(HDMICEC_EVENT_ON_STANDBY_MSG_RECEIVED, params);
       }

    } // namespace Plugin
} // namespace WPEFramework

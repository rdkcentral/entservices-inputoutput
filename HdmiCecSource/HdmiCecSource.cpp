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

#define HDMICECSOURCE_METHOD_SET_ENABLED "setEnabled"
#define HDMICECSOURCE_METHOD_GET_ENABLED "getEnabled"
#define HDMICECSOURCE_METHOD_OTP_SET_ENABLED "setOTPEnabled"
#define HDMICECSOURCE_METHOD_OTP_GET_ENABLED "getOTPEnabled"
#define HDMICECSOURCE_METHOD_SET_OSD_NAME "setOSDName"
#define HDMICECSOURCE_METHOD_GET_OSD_NAME "getOSDName"
#define HDMICECSOURCE_METHOD_SET_VENDOR_ID "setVendorId"
#define HDMICECSOURCE_METHOD_GET_VENDOR_ID "getVendorId"
#define HDMICECSOURCE_METHOD_PERFORM_OTP_ACTION "performOTPAction"
#define HDMICECSOURCE_METHOD_SEND_STANDBY_MESSAGE "sendStandbyMessage"
#define HDMICECSOURCE_METHOD_GET_ACTIVE_SOURCE_STATUS "getActiveSourceStatus"
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
        : PluginHost::JSONRPC(),cecEnableStatus(false),smConnection(nullptr), m_sendKeyEventThreadRun(false)
        , _pwrMgrNotification(*this)
        , _registeredEventHandlers(false)
        {
            LOGWARN("ctor");
            _engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
            _communicatorClient = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(_engine));
        }

        HdmiCecSource::~HdmiCecSource()
        {
            LOGWARN("dtor");
        }

        const string HdmiCecSource::Initialize(PluginHost::IShell* /* service */)
        {
           LOGWARN("Initlaizing CEC_2");
           uint32_t res = Core::ERROR_GENERAL;
           PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
           PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;

           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return (std::string("Not supported"));
           }

           string msg;
           HdmiCecSource::_instance = this;
           smConnection = NULL;
           cecEnableStatus = false;
           _hdmiCecSource = _service->Root<Exchange::IHdmiCecSource>(_connectionId, 5000, _T("HdmiCecSourceImplementation"));

           if(nullptr != _hdmiCecSource)
           {
                _hdmiCecSource->Register(&_hdmiCecSourceNotification);
              }
              else
              {
                msg = "HdmiCecSource plugin is not available";
                LOGERR("HdmiCecSource plugin is not available. Failed to activate HdmiCecSource Plugin");
                return msg;
           }
           Register(HDMICECSOURCE_METHOD_SET_ENABLED, &HdmiCecSource::setEnabledWrapper, this);
           Register(HDMICECSOURCE_METHOD_GET_ENABLED, &HdmiCecSource::getEnabledWrapper, this);
           Register(HDMICECSOURCE_METHOD_OTP_SET_ENABLED, &HdmiCecSource::setOTPEnabledWrapper, this);
           Register(HDMICECSOURCE_METHOD_OTP_GET_ENABLED, &HdmiCecSource::getOTPEnabledWrapper, this);
           Register(HDMICECSOURCE_METHOD_SET_OSD_NAME, &HdmiCecSource::setOSDNameWrapper, this);
           Register(HDMICECSOURCE_METHOD_GET_OSD_NAME, &HdmiCecSource::getOSDNameWrapper, this);
           Register(HDMICECSOURCE_METHOD_SET_VENDOR_ID, &HdmiCecSource::setVendorIdWrapper, this);
           Register(HDMICECSOURCE_METHOD_GET_VENDOR_ID, &HdmiCecSource::getVendorIdWrapper, this);
           Register(HDMICECSOURCE_METHOD_PERFORM_OTP_ACTION, &HdmiCecSource::performOTPActionWrapper, this);
           Register(HDMICECSOURCE_METHOD_SEND_STANDBY_MESSAGE, &HdmiCecSource::sendStandbyMessageWrapper, this);
           Register(HDMICECSOURCE_METHOD_GET_ACTIVE_SOURCE_STATUS, &HdmiCecSource::getActiveSourceStatusWrapper, this);
           Register(HDMICECSOURCE_METHOD_SEND_KEY_PRESS,&HdmiCecSource::sendRemoteKeyPressWrapper,this);
           Register("getDeviceList", &HdmiCecSource::getDeviceList, this);
           if (Utils::IARM::init()) {


               //Initialize cecEnableStatus to false in ctor
               cecEnableStatus = false;

               logicalAddressDeviceType = "None";
               logicalAddress = 0xFF;

               //CEC plugin functionalities will only work if CECmgr is available. If plugin Initialize failure upper layer will call dtor directly.
               InitializeIARM();
               InitializePowerManager();

               // load persistence setting
               loadSettings();

               try
               {
                   //TODO(MROLLINS) this is probably per process so we either need to be running in our own process or be carefull no other plugin is calling it
                   device::Manager::Initialize();
                   std::string strVideoPort = device::Host::getInstance().getDefaultVideoPortName();
                   device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort(strVideoPort.c_str());
                   if (vPort.isDisplayConnected())
                   {
                       std::vector<uint8_t> edidVec;
                       vPort.getDisplay().getEDIDBytes(edidVec);
                       //Set LG vendor id if connected with LG TV
                       if(edidVec.at(8) == 0x1E && edidVec.at(9) == 0x6D)
                       {
                           isLGTvConnected = true;
                       }
                       LOGINFO("manufacturer byte from edid :%x: %x  isLGTvConnected :%d",edidVec.at(8),edidVec.at(9),isLGTvConnected);
                   }
                }
                catch(...)
                {
                    LOGWARN("Exception in getting edid info .\r\n");
                }

                // get power state:
                ASSERT (_powerManagerPlugin);
                if (_powerManagerPlugin){
                    res = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
                    if (Core::ERROR_NONE == res)
                    {
                        powerState = (pwrStateCur == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON)?0:1 ;
                        LOGINFO("Current state is PowerManagerPlugin: (%d) powerState :%d \n",pwrStateCur,powerState);
                    }
                }

                if (cecSettingEnabled)
                {
                   try
                   {
                       CECEnable();
                   }
                   catch(...)
                   {
                       LOGWARN("Exception while enabling CEC settings .\r\n");
                   }
                }
           } else {
               msg = "IARM bus is not available";
               LOGERR("IARM bus is not available. Failed to activate HdmiCecSource Plugin");
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
           LOGINFO("Disconnect from the COM-RPC socket\n");
           // Disconnect from the COM-RPC socket
           if (_communicatorClient.IsValid())
           {
               _communicatorClient->Close(RPC::CommunicationTimeOut);
               _communicatorClient.Release();
           }
           if(_engine.IsValid())
           {
               _engine.Release();
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
           smConnection = NULL;

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

       uint32_t HdmiCecSource::setEnabledWrapper(const JsonObject& parameters, JsonObject& response)
       {
           LOGINFOMETHOD();

            bool enabled = false;

            if (parameters.HasLabel("enabled"))
            {
                getBoolParameter("enabled", enabled);
            }
            else
            {
                returnResponse(false);
            }

            HdmiCecSource::_hdmiCecSource->SetEnabled(enabled,true);
            returnResponse(true);
       }

       uint32_t HdmiCecSource::getEnabledWrapper(const JsonObject& parameters, JsonObject& response)
       {
            bool enabled;
            HdmiCecSource::_hdmiCecSource->GetEnabled(&enabled);
            response["enabled"] = enabled;
            returnResponse(true);
       }
       uint32_t HdmiCecSource::setOTPEnabledWrapper(const JsonObject& parameters, JsonObject& response)
       {
           LOGINFOMETHOD();

            bool enabled = false;

            if (parameters.HasLabel("enabled"))
            {
                getBoolParameter("enabled", enabled);
            }
            else
            {
                returnResponse(false);
            }

            HdmiCecSource::_hdmiCecSource->SetOTPEnabled(enabled);
            returnResponse(true);
       }

       uint32_t HdmiCecSource::getOTPEnabledWrapper(const JsonObject& parameters, JsonObject& response)
       {
            bool enabled;
            HdmiCecSource::_hdmiCecSource->GetOTPEnabled(&enabled);
            response["enabled"] = enabled;
            returnResponse(true);
       }

       uint32_t HdmiCecSource::setOSDNameWrapper(const JsonObject& parameters, JsonObject& response)
       {
           LOGINFOMETHOD();

            if (parameters.HasLabel("name"))
            {
                std::string osd = parameters["name"].String();
                LOGINFO("setOSDNameWrapper osdName: %s",osd.c_str());
                HdmiCecSource::_hdmiCecSource->SetOSDName(osd);          
            }
            else
            {
                returnResponse(false);
            }
            returnResponse(true);
        }

        uint32_t HdmiCecSource::getOSDNameWrapper(const JsonObject& parameters, JsonObject& response)
        {
            std::string osdName;
            HdmiCecSource::_hdmiCecSource->GetOSDName(&osdName);
            response["name"] = osdName;
            LOGINFO("getOSDNameWrapper osdName : %s \n",osdName.c_str());
            returnResponse(true);
        }

        uint32_t HdmiCecSource::setVendorIdWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            if (parameters.HasLabel("vendorid"))
            {
                std::string id = parameters["vendorid"].String();

                LOGINFO("setVendorIdWrapper vendorId: %s",id.c_str());
                HdmiCecSource::_hdmiCecSource->SetVendorId(id);
            }
            else
            {
                returnResponse(false);
            }
            returnResponse(true);
        }

        uint32_t HdmiCecSource::getVendorIdWrapper(const JsonObject& parameters, JsonObject& response)
        {
            std::string vendorId;
            HdmiCecSource::_hdmiCecSource->GetVendorId(&vendorId);
            response["vendorid"] = vendorId.toString();
            returnResponse(true);
        }


        uint32_t HdmiCecSource::performOTPActionWrapper(const JsonObject& parameters, JsonObject& response)
        {
            if(HdmiCecSource::_hdmiCecSource->PerformOTPAction() == Core::ERROR_NONE)
            {
                returnResponse(true);
            }
            else
            {
                returnResponse(false);
            }
        }

        uint32_t HdmiCecSource::sendStandbyMessageWrapper(const JsonObject& parameters, JsonObject& response)
       {
	        if(HdmiCecSource::_hdmiCecSource->SendStandbyMessage() == Core::ERROR_NONE)
	        { 
                    returnResponse(true);
	        }  
	        else
	        {
	            returnResponse(false);
	        } 
       }

       
	   uint32_t HdmiCecSource::sendRemoteKeyPressWrapper(const JsonObject& parameters, JsonObject& response)
		{
            returnIfParamNotFound(parameters, "logicalAddress");
			returnIfParamNotFound(parameters, "keyCode");
			string logicalAddress = parameters["logicalAddress"].String();
			string keyCode = parameters["keyCode"].String();
            HdmiCecSource::_hdmiCecSource->sendRemoteKeyPress(logicalAddress, keyCode);
			
		}

        uint32_t HdmiCecSource::GetActiveSourceStatusWrapper(const JsonObject& parameters, JsonObject& response)
        {
            bool isActiveSource;
            HdmiCecSource::_hdmiCecSource->GetActiveSourceStatus(&isActiveSource);
            response["isActiveSource"] = isActiveSource;
            returnResponse(true);
        }

        uint32_t HdmiCecSource::getDeviceList(const JsonObject& parameters, JsonObject& response)
        {
            Exchange::IHdmiCecSource::IHdmiCecSourceDeviceListIterator* devices = nullptr;
            uint32_t result = HdmiCecSource::_hdmiCecSource->getDeviceList(devices);
            if (result == Core::ERROR_NONE && devices != nullptr)
            {
                JsonArray deviceArray;
                while (devices->Next())
                {
                    Exchange::HdmiCecSourceDevices device;
                    devices->Current(device);
                
                    JsonObject deviceJson;
                    deviceJson["logicalAddress"] = device.logicalAddress;
                    deviceJson["osdName"] = device.osdName;
                    deviceJson["vendorID"] = device.vendorID;
                
                    deviceArray.Add(deviceJson);
                }
                response["devices"] = deviceArray;
                devices->Release();
            }
            returnResponse(result);
        }

    } // namespace Plugin
} // namespace WPEFramework

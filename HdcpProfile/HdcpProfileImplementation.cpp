/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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

 #include <string> 

 #include "HdcpProfileImplementation.h"
 
 #include "videoOutputPort.hpp"
 #include "videoOutputPortConfig.hpp"
 #include "dsMgr.h"
 #include "manager.hpp"
 #include "host.hpp"
 
 #include "UtilsJsonRpc.h"
 #include "UtilsIarm.h"
 
 #include "UtilsSynchroIarm.hpp"
 
 #define HDMI_HOT_PLUG_EVENT_CONNECTED 0
 #define HDMI_HOT_PLUG_EVENT_DISCONNECTED 1
 
 #define API_VERSION_NUMBER_MAJOR 1
 #define API_VERSION_NUMBER_MINOR 0
 #define API_VERSION_NUMBER_PATCH 9
 
 using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
 
 namespace WPEFramework
 {
     namespace Plugin
     {
         SERVICE_REGISTRATION(HdcpProfileImplementation, 1, 0);
         HdcpProfileImplementation *HdcpProfileImplementation::_instance = nullptr;
         
         PowerManagerInterfaceRef HdcpProfileImplementation::_powerManagerPlugin;        
         
         
         HdcpProfileImplementation::HdcpProfileImplementation()
         : _adminLock(), mShell(nullptr)
         {
             LOGINFO("Create HdcpProfileImplementation Instance");
             HdcpProfileImplementation::_instance = this;
         }
 
         HdcpProfileImplementation::~HdcpProfileImplementation()
         {
             LOGINFO("Call HdcpProfileImplementation destructor\n");
             HdcpProfileImplementation::_instance = nullptr;
             mShell = nullptr;
         }
 
         void HdcpProfileImplementation::InitializePowerManager(PluginHost::IShell *service)
         {
             _powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
                                     .withIShell(service)
                                     .withRetryIntervalMS(200)
                                     .withRetryCount(25)
                                     .createInterface();
         }
 
         void HdcpProfileImplementation::InitializeIARM()
         {
             Utils::IARM::init();
 
             IARM_Result_t res;
             IARM_CHECK( Utils::Synchro::RegisterLockedIarmEventHandler<HdcpProfileImplementation>(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, dsHdmiEventHandler) );
             IARM_CHECK( Utils::Synchro::RegisterLockedIarmEventHandler<HdcpProfileImplementation>(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_HDCP_STATUS, dsHdmiEventHandler) );
         }
 
         void HdcpProfileImplementation::DeinitializeIARM()
         {
             if (Utils::IARM::isConnected())
             {
                 IARM_Result_t res;
                 IARM_CHECK( Utils::Synchro::RemoveLockedEventHandler<HdcpProfileImplementation>(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, dsHdmiEventHandler) );
                 IARM_CHECK( Utils::Synchro::RemoveLockedEventHandler<HdcpProfileImplementation>(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_HDCP_STATUS, dsHdmiEventHandler) );
             }
         }
 
         void HdcpProfileImplementation::onHdmiOutputHotPlug(int connectStatus)
         {
             if (HDMI_HOT_PLUG_EVENT_CONNECTED == connectStatus)
             {
                 LOGINFO("HDMI_HOT_PLUG Status[%d]",connectStatus);
             }
             onHdcpProfileDisplayConnectionChanged();
         }
 
         void HdcpProfileImplementation::onHdcpProfileDisplayConnectionChanged()
         {
             HDCPStatus hdcpstatus;
             GetHDCPStatusInternal(hdcpstatus);
             dispatchEvent(HDCPPROFILE_EVENT_DISPLAYCONNECTIONCHANGED, hdcpstatus);
             logHdcpStatus("onHdcpProfileDisplayConnectionChanged", hdcpstatus);
         }
 
         void HdcpProfileImplementation::logHdcpStatus (const char *trigger, HDCPStatus& status)
         {
             LOGWARN("[%s]-HDCPStatus::isConnected: %s", trigger, status.isConnected ? "true" : "false");
             LOGWARN("[%s]-HDCPStatus::isHDCPEnabled: %s", trigger, status.isHDCPEnabled ? "true" : "false");
             LOGWARN("[%s]-HDCPStatus::isHDCPCompliant: %s", trigger, status.isHDCPCompliant ? "true" : "false");
             LOGWARN("[%s]-HDCPStatus::supportedHDCPVersion: %s", trigger, status.supportedHDCPVersion.c_str());
             LOGWARN("[%s]-HDCPStatus::receiverHDCPVersion: %s", trigger, status.receiverHDCPVersion.c_str());
             LOGWARN("[%s]-HDCPStatus::currentHDCPVersion: %s", trigger, status.currentHDCPVersion.c_str());
             LOGWARN("[%s]-HDCPStatus::hdcpReason: %d", trigger, status.hdcpReason);
             LOGWARN("[%s]-HDCPStatus Response: %s, %s, %s, %s, %s, %s, %d", trigger, 
                     status.isConnected ? "true" : "false",
                     status.isHDCPEnabled ? "true" : "false",
                     status.isHDCPCompliant ? "true" : "false",
                     status.supportedHDCPVersion.c_str(),
                     status.receiverHDCPVersion.c_str(),
                     status.currentHDCPVersion.c_str(),
                     status.hdcpReason);
        }
 
         void HdcpProfileImplementation::onHdmiOutputHDCPStatusEvent(int hdcpStatus)
         {
             LOGINFO("hdcpStatus[%d]",hdcpStatus);
             onHdcpProfileDisplayConnectionChanged();
         }
 
         void HdcpProfileImplementation::dsHdmiEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
         {
             uint32_t res = Core::ERROR_GENERAL;
             PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
             PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
 
             if(!HdcpProfileImplementation::_instance)
                 return;
 
             if (IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG == eventId)
             {
                 IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                 int hdmi_hotplug_event = eventData->data.hdmi_hpd.event;
                 LOGINFO("Received IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG  event data:%d \r\n", hdmi_hotplug_event);
 
                 HdcpProfileImplementation::_instance->onHdmiOutputHotPlug(hdmi_hotplug_event);
             }
             else if (IARM_BUS_DSMGR_EVENT_HDCP_STATUS == eventId)
             {
                 ASSERT (_powerManagerPlugin);
                 if (_powerManagerPlugin){
                     res = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
                     if (Core::ERROR_NONE != res)
                     {
                         LOGWARN("Failed to Invoke RPC method: GetPowerState");
                     }
                     else
                     {
                         IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                         int hdcpStatus = eventData->data.hdmi_hdcp.hdcpStatus;
                         LOGINFO("Received IARM_BUS_DSMGR_EVENT_HDCP_STATUS  event data:%d  param.curState: %d \r\n", hdcpStatus,pwrStateCur);
                         HdcpProfileImplementation::_instance->onHdmiOutputHDCPStatusEvent(hdcpStatus);
                     }
                 }
             }
         }
 
         /**
          * Register a notification callback
          */
         Core::hresult HdcpProfileImplementation::Register(Exchange::IHdcpProfile::INotification *notification)
         {
             ASSERT(nullptr != notification);
 
             _adminLock.Lock();
             printf("HdcpProfileImplementation::Register: notification = %p", notification);
             LOGINFO("Register notification");
 
             // Make sure we can't register the same notification callback multiple times
             if (std::find(_hdcpProfileNotification.begin(), _hdcpProfileNotification.end(), notification) == _hdcpProfileNotification.end())
             {
                 _hdcpProfileNotification.push_back(notification);
                 notification->AddRef();
             }
             else
             {
                 LOGERR("same notification is registered already");
             }
 
            _adminLock.Unlock();
 
             return Core::ERROR_NONE;
         }
 
         /**
          * Unregister a notification callback
          */
         Core::hresult HdcpProfileImplementation::Unregister(Exchange::IHdcpProfile::INotification *notification)
         {
             Core::hresult status = Core::ERROR_GENERAL;
 
             ASSERT(nullptr != notification);
 
             _adminLock.Lock();
 
             // we just unregister one notification once
             auto itr = std::find(_hdcpProfileNotification.begin(), _hdcpProfileNotification.end(), notification);
             if (itr != _hdcpProfileNotification.end())
             {
                 (*itr)->Release();
                 LOGINFO("Unregister notification");
                 _hdcpProfileNotification.erase(itr);
                 status = Core::ERROR_NONE;
             }
             else
             {
                 LOGERR("notification not found");
             }
 
             _adminLock.Unlock();
 
             return status;
         }
 
         uint32_t HdcpProfileImplementation::Configure(PluginHost::IShell* service)
         {
             uint32_t result = Core::ERROR_NONE;
             _service = service;
             _service->AddRef();
             ASSERT(service != nullptr);
             InitializeIARM();
             InitializePowerManager(service);
             return result;
         }
 
         void HdcpProfileImplementation::dispatchEvent(Event event, const HDCPStatus &hdcpstatus)
         {
             Core::IWorkerPool::Instance().Submit(Job::Create(this, event, hdcpstatus));
         }
 
         void HdcpProfileImplementation::Dispatch(Event event,const HDCPStatus& hdcpstatus)
         {
             _adminLock.Lock();
 
             std::list<Exchange::IHdcpProfile::INotification *>::const_iterator index(_hdcpProfileNotification.begin());
 
             switch (event)
             {
                 case HDCPPROFILE_EVENT_DISPLAYCONNECTIONCHANGED:
                 {
                     while (index != _hdcpProfileNotification.end())
                     {
                         (*index)->OnDisplayConnectionChanged(hdcpstatus);
                         ++index;
                     }
                 }
                 break;
 
             default:
                 LOGWARN("Event[%u] not handled", event);
                 break;
             }
             _adminLock.Unlock();
         }
 
         bool HdcpProfileImplementation::GetHDCPStatusInternal(HDCPStatus& hdcpstatus)
         {
             bool isConnected     = false;
             bool isHDCPCompliant = false;
             bool isHDCPEnabled   = true;
             int eHDCPEnabledStatus   = dsHDCP_STATUS_UNPOWERED;
             dsHdcpProtocolVersion_t hdcpProtocol = dsHDCP_VERSION_MAX;
             dsHdcpProtocolVersion_t hdcpReceiverProtocol = dsHDCP_VERSION_MAX;
             dsHdcpProtocolVersion_t hdcpCurrentProtocol = dsHDCP_VERSION_MAX;
 
             try
             {
                 std::string strVideoPort = device::Host::getInstance().getDefaultVideoPortName();
                 device::VideoOutputPort vPort = device::VideoOutputPortConfig::getInstance().getPort(strVideoPort.c_str());
                 isConnected        = vPort.isDisplayConnected();
                 hdcpProtocol       = (dsHdcpProtocolVersion_t)vPort.getHDCPProtocol();
                 eHDCPEnabledStatus = vPort.getHDCPStatus();
                 if(isConnected)
                 {
                     isHDCPCompliant    = (eHDCPEnabledStatus == dsHDCP_STATUS_AUTHENTICATED);
                     isHDCPEnabled      = vPort.isContentProtected();
                     hdcpReceiverProtocol = (dsHdcpProtocolVersion_t)vPort.getHDCPReceiverProtocol();
                     hdcpCurrentProtocol  = (dsHdcpProtocolVersion_t)vPort.getHDCPCurrentProtocol();
                 }
                 else
                 {
                     isHDCPCompliant = false;
                     isHDCPEnabled = false;
                 }
             }
             catch (const std::exception& e)
             {
                 LOGWARN("DS exception caught from %s\r\n", __FUNCTION__);
             }
 
             hdcpstatus.isConnected = isConnected;
             hdcpstatus.isHDCPCompliant = isHDCPCompliant;
             hdcpstatus.isHDCPEnabled = isHDCPEnabled;
             hdcpstatus.hdcpReason = static_cast<WPEFramework::Exchange::IHdcpProfile::HDCPReason>(eHDCPEnabledStatus);
 
             if(hdcpProtocol == dsHDCP_VERSION_2X)
             {
                 hdcpstatus.supportedHDCPVersion = "2.2";
             }
             else
             {
                 hdcpstatus.supportedHDCPVersion = "1.4";
             }
 
             if(hdcpReceiverProtocol == dsHDCP_VERSION_2X)
             {
                 hdcpstatus.receiverHDCPVersion = "2.2";
             }
             else
             {
                 hdcpstatus.receiverHDCPVersion = "1.4";
             }
 
             if(hdcpCurrentProtocol == dsHDCP_VERSION_2X)
             {
                 hdcpstatus.currentHDCPVersion = "2.2";
             }
             else
             {
                 hdcpstatus.currentHDCPVersion = "1.4";
             }
             return true;
         }
 
         Core::hresult HdcpProfileImplementation::GetHDCPStatus(HDCPStatus& hdcpstatus,bool& success)
         {
             success = GetHDCPStatusInternal(hdcpstatus);
             return Core::ERROR_NONE;
         }
 
         Core::hresult HdcpProfileImplementation::GetSettopHDCPSupport(string& supportedHDCPVersion,bool& isHDCPSupported,bool& success)
         {
             dsHdcpProtocolVersion_t hdcpProtocol = dsHDCP_VERSION_MAX;
 
             try
             {
                 std::string strVideoPort = device::Host::getInstance().getDefaultVideoPortName();
                 device::VideoOutputPort vPort = device::VideoOutputPortConfig::getInstance().getPort(strVideoPort.c_str());
                 hdcpProtocol = (dsHdcpProtocolVersion_t)vPort.getHDCPProtocol();
             }
             catch (const std::exception& e)
             {
                 LOGWARN("DS exception caught from %s\r\n", __FUNCTION__);
             }
 
             if(hdcpProtocol == dsHDCP_VERSION_2X)
             {
                 supportedHDCPVersion = "2.2";
                 LOGWARN("supportedHDCPVersion :2.2");
             }
             else
             {
                 supportedHDCPVersion = "1.4";
                 LOGWARN("supportedHDCPVersion :1.4");
             }
 
             isHDCPSupported = true;
 
             success = true;
             return Core::ERROR_NONE;
         }
        
 
     } // namespace Plugin
 } // namespace WPEFramework

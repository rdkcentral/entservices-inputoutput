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

#include "HdcpProfileImplementation.h"
#include "PowerManagerInterface.h"
#include "UtilsJsonRpc.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(HdcpProfileImplementation, 1, 0);
        HdcpProfileImplementation *HdcpProfileImplementation::_instance = nullptr;

        HdcpProfileImplementation::HdcpProfileImplementation()
        : _adminLock(), mShell(nullptr)
        {
            LOGINFO("Create HdcpProfileImplementation Instance");
            printf("HdcpProfileImplementation::HdcpProfileImplementation: this = %p", this);
            HdcpProfileImplementation::_instance = this;
            LOGINFO("HdcpProfileImplementation Instance created");
        }

        HdcpProfileImplementation::~HdcpProfileImplementation()
        {
            LOGINFO("Call HdcpProfileImplementation destructor\n");
            printf("HdcpProfileImplementation::~HdcpProfileImplementation: this = %p", this);
            HdcpProfileImplementation::_instance = nullptr;
            mShell = nullptr;
            HdcpProfileImplementation::_instance = nullptr;
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
            printf("HdcpProfileImplementation::Unregister: notification = %p", notification);
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
        
        // uint32_t HdcpProfileImplementation::Configure(PluginHost::IShell *shell)
        // { 
        //     LOGINFO("Configuring HdcpProfileImplementation");
        //     ASSERT(shell != nullptr);

        //     mShell = shell; 
        //     mShell->AddRef();

        //     return Core::ERROR_NONE;
        // }
        void HdcpProfileImplementation::dispatchEvent(Event event, const JsonObject &params)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
        }

        void HdcpProfileImplementation::Dispatch(Event event, const JsonObject params)
        {
            _adminLock.Lock();

            std::list<Exchange::IHdcpProfile::INotification *>::const_iterator index(_hdcpProfileNotification.begin());

            switch (event)
            {
				case HDCPPROFILE_EVENT_DISPLAYCONNECTIONCHANGED:
					while (index != _hdcpProfileNotification.end())
					{
                        HDCPStatus hdcpstatus; 
                        hdcpstatus.isConnected = params["isConnected"].Boolean();
                        hdcpstatus.isHDCPCompliant = params["isHDCPCompliant"].Boolean();
                        hdcpstatus.isHDCPEnabled = params["isHDCPEnabled"].Boolean();
                        hdcpstatus.hdcpReason = params["hdcpReason"].Number();
                        hdcpstatus.supportedHDCPVersion = params["supportedHDCPVersion"].String();
                        hdcpstatus.receiverHDCPVersion = params["receiverHDCPVersion"].String();
                        hdcpstatus.currentHDCPVersion = params["currentHDCPVersion"].String();
                        (*index)->OnDisplayConnectionChanged(hdcpstatus);
                        ++index;
					}
                break;

            default:
                LOGWARN("Event[%u] not handled", event);
                break;
            }
            _adminLock.Unlock();
        }

        Core::hresult HdcpProfileImplementation::GetHDCPStatus(HDCPStatus& hdcpstatus,bool& success)
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
            hdcpstatus.hdcpReason = eHDCPEnabledStatus;

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
            success = true;
            return Core::ERROR_NONE;
        }

		Core::hresult HdcpProfileImplementation::GetSettopHDCPSupport( SupportedHdcpInfo& supportedHdcpInfo,bool& success)
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
                supportedHdcpInfo.supportedHDCPVersion = "2.2";
                LOGWARN("supportedHDCPVersion :2.2");
            }
            else
            {
                supportedHdcpInfo.supportedHDCPVersion = "1.4";
                LOGWARN("supportedHDCPVersion :1.4");
            }

            supportedHdcpInfo.isHDCPSupported = true;

			success = true;
            return Core::ERROR_NONE;
        }
       

    } // namespace Plugin
} // namespace WPEFramework

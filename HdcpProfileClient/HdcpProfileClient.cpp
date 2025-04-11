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

#include "HdcpProfileClient.h"

#include "UtilsJsonRpc.h"

#include <glib.h>
#include <glib/gstdio.h>

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

using namespace std;

namespace WPEFramework {

    namespace {

        static Plugin::Metadata<Plugin::HdcpProfileClient> metadata(
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

    namespace Plugin {

        SERVICE_REGISTRATION(HdcpProfileClient, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        HdcpProfileClient* HdcpProfileClient::_instance = nullptr;

        HdcpProfileClient::HdcpProfileClient()
        :_notification(this),_notification1(this)
        {
            LOGINFO("ctor");
            LOGINFO("HdcpProfileClient Instance creating");
            HdcpProfileClient::_instance = this;
            LOGINFO("HdcpProfileClient Instance created");
            //Register("checkcomrpccomm", &HdcpProfileClient::checkcomrpccomm, this);
        }

        HdcpProfileClient::~HdcpProfileClient()
        {
            LOGINFO("dtor");
        }

        const string HdcpProfileClient::Initialize(PluginHost::IShell* service)
        {
            LOGINFO("Initialize");
             Exchange::IHdcpProfile* _remotStoreObject = nullptr;
            ASSERT(service != nullptr);
            ASSERT(m_service == nullptr);
            LOGINFO("After assert");
            m_service = service;
            m_service->AddRef();
            LOGINFO("After AddRef");
            _remotStoreObject = m_service->QueryInterfaceByCallsign<Exchange::IHdcpProfile>("org.rdk.HdcpProfile");
            if (_remotStoreObject == nullptr) {
                LOGINFO("Failed to get IHdcpProfile interface");
                return "Failed to get IHdcpProfile interface";
            }


            Exchange::IHdcpProfile::HDCPStatus _hdcpstatus;
            bool _success = false;
            _remotStoreObject->GetHDCPStatus(_hdcpstatus, _success);
            if (!_success) {
                LOGINFO("getHDCPStatus failed");
                _remotStoreObject->Release();
                return "getHDCPStatus failed";
            }

            LOGINFO("getHDCPStatus: isConnected: %d isHDCPCompliant: %d isHDCPEnabled: %d hdcpReason: %d supportedHDCPVersion: %s receiverHDCPVersion: %s currentHDCPVersion: %s",
                _hdcpstatus.isConnected, _hdcpstatus.isHDCPCompliant, _hdcpstatus.isHDCPEnabled, _hdcpstatus.hdcpReason,
                _hdcpstatus.supportedHDCPVersion.c_str(), _hdcpstatus.receiverHDCPVersion.c_str(), _hdcpstatus.currentHDCPVersion.c_str());
            
                string supportedHDCPVersion = "0";
                bool isHDCPSupported = false;
                bool success1 = false;
                _remotStoreObject->GetSettopHDCPSupport(supportedHDCPVersion,isHDCPSupported, success1);
                if (!success1) {
                    LOGINFO("getSettopHDCPSupport failed");
                    return "getSettopHDCPSupport failed";
                }
                LOGINFO("getSettopHDCPSupport: supportedHDCPVersion: %s isHDCPSupported: %d",
                    supportedHDCPVersion.c_str(), isHDCPSupported);


            Exchange::IScreenCapture* _remotStoreObject1 = nullptr;
            _remotStoreObject1 = m_service->QueryInterfaceByCallsign<Exchange::IScreenCapture>("org.rdk.ScreenCapture");
            if (_remotStoreObject1 == nullptr) {
                LOGINFO("Failed to get IScreenCapture interface");
                _remotStoreObject->Release();
                return "Failed to get IScreenCapture interface";
            }
            else
            {
                LOGINFO("Got IScreenCapture interface-%p", _remotStoreObject1);
            }
           
            Exchange::IScreenCapture::Result result;
            _remotStoreObject1->UploadScreenCapture("http://server/cgi-bin/upload.cgi","12345",result);
            if (!result.success) {
                LOGINFO("UploadScreenCapture failed");
                _remotStoreObject1->Release();
                return "UploadScreenCapture failed";
            }
            LOGINFO("UploadScreenCapture: result: %d", result.success);
           

            // On success, return an empty string
            return string();
        }


       
        void HdcpProfileClient::OnDisplayConnectionChanged(const Exchange::IHdcpProfile::HDCPStatus& hdcpstatus)
        {
            LOGINFO("OnDisplayConnectionChanged callback triggered: isConnected: %d isHDCPCompliant: %d isHDCPEnabled: %d hdcpReason: %d supportedHDCPVersion: %s receiverHDCPVersion: %s currentHDCPVersion: %s",
                 hdcpstatus.isConnected, hdcpstatus.isHDCPCompliant, hdcpstatus.isHDCPEnabled, hdcpstatus.hdcpReason,
                 hdcpstatus.supportedHDCPVersion.c_str(), hdcpstatus.receiverHDCPVersion.c_str(), hdcpstatus.currentHDCPVersion.c_str());
         }

        void HdcpProfileClient::UploadComplete(const bool& status,const std::string& message,const std::string& call_guid)
        {
            LOGINFO("UploadComplete callback triggered: status: %d message: %s call_guid: %s", status, message.c_str(), call_guid.c_str());
        }


        void HdcpProfileClient::Deinitialize(PluginHost::IShell*  service )
        {
            LOGINFO("Deinitialize");
            ASSERT(service == m_service);
            LOGINFO("Deinitialize: service == m_service");
            m_service->Release();
            m_service = nullptr;
        }

       

    } // namespace Plugin
} // namespace WPEFramework

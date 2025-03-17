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

#include "UtilsIarm.h"
#include "UtilsJsonRpc.h"
#include "UtilssyncPersistFile.h"
#include "UtilsSearchRDKProfile.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 8

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

        const string HdmiCecSource::Initialize(PluginHost::IShell *service)
        {
           LOGWARN("Initlaizing CEC_2");

           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return (std::string("Not supported"));
           }

           string msg;
           _service = service;
           _service->AddRef();
           _service->Register(&_notification);
           HdmiCecSource::_instance = this;
           _hdmiCecSource = _service->Root<Exchange::IHdmiCecSource>(_connectionId, 5000, _T("HdmiCecSourceImplementation"));

           if(nullptr != _hdmiCecSource)
            {
                _hdmiCecSource->Configure(service);
                _hdmiCecSource->Register(&_notification);
                Exchange::JHdmiCecSource::Register(*this, _hdmiCecSource);
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
           

           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return ;
           }

           bool enabled = false;
           bool ret = false;
           HdmiCecSource::_hdmiCecSource->GetEnabled(enabled,ret);

           if(ret && enabled)
           {
                ret = false;
                HdmiCecSource::_hdmiCecSource->SetEnabled(false,ret);
           }
           HdmiCecSource::_notification.OnActiveSourceStatusUpdated(false);

           HdmiCecSource::_instance = nullptr;
        }

        string HdmiCecSource::Information() const
        {
            return(string());
        }

    } // namespace Plugin
} // namespace WPEFramework

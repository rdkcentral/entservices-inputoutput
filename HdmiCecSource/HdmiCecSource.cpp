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

        const string HdmiCecSource::Initialize(PluginHost::IShell *service)
        {
           Core::hresult res = Core::ERROR_GENERAL;
           LOGWARN("Initializing HdmiCecSource plugin \n");

           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return (std::string("Not supported"));
           }

           string msg = "";

           ASSERT(nullptr != service);
           ASSERT(nullptr == _service);
           ASSERT(nullptr == _hdmiCecSource);
           ASSERT(0 == _connectionId);


           _service = service;
           _service->AddRef();
           _hdmiCecSource = _service->Root<Exchange::IHdmiCecSource>(_connectionId, 5000, _T("HdmiCecSourceImplementation"));

           if(nullptr != _hdmiCecSource)
            {
                res = _hdmiCecSource->Configure(service);
                if (res != Core::ERROR_NONE)
                {
                    msg = "HdmiCecSource plugin platform configuration error";
                    LOGINFO("HdmiCecSource plugin configuration failed. Failed to activate HdmiCecSource Plugin");
                    _hdmiCecSource->Release();
                    _hdmiCecSource = nullptr;
                }
                else
                {
                    _service->Register(&_notification);
                    _hdmiCecSource->Register(&_notification);
                    Exchange::JHdmiCecSource::Register(*this, _hdmiCecSource);
                    LOGINFO("HdmiCecSource plugin is available. Successfully activated HdmiCecSource Plugin");
                }
            }
            else
            {
                msg = "HdmiCecSource plugin is not available";
                LOGINFO("HdmiCecSource plugin is not available. Failed to activate HdmiCecSource Plugin");
            }

            if (0 != msg.length())
            {
                // Only clean up rest of them; _hdmiCecSource already handled.
                _connectionId = 0;
                _service->Release();
                _service = nullptr;
            }

           return msg;
        }


        void HdmiCecSource::Deinitialize(PluginHost::IShell* service)
        {
           LOGWARN("Deinitialize HdmiCecSource plugin \n");

           ASSERT(nullptr != service);


           profileType = searchRdkProfile();

           if (profileType == TV || profileType == NOT_FOUND)
           {
                LOGINFO("Invalid profile type for STB \n");
                return ;
           }

           // Unregister and cleanup
           if(nullptr != _hdmiCecSource)
           {
             HdmiCecSource::_notification.OnActiveSourceStatusUpdated(false);
           if(nullptr != _hdmiCecSource)
           {
             _notification.OnActiveSourceStatusUpdated(false);
             _hdmiCecSource->Unregister(&_notification);
             Exchange::JHdmiCecSource::Unregister(*this);
             _hdmiCecSource->Release();
             _hdmiCecSource = nullptr;
           }

           if (_connectionId != 0 && _service != nullptr)
           {
             RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);
             if (connection != nullptr)
             {
                try{
                    connection->Terminate();
                }
                catch(const std::exception& e)
                {
                    std::string errorMessage = "Failed to terminate connection: ";
                    errorMessage += e.what();
                    LOGWARN("%s",errorMessage.c_str());
                }

                connection->Release();
             }
           }

           if (_service != nullptr)
           {
               _connectionId = 0;
               _service->Unregister(&_notification);
               _service->Release();
               _service = nullptr;
           }
           LOGINFO("HdmiCecSource plugin is deactivated. Successfully deactivated HdmiCecSource Plugin");
        }

        string HdmiCecSource::Information() const
        {
            return("This HdmiCecSource PLugin Facilitates the HDMI CEC Source Control");
        }

        void HdmiCecSource::Deactivated(RPC::IRemoteConnection* connection)
        {
            if (connection->Id() == _connectionId)
            {
                ASSERT(_service != nullptr);
                Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }

    } // namespace Plugin
} // namespace WPEFramework

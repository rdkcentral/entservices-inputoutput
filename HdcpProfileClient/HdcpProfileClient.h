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

#pragma once

#include "Module.h"
#include <interfaces/IHdcpProfile.h>
#include <interfaces/IScreenCapture.h>


namespace WPEFramework {
    namespace Plugin {

        class HdcpProfileClient : public PluginHost::IPlugin {
        private:

            HdcpProfileClient(const HdcpProfileClient&) = delete;
            HdcpProfileClient& operator=(const HdcpProfileClient&) = delete;



        private:
            class HdcpProfileNotification : public Exchange::IHdcpProfile::INotification {
                private:
                HdcpProfileNotification(const HdcpProfileNotification&) = delete;
                HdcpProfileNotification& operator=(const HdcpProfileNotification&) = delete;
    
            public:
                explicit HdcpProfileNotification(HdcpProfileClient* parent)
                    : _parent(*parent)
                {
                }
                ~HdcpProfileNotification() override = default;
    
            public:
                void OnDisplayConnectionChanged(const Exchange::IHdcpProfile::HDCPStatus& hdcpstatus) override
                {
                    _parent.OnDisplayConnectionChanged(hdcpstatus);
                }
    
                BEGIN_INTERFACE_MAP(HdcpProfileNotification)
                INTERFACE_ENTRY(Exchange::IHdcpProfile::INotification)
                END_INTERFACE_MAP
    
            private:
                HdcpProfileClient& _parent;
            };
            






            class ScreenCaptureNotification : public Exchange::IScreenCapture::INotification {
                private:
                ScreenCaptureNotification(const ScreenCaptureNotification&) = delete;
                ScreenCaptureNotification& operator=(const ScreenCaptureNotification&) = delete;
    
                public:
                    explicit ScreenCaptureNotification(HdcpProfileClient* parent)
                        : _parent(*parent)
                    {
                    }
                    ~ScreenCaptureNotification() override = default;
        
                public:
                    void UploadComplete(const bool& status,const std::string& message,const std::string& call_guid) override
                    {
                        _parent.UploadComplete(status,message,call_guid);
                    }
        
                    BEGIN_INTERFACE_MAP(ScreenCaptureNotification)
                    INTERFACE_ENTRY(Exchange::IScreenCapture::INotification)
                    END_INTERFACE_MAP
        
                private:
                    HdcpProfileClient& _parent;
            };
           

        public:
            HdcpProfileClient();
            virtual ~HdcpProfileClient();
            virtual const string Initialize(PluginHost::IShell* shell) override ;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }

            BEGIN_INTERFACE_MAP(HdcpProfileClient)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            END_INTERFACE_MAP

        public:
            static HdcpProfileClient* _instance;
            PluginHost::IShell* m_service = nullptr;
            Core::Sink<HdcpProfileNotification> _notification;
            Core::Sink<ScreenCaptureNotification> _notification1;

            void OnDisplayConnectionChanged(const Exchange::IHdcpProfile::HDCPStatus& hdcpstatus);
            void UploadComplete(const bool& status,const std::string& message,const std::string& call_guid);
        };
    } // namespace Plugin
} // namespace WPEFramework

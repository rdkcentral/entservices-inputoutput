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

#include "libIBus.h"

#include "Module.h"
#include <interfaces/IPowerManager.h>
#include "PowerManagerInterface.h"

namespace WPEFramework {

    namespace Plugin {
		// This is a server for a JSONRPC communication channel.
		// For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
		// By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
		// This realization of this interface implements, by default, the following methods on this plugin
		// - exists
		// - register
		// - unregister
		// Any other methood to be handled by this plugin  can be added can be added by using the
		// templated methods Register on the PluginHost::JSONRPC class.
		// As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
		// this class exposes a public method called, Notify(), using this methods, all subscribed clients
		// will receive a JSONRPC message as a notification, in case this method is called.
        class HdcpProfile : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        private:

            // We do not allow this plugin to be copied !!
            HdcpProfile(const HdcpProfile&) = delete;
            HdcpProfile& operator=(const HdcpProfile&) = delete;
            Core::ProxyType<RPC::InvokeServerType<1, 0, 4> > _engine;
            Core::ProxyType<RPC::CommunicatorClient> _communicatorClient;
            static PowerManagerInterfaceRef _powerManagerPlugin;

            void InitializeIARM();
            void DeinitializeIARM();

            void RegisterAll();
            void UnregisterAll();
            void InitializePowerManager();

            //Begin methods
            uint32_t getHDCPStatusWrapper(const JsonObject& parameters, JsonObject& response);
            uint32_t getSettopHDCPSupportWrapper(const JsonObject& parameters, JsonObject& response);

            //End methods

            JsonObject getHDCPStatus();
            void onHdmiOutputHotPlug(int connectStatus);
            void onHdmiOutputHDCPStatusEvent(int);
            void logHdcpStatus (const char *trigger, const JsonObject& status);
            static void dsHdmiEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

        public:
            HdcpProfile();
            virtual ~HdcpProfile();
            virtual const string Initialize(PluginHost::IShell* shell) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }

            void terminate();

            BEGIN_INTERFACE_MAP(HdcpProfile)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            END_INTERFACE_MAP

            static HdcpProfile* _instance;
        };
	} // namespace Plugin
} // namespace WPEFramework


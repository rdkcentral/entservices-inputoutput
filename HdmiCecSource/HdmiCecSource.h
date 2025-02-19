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

#include <stdint.h>
#include <mutex>
#include <condition_variable>

#include "ccec/FrameListener.hpp"
#include "ccec/Connection.hpp"
#include "libIARM.h"
#include "ccec/Assert.hpp"
#include "ccec/Messages.hpp"
#include "ccec/MessageDecoder.hpp"
#include "ccec/MessageProcessor.hpp"
#include <thread>

#undef Assert // this define from Connection.hpp conflicts with WPEFramework

#include "Module.h"

#include "UtilsBIT.h"
#include "UtilsThreadRAII.h"

#include <interfaces/IPowerManager.h>
#include "PowerManagerInterface.h"
#include <interfaces/IHdmiCecSource.h>

using namespace WPEFramework;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

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
        class HdmiCecSource : public PluginHost::IPlugin, public PluginHost::JSONRPC {

            private:
                class Notification : public RPC::IRemoteConnection::INotification,
                                     public Exchange::IHdmiCecSource::INotification
                    {
                       private:
                           Notification() = delete;
                           Notification(const Notification&) = delete;
                           Notification& operator=(const Notification&) = delete;

                        public:
                            explicit Notification(HdmiCecSource* parent)
                                : _parent(*parent)
                                {
                                    ASSERT(parent != nullptr);
                                }

                                virtual ~Notification()
                                {
                                }

                                BEGIN_INTERFACE_MAP(Notification) 
                                INTERFACE_ENTRY(Exchange::IHdmiCecSource::INotification)
                                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                                END_INTERFACE_MAP

                                void Activated(RPC::IRemoteConnection*) override;
                                void Deactivated(RPC::IRemoteConnection *connection) override;

                                void OnDeviceAdded() override;
                                void OnDeviceRemoved(const uint8_t logicalAddress) override;
                                void OnDeviceInfoUpdated(const int logicalAddress) override;
                                void OnActiveSourceStatusUpdated(const bool isActiveSource) override;
                                void standbyMessageReceived(const int8_t logicalAddress) override;

                    }

            public:
                // We do not allow this plugin to be copied !!
                HdmiCecSource(const HdmiCecSource&) = delete;
                HdmiCecSource& operator=(const HdmiCecSource&) = delete;

                HdmiCecSource();
                virtual ~HdmiCecSource();

                BEGIN_INTERFACE_MAP(HdmiCecSource)
                INTERFACE_ENTRY(PluginHost::IPlugin)
                INTERFACE_ENTRY(PluginHost::IDispatcher)
                INTERFACE_AGGREGATE(Exchange::IHdmiCecSource, _hdmiCecSource)
                END_INTERFACE_MAP

                //  IPlugin methods
                // -------------------------------------------------------------------------------------------------------
		        const string Initialize(PluginHost::IShell* service) override;
                void Deinitialize(PluginHost::IShell* service) override;
                //Begin methods
                uint32_t setEnabledWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t getEnabledWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t setOTPEnabledWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t getOTPEnabledWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t setOSDNameWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t getOSDNameWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t setVendorIdWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t getVendorIdWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t performOTPActionWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t sendStandbyMessageWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t getDeviceList (const JsonObject& parameters, JsonObject& response);
                uint32_t GetActiveSourceStatusWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t sendRemoteKeyPressWrapper(const JsonObject& parameters, JsonObject& response);
                uint32_t sendKeyPressEventWrapper(const JsonObject& parameters, JsonObject& response);

            private:
                void Deactivated(RPC::IRemoteConnection* connection);

            private:
                PluginHost::IShell* _service{};
                Exchange::IHdmiCecSource* _hdmiCecSource;
        };
	} // namespace Plugin
} // namespace WPEFramework





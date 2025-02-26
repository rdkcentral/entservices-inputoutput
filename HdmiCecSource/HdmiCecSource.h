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

                                void OnDeviceAdded() override
                                {
                                    LOGINFO("OnDeviceAdded");
                                    JsonObject params;
                                    _parent.Notify(_T("onDeviceAdded"), params);
                                }
                                void OnDeviceRemoved(const uint8_t logicalAddress) override
                                {
                                    LOGINFO("OnDeviceRemoved");
                                    JsonObject params;
                                    params["logicalAddress"] = logicalAddress;
                                    _parent.Notify(_T("onDeviceRemoved"), params);
                                }
                                void OnDeviceInfoUpdated(const int logicalAddress) override
                                {
                                    LOGINFO("OnDeviceInfoUpdated");
                                    JsonObject params;
                                    params["logicalAddress"] = logicalAddress;
                                    _parent.Notify(_T("onDeviceInfoUpdated"), params);
                                }
                                void OnActiveSourceStatusUpdated(const bool isActiveSource) override
                                {
                                    LOGINFO("OnActiveSourceStatusUpdated");
                                    JsonObject params;
                                    params["isActiveSource"] = isActiveSource;
                                    _parent.Notify(_T("onActiveSourceStatusUpdated"), params);
                                }
                                void StandbyMessageReceived(const int8_t logicalAddress) override
                                {
                                    LOGINFO("StandbyMessageReceived");
                                    JsonObject params;
                                    params["logicalAddress"] = logicalAddress;
                                    _parent.Notify(_T("standbyMessageReceived"), params);
                                }
                                void SendKeyReleaseMsgEvent(const int8_t logicalAddress) override
                                {
                                    LOGINFO("SendKeyReleaseMsgEvent");
                                    JsonObject params;
                                    params["logicalAddress"] = logicalAddress;
                                    _parent.Notify(_T("SendKeyReleaseMsgEvent"), params);
                                }
                                void SendKeyPressMsgEvent(const int8_t logicalAddress, const int32_t keyCode) override
                                {
                                    LOGINFO("SendKeyPressMsgEvent");
                                    JsonObject params;
                                    params["logicalAddress"] = logicalAddress;
                                    params["keyCode"] = keyCode;
                                    _parent.Notify(_T("sendKeyPressMsgEvent"), params);
                                }

                            private:
                                HdmiCecSource &_parent;

                    };

            public:
                // We do not allow this plugin to be copied !!
                HdmiCecSource(const HdmiCecSource&) = delete;
                HdmiCecSource& operator=(const HdmiCecSource&) = delete;
                static HdmiCecSource* _instance;

                HdmiCecSource()
                : PluginHost::IPlugin()
                , PluginHost::JSONRPC()
                , _connectionId(0)
                , _hdmiCecSource(nullptr)
                , _notification(this)
                , _service(nullptr)
                {

                }
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

            private:
                void Deactivated(RPC::IRemoteConnection* connection);

            private:
                PluginHost::IShell* _service{};
                Exchange::IHdmiCecSource* _hdmiCecSource;
                Core::Sink<Notification> _notification;
                uint32_t _connectionId;
        };
	} // namespace Plugin
} // namespace WPEFramework





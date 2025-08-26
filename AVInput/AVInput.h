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

#pragma once

#include "Module.h"
#include <core/JSON.h>
#include <interfaces/IAVInput.h>
#include <interfaces/json/JAVInput.h>
#include <interfaces/json/JsonData_AVInput.h>

#include "UtilsLogging.h"
#include "tracing/Logging.h"

namespace WPEFramework {
namespace Plugin {
    
    class AVInput : public PluginHost::IPlugin, public PluginHost::JSONRPC {

    public:

        AVInput(const AVInput&) = delete;
        AVInput& operator=(const AVInput&) = delete;

        AVInput();
        virtual ~AVInput();

        BEGIN_INTERFACE_MAP(AVInput)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IAVInput, _avInput)
        END_INTERFACE_MAP

        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    protected:

        void RegisterAll();
        void UnregisterAll();

    private:

        PluginHost::IShell* _service {};
        uint32_t _connectionId {};
        Exchange::IAVInput* _avInput {};
        Core::Sink<Notification> _avInputNotification;

        void Deactivated(RPC::IRemoteConnection* connection);

        class Notification : public RPC::IRemoteConnection::INotification,
                             public Exchange::IAVInput::IDevicesChangedNotification,
                             public Exchange::IAVInput::ISignalChangedNotification,
                             public Exchange::IAVInput::IInputStatusChangedNotification,
                             public Exchange::IAVInput::IVideoStreamInfoUpdateNotification,
                             public Exchange::IAVInput::IGameFeatureStatusUpdateNotification,
                             public Exchange::IAVInput::IHdmiContentTypeUpdateNotification {

        public:
        
            explicit Notification(AVInput* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            virtual ~Notification()
            {
            }

            template <typename T>
            T* baseInterface()
            {
                static_assert(std::is_base_of<T, Notification>(), "base type mismatch");
                return static_cast<T*>(this);
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IAVInput::IDevicesChangedNotification)
            INTERFACE_ENTRY(Exchange::IAVInput::ISignalChangedNotification)
            INTERFACE_ENTRY(Exchange::IAVInput::IInputStatusChangedNotification)
            INTERFACE_ENTRY(Exchange::IAVInput::IVideoStreamInfoUpdateNotification)
            INTERFACE_ENTRY(Exchange::IAVInput::IGameFeatureStatusUpdateNotification)
            INTERFACE_ENTRY(Exchange::IAVInput::IHdmiContentTypeUpdateNotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

            void Activated(RPC::IRemoteConnection*) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

            void OnDevicesChanged(const string& devices) override
            {
                LOGINFO("OnDevicesChanged: devices %s\n", devices.c_str());
                Exchange::JAVInput::Event::OnDevicesChanged(_parent, devices);
            }

            void OnSignalChanged(const Exchange::IAVInput::InputSignalInfo& info) override
            {
                LOGINFO("OnSignalChanged: id %d, locator %s, status %s\n", info.id, info.locator.c_str(), info.status.c_str());
                Exchange::JAVInput::Event::OnSignalChanged(_parent, info);
            }

            void OnInputStatusChanged(const Exchange::IAVInput::InputStatus& status) override
            {
                LOGINFO("OnInputStatusChanged: id %d, locator %s, status %s, plane %d\n", status.info.id, status.info.locator.c_str(), status.info.status.c_str(), status.plane);
                Exchange::JAVInput::Event::OnInputStatusChanged(_parent, status);
            }

            void VideoStreamInfoUpdate(const Exchange::IAVInput::InputVideoMode& videoMode) override
            {
                LOGINFO("VideoStreamInfoUpdate: id %d, width %d, height %d, frameRateN %d, frameRateD %d, progressive %d, locator %s\n",
                    videoMode.id, videoMode.width, videoMode.height, videoMode.frameRateN, videoMode.frameRateD,
                    videoMode.progressive, videoMode.locator.c_str());
                Exchange::JAVInput::Event::VideoStreamInfoUpdate(_parent, videoMode);
            }

            void GameFeatureStatusUpdate(const Exchange::IAVInput::GameFeatureStatus& status) override
            {
                LOGINFO("GameFeatureStatusUpdate: id %d, gameFeature %s, allmMode %d\n",
                    status.id, status.gameFeature.c_str(), static_cast<int>(status.allmMode));
                Exchange::JAVInput::Event::GameFeatureStatusUpdate(_parent, status);
            }

            void HdmiContentTypeUpdate(const Exchange::IAVInput::ContentInfo& contentInfo) override
            {
                LOGINFO("HdmiContentTypeUpdate: id %d, contentType %d\n", contentInfo.id, contentInfo.contentType);
                Exchange::JAVInput::Event::HdmiContentTypeUpdate(_parent, contentInfo);
            }

        private:
            AVInput& _parent;

            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
        };

    }; // AVInput
} // namespace Plugin
} // namespace WPEFramework

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
#include <interfaces/IAVInput.h>
#include <interfaces/json/JAVInput.h>
#include <interfaces/json/JsonData_AVInput.h>

#include "AVInputImplementation.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"

#include "dsMgr.h"
#include "hdmiIn.hpp"
#include "compositeIn.hpp"
#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"
#include "host.hpp"
#include "exception.hpp"
#include <vector>
#include <algorithm>

namespace WPEFramework
{
    namespace Plugin
    {
        class AVInput : public PluginHost::IPlugin, public PluginHost::JSONRPC
        {
        private:
            class Notification : public RPC::IRemoteConnection::INotification, public Exchange::IAVInput::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification &) = delete;
                Notification &operator=(const Notification &) = delete;

            public:
                explicit Notification(AVInput *parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification()
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(Exchange::IAVInput::INotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection *) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                    _parent.Deactivated(connection);
                }

                void OnDevicesChanged(WPEFramework::Exchange::IAVInput::IInputDeviceIterator *const devices) override
                {
                    LOGINFO("OnDevicesChanged\n");
                    Exchange::JAVInput::Event::OnDevicesChanged(_parent, devices);
                }

                void OnSignalChanged(const InputSignalInfo &info) override
                {
                    LOGINFO("OnSignalChanged: id %d, locator %s, status %s\n", info.id, info.locator.c_str(), info.status.c_str());
                    Exchange::JAVInput::Event::OnSignalChanged(_parent, info);
                }

                void OnInputStatusChanged(const InputSignalInfo &info) override
                {
                    LOGINFO("OnInputStatusChanged: id %d, locator %s, status %s\n", info.id, info.locator.c_str(), info.status.c_str());
                    Exchange::JAVInput::Event::OnInputStatusChanged(_parent, info);
                }

                void VideoStreamInfoUpdate(const InputVideoMode &videoMode) override
                {
                    LOGINFO("VideoStreamInfoUpdate: id %d, width %d, height %d, frameRateN %d, frameRateD %d, progressive %d, locator %s\n",
                            videoMode.id, videoMode.width, videoMode.height, videoMode.frameRateN, videoMode.frameRateD,
                            videoMode.progressive, videoMode.locator.c_str());
                    Exchange::JAVInput::Event::VideoStreamInfoUpdate(_parent, videoMode);
                }

                void GameFeatureStatusUpdate(const GameFeatureStatus &status) override
                {
                    LOGINFO("GameFeatureStatusUpdate: id %d, gameFeature %s, allmMode %d\n",
                            status.id, status.gameFeature.c_str(), static_cast<int>(status.allmMode));
                    Exchange::JAVInput::Event::GameFeatureStatusUpdate(_parent, status);
                }

                void HdmiContentTypeUpdate(int contentType) override
                {
                    LOGINFO("HdmiContentTypeUpdate: contentType %d\n", contentType);
                    Exchange::JAVInput::Event::HdmiContentTypeUpdate(_parent, contentType);
                }

            private:
                AVInput &_parent;
            };

        public:
            AVInput(const AVInput &) = delete;
            AVInput &operator=(const AVInput &) = delete;

            AVInput();
            virtual ~AVInput();

            BEGIN_INTERFACE_MAP(AVInput)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IAVInput, _avInput)
            END_INTERFACE_MAP

            //  IPlugin methods
            // -------------------------------------------------------------------------------------------------------
            const string Initialize(PluginHost::IShell *service) override;
            void Deinitialize(PluginHost::IShell *service) override;
            string Information() const override;

        protected:
            void InitializeIARM();
            void DeinitializeIARM();

            void RegisterAll();
            void UnregisterAll();

        private:
            PluginHost::IShell *_service{};
            uint32_t _connectionId{};
            Exchange::IAVInput *_avInput{};
            Core::Sink<Notification> _avInputNotification;

            void Deactivated(RPC::IRemoteConnection *connection);

            static void dsAVEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVSignalStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVVideoModeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVGameFeatureStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAviContentTypeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

        }; // AVInput
    } // namespace Plugin
} // namespace WPEFramework

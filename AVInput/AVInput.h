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

                void OnSignalChanged(uint8_t id, const InputSignalInfo &info) override
                {
                    LOGINFO("OnSignalChanged: id %d, locator %s, status %s\n", id, info.locator.c_str(), info.status.c_str());
                    Exchange::JAVInput::Event::OnSignalChanged(_parent, id, info);
                }

                void OnInputStatusChanged(uint8_t id, const InputSignalInfo &info) override
                {
                    LOGINFO("OnInputStatusChanged: id %d, locator %s, status %s\n", id, info.locator.c_str(), info.status.c_str());
                    Exchange::JAVInput::Event::OnInputStatusChanged(_parent, id, info);
                }

                void videoStreamInfoUpdate(uint8_t id, const InputVideoMode &videoMode) override
                {
                    LOGINFO("videoStreamInfoUpdate: id %d, videoMode %d\n", id, videoMode);
                    Exchange::JAVInput::Event::videoStreamInfoUpdate(_parent, id, videoMode);
                }

                void gameFeatureStatusUpdate(uint8_t id, const GameFeatureStatus &status) override
                {
                    LOGINFO("gameFeatureStatusUpdate: id %d, status %d\n", id, status);
                    Exchange::JAVInput::Event::gameFeatureStatusUpdate(_parent, id, status);
                }

                void aviContentTypeUpdate(uint8_t id, int contentType) override
                {
                    LOGINFO("aviContentTypeUpdate: id %d, contentType %d\n", id, contentType);
                    Exchange::JAVInput::Event::aviContentTypeUpdate(_parent, id, contentType);
                }

            private:
                AVInput &_parent;
            };

            static void dsAVEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVSignalStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVVideoModeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVGameFeatureStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAviContentTypeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

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

            dsVRRType_t m_currentVrrType;

            //  IPlugin methods
            // -------------------------------------------------------------------------------------------------------
            const string Initialize(PluginHost::IShell *service) override;
            void Deinitialize(PluginHost::IShell *service) override;
            string Information() const override;

        private:
            void Deactivated(RPC::IRemoteConnection *connection);

        private:
            PluginHost::IShell *_service{};
            uint32_t _connectionId{};
            Exchange::IAVInput *_avInput{};
            Core::Sink<Notification> _avInputNotification;
        };
    } // namespace Plugin
} // namespace WPEFramework

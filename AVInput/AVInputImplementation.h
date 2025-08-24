/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.Fv
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Module.h"

#include "dsMgr.h"
#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"

#include "hdmiIn.hpp"
#include "compositeIn.hpp"
#include "exception.hpp"
#include "host.hpp"

#include <vector>
#include <algorithm>

#include <interfaces/Ids.h>
#include <interfaces/IAVInput.h>

#include <com/com.h>
#include <core/core.h>
#include <boost/variant.hpp>

#define DEFAULT_PRIM_VOL_LEVEL 25
#define MAX_PRIM_VOL_LEVEL 100
#define DEFAULT_INPUT_VOL_LEVEL 100

using ParamsType = boost::variant<string &,
                                  WPEFramework::Exchange::IAVInput::InputSignalInfo,
                                  WPEFramework::Exchange::IAVInput::InputStatus,
                                  WPEFramework::Exchange::IAVInput::InputVideoMode,
                                  WPEFramework::Exchange::IAVInput::ContentInfo,
                                  WPEFramework::Exchange::IAVInput::GameFeatureStatus,
                                  int>;

namespace WPEFramework
{
    namespace Plugin
    {
        class AVInputImplementation : public Exchange::IAVInput
        {
        protected:
            // TODO: Why are these here and not following definitions of other JSON calls?
            uint32_t endpoint_numberOfInputs(const JsonObject &parameters, JsonObject &response);
            uint32_t endpoint_currentVideoMode(const JsonObject &parameters, JsonObject &response);
            uint32_t endpoint_contentProtected(const JsonObject &parameters, JsonObject &response);

        public:
            // We do not allow this plugin to be copied !!
            AVInputImplementation();
            ~AVInputImplementation() override;

            // We do not allow this plugin to be copied !!
            AVInputImplementation(const AVInputImplementation &) = delete;
            AVInputImplementation &operator=(const AVInputImplementation &) = delete;

            BEGIN_INTERFACE_MAP(AVInputImplementation)
            INTERFACE_ENTRY(Exchange::IAVInput)
            END_INTERFACE_MAP

        public:
            enum Event
            {
                ON_AVINPUT_DEVICES_CHANGED,
                ON_AVINPUT_SIGNAL_CHANGED,
                ON_AVINPUT_STATUS_CHANGED,
                ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE,
                ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE,
                ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE
            };

            class EXTERNAL Job : public Core::IDispatch
            {
            protected:
                Job(AVInputImplementation *avInputImplementation, Event event, ParamsType &params)
                    : _avInputImplementation(avInputImplementation), _event(event), _params(params)
                {
                    if (_avInputImplementation != nullptr)
                    {
                        _avInputImplementation->AddRef();
                    }
                }

            public:
                Job() = delete;
                Job(const Job &) = delete;
                Job &operator=(const Job &) = delete;
                ~Job()
                {
                    if (_avInputImplementation != nullptr)
                    {
                        _avInputImplementation->Release();
                    }
                }

            public:
                static Core::ProxyType<Core::IDispatch> Create(AVInputImplementation *avInputImplementation, Event event, ParamsType params)
                {
#ifndef USE_THUNDER_R4
                    return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(avInputImplementation, event, params)));
#else
                    return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(avInputImplementation, event, params)));
#endif
                }

                virtual void Dispatch()
                {
                    _avInputImplementation->Dispatch(_event, _params);
                }

            private:
                AVInputImplementation *_avInputImplementation;
                const Event _event;
                ParamsType _params;
            };

        public:
            virtual Core::hresult Register(Exchange::IAVInput::IDevicesChangedNotification *notification) override;
            virtual Core::hresult Unregister(Exchange::IAVInput::IDevicesChangedNotification *notification) override;
            virtual Core::hresult Register(Exchange::IAVInput::ISignalChangedNotification *notification) override;
            virtual Core::hresult Unregister(Exchange::IAVInput::ISignalChangedNotification *notification) override;
            virtual Core::hresult Register(Exchange::IAVInput::IInputStatusChangedNotification *notification) override;
            virtual Core::hresult Unregister(Exchange::IAVInput::IInputStatusChangedNotification *notification) override;
            virtual Core::hresult Register(Exchange::IAVInput::IVideoStreamInfoUpdateNotification *notification) override;
            virtual Core::hresult Unregister(Exchange::IAVInput::IVideoStreamInfoUpdateNotification *notification) override;
            virtual Core::hresult Register(Exchange::IAVInput::IGameFeatureStatusUpdateNotification *notification) override;
            virtual Core::hresult Unregister(Exchange::IAVInput::IGameFeatureStatusUpdateNotification *notification) override;
            virtual Core::hresult Register(Exchange::IAVInput::IHdmiContentTypeUpdateNotification *notification) override;
            virtual Core::hresult Unregister(Exchange::IAVInput::IHdmiContentTypeUpdateNotification *notification) override;

            Core::hresult NumberOfInputs(uint32_t &inputCount) override;
            Core::hresult GetInputDevices(int type, Exchange::IAVInput::IInputDeviceIterator *&devices) override;
            Core::hresult WriteEDID(int id, const string &edid) override;
            Core::hresult ReadEDID(int id, string &edid) override;
            Core::hresult GetRawSPD(int id, string &spd) override;
            Core::hresult GetSPD(int id, string &spd) override;
            Core::hresult SetEdidVersion(int id, const string &version) override;
            Core::hresult GetEdidVersion(int id, string &version) override;
            Core::hresult SetEdid2AllmSupport(int id, bool allm) override;
            Core::hresult GetEdid2AllmSupport(int id, bool &allm) override;
            Core::hresult SetVRRSupport(int id, bool vrrSupport) override;
            Core::hresult GetVRRSupport(int id, bool &vrrSupport) override;
            Core::hresult SetAudioMixerLevels(MixerLevels levels) override;
            Core::hresult GetHdmiVersion(int id, string &hdmiVersion) override;
            Core::hresult StartInput(int id, int type, bool audioMix, int planeType, bool topMostPlane) override;
            Core::hresult StopInput(int type) override;
            Core::hresult SetVideoRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t type) override;
            Core::hresult CurrentVideoMode(string &currentVideoMode, string &message) override;
            Core::hresult GetSupportedGameFeatures(IStringIterator *&features) override;
            Core::hresult GetGameFeatureStatus(int id, const string &feature, bool &mode) override;
            Core::hresult ContentProtected(bool &isContentProtected) override;

            void AVInputHotplug(int input, int connect, int type);
            void AVInputSignalChange(int port, int signalStatus, int type);
            void AVInputStatusChange(int port, bool isPresented, int type);
            void AVInputVideoModeUpdate(int port, dsVideoPortResolution_t resolution, int type);
            void hdmiInputAviContentTypeChange(int port, int content_type);
            void AVInputALLMChange(int port, bool allm_mode);
            void AVInputVRRChange(int port, dsVRRType_t vrr_type, bool vrr_mode);

        private:
            mutable Core::CriticalSection _adminLock;
            PluginHost::IShell *_service;

            template <typename T>
            uint32_t Register(std::list<T *> &list, T *notification);
            template <typename T>
            uint32_t Unregister(std::list<T *> &list, T *notification);

            std::list<Exchange::IAVInput::IDevicesChangedNotification *> _devicesChangedNotifications;
            std::list<Exchange::IAVInput::ISignalChangedNotification *> _signalChangedNotifications;
            std::list<Exchange::IAVInput::IInputStatusChangedNotification *> _inputStatusChangedNotifications;
            std::list<Exchange::IAVInput::IVideoStreamInfoUpdateNotification *> _videoStreamInfoUpdateNotifications;
            std::list<Exchange::IAVInput::IGameFeatureStatusUpdateNotification *> _gameFeatureStatusUpdateNotifications;
            std::list<Exchange::IAVInput::IHdmiContentTypeUpdateNotification *> _hdmiContentTypeUpdateNotifications;

            int m_primVolume;
            int m_inputVolume; // Player Volume

            void InitializeIARM();
            void DeinitializeIARM();

            void dispatchEvent(Event, const ParamsType params);
            void Dispatch(Event event, const ParamsType params);

            static string currentVideoMode(bool &success);

            // Begin methods
            uint32_t getInputDevicesWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t writeEDIDWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t readEDIDWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getRawSPDWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getSPDWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t setEdidVersionWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getEdidVersionWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t setEdid2AllmSupportWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getEdid2AllmSupportWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t setVRRSupportWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getVRRSupportWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getVRRFrameRateWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t setAudioMixerLevelsWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t startInputWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t stopInputWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t setVideoRectangleWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getSupportedGameFeaturesWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getGameFeatureStatusWrapper(const JsonObject &parameters, JsonObject &response);
            uint32_t getHdmiVersionWrapper(const JsonObject &parameters, JsonObject &response);

            Core::hresult getInputDevices(int type, std::list<WPEFramework::Exchange::IAVInput::InputDevice> devices);
            JsonArray devicesToJson(Exchange::IAVInput::IInputDeviceIterator *devices);

            bool getALLMStatus(int iPort);
            bool getVRRStatus(int iPort, dsHdmiInVrrStatus_t *vrrStatus);
            std::string GetRawSPD(int iPort);

            static void dsAVEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVSignalStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVVideoModeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAVGameFeatureStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsAviContentTypeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

        public:
            static AVInputImplementation *_instance;
            dsVRRType_t m_currentVrrType;

            friend class Job;
        };
    } // namespace Plugin
} // namespace WPEFramework

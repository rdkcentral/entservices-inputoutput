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

#include "AVInputImplementation.h"
#include <curl/curl.h>
#include <fstream>
#include <time.h>

#include "UtilsJsonRpc.h"

#define STR_ALLM "ALLM"
#define VRR_TYPE_HDMI "VRR-HDMI"
#define VRR_TYPE_FREESYNC "VRR-FREESYNC"
#define VRR_TYPE_FREESYNC_PREMIUM "VRR-FREESYNC-PREMIUM"
#define VRR_TYPE_FREESYNC_PREMIUM_PRO "VRR-FREESYNC-PREMIUM-PRO"

#define AV_HOT_PLUG_EVENT_CONNECTED 0
#define AV_HOT_PLUG_EVENT_DISCONNECTED 1

static bool isAudioBalanceSet = false;
static int planeType = 0;

using namespace std;

namespace WPEFramework {
namespace Plugin {
    SERVICE_REGISTRATION(AVInputImplementation, 1, 0);
    AVInputImplementation* AVInputImplementation::_instance = nullptr;

    AVInputImplementation::AVInputImplementation()
        : _adminLock()
        , _service(nullptr)
    {
        LOGINFO("Create AVInputImplementation Instance");
        AVInputImplementation::_instance = this;
        InitializeIARM();
    }

    AVInputImplementation::~AVInputImplementation()
    {
        DeinitializeIARM();

        AVInputImplementation::_instance = nullptr;
        _service = nullptr;
    }

    void AVInputImplementation::InitializeIARM()
    {
        if (Utils::IARM::init()) {
            IARM_Result_t res;
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG,
                dsAVEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS,
                dsAVSignalStatusEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS,
                dsAVStatusEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE,
                dsAVVideoModeEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS,
                dsAVGameFeatureStatusEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS,
                dsAVGameFeatureStatusEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG,
                dsAVEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS,
                dsAVSignalStatusEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS,
                dsAVStatusEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE,
                dsAVVideoModeEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE,
                dsAviContentTypeEventHandler));
        }
    }

    void AVInputImplementation::DeinitializeIARM()
    {
        if (Utils::IARM::isConnected()) {
            IARM_Result_t res;
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG, dsAVEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, dsAVSignalStatusEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS, dsAVStatusEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, dsAVVideoModeEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS, dsAVGameFeatureStatusEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS, dsAVGameFeatureStatusEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG, dsAVEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, dsAVSignalStatusEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS, dsAVStatusEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE, dsAVVideoModeEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE, dsAviContentTypeEventHandler));
        }
    }

    void AVInputImplementation::dsAviContentTypeEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        if (!AVInputImplementation::_instance)
            return;

        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE == eventId) {
            IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
            int hdmi_in_port = eventData->data.hdmi_in_content_type.port;
            int avi_content_type = eventData->data.hdmi_in_content_type.aviContentType;
            LOGINFO("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE  event  port: %d, Content Type : %d", hdmi_in_port, avi_content_type);

            AVInputImplementation::_instance->hdmiInputAviContentTypeChange(hdmi_in_port, avi_content_type);
        }
    }

    void AVInputImplementation::dsAVEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        if (!AVInputImplementation::_instance)
            return;

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG == eventId) {
            int hdmiin_hotplug_port = eventData->data.hdmi_in_connect.port;
            int hdmiin_hotplug_conn = eventData->data.hdmi_in_connect.isPortConnected;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG  event data:%d", hdmiin_hotplug_port);
            AVInputImplementation::_instance->AVInputHotplug(hdmiin_hotplug_port, hdmiin_hotplug_conn ? AV_HOT_PLUG_EVENT_CONNECTED : AV_HOT_PLUG_EVENT_DISCONNECTED, HDMI);
        } else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG == eventId) {
            int compositein_hotplug_port = eventData->data.composite_in_connect.port;
            int compositein_hotplug_conn = eventData->data.composite_in_connect.isPortConnected;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG  event data:%d", compositein_hotplug_port);
            AVInputImplementation::_instance->AVInputHotplug(compositein_hotplug_port, compositein_hotplug_conn ? AV_HOT_PLUG_EVENT_CONNECTED : AV_HOT_PLUG_EVENT_DISCONNECTED, COMPOSITE);
        }
    }

    void AVInputImplementation::dsAVSignalStatusEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        if (!AVInputImplementation::_instance)
            return;
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS == eventId) {
            int hdmi_in_port = eventData->data.hdmi_in_sig_status.port;
            int hdmi_in_signal_status = eventData->data.hdmi_in_sig_status.status;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS  event  port: %d, signal status: %d", hdmi_in_port, hdmi_in_signal_status);
            AVInputImplementation::_instance->AVInputSignalChange(hdmi_in_port, hdmi_in_signal_status, HDMI);
        } else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS == eventId) {
            int composite_in_port = eventData->data.composite_in_sig_status.port;
            int composite_in_signal_status = eventData->data.composite_in_sig_status.status;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS  event  port: %d, signal status: %d", composite_in_port, composite_in_signal_status);
            AVInputImplementation::_instance->AVInputSignalChange(composite_in_port, composite_in_signal_status, COMPOSITE);
        }
    }

    void AVInputImplementation::dsAVStatusEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        if (!AVInputImplementation::_instance)
            return;
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS == eventId) {
            int hdmi_in_port = eventData->data.hdmi_in_status.port;
            bool hdmi_in_status = eventData->data.hdmi_in_status.isPresented;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS  event  port: %d, started: %d", hdmi_in_port, hdmi_in_status);
            AVInputImplementation::_instance->AVInputStatusChange(hdmi_in_port, hdmi_in_status, HDMI);
        } else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS == eventId) {
            int composite_in_port = eventData->data.composite_in_status.port;
            bool composite_in_status = eventData->data.composite_in_status.isPresented;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS  event  port: %d, started: %d", composite_in_port, composite_in_status);
            AVInputImplementation::_instance->AVInputStatusChange(composite_in_port, composite_in_status, COMPOSITE);
        }
    }

    void AVInputImplementation::dsAVVideoModeEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        if (!AVInputImplementation::_instance)
            return;

        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE == eventId) {
            IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
            int hdmi_in_port = eventData->data.hdmi_in_video_mode.port;
            dsVideoPortResolution_t resolution = {};
            resolution.pixelResolution = eventData->data.hdmi_in_video_mode.resolution.pixelResolution;
            resolution.interlaced = eventData->data.hdmi_in_video_mode.resolution.interlaced;
            resolution.frameRate = eventData->data.hdmi_in_video_mode.resolution.frameRate;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE  event  port: %d, pixelResolution: %d, interlaced : %d, frameRate: %d \n", hdmi_in_port, resolution.pixelResolution, resolution.interlaced, resolution.frameRate);
            AVInputImplementation::_instance->AVInputVideoModeUpdate(hdmi_in_port, resolution, HDMI);
        } else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE == eventId) {
            IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
            int composite_in_port = eventData->data.composite_in_video_mode.port;
            dsVideoPortResolution_t resolution = {};
            resolution.pixelResolution = eventData->data.composite_in_video_mode.resolution.pixelResolution;
            resolution.interlaced = eventData->data.composite_in_video_mode.resolution.interlaced;
            resolution.frameRate = eventData->data.composite_in_video_mode.resolution.frameRate;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE  event  port: %d, pixelResolution: %d, interlaced : %d, frameRate: %d \n", composite_in_port, resolution.pixelResolution, resolution.interlaced, resolution.frameRate);
            AVInputImplementation::_instance->AVInputVideoModeUpdate(composite_in_port, resolution, COMPOSITE);
        }
    }

    void AVInputImplementation::dsAVGameFeatureStatusEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        if (!AVInputImplementation::_instance)
            return;

        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS == eventId) {
            IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
            int hdmi_in_port = eventData->data.hdmi_in_allm_mode.port;
            bool allm_mode = eventData->data.hdmi_in_allm_mode.allm_mode;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS  event  port: %d, ALLM Mode: %d", hdmi_in_port, allm_mode);

            AVInputImplementation::_instance->AVInputALLMChange(hdmi_in_port, allm_mode);
        }
        if (IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS == eventId) {
            IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
            int hdmi_in_port = eventData->data.hdmi_in_vrr_mode.port;
            dsVRRType_t new_vrrType = eventData->data.hdmi_in_vrr_mode.vrr_type;
            LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS  event  port: %d, VRR Type: %d", hdmi_in_port, new_vrrType);

            if (new_vrrType == dsVRR_NONE) {
                if (AVInputImplementation::_instance->m_currentVrrType != dsVRR_NONE) {
                    AVInputImplementation::_instance->AVInputVRRChange(hdmi_in_port, AVInputImplementation::_instance->m_currentVrrType, false);
                }
            } else {
                if (AVInputImplementation::_instance->m_currentVrrType != dsVRR_NONE) {
                    AVInputImplementation::_instance->AVInputVRRChange(hdmi_in_port, AVInputImplementation::_instance->m_currentVrrType, false);
                }
                AVInputImplementation::_instance->AVInputVRRChange(hdmi_in_port, new_vrrType, true);
            }
            AVInputImplementation::_instance->m_currentVrrType = new_vrrType;
        }
    }

    template <typename T>
    Core::hresult AVInputImplementation::Register(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _adminLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(list.begin(), list.end(), notification) == list.end()) {
            list.push_back(notification);
            notification->AddRef();
            status = Core::ERROR_NONE;
        }

        _adminLock.Unlock();
        return status;
    }

    template <typename T>
    Core::hresult AVInputImplementation::Unregister(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _adminLock.Lock();

        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(list.begin(), list.end(), notification);
        if (itr != list.end()) {
            (*itr)->Release();
            list.erase(itr);
            status = Core::ERROR_NONE;
        }

        _adminLock.Unlock();
        return status;
    }

    Core::hresult AVInputImplementation::Register(Exchange::IAVInput::IDevicesChangedNotification* notification)
    {
        Core::hresult errorCode = Register(_devicesChangedNotifications, notification);
        LOGINFO("IDevicesChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::IDevicesChangedNotification* notification)
    {
        Core::hresult errorCode = Unregister(_devicesChangedNotifications, notification);
        LOGINFO("IDevicesChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Register(Exchange::IAVInput::ISignalChangedNotification* notification)
    {
        Core::hresult errorCode = Register(_signalChangedNotifications, notification);
        LOGINFO("ISignalChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::ISignalChangedNotification* notification)
    {
        Core::hresult errorCode = Unregister(_signalChangedNotifications, notification);
        LOGINFO("ISignalChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Register(Exchange::IAVInput::IInputStatusChangedNotification* notification)
    {
        Core::hresult errorCode = Register(_inputStatusChangedNotifications, notification);
        LOGINFO("IInputStatusChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::IInputStatusChangedNotification* notification)
    {
        Core::hresult errorCode = Unregister(_inputStatusChangedNotifications, notification);
        LOGINFO("IInputStatusChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Register(Exchange::IAVInput::IVideoStreamInfoUpdateNotification* notification)
    {
        Core::hresult errorCode = Register(_videoStreamInfoUpdateNotifications, notification);
        LOGINFO("IVideoStreamInfoUpdateNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::IVideoStreamInfoUpdateNotification* notification)
    {
        Core::hresult errorCode = Unregister(_videoStreamInfoUpdateNotifications, notification);
        LOGINFO("IVideoStreamInfoUpdateNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Register(Exchange::IAVInput::IGameFeatureStatusUpdateNotification* notification)
    {
        Core::hresult errorCode = Register(_gameFeatureStatusUpdateNotifications, notification);
        LOGINFO("IGameFeatureStatusUpdateNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::IGameFeatureStatusUpdateNotification* notification)
    {
        Core::hresult errorCode = Unregister(_gameFeatureStatusUpdateNotifications, notification);
        LOGINFO("IGameFeatureStatusUpdateNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Register(Exchange::IAVInput::IHdmiContentTypeUpdateNotification* notification)
    {
        Core::hresult errorCode = Register(_hdmiContentTypeUpdateNotifications, notification);
        LOGINFO("IHdmiContentTypeUpdateNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::IHdmiContentTypeUpdateNotification* notification)
    {
        Core::hresult errorCode = Unregister(_hdmiContentTypeUpdateNotifications, notification);
        LOGINFO("IHdmiContentTypeUpdateNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    void AVInputImplementation::dispatchEvent(Event event, const ParamsType params)
    {
        Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
    }

    void AVInputImplementation::Dispatch(Event event, const ParamsType params)
    {
        using namespace WPEFramework::Exchange;

        _adminLock.Lock();

        switch (event) {
        case ON_AVINPUT_DEVICES_CHANGED: {

            if (auto* const devices = boost::get<Exchange::IAVInput::IInputDeviceIterator* const>(&params)) {
                LOGINFO("ON_AVINPUT_DEVICES_CHANGED");

                std::list<IAVInput::IDevicesChangedNotification*>::const_iterator index(_devicesChangedNotifications.begin());

                while (index != _devicesChangedNotifications.end()) {
                    (*index)->OnDevicesChanged(*devices);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_SIGNAL_CHANGED: {

            if (const auto* tupleValue = boost::get<std::tuple<int, string, string>>(&params)) {
                int id = std::get<0>(*tupleValue);
                string locator = std::get<1>(*tupleValue);
                string signalStatus = std::get<2>(*tupleValue);

                std::list<IAVInput::ISignalChangedNotification*>::const_iterator index(_signalChangedNotifications.begin());

                while (index != _signalChangedNotifications.end()) {
                    (*index)->OnSignalChanged(id, locator, signalStatus);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_STATUS_CHANGED: {

            if (const auto* tupleValue = boost::get<std::tuple<int, string, string, int>>(&params)) {
                int id = std::get<0>(*tupleValue);
                string locator = std::get<1>(*tupleValue);
                string status = std::get<2>(*tupleValue);
                int plane = std::get<3>(*tupleValue);

                std::list<IAVInput::IInputStatusChangedNotification*>::const_iterator index(_inputStatusChangedNotifications.begin());

                while (index != _inputStatusChangedNotifications.end()) {
                    (*index)->OnInputStatusChanged(id, locator, status, plane);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE: {
            if (const auto* tupleValue = boost::get<std::tuple<int, string, int, int, bool, int, int>>(&params)) {
                int id = std::get<0>(*tupleValue);
                string locator = std::get<1>(*tupleValue);
                int width = std::get<2>(*tupleValue);
                int height = std::get<3>(*tupleValue);
                bool progressive = std::get<4>(*tupleValue);
                int frameRateN = std::get<5>(*tupleValue);
                int frameRateD = std::get<6>(*tupleValue);

                std::list<IAVInput::IVideoStreamInfoUpdateNotification*>::const_iterator index(_videoStreamInfoUpdateNotifications.begin());

                while (index != _videoStreamInfoUpdateNotifications.end()) {
                    (*index)->VideoStreamInfoUpdate(id, locator, width, height, progressive, frameRateN, frameRateD);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE: {
            if (const auto* tupleValue = boost::get<std::tuple<int, string, bool>>(&params)) {
                int id = std::get<0>(*tupleValue);
                string gameFeature = std::get<1>(*tupleValue);
                bool mode = std::get<2>(*tupleValue);

                std::list<IAVInput::IGameFeatureStatusUpdateNotification*>::const_iterator index(_gameFeatureStatusUpdateNotifications.begin());

                while (index != _gameFeatureStatusUpdateNotifications.end()) {
                    (*index)->GameFeatureStatusUpdate(id, gameFeature, mode);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE: {
            if (const auto* tupleValue = boost::get<std::tuple<int, int>>(&params)) {
                int id = std::get<0>(*tupleValue);
                int aviContentType = std::get<1>(*tupleValue);

                std::list<IHdmiContentTypeUpdateNotification*>::const_iterator index(_hdmiContentTypeUpdateNotifications.begin());

                while (index != _hdmiContentTypeUpdateNotifications.end()) {
                    (*index)->HdmiContentTypeUpdate(id, aviContentType);
                    ++index;
                }
            }
            break;
        }

        default: {
            LOGWARN("Event[%u] not handled", event);
            break;
        }
        }
        _adminLock.Unlock();
    }

    Core::hresult AVInputImplementation::ContentProtected(bool& isContentProtected, bool& success)
    {
        // "This is the way it's done in Service Manager"
        isContentProtected = true;
        success = true;
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::NumberOfInputs(uint32_t& numberOfInputs, bool& success)
    {
        try {
            numberOfInputs = device::HdmiInput::getInstance().getNumberOfInputs();
            success = true;
        } catch (...) {
            LOGERR("Exception caught");
            success = false;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::CurrentVideoMode(string& currentVideoMode, bool& success)
    {
        try {
            currentVideoMode = device::HdmiInput::getInstance().getCurrentVideoMode();
            success = true;
        } catch (...) {
            LOGERR("Exception caught");
            success = false;
        }

        return Core::ERROR_NONE;
    }
    Core::hresult AVInputImplementation::StartInput(const int portId, const string& typeOfInput, const bool requestAudioMix, const int plane, const bool topMost, SuccessResult& successResult)
    {
        successResult.success = true;

        try {
            if (strcmp(typeOfInput.c_str(), INPUT_TYPE_HDMI) == 0)
                device::HdmiInput::getInstance().selectPort(portId, requestAudioMix, plane, topMost);
            } else if (strcmp(typeOfInput.c_str(), INPUT_TYPE_COMPOSITE) == 0) {
                device::CompositeInput::getInstance().selectPort(portId);
            } else {
                LOGWARN("Invalid input type passed to StartInput");
                successResult.success = false;
            }
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            successResult.success = false;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::StopInput(const string& typeOfInput, SuccessResult& successResult)
    {
        Core::hresult ret = Core::ERROR_NONE;
        successResult.success = true;

        try {
            planeType = -1;
            if (isAudioBalanceSet) {
                device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, MAX_PRIM_VOL_LEVEL);
                device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, DEFAULT_INPUT_VOL_LEVEL);
                isAudioBalanceSet = false;
            }
            if (strcmp(typeOfInput.c_str(), INPUT_TYPE_HDMI) == 0) {
                device::HdmiInput::getInstance().selectPort(-1);
            } else if (strcmp(typeOfInput.c_str(), INPUT_TYPE_COMPOSITE) == 0) {
                device::CompositeInput::getInstance().selectPort(-1);
            } else {
                LOGWARN("Invalid input type passed to StopInput");
                successResult.success = false;
                ret = Core::ERROR_GENERAL;
            }
        } catch (const device::Exception& err) {
            LOGWARN("AVInputImplementation::StopInput Failed");
            successResult.success = false;
            ret = Core::ERROR_GENERAL;
        }

        return ret;
    }

    Core::hresult AVInputImplementation::SetVideoRectangle(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const string& typeOfInput, SuccessResult& successResult)
    {
        successResult.success = true;

        try {
            if (strcmp(typeOfInput.c_str(), INPUT_TYPE_HDMI) == 0) {
                device::HdmiInput::getInstance().scaleVideo(x, y, w, h);
            } else {
                device::CompositeInput::getInstance().scaleVideo(x, y, w, h);
            }
        } catch (const device::Exception& err) {
            successResult.success = false;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::getInputDevices(const string& typeOfInput, std::list<WPEFramework::Exchange::IAVInput::InputDevice> &inputDeviceList)
    {
        int num = 0;
        bool isHdmi = true;

        try {
            if (strcmp(typeOfInput.c_str(), INPUT_TYPE_HDMI) == 0) {
                num = device::HdmiInput::getInstance().getNumberOfInputs();
            } else if (strcmp(typeOfInput.c_str(), INPUT_TYPE_COMPOSITE) == 0) {
                num = device::CompositeInput::getInstance().getNumberOfInputs();
                isHdmi = false;
            } else {
                LOGERR("getInputDevices: Invalid input type");
                return Core::ERROR_GENERAL;
            }

            if (num > 0) {
                int i = 0;
                for (i = 0; i < num; i++) {
                    // Input ID is aleays 0-indexed, continuous number starting 0
                    WPEFramework::Exchange::IAVInput::InputDevice inputDevice;

                    inputDevice.id = i;
                    std::stringstream locator;
                    if (isHdmi) {
                        locator << "hdmiin://localhost/deviceid/" << i;
                        inputDevice.connected = device::HdmiInput::getInstance().isPortConnected(i);
                    } else {
                        locator << "cvbsin://localhost/deviceid/" << i;
                        inputDevice.connected = device::CompositeInput::getInstance().isPortConnected(i);
                    }
                    inputDevice.locator = locator.str();
                    LOGINFO("getInputDevices id %d, locator=[%s], connected=[%d]", i, inputDevice.locator.c_str(), inputDevice.connected);
                    inputDeviceList.push_back(inputDevice);
                }
            }
        } catch (const std::exception& e) {
            LOGERR("AVInputService::getInputDevices Failed");
            return Core::ERROR_GENERAL;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetInputDevices(const string& typeOfInput, IInputDeviceIterator*& devices, bool& success)
    {
        Core::hresult result;
        std::list<WPEFramework::Exchange::IAVInput::InputDevice> inputDeviceList;
        success = false;

        if(strcmp(typeOfInput.c_str(), INPUT_TYPE_ALL) == 0) {
            result = getInputDevices(HDMI, inputDeviceList);
            if (result == Core::ERROR_NONE) {
                result = getInputDevices(COMPOSITE, inputDeviceList);
            }
        } else if((strcmp(typeOfInput.c_str(), INPUT_TYPE_HDMI) == 0) || (strcmp(typeOfInput.c_str(), INPUT_TYPE_COMPOSITE) == 0)) {
            result = getInputDevices(typeOfInput, inputDeviceList);
        } else {
            LOGERR("GetInputDevices: Invalid input type");
            return Core::ERROR_NONE;
        }

        if(Core::ERROR_NONE == result) {
            devices = Core::Service<RPC::IteratorType<IInputDeviceIterator>>::Create<IInputDeviceIterator>(inputDeviceList);
            success = true;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::WriteEDID(const int portId, const string& message, SuccessResult& successResult)
    {
        // TODO: This wasn't implemented in the original code, do we want to implement it?
        successResult.success = true;
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::ReadEDID(const int portId, string& EDID, bool& success)
    {
        vector<uint8_t> edidVec({ 'u', 'n', 'k', 'n', 'o', 'w', 'n' });

        try {
            vector<uint8_t> edidVec2;
            device::HdmiInput::getInstance().getEDIDBytesInfo(portId, edidVec2);
            edidVec = edidVec2; // edidVec must be "unknown" unless we successfully get to this line

            // convert to base64
            uint16_t size = min(edidVec.size(), (size_t)numeric_limits<uint16_t>::max());

            LOGWARN("AVInputImplementation::readEDID size:%d edidVec.size:%zu", size, edidVec.size());
            if (edidVec.size() > (size_t)numeric_limits<uint16_t>::max()) {
                LOGERR("Size too large to use ToString base64 wpe api");
                success = false;
                return Core::ERROR_NONE;
            }
            Core::ToString((uint8_t*)&edidVec[0], size, true, EDID);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            success = false;
            return Core::ERROR_NONE;
        }

        success = true;
        return Core::ERROR_NONE;
    }

    /**
     * @brief This function is used to translate HDMI/COMPOSITE input hotplug to
     * deviceChanged event.
     *
     * @param[in] input Number of input port integer.
     * @param[in] connection status of input port integer.
     */
    void AVInputImplementation::AVInputHotplug(int input, int connect, int type)
    {
        LOGWARN("AVInputHotplug [%d, %d, %d]", input, connect, type);

        IInputDeviceIterator* devices;
        bool success;

        string typeOfInput;
        switch(type) {
            case HDMI:
                typeOfInput = INPUT_TYPE_HDMI;
                break;
            case COMPOSITE:
                typeOfInput = INPUT_TYPE_COMPOSITE;
                break;
            case ALL:
                typeOfInput = INPUT_TYPE_ALL;
                break;
            default:
                LOGERR("AVInputHotplug: Invalid input type");
                return;
        }

        Core::hresult result = GetInputDevices(typeOfInput, devices, success);
        if (Core::ERROR_NONE != result) {
            LOGERR("AVInputHotplug [%d, %d, %d]: Failed to get devices", input, connect, type);
            return;
        }

        ParamsType params = devices;
        dispatchEvent(ON_AVINPUT_DEVICES_CHANGED, params);
    }

    /**
     * @brief This function is used to translate HDMI/COMPOSITE input signal change to
     * signalChanged event.
     *
     * @param[in] port HDMI/COMPOSITE In port id.
     * @param[in] signalStatus signal status of HDMI/COMPOSITE In port.
     * @param[in] type HDMI/COMPOSITE In type.
     */
    void AVInputImplementation::AVInputSignalChange(int port, int signalStatus, int type)
    {
        LOGWARN("AVInputSignalStatus [%d, %d, %d]", port, signalStatus, type);

        string signalStatusStr;

        std::stringstream locator;
        if (type == HDMI) {
            locator << "hdmiin://localhost/deviceid/" << port;
        } else {
            locator << "cvbsin://localhost/deviceid/" << port;
        }

        /* values of dsHdmiInSignalStatus_t and dsCompInSignalStatus_t are same
       Hence used only HDMI macro for case statement */
        switch (signalStatus) {
        case dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL:
            signalStatusStr = "noSignal";
            break;

        case dsHDMI_IN_SIGNAL_STATUS_UNSTABLE:
            signalStatusStr = "unstableSignal";
            break;

        case dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED:
            signalStatusStr = "notSupportedSignal";
            break;

        case dsHDMI_IN_SIGNAL_STATUS_STABLE:
            signalStatusStr = "stableSignal";
            break;

        default:
            signalStatusStr = "none";
            break;
        }

        ParamsType params = std::make_tuple(port, locator.str(), signalStatusStr);
        dispatchEvent(ON_AVINPUT_SIGNAL_CHANGED, params);
    }

    /**
     * @brief This function is used to translate HDMI/COMPOSITE input status change to
     * inputStatusChanged event.
     *
     * @param[in] port HDMI/COMPOSITE In port id.
     * @param[in] isPresented HDMI/COMPOSITE In presentation started/stopped.
     * @param[in] type HDMI/COMPOSITE In type.
     */
    void AVInputImplementation::AVInputStatusChange(int port, bool isPresented, int type)
    {
        LOGWARN("avInputStatus [%d, %d, %d]", port, isPresented, type);

        std::stringstream locator;

        if (type == HDMI) {
            locator << "hdmiin://localhost/deviceid/" << port;
        } else if (type == COMPOSITE) {
            locator << "cvbsin://localhost/deviceid/" << port;
        }

        string status = isPresented ? "started" : "stopped";
        ParamsType params = std::make_tuple(port, locator.str(), status, planeType);
        dispatchEvent(ON_AVINPUT_STATUS_CHANGED, params);
    }

    /**
     * @brief This function is used to translate HDMI input video mode change to
     * videoStreamInfoUpdate event.
     *
     * @param[in] port HDMI In port id.
     * @param[in] resolution resolution of HDMI In port.
     * @param[in] type HDMI/COMPOSITE In type.
     */
        void AVInputImplementation::AVInputVideoModeUpdate(int port, dsVideoPortResolution_t resolution, int type)
    {
        int width;
        int height;
        bool progressive;
        int frameRateN;
        int frameRateD;

        std::stringstream locator;

        LOGWARN("AVInputVideoModeUpdate [%d]", port);

        if (type == HDMI) {
            locator << "hdmiin://localhost/deviceid/" << port;

            switch (resolution.pixelResolution) {

            case dsVIDEO_PIXELRES_720x480:
                width = 720;
                height = 480;
                break;

            case dsVIDEO_PIXELRES_720x576:
                width = 720;
                height = 576;
                break;

            case dsVIDEO_PIXELRES_1280x720:
                width = 1280;
                height = 720;
                break;

            case dsVIDEO_PIXELRES_1920x1080:
                width = 1920;
                height = 1080;
                break;

            case dsVIDEO_PIXELRES_3840x2160:
                width = 3840;
                height = 2160;
                break;

            case dsVIDEO_PIXELRES_4096x2160:
                width = 4096;
                height = 2160;
                break;

            default:
                width = 1920;
                height = 1080;
                break;
            }

            progressive = (!resolution.interlaced);

        } else if (type == COMPOSITE) {
            locator << "cvbsin://localhost/deviceid/" << port;

            switch (resolution.pixelResolution) {
            case dsVIDEO_PIXELRES_720x480:
                width = 720;
                height = 480;
                break;

            case dsVIDEO_PIXELRES_720x576:
                width = 720;
                height = 576;
                break;

            default:
                width = 720;
                height = 576;
                break;
            }

            progressive = false;
        }

        switch (resolution.frameRate) {
        case dsVIDEO_FRAMERATE_24:
            frameRateN = 24000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_25:
            frameRateN = 25000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_30:
            frameRateN = 30000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_50:
            frameRateN = 50000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_60:
            frameRateN = 60000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_23dot98:
            frameRateN = 24000;
            frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_29dot97:
            frameRateN = 30000;
            frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_59dot94:
            frameRateN = 60000;
            frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_100:
            frameRateN = 100000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_119dot88:
            frameRateN = 120000;
            frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_120:
            frameRateN = 120000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_200:
            frameRateN = 200000;
            frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_239dot76:
            frameRateN = 240000;
            frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_240:
            frameRateN = 240000;
            frameRateD = 1000;
            break;

        default:
            frameRateN = 60000;
            frameRateD = 1000;
            break;
        }

        ParamsType params = std::make_tuple(port, locator.str(), width, height, progressive, frameRateN, frameRateD);
        dispatchEvent(ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE, params);
    }


    void AVInputImplementation::hdmiInputAviContentTypeChange(int port, int content_type)
    {
        ParamsType params = std::make_tuple(port, content_type);
        dispatchEvent(ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE, params);
    }

    void AVInputImplementation::AVInputALLMChange(int port, bool allm_mode)
    {
        ParamsType params = std::make_tuple(port, STR_ALLM, allm_mode);
        dispatchEvent(ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE, params);
    }

    void AVInputImplementation::AVInputVRRChange(int port, dsVRRType_t vrr_type, bool vrr_mode)
    {
        string gameFeature;

        switch (vrr_type) {
        case dsVRR_HDMI_VRR:
            gameFeature = VRR_TYPE_HDMI;
            break;
        case dsVRR_AMD_FREESYNC:
            gameFeature = VRR_TYPE_FREESYNC;
            break;
        case dsVRR_AMD_FREESYNC_PREMIUM:
            gameFeature = VRR_TYPE_FREESYNC_PREMIUM;
            break;
        case dsVRR_AMD_FREESYNC_PREMIUM_PRO:
            gameFeature = VRR_TYPE_FREESYNC_PREMIUM_PRO;
            break;
        default:
            break;
        }

        ParamsType params = std::make_tuple(port, gameFeature, vrr_mode);
        dispatchEvent(ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE, params);
    }

    Core::hresult AVInputImplementation::GetSupportedGameFeatures(IStringIterator*& features, bool& success)
    {
        Core::hresult result = Core::ERROR_NONE;
        success = true;
        features = nullptr;
        std::vector<std::string> supportedFeatures;
        try {
            device::HdmiInput::getInstance().getSupportedGameFeatures(supportedFeatures);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION0();
            success = false;
            result = Core::ERROR_GENERAL;
        }

        if (!supportedFeatures.empty() && result == Core::ERROR_NONE) {
            features = Core::Service<RPC::IteratorType<IStringIterator>>::Create<IStringIterator>(supportedFeatures);
        } else {
            success = false;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetGameFeatureStatus(const int portId, const string& gameFeature, bool& mode, bool& success)
    {
        if (gameFeature == STR_ALLM) {
            mode = getALLMStatus(portId);
        } else if (gameFeature == VRR_TYPE_HDMI) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(portId, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_HDMI_VRR);
        } else if (gameFeature == VRR_TYPE_FREESYNC) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(portId, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_AMD_FREESYNC);
        } else if (gameFeature == VRR_TYPE_FREESYNC_PREMIUM) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(portId, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_AMD_FREESYNC_PREMIUM);
        } else if (gameFeature == VRR_TYPE_FREESYNC_PREMIUM_PRO) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(portId, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_AMD_FREESYNC_PREMIUM_PRO);
        } else {
            LOGWARN("AVInputImplementation::GetGameFeatureStatus Unsupported feature: %s", gameFeature.c_str());
            success = false;
            return Core::ERROR_NOT_SUPPORTED;
        }

        success = true;
        return Core::ERROR_NONE;
    }

    bool AVInputImplementation::getALLMStatus(int iPort)
    {
        bool allm = false;

        try {
            device::HdmiInput::getInstance().getHdmiALLMStatus(iPort, &allm);
            LOGWARN("AVInputImplementation::getALLMStatus ALLM MODE: %d", allm);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
        }
        return allm;
    }

    bool AVInputImplementation::getVRRStatus(int iPort, dsHdmiInVrrStatus_t* vrrStatus)
    {
        bool ret = true;
        try {
            device::HdmiInput::getInstance().getVRRStatus(iPort, vrrStatus);
            LOGWARN("AVInputImplementation::getVRRStatus VRR TYPE: %d, VRR FRAMERATE: %f", vrrStatus->vrrType, vrrStatus->vrrAmdfreesyncFramerate_Hz);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
            ret = false;
        }
        return ret;
    }

    // <pca>
    Core::hresult AVInputImplementation::GetVRRFrameRate(const int portId, double& currentVRRVideoFrameRate, bool& success)
    {
        dsHdmiInVrrStatus_t vrrStatus;
        vrrStatus.vrrAmdfreesyncFramerate_Hz = 0;

        success = getVRRStatus(portId, &vrrStatus);
        if(success == true)
        {
            currentVRRVideoFrameRate = vrrStatus.vrrAmdfreesyncFramerate_Hz;
        }

        return Core::ERROR_NONE;
    }
    // </pca>

    Core::hresult AVInputImplementation::GetRawSPD(const int portId, string& HDMISPD, bool& success)
    {
        LOGINFO("AVInputImplementation::getSPDInfo");
        vector<uint8_t> spdVect({ 'u', 'n', 'k', 'n', 'o', 'w', 'n' });
        HDMISPD.clear();
        try {
            LOGWARN("AVInputImplementation::getSPDInfo");
            vector<uint8_t> spdVect2;
            device::HdmiInput::getInstance().getHDMISPDInfo(portId, spdVect2);
            spdVect = spdVect2; // spdVect must be "unknown" unless we successfully get to this line

            // convert to base64
            uint16_t size = min(spdVect.size(), (size_t)numeric_limits<uint16_t>::max());

            LOGWARN("AVInputImplementation::getSPD size:%d spdVec.size:%zu", size, spdVect.size());

            if (spdVect.size() > (size_t)numeric_limits<uint16_t>::max()) {
                LOGERR("Size too large to use ToString base64 wpe api");
                success = false;
                return Core::ERROR_NONE;
            }

            LOGINFO("------------getSPD: ");
            for (size_t itr = 0; itr < spdVect.size(); itr++) {
                LOGINFO("%02X ", spdVect[itr]);
            }
            Core::ToString((uint8_t*)&spdVect[0], size, false, HDMISPD);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            success = false;
            return Core::ERROR_NONE;
        }

        success = true;
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetSPD(const int portId, string& HDMISPD, bool& success)
    {
        vector<uint8_t> spdVect({ 'u', 'n', 'k', 'n', 'o', 'w', 'n' });

        LOGINFO("AVInputImplementation::getSPDInfo");

        try {
            LOGWARN("AVInputImplementation::getSPDInfo");
            vector<uint8_t> spdVect2;
            device::HdmiInput::getInstance().getHDMISPDInfo(portId, spdVect2);
            spdVect = spdVect2; // edidVec must be "unknown" unless we successfully get to this line

            // convert to base64
            uint16_t size = min(spdVect.size(), (size_t)numeric_limits<uint16_t>::max());

            LOGWARN("AVInputImplementation::getSPD size:%d spdVec.size:%zu", size, spdVect.size());

            if (spdVect.size() > (size_t)numeric_limits<uint16_t>::max()) {
                LOGERR("Size too large to use ToString base64 wpe api");
                success = false;
                return Core::ERROR_NONE;
            }

            LOGINFO("------------getSPD: ");
            for (size_t itr = 0; itr < spdVect.size(); itr++) {
                LOGINFO("%02X ", spdVect[itr]);
            }

            if (spdVect.size() > 0) {
                struct dsSpd_infoframe_st pre;
                memcpy(&pre, spdVect.data(), sizeof(struct dsSpd_infoframe_st));

                char str[200] = { 0 };
                snprintf(str, sizeof(str), "Packet Type:%02X,Version:%u,Length:%u,vendor name:%s,product des:%s,source info:%02X",
                    pre.pkttype, pre.version, pre.length, pre.vendor_name, pre.product_des, pre.source_info);
                HDMISPD = str;
            }
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            success = false;
            return Core::ERROR_NONE;
        }

        success = true;
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::SetMixerLevels(const int primaryVolume, const int inputVolume, SuccessResult& successResult)
    {
        try {
            device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, primaryVolume);
            device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, inputVolume);
        } catch (...) {
            LOGWARN("Not setting SoC volume !!!\n");
            successResult.success = false;
            return Core::ERROR_NONE;
        }

        isAudioBalanceSet = true;
        successResult.success = true;
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::SetEdid2AllmSupport(const int portId, const bool allmSupport, SuccessResult& successResult)
    {
        successResult.success = true;

        try {
            device::HdmiInput::getInstance().setEdid2AllmSupport(portId, allmSupport);
            LOGWARN("AVInput -  allmsupport:%d", allmSupport);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            successResult.success = false;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetEdid2AllmSupport(const int portId, bool& allmSupport, bool& success)
    {
        try {
            device::HdmiInput::getInstance().getEdid2AllmSupport(portId, &allmSupport);
            success = true;
            LOGINFO("AVInput - getEdid2AllmSupport:%d", allmSupport);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            success = false;
        }
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetVRRSupport(const int portId, bool& vrrSupport)
    {
        Core::hresult ret = Core::ERROR_NONE;

        try {
            device::HdmiInput::getInstance().getVRRSupport(portId, &vrrSupport);
            LOGINFO("AVInput - getVRRSupport:%d", vrrSupport);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    Core::hresult AVInputImplementation::SetVRRSupport(const int portId, const bool vrrSupport)
    {
        Core::hresult ret = Core::ERROR_NONE;
        try {
            device::HdmiInput::getInstance().setVRRSupport(portId, vrrSupport);
            LOGWARN("AVInput -  vrrSupport:%d", vrrSupport);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    Core::hresult AVInputImplementation::GetHdmiVersion(const int portId, string& HdmiCapabilityVersion, bool& success)
    {
        dsHdmiMaxCapabilityVersion_t hdmiCapVersion = HDMI_COMPATIBILITY_VERSION_14;

        try {
            device::HdmiInput::getInstance().getHdmiVersion(portId, &hdmiCapVersion);
            LOGWARN("AVInputImplementation::GetHdmiVersion Hdmi Version:%d", hdmiCapVersion);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            return Core::ERROR_NONE;
        }

        switch ((int)hdmiCapVersion) {
        case HDMI_COMPATIBILITY_VERSION_14:
            HdmiCapabilityVersion = "1.4";
            success = true;
            break;
        case HDMI_COMPATIBILITY_VERSION_20:
            HdmiCapabilityVersion = "2.0";
            success = true;
            break;
        case HDMI_COMPATIBILITY_VERSION_21:
            HdmiCapabilityVersion = "2.1";
            success = true;
            break;
        default:
            success = false;
            return Core::ERROR_NONE;
        }

        if (hdmiCapVersion == HDMI_COMPATIBILITY_VERSION_MAX) {
            success = false;
            return Core::ERROR_NONE;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::SetEdidVersion(const int portId, const string& edidVersion, SuccessResult& successResult)
    {
        int edidVer = -1;
        successResult.success = true;

        if (strcmp(edidVersion.c_str(), "HDMI1.4") == 0) {
            edidVer = HDMI_EDID_VER_14;
        } else if (strcmp(edidVersion.c_str(), "HDMI2.0") == 0) {
            edidVer = HDMI_EDID_VER_20;
        } else {
            LOGERR("Invalid EDID Version: %s", edidVersion.c_str());
            successResult.success = false;
            return Core::ERROR_NONE;
        }

        try {
            device::HdmiInput::getInstance().setEdidVersion(portId, edidVer);
            LOGWARN("AVInputImplementation::setEdidVersion EDID Version: %s", edidVersion.c_str());
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            successResult.success = false;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetEdidVersion(const int portId, string& edidVersion, bool& success)
    {
        success = true;
        int version = -1;

        try {
            device::HdmiInput::getInstance().getEdidVersion(portId, &version);
            LOGWARN("AVInputImplementation::getEdidVersion EDID Version:%d", version);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            success = false;
            return Core::ERROR_NONE;
        }

        switch (version) {
        case HDMI_EDID_VER_14:
            edidVersion = "HDMI1.4";
            break;
        case HDMI_EDID_VER_20:
            edidVersion = "HDMI2.0";
            break;
        default:
            success = false;
            return Core::ERROR_NONE;
        }

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework

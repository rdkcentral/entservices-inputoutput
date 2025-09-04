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

int getTypeOfInput(string sType)
{
    int iType = -1;
    if (strcmp(sType.c_str(), "HDMI") == 0)
        iType = HDMI;
    else if (strcmp(sType.c_str(), "COMPOSITE") == 0)
        iType = COMPOSITE;
    else
        throw "Invalide type of INPUT, please specify HDMI/COMPOSITE";
    return iType;
}

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
            if (const string* devices = boost::get<string>(&params)) {

                std::list<IAVInput::IDevicesChangedNotification*>::const_iterator index(_devicesChangedNotifications.begin());

                printf("*** _DEBUG: printf: ON_AVINPUT_DEVICES_CHANGED: devices=%s\n", devices->c_str());
                LOGINFO("*** _DEBUG: ON_AVINPUT_DEVICES_CHANGED: devices=%s\n", devices->c_str());

                while (index != _devicesChangedNotifications.end()) {
                    (*index)->OnDevicesChanged(*devices);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_SIGNAL_CHANGED: {

            if (const IAVInput::InputSignalInfo* inputSignalInfo = boost::get<IAVInput::InputSignalInfo>(&params)) {
                std::list<IAVInput::ISignalChangedNotification*>::const_iterator index(_signalChangedNotifications.begin());

                while (index != _signalChangedNotifications.end()) {
                    (*index)->OnSignalChanged(*inputSignalInfo);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_STATUS_CHANGED: {

            if (const IAVInput::InputStatus* inputStatus = boost::get<IAVInput::InputStatus>(&params)) {
                std::list<IAVInput::IInputStatusChangedNotification*>::const_iterator index(_inputStatusChangedNotifications.begin());

                while (index != _inputStatusChangedNotifications.end()) {
                    (*index)->OnInputStatusChanged(*inputStatus);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE: {
            if (const IAVInput::InputVideoMode* inputVideoMode = boost::get<IAVInput::InputVideoMode>(&params)) {
                std::list<IAVInput::IVideoStreamInfoUpdateNotification*>::const_iterator index(_videoStreamInfoUpdateNotifications.begin());

                while (index != _videoStreamInfoUpdateNotifications.end()) {
                    (*index)->VideoStreamInfoUpdate(*inputVideoMode);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE: {
            if (const IAVInput::GameFeatureStatus* gameFeatureStatus = boost::get<IAVInput::GameFeatureStatus>(&params)) {
                std::list<IAVInput::IGameFeatureStatusUpdateNotification*>::const_iterator index(_gameFeatureStatusUpdateNotifications.begin());

                while (index != _gameFeatureStatusUpdateNotifications.end()) {
                    (*index)->GameFeatureStatusUpdate(*gameFeatureStatus);
                    ++index;
                }
            }
            break;
        }
        case ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE: {
            if (const IAVInput::ContentInfo* contentInfo = boost::get<IAVInput::ContentInfo>(&params)) {
                std::list<IHdmiContentTypeUpdateNotification*>::const_iterator index(_hdmiContentTypeUpdateNotifications.begin());

                while (index != _hdmiContentTypeUpdateNotifications.end()) {
                    (*index)->HdmiContentTypeUpdate(*contentInfo);
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

    void setResponseArray(JsonObject& response, const char* key, const vector<string>& items)
    {
        JsonArray arr;
        for (auto& i : items)
            arr.Add(JsonValue(i));

        response[key] = arr;

        string json;
        response.ToString(json);
        LOGINFO("%s: result json %s\n", __FUNCTION__, json.c_str());
    }

    uint32_t AVInputImplementation::endpoint_numberOfInputs(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        uint32_t count;
        Core::hresult ret = NumberOfInputs(count);

        if (ret == Core::ERROR_NONE) {
            response[_T("numberOfInputs")] = count;
        }

        returnResponse(ret == Core::ERROR_NONE);
    }

    uint32_t AVInputImplementation::endpoint_currentVideoMode(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string mode;
        string message;

        Core::hresult ret = CurrentVideoMode(mode, message);
        if (ret == Core::ERROR_NONE) {
            response[_T("currentVideoMode")] = mode;
        }

        returnResponse(ret == Core::ERROR_NONE);
    }

    Core::hresult AVInputImplementation::ContentProtected(bool& isContentProtected)
    {
        // "This is the way it's done in Service Manager"
        isContentProtected = true;
        return Core::ERROR_NONE;
    }

    uint32_t AVInputImplementation::endpoint_contentProtected(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        bool isContentProtected;
        Core::hresult ret = ContentProtected(isContentProtected);

        if (ret == Core::ERROR_NONE) {
            response[_T("isContentProtected")] = isContentProtected;
        }

        returnResponse(ret == Core::ERROR_NONE);
    }

    Core::hresult AVInputImplementation::NumberOfInputs(uint32_t& inputCount)
    {
        Core::hresult ret = Core::ERROR_NONE;

        try {
            printf("AVInputImplementation::NumberOfInputs: Calling HdmiInput::getNumberOfInputs...\n");
            inputCount = device::HdmiInput::getInstance().getNumberOfInputs();
            printf("AVInputImplementation::NumberOfInputs: inputCount=%d\n", inputCount);
        } catch (const std::exception& ex) {
            LOGERR("Exception caught: %s", ex.what());
            ret = Core::ERROR_GENERAL;
        } catch (...) {
            LOGERR("Exception caught");
            ret = Core::ERROR_GENERAL;
        }

        return ret;
    }

    Core::hresult AVInputImplementation::CurrentVideoMode(string& currentVideoMode, string& message)
    {
        Core::hresult ret = Core::ERROR_NONE;

        try {
            currentVideoMode = device::HdmiInput::getInstance().getCurrentVideoMode();
            // TODO: How is message set?
        } catch (const std::exception& ex) {
            LOGERR("std::exception caught: %s", ex.what());
            ret = Core::ERROR_GENERAL;
        } catch (...) {
            LOGERR("Exception caught");
            ret = Core::ERROR_GENERAL;
        }

        return ret;
    }

    Core::hresult AVInputImplementation::StartInput(int id, int type, bool audioMix, int planeType, bool topMostPlane)
    {
        Core::hresult ret = Core::ERROR_NONE;
        try {
            if (type == HDMI) {
                device::HdmiInput::getInstance().selectPort(id, audioMix, planeType, topMostPlane);
            } else if (type == COMPOSITE) {
                device::CompositeInput::getInstance().selectPort(id);
            } else {
                LOGWARN("Invalid input type passed to StartInput");
                ret = Core::ERROR_GENERAL;
            }
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    uint32_t AVInputImplementation::startInputWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string sPortId = parameters["portId"].String();
        string sType = parameters["typeOfInput"].String();
        bool audioMix = parameters["requestAudioMix"].Boolean();
        int portId = 0;
        int iType = 0;
        int planeType = 0;
        bool topMostPlane = parameters["topMost"].Boolean();
        LOGINFO("topMost value in thunder: %d\n", topMostPlane);

        if (parameters.HasLabel("portId") && parameters.HasLabel("typeOfInput")) {
            try {
                portId = stoi(sPortId);
                iType = getTypeOfInput(sType);
                if (parameters.HasLabel("plane")) {
                    planeType = stoi(parameters["plane"].String());
                }
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        Core::hresult ret = StartInput(portId, iType, audioMix, planeType, topMostPlane);
        returnResponse(ret == Core::ERROR_NONE);
    }

    uint32_t AVInputImplementation::stopInputWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string sType = parameters["typeOfInput"].String();
        int iType = 0;

        if (parameters.HasLabel("typeOfInput")) {
            try {
                iType = getTypeOfInput(sType);
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        Core::hresult ret = StopInput(iType);
        returnResponse(ret == Core::ERROR_NONE);
    }

    Core::hresult AVInputImplementation::StopInput(int type)
    {
        Core::hresult ret = Core::ERROR_NONE;

        try {
            planeType = -1;
            if (isAudioBalanceSet) {
                device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, MAX_PRIM_VOL_LEVEL);
                device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, DEFAULT_INPUT_VOL_LEVEL);
                isAudioBalanceSet = false;
            }
            if (type == HDMI) {
                device::HdmiInput::getInstance().selectPort(-1);
            } else if (type == COMPOSITE) {
                device::CompositeInput::getInstance().selectPort(-1);
            } else {
                LOGWARN("Invalid input type passed to StopInput");
                ret = Core::ERROR_GENERAL;
            }
        } catch (const device::Exception& err) {
            LOGWARN("AVInputImplementation::StopInput Failed");
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    uint32_t AVInputImplementation::setVideoRectangleWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        bool result = true;
        if (!parameters.HasLabel("x") && !parameters.HasLabel("y")) {
            result = false;
            LOGWARN("please specif coordinates (x,y)");
        }

        if (!parameters.HasLabel("w") && !parameters.HasLabel("h")) {
            result = false;
            LOGWARN("please specify window width and height (w,h)");
        }

        if (!parameters.HasLabel("typeOfInput")) {
            result = false;
            LOGWARN("please specify type of input HDMI/COMPOSITE");
        }

        if (result) {
            int x = 0;
            int y = 0;
            int w = 0;
            int h = 0;
            int t = 0;
            string sType;

            try {
                if (parameters.HasLabel("x")) {
                    x = parameters["x"].Number();
                }
                if (parameters.HasLabel("y")) {
                    y = parameters["y"].Number();
                }
                if (parameters.HasLabel("w")) {
                    w = parameters["w"].Number();
                }
                if (parameters.HasLabel("h")) {
                    h = parameters["h"].Number();
                }
                if (parameters.HasLabel("typeOfInput")) {
                    sType = parameters["typeOfInput"].String();
                    t = getTypeOfInput(sType);
                }
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }

            if (Core::ERROR_NONE != SetVideoRectangle(x, y, w, h, t)) {
                LOGWARN("AVInputService::setVideoRectangle Failed");
                returnResponse(false);
            }
            returnResponse(true);
        }
        returnResponse(false);
    }

    Core::hresult AVInputImplementation::SetVideoRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t type)
    {
        Core::hresult ret = Core::ERROR_NONE;

        try {
            if (type == HDMI) {
                device::HdmiInput::getInstance().scaleVideo(x, y, width, height);
            } else {
                device::CompositeInput::getInstance().scaleVideo(x, y, width, height);
            }
        } catch (const device::Exception& err) {
            ret = Core::ERROR_GENERAL;
        }

        return ret;
    }

    JsonArray AVInputImplementation::devicesToJson(Exchange::IAVInput::IInputDeviceIterator* devices)
    {
        JsonArray deviceArray;
        if (devices != nullptr) {
            WPEFramework::Exchange::IAVInput::InputDevice device;

            devices->Reset(0);

            while (devices->Next(device)) {
                JsonObject obj;
                obj["id"] = device.id;
                obj["locator"] = device.locator;
                obj["connected"] = device.connected;
                deviceArray.Add(obj);
            }
        }
        return deviceArray;
    }

    uint32_t AVInputImplementation::getInputDevicesWrapper(const JsonObject& parameters, JsonObject& response)
    {
        IInputDeviceIterator* devices = nullptr;
        Core::hresult result;

        LOGINFOMETHOD();

        if (parameters.HasLabel("typeOfInput")) {
            string sType = parameters["typeOfInput"].String();
            int iType = 0;

            try {
                iType = getTypeOfInput(sType);
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }

            result = GetInputDevices(iType, devices);
        } else {
            std::list<WPEFramework::Exchange::IAVInput::InputDevice> hdmiDevices;
            result = getInputDevices(HDMI, hdmiDevices);

            if (Core::ERROR_NONE == result) {
                std::list<WPEFramework::Exchange::IAVInput::InputDevice> compositeDevices;
                result = getInputDevices(COMPOSITE, compositeDevices);

                if (Core::ERROR_NONE == result) {
                    std::list<WPEFramework::Exchange::IAVInput::InputDevice> combinedDevices = hdmiDevices;
                    combinedDevices.insert(combinedDevices.end(), compositeDevices.begin(), compositeDevices.end());
                    devices = Core::Service<RPC::IteratorType<IInputDeviceIterator>>::Create<IInputDeviceIterator>(combinedDevices);
                }
            }
        }

        if (devices != nullptr && Core::ERROR_NONE == result) {
            response["devices"] = devicesToJson(devices);
            devices->Release();
        }

        returnResponse(Core::ERROR_NONE == result);
    }

    uint32_t AVInputImplementation::writeEDIDWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string sPortId = parameters["portId"].String();
        int portId = 0;
        std::string message;

        if (parameters.HasLabel("portId") && parameters.HasLabel("message")) {
            portId = stoi(sPortId);
            message = parameters["message"].String();
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        returnResponse(Core::ERROR_NONE == WriteEDID(portId, message));
    }

    uint32_t AVInputImplementation::readEDIDWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string sPortId = parameters["portId"].String();
        int portId = 0;
        try {
            portId = stoi(sPortId);
        } catch (...) {
            LOGWARN("Invalid Arguments");
            returnResponse(false);
        }

        string edid;

        Core::hresult result = ReadEDID(portId, edid);
        if (Core::ERROR_NONE != result || edid.empty()) {
            returnResponse(false);
        } else {
            response["EDID"] = edid;
            returnResponse(true);
        }
    }

    Core::hresult AVInputImplementation::getInputDevices(int type, std::list<WPEFramework::Exchange::IAVInput::InputDevice> devices)
    {
        Core::hresult result = Core::ERROR_NONE;

        try {
            int num = 0;
            if (type == HDMI) {
                num = device::HdmiInput::getInstance().getNumberOfInputs();
            } else if (type == COMPOSITE) {
                num = device::CompositeInput::getInstance().getNumberOfInputs();
            }
            if (num > 0) {
                int i = 0;
                for (i = 0; i < num; i++) {
                    // Input ID is aleays 0-indexed, continuous number starting 0
                    WPEFramework::Exchange::IAVInput::InputDevice inputDevice;

                    inputDevice.id = i;
                    std::stringstream locator;
                    if (type == HDMI) {
                        locator << "hdmiin://localhost/deviceid/" << i;
                        inputDevice.connected = device::HdmiInput::getInstance().isPortConnected(i);
                    } else if (type == COMPOSITE) {
                        locator << "cvbsin://localhost/deviceid/" << i;
                        inputDevice.connected = device::CompositeInput::getInstance().isPortConnected(i);
                    }
                    inputDevice.locator = locator.str();
                    LOGWARN("AVInputService::getInputDevices id %d, locator=[%s], connected=[%d]", i, inputDevice.locator.c_str(), inputDevice.connected);
                    devices.push_back(inputDevice);
                }
            }
        } catch (const std::exception& e) {
            LOGWARN("AVInputService::getInputDevices Failed");
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    Core::hresult AVInputImplementation::GetInputDevices(int type, Exchange::IAVInput::IInputDeviceIterator*& devices)
    {
        std::list<WPEFramework::Exchange::IAVInput::InputDevice> list;

        Core::hresult result = getInputDevices(type, list);

        if (Core::ERROR_NONE == result) {
            devices = Core::Service<RPC::IteratorType<IInputDeviceIterator>>::Create<IInputDeviceIterator>(list);
        } else {
            devices = nullptr;
        }

        return result;
    }

    Core::hresult AVInputImplementation::WriteEDID(int id, const string& edid)
    {
        // TODO: This wasn't implemented in the original code, do we want to implement it?
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::ReadEDID(int id, string& edid)
    {
        vector<uint8_t> edidVec({ 'u', 'n', 'k', 'n', 'o', 'w', 'n' });

        try {
            vector<uint8_t> edidVec2;
            device::HdmiInput::getInstance().getEDIDBytesInfo(id, edidVec2);
            edidVec = edidVec2; // edidVec must be "unknown" unless we successfully get to this line

            // convert to base64
            uint16_t size = min(edidVec.size(), (size_t)numeric_limits<uint16_t>::max());

            LOGWARN("AVInputImplementation::readEDID size:%d edidVec.size:%zu", size, edidVec.size());
            if (edidVec.size() > (size_t)numeric_limits<uint16_t>::max()) {
                LOGERR("Size too large to use ToString base64 wpe api");
                return Core::ERROR_GENERAL;
            }
            Core::ToString((uint8_t*)&edidVec[0], size, true, edid);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            return Core::ERROR_GENERAL;
        }

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

        Core::hresult result = GetInputDevices(type, devices);
        if (Core::ERROR_NONE != result) {
            LOGERR("AVInputHotplug [%d, %d, %d]: Failed to get devices", input, connect, type);
            return;
        }

        JsonArray jsonArray = devicesToJson(devices);
        string jsonString;
        jsonArray.ToString(jsonString);
        ParamsType params = jsonString;
        dispatchEvent(ON_AVINPUT_STATUS_CHANGED, params);
    }

    /**
     * @brief This function is used to translate HDMI/COMPOSITE input signal change to
     * signalChanged event.
     *
     * @param[in] port HDMI/COMPOSITE In port id.
     * @param[in] signalStatus signal status of HDMI/COMPOSITE In port.
     */
    void AVInputImplementation::AVInputSignalChange(int port, int signalStatus, int type)
    {
        LOGWARN("AVInputSignalStatus [%d, %d, %d]", port, signalStatus, type);

        Exchange::IAVInput::InputSignalInfo signalInfo;
        signalInfo.id = port;

        std::stringstream locator;
        if (type == HDMI) {
            locator << "hdmiin://localhost/deviceid/" << port;
        } else {
            locator << "cvbsin://localhost/deviceid/" << port;
        }

        signalInfo.locator = locator.str();

        /* values of dsHdmiInSignalStatus_t and dsCompInSignalStatus_t are same
       Hence used only HDMI macro for case statement */
        switch (signalStatus) {
        case dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL:
            signalInfo.status = "noSignal";
            break;

        case dsHDMI_IN_SIGNAL_STATUS_UNSTABLE:
            signalInfo.status = "unstableSignal";
            break;

        case dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED:
            signalInfo.status = "notSupportedSignal";
            break;

        case dsHDMI_IN_SIGNAL_STATUS_STABLE:
            signalInfo.status = "stableSignal";
            break;

        default:
            signalInfo.status = "none";
            break;
        }

        ParamsType params = signalInfo;
        dispatchEvent(ON_AVINPUT_SIGNAL_CHANGED, params);
    }

    /**
     * @brief This function is used to translate HDMI/COMPOSITE input status change to
     * inputStatusChanged event.
     *
     * @param[in] port HDMI/COMPOSITE In port id.
     * @param[bool] isPresented HDMI/COMPOSITE In presentation started/stopped.
     */
    void AVInputImplementation::AVInputStatusChange(int port, bool isPresented, int type)
    {
        LOGWARN("avInputStatus [%d, %d, %d]", port, isPresented, type);

        std::stringstream locator;
        Exchange::IAVInput::InputSignalInfo inputSignalInfo;

        inputSignalInfo.id = port;

        if (type == HDMI) {
            locator << "hdmiin://localhost/deviceid/" << port;
        } else if (type == COMPOSITE) {
            locator << "cvbsin://localhost/deviceid/" << port;
        }

        inputSignalInfo.locator = locator.str();

        if (isPresented) {
            inputSignalInfo.status = "started";
        } else {
            inputSignalInfo.status = "stopped";
        }

        Exchange::IAVInput::InputStatus status;
        status.info = inputSignalInfo;
        status.plane = planeType;
        ParamsType params = status;

        dispatchEvent(ON_AVINPUT_STATUS_CHANGED, params);
    }

    /**
     * @brief This function is used to translate HDMI input video mode change to
     * videoStreamInfoUpdate event.
     *
     * @param[in] port HDMI In port id.
     * @param[dsVideoPortResolution_t] video resolution data
     */
    void AVInputImplementation::AVInputVideoModeUpdate(int port, dsVideoPortResolution_t resolution, int type)
    {
        WPEFramework::Exchange::IAVInput::InputVideoMode inputVideoMode;
        inputVideoMode.id = port;
        std::stringstream locator;

        LOGWARN("AVInputVideoModeUpdate [%d]", port);

        if (type == HDMI) {
            locator << "hdmiin://localhost/deviceid/" << port;

            switch (resolution.pixelResolution) {

            case dsVIDEO_PIXELRES_720x480:
                inputVideoMode.width = 720;
                inputVideoMode.height = 480;
                break;

            case dsVIDEO_PIXELRES_720x576:
                inputVideoMode.width = 720;
                inputVideoMode.height = 576;
                break;

            case dsVIDEO_PIXELRES_1280x720:
                inputVideoMode.width = 1280;
                inputVideoMode.height = 720;
                break;

            case dsVIDEO_PIXELRES_1920x1080:
                inputVideoMode.width = 1920;
                inputVideoMode.height = 1080;
                break;

            case dsVIDEO_PIXELRES_3840x2160:
                inputVideoMode.width = 3840;
                inputVideoMode.height = 2160;
                break;

            case dsVIDEO_PIXELRES_4096x2160:
                inputVideoMode.width = 4096;
                inputVideoMode.height = 2160;
                break;

            default:
                inputVideoMode.width = 1920;
                inputVideoMode.height = 1080;
                break;
            }
            inputVideoMode.progressive = (!resolution.interlaced);
        } else if (type == COMPOSITE) {
            locator << "cvbsin://localhost/deviceid/" << port;

            switch (resolution.pixelResolution) {
            case dsVIDEO_PIXELRES_720x480:
                inputVideoMode.width = 720;
                inputVideoMode.height = 480;
                break;

            case dsVIDEO_PIXELRES_720x576:
                inputVideoMode.width = 720;
                inputVideoMode.height = 576;
                break;

            default:
                inputVideoMode.width = 720;
                inputVideoMode.height = 576;
                break;
            }

            inputVideoMode.progressive = false;
        }

        inputVideoMode.locator = locator.str();

        switch (resolution.frameRate) {
        case dsVIDEO_FRAMERATE_24:
            inputVideoMode.frameRateN = 24000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_25:
            inputVideoMode.frameRateN = 25000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_30:
            inputVideoMode.frameRateN = 30000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_50:
            inputVideoMode.frameRateN = 50000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_60:
            inputVideoMode.frameRateN = 60000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_23dot98:
            inputVideoMode.frameRateN = 24000;
            inputVideoMode.frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_29dot97:
            inputVideoMode.frameRateN = 30000;
            inputVideoMode.frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_59dot94:
            inputVideoMode.frameRateN = 60000;
            inputVideoMode.frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_100:
            inputVideoMode.frameRateN = 100000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_119dot88:
            inputVideoMode.frameRateN = 120000;
            inputVideoMode.frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_120:
            inputVideoMode.frameRateN = 120000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_200:
            inputVideoMode.frameRateN = 200000;
            inputVideoMode.frameRateD = 1000;
            break;

        case dsVIDEO_FRAMERATE_239dot76:
            inputVideoMode.frameRateN = 240000;
            inputVideoMode.frameRateD = 1001;
            break;

        case dsVIDEO_FRAMERATE_240:
            inputVideoMode.frameRateN = 240000;
            inputVideoMode.frameRateD = 1000;
            break;

        default:
            inputVideoMode.frameRateN = 60000;
            inputVideoMode.frameRateD = 1000;
            break;
        }

        ParamsType params = inputVideoMode;

        dispatchEvent(ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE, params);
    }

    void AVInputImplementation::hdmiInputAviContentTypeChange(int port, int content_type)
    {
        WPEFramework::Exchange::IAVInput::ContentInfo contentInfo;
        contentInfo.id = port;
        contentInfo.contentType = content_type;

        ParamsType params = contentInfo;
        dispatchEvent(ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE, params);
    }

    void AVInputImplementation::AVInputALLMChange(int port, bool allm_mode)
    {
        WPEFramework::Exchange::IAVInput::GameFeatureStatus status;
        status.id = port;
        status.gameFeature = STR_ALLM;
        status.allmMode = allm_mode;

        ParamsType params = status;
        dispatchEvent(ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE, params);
    }

    void AVInputImplementation::AVInputVRRChange(int port, dsVRRType_t vrr_type, bool vrr_mode)
    {
        WPEFramework::Exchange::IAVInput::GameFeatureStatus status;
        status.id = port;
        status.allmMode = vrr_mode;

        switch (vrr_type) {
        case dsVRR_HDMI_VRR:
            status.gameFeature = VRR_TYPE_HDMI;
            break;
        case dsVRR_AMD_FREESYNC:
            status.gameFeature = VRR_TYPE_FREESYNC;
            break;
        case dsVRR_AMD_FREESYNC_PREMIUM:
            status.gameFeature = VRR_TYPE_FREESYNC_PREMIUM;
            break;
        case dsVRR_AMD_FREESYNC_PREMIUM_PRO:
            status.gameFeature = VRR_TYPE_FREESYNC_PREMIUM_PRO;
            break;
        default:
            break;
        }

        ParamsType params = status;
        dispatchEvent(ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE, params);
    }

    Core::hresult AVInputImplementation::GetSupportedGameFeatures(IStringIterator*& features)
    {
        Core::hresult result = Core::ERROR_NONE;
        features = nullptr;
        std::vector<std::string> supportedFeatures;
        try {
            device::HdmiInput::getInstance().getSupportedGameFeatures(supportedFeatures);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION0();
            result = Core::ERROR_GENERAL;
        }

        if (!supportedFeatures.empty() && result == Core::ERROR_NONE) {
            features = Core::Service<RPC::IteratorType<IStringIterator>>::Create<IStringIterator>(supportedFeatures);
        } else {
            result = Core::ERROR_GENERAL;
        }
        return result;
    }

    uint32_t AVInputImplementation::getSupportedGameFeaturesWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();
        IStringIterator* features = nullptr;
        Core::hresult result = GetSupportedGameFeatures(features);

        if (result != Core::ERROR_NONE || features == nullptr) {
            returnResponse(false);
        } else {
            vector<string> supportedFeatures;
            features->Reset(0);
            string feature;
            while (features->Next(feature)) {
                supportedFeatures.push_back(feature);
            }
            features->Release();
            setResponseArray(response, "supportedGameFeatures", supportedFeatures);
            returnResponse(true);
        }
    }

    uint32_t AVInputImplementation::getGameFeatureStatusWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        if (!(parameters.HasLabel("portId") && parameters.HasLabel("gameFeature"))) {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        string sPortId = parameters["portId"].String();
        string sGameFeature = parameters["gameFeature"].String();
        int portId = 0;
        try {
            portId = stoi(sPortId);
        } catch (...) {
            LOGWARN("Invalid Arguments");
            returnResponse(false);
        }

        bool mode = false;
        Core::hresult ret = GetGameFeatureStatus(portId, sGameFeature, mode);
        if (ret == Core::ERROR_NONE) {
            response["mode"] = mode;
            returnResponse(true);
        } else {
            LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper Mode is not supported. Supported mode: ALLM, VRR-HDMI, VRR-FREESYNC, VRR-FREESYNC-PREMIUM, VRR-FREESYNC-PREMIUM-PRO");
            returnResponse(false);
        }
    }

    Core::hresult AVInputImplementation::GetGameFeatureStatus(int id, const string& feature, bool& mode)
    {
        // TODO: The current docs state that the id parameter is optional, but that's not the case in existing code. Which is correct?

        if (feature == STR_ALLM) {
            mode = getALLMStatus(id);
        } else if (feature == VRR_TYPE_HDMI) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(id, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_HDMI_VRR);
        } else if (feature == VRR_TYPE_FREESYNC) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(id, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_AMD_FREESYNC);
        } else if (feature == VRR_TYPE_FREESYNC_PREMIUM) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(id, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_AMD_FREESYNC_PREMIUM);
        } else if (feature == VRR_TYPE_FREESYNC_PREMIUM_PRO) {
            dsHdmiInVrrStatus_t vrrStatus;
            getVRRStatus(id, &vrrStatus);
            mode = (vrrStatus.vrrType == dsVRR_AMD_FREESYNC_PREMIUM_PRO);
        } else {
            LOGWARN("AVInputImplementation::GetGameFeatureStatus Unsupported feature: %s", feature.c_str());
            return Core::ERROR_NOT_SUPPORTED;
        }

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

    uint32_t AVInputImplementation::getRawSPDWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string sPortId = parameters["portId"].String();
        int portId = 0;
        if (parameters.HasLabel("portId")) {
            try {
                portId = stoi(sPortId);
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        string spdInfo;
        Core::hresult ret = GetRawSPD(portId, spdInfo);
        response["HDMISPD"] = spdInfo;
        if (spdInfo.empty() || Core::ERROR_NONE != ret) {
            returnResponse(false);
        } else {
            returnResponse(true);
        }
    }

    uint32_t AVInputImplementation::getSPDWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        string sPortId = parameters["portId"].String();
        int portId = 0;
        if (parameters.HasLabel("portId")) {
            try {
                portId = stoi(sPortId);
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        string spdInfo;
        Core::hresult ret = GetSPD(portId, spdInfo);
        response["HDMISPD"] = spdInfo;
        if (spdInfo.empty() || Core::ERROR_NONE != ret) {
            returnResponse(false);
        } else {
            returnResponse(true);
        }
    }

    Core::hresult AVInputImplementation::GetRawSPD(int id, string& spd)
    {
        LOGINFO("AVInputImplementation::getSPDInfo");
        vector<uint8_t> spdVect({ 'u', 'n', 'k', 'n', 'o', 'w', 'n' });
        spd.clear();
        try {
            LOGWARN("AVInputImplementation::getSPDInfo");
            vector<uint8_t> spdVect2;
            device::HdmiInput::getInstance().getHDMISPDInfo(id, spdVect2);
            spdVect = spdVect2; // spdVect must be "unknown" unless we successfully get to this line

            // convert to base64
            uint16_t size = min(spdVect.size(), (size_t)numeric_limits<uint16_t>::max());

            LOGWARN("AVInputImplementation::getSPD size:%d spdVec.size:%zu", size, spdVect.size());

            if (spdVect.size() > (size_t)numeric_limits<uint16_t>::max()) {
                LOGERR("Size too large to use ToString base64 wpe api");
                return Core::ERROR_GENERAL;
            }

            LOGINFO("------------getSPD: ");
            for (size_t itr = 0; itr < spdVect.size(); itr++) {
                LOGINFO("%02X ", spdVect[itr]);
            }
            Core::ToString((uint8_t*)&spdVect[0], size, false, spd);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            return Core::ERROR_GENERAL;
        }
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::GetSPD(int id, string& spd)
    {
        vector<uint8_t> spdVect({ 'u', 'n', 'k', 'n', 'o', 'w', 'n' });

        LOGINFO("AVInputImplementation::getSPDInfo");

        try {
            LOGWARN("AVInputImplementation::getSPDInfo");
            vector<uint8_t> spdVect2;
            device::HdmiInput::getInstance().getHDMISPDInfo(id, spdVect2);
            spdVect = spdVect2; // edidVec must be "unknown" unless we successfully get to this line

            // convert to base64
            uint16_t size = min(spdVect.size(), (size_t)numeric_limits<uint16_t>::max());

            LOGWARN("AVInputImplementation::getSPD size:%d spdVec.size:%zu", size, spdVect.size());

            if (spdVect.size() > (size_t)numeric_limits<uint16_t>::max()) {
                LOGERR("Size too large to use ToString base64 wpe api");
                return Core::ERROR_GENERAL;
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
                spd = str;
            }
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            return Core::ERROR_GENERAL;
        }

        return Core::ERROR_NONE;
    }

    uint32_t AVInputImplementation::setAudioMixerLevelsWrapper(const JsonObject& parameters, JsonObject& response)
    {
        returnIfParamNotFound(parameters, "primaryVolume");
        returnIfParamNotFound(parameters, "inputVolume");

        int primVol = 0, inputVol = 0;
        try {
            primVol = parameters["primaryVolume"].Number();
            inputVol = parameters["inputVolume"].Number();
        } catch (...) {
            LOGERR("Incompatible params passed !!!\n");
            response["success"] = false;
            returnResponse(false);
        }

        if ((primVol >= 0) && (inputVol >= 0)) {
            m_primVolume = primVol;
            m_inputVolume = inputVol;
        } else {
            LOGERR("Incompatible params passed !!!\n");
            response["success"] = false;
            returnResponse(false);
        }
        if (m_primVolume > MAX_PRIM_VOL_LEVEL) {
            LOGWARN("Primary Volume greater than limit. Set to MAX_PRIM_VOL_LEVEL(100) !!!\n");
            m_primVolume = MAX_PRIM_VOL_LEVEL;
        }
        if (m_inputVolume > DEFAULT_INPUT_VOL_LEVEL) {
            LOGWARN("INPUT Volume greater than limit. Set to DEFAULT_INPUT_VOL_LEVEL(100) !!!\n");
            m_inputVolume = DEFAULT_INPUT_VOL_LEVEL;
        }

        LOGINFO("GLOBAL primary Volume=%d input Volume=%d \n", m_primVolume, m_inputVolume);

        returnResponse(Core::ERROR_NONE == SetAudioMixerLevels({ m_primVolume, m_inputVolume }));
    }

    Core::hresult AVInputImplementation::SetAudioMixerLevels(Exchange::IAVInput::MixerLevels levels)
    {
        try {
            device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, levels.primaryVolume);
            device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, levels.playerVolume);
        } catch (...) {
            LOGWARN("Not setting SoC volume !!!\n");
            return Core::ERROR_GENERAL;
        }

        isAudioBalanceSet = true;
        return Core::ERROR_NONE;
    }

    Core::hresult AVInputImplementation::SetEdid2AllmSupport(int portId, bool allmSupport)
    {
        Core::hresult ret = Core::ERROR_NONE;
        try {
            device::HdmiInput::getInstance().setEdid2AllmSupport(portId, allmSupport);
            LOGWARN("AVInput -  allmsupport:%d", allmSupport);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    uint32_t AVInputImplementation::setEdid2AllmSupportWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        returnIfParamNotFound(parameters, "portId");
        returnIfParamNotFound(parameters, "allmSupport");

        int portId = 0;
        string sPortId = parameters["portId"].String();
        bool allmSupport = parameters["allmSupport"].Boolean();

        try {
            portId = stoi(sPortId);
        } catch (const std::exception& err) {
            LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
            returnResponse(false);
        }

        returnResponse(Core::ERROR_NONE == SetEdid2AllmSupport(portId, allmSupport));
    }

    Core::hresult AVInputImplementation::GetEdid2AllmSupport(int portId, bool& allmSupportValue)
    {
        Core::hresult ret = Core::ERROR_NONE;
        try {
            device::HdmiInput::getInstance().getEdid2AllmSupport(portId, &allmSupportValue);
            LOGINFO("AVInput - getEdid2AllmSupport:%d", allmSupportValue);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(portId));
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    uint32_t AVInputImplementation::getEdid2AllmSupportWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();
        string sPortId = parameters["portId"].String();

        int portId = 0;
        bool allmSupport = true;
        returnIfParamNotFound(parameters, "portId");

        try {
            portId = stoi(sPortId);
        } catch (const std::exception& err) {
            LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
            returnResponse(false);
        }

        Core::hresult result = GetEdid2AllmSupport(portId, allmSupport);
        if (Core::ERROR_NONE == result) {
            response["allmSupport"] = allmSupport;
        }

        returnResponse(Core::ERROR_NONE == result);
    }

    Core::hresult AVInputImplementation::GetVRRSupport(int portId, bool& vrrSupport)
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

    uint32_t AVInputImplementation::getVRRSupportWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();
        returnIfParamNotFound(parameters, "portId");
        string sPortId = parameters["portId"].String();

        int portId = 0;
        bool vrrSupport = true;

        try {
            portId = stoi(sPortId);
        } catch (const std::exception& err) {
            LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
            returnResponse(false);
        }

        Core::hresult result = GetVRRSupport(portId, vrrSupport);

        if (Core::ERROR_NONE == result) {
            response["vrrSupport"] = vrrSupport;
        }

        returnResponse(Core::ERROR_NONE == result);
    }

    Core::hresult AVInputImplementation::SetVRRSupport(int id, bool vrrSupport)
    {
        Core::hresult ret = Core::ERROR_NONE;
        try {
            device::HdmiInput::getInstance().setVRRSupport(id, vrrSupport);
            LOGWARN("AVInput -  vrrSupport:%d", vrrSupport);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            ret = Core::ERROR_GENERAL;
        }
        return ret;
    }

    uint32_t AVInputImplementation::setVRRSupportWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();

        returnIfParamNotFound(parameters, "portId");
        returnIfParamNotFound(parameters, "vrrSupport");

        int portId = 0;
        string sPortId = parameters["portId"].String();
        bool vrrSupport = parameters["vrrSupport"].Boolean();

        try {
            portId = stoi(sPortId);
        } catch (const std::exception& err) {
            LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
            returnResponse(false);
        }

        returnResponse(Core::ERROR_NONE == SetVRRSupport(portId, vrrSupport));
    }

    uint32_t AVInputImplementation::getVRRFrameRateWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();
        returnIfParamNotFound(parameters, "portId");
        string sPortId = parameters["portId"].String();

        int portId = 0;
        dsHdmiInVrrStatus_t vrrStatus;
        vrrStatus.vrrAmdfreesyncFramerate_Hz = 0;

        try {
            portId = stoi(sPortId);
        } catch (const std::exception& err) {
            LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
            returnResponse(false);
        }

        bool result = getVRRStatus(portId, &vrrStatus);
        if (result == true) {
            response["currentVRRVideoFrameRate"] = vrrStatus.vrrAmdfreesyncFramerate_Hz;
            returnResponse(true);
        } else {
            returnResponse(false);
        }
    }

    uint32_t AVInputImplementation::setEdidVersionWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();
        string sPortId = parameters["portId"].String();
        int portId = 0;
        string sVersion = "";
        if (parameters.HasLabel("portId") && parameters.HasLabel("edidVersion")) {
            try {
                portId = stoi(sPortId);
                sVersion = parameters["edidVersion"].String();
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        returnResponse(Core::ERROR_NONE == SetEdidVersion(portId, sVersion));
    }

    Core::hresult AVInputImplementation::GetHdmiVersion(int id, string& hdmiVersion)
    {
        Core::hresult ret = Core::ERROR_NONE;
        dsHdmiMaxCapabilityVersion_t hdmiCapVersion = HDMI_COMPATIBILITY_VERSION_14;

        try {
            device::HdmiInput::getInstance().getHdmiVersion(id, &hdmiCapVersion);
            LOGWARN("AVInputImplementation::GetHdmiVersion Hdmi Version:%d", hdmiCapVersion);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            return Core::ERROR_GENERAL;
        }

        switch ((int)hdmiCapVersion) {
        case HDMI_COMPATIBILITY_VERSION_14:
            hdmiVersion = "1.4";
            break;
        case HDMI_COMPATIBILITY_VERSION_20:
            hdmiVersion = "2.0";
            break;
        case HDMI_COMPATIBILITY_VERSION_21:
            hdmiVersion = "2.1";
            break;
        default:
            return Core::ERROR_GENERAL;
        }

        if (hdmiCapVersion == HDMI_COMPATIBILITY_VERSION_MAX) {
            return Core::ERROR_GENERAL;
        }

        return ret;
    }

    uint32_t AVInputImplementation::getHdmiVersionWrapper(const JsonObject& parameters, JsonObject& response)
    {
        LOGINFOMETHOD();
        returnIfParamNotFound(parameters, "portId");
        string sPortId = parameters["portId"].String();
        int portId = 0;

        try {
            portId = stoi(sPortId);
        } catch (const std::exception& err) {
            LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
            returnResponse(false);
        }

        string hdmiVersion;
        Core::hresult ret = GetHdmiVersion(portId, hdmiVersion);

        if (ret == Core::ERROR_NONE) {
            response["HdmiCapabilityVersion"] = hdmiVersion;
            returnResponse(true);
        } else {
            returnResponse(false);
        }
    }

    Core::hresult AVInputImplementation::SetEdidVersion(int id, const string& version)
    {
        Core::hresult ret = Core::ERROR_NONE;
        int edidVer = -1;

        if (strcmp(version.c_str(), "HDMI1.4") == 0) {
            edidVer = HDMI_EDID_VER_14;
        } else if (strcmp(version.c_str(), "HDMI2.0") == 0) {
            edidVer = HDMI_EDID_VER_20;
        } else {
            LOGERR("Invalid EDID Version: %s", version.c_str());
            return Core::ERROR_GENERAL;
        }

        try {
            device::HdmiInput::getInstance().setEdidVersion(id, edidVer);
            LOGWARN("AVInputImplementation::setEdidVersion EDID Version: %s", version.c_str());
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            ret = Core::ERROR_GENERAL;
        }

        return ret;
    }

    uint32_t AVInputImplementation::getEdidVersionWrapper(const JsonObject& parameters, JsonObject& response)
    {
        string sPortId = parameters["portId"].String();
        int portId = 0;

        LOGINFOMETHOD();
        if (parameters.HasLabel("portId")) {
            try {
                portId = stoi(sPortId);
            } catch (...) {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }
        } else {
            LOGWARN("Required parameters are not passed");
            returnResponse(false);
        }

        string version;
        Core::hresult ret = GetEdidVersion(portId, version);
        if (Core::ERROR_NONE == ret) {
            response["edidVersion"] = version;
            returnResponse(true);
        } else {
            LOGERR("Failed to get EDID version for port %d", portId);
            returnResponse(false);
        }
    }

    Core::hresult AVInputImplementation::GetEdidVersion(int id, string& version)
    {
        Core::hresult ret = Core::ERROR_NONE;
        int edidVersion = -1;

        try {
            device::HdmiInput::getInstance().getEdidVersion(id, &edidVersion);
            LOGWARN("AVInputImplementation::getEdidVersion EDID Version:%d", edidVersion);
        } catch (const device::Exception& err) {
            LOG_DEVICE_EXCEPTION1(std::to_string(id));
            return Core::ERROR_GENERAL;
        }

        switch (edidVersion) {
        case HDMI_EDID_VER_14:
            version = "HDMI1.4";
            break;
        case HDMI_EDID_VER_20:
            version = "HDMI2.0";
            break;
        default:
            return Core::ERROR_GENERAL;
        }

        return ret;
    }

} // namespace Plugin
} // namespace WPEFramework

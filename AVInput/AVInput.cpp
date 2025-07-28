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

#include "AVInput.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 7
#define API_VERSION_NUMBER_PATCH 1

#define AV_HOT_PLUG_EVENT_CONNECTED 0
#define AV_HOT_PLUG_EVENT_DISCONNECTED 1

namespace WPEFramework
{
    namespace
    {

        static Plugin::Metadata<Plugin::AVInput> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {});
    }

    namespace Plugin
    {

        SERVICE_REGISTRATION(AVInput, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        AVInput::AVInput() : _service(nullptr), _connectionId(0), _avInput(nullptr), _avInputNotification(this)
        {
            SYSLOG(Logging::Startup, (_T("AVInput Constructor")));
        }

        AVInput::~AVInput()
        {
            SYSLOG(Logging::Shutdown, (string(_T("AVInput Destructor"))));
        }

        const string AVInput::Initialize(PluginHost::IShell *service)
        {
            string message = "";

            ASSERT(nullptr != service);
            ASSERT(nullptr == _service);
            ASSERT(nullptr == _avInput);
            ASSERT(0 == _connectionId);

            SYSLOG(Logging::Startup, (_T("DeviceDiagnostics::Initialize: PID=%u"), getpid()));

            _service = service;
            _service->AddRef();
            _service->Register(&_avInputNotification);

            _avInput = service->Root<Exchange::IAVInput>(_connectionId, 5000, _T("AVInputImplementation"));

            if (nullptr != _avInput)
            {
                // Register for notifications
                _avInput->Register(&_avInputNotification);
                // Invoking Plugin API register to wpeframework
                Exchange::JAVInput::Register(*this, _avInput);

                InitializeIARM(); // <pca> TODO: Do we want to do this here/at all? </pca>
            }
            else
            {
                SYSLOG(Logging::Startup, (_T("AVInput::Initialize: Failed to initialise AVInput plugin")));
                message = _T("AVInput plugin could not be initialised");
            }

            return message;
        }

        void AVInput::Deinitialize(PluginHost::IShell *service)
        {
            ASSERT(_service == service);

            SYSLOG(Logging::Shutdown, (string(_T("AVInput::Deinitialize"))));

            // Make sure the Activated and Deactivated are no longer called before we start cleaning up..
            _service->Unregister(&_avInputNotification);

            if (nullptr != _avInput)
            {
                DeinitializeIARM(); // <pca> TODO: Do we want to do this here/at all? </pca>

                _avInput->Unregister(&_avInputNotification);
                Exchange::JAVInput::Unregister(*this);

                // Stop processing:
                RPC::IRemoteConnection *connection = service->RemoteConnection(_connectionId);
                VARIABLE_IS_NOT_USED uint32_t result = _avInput->Release();

                _avInput = nullptr;

                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // If this was running in a (container) process...
                if (nullptr != connection)
                {
                    // Lets trigger the cleanup sequence for
                    // out-of-process code. Which will guard
                    // that unwilling processes, get shot if
                    // not stopped friendly :-)
                    try
                    {
                        connection->Terminate();
                        // Log success if needed
                        LOGWARN("Connection terminated successfully.");
                    }
                    catch (const std::exception &e)
                    {
                        std::string errorMessage = "Failed to terminate connection: ";
                        errorMessage += e.what();
                        LOGWARN("%s", errorMessage.c_str());
                    }

                    connection->Release();
                }
            }

            _connectionId = 0;
            _service->Release();
            _service = nullptr;
            SYSLOG(Logging::Shutdown, (string(_T("AVInput de-initialised"))));
        }

        string AVInput::Information() const
        {
            return (string());
        }

        void AVInput::InitializeIARM()
        {
            if (Utils::IARM::init())
            {
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

        void AVInput::DeinitializeIARM()
        {
            if (Utils::IARM::isConnected())
            {
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

        void AVInput::dsAviContentTypeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (!AVInputImplementation::_instance)
                return;

            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE == eventId)
            {
                IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                int hdmi_in_port = eventData->data.hdmi_in_content_type.port;
                int avi_content_type = eventData->data.hdmi_in_content_type.aviContentType;
                LOGINFO("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE  event  port: %d, Content Type : %d", hdmi_in_port, avi_content_type);

                AVInputImplementation::_instance->hdmiInputAviContentTypeChange(hdmi_in_port, avi_content_type);
            }
        }

        void AVInput::dsAVEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (!AVInputImplementation::_instance)
                return;

            IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG == eventId)
            {
                int hdmiin_hotplug_port = eventData->data.hdmi_in_connect.port;
                int hdmiin_hotplug_conn = eventData->data.hdmi_in_connect.isPortConnected;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG  event data:%d", hdmiin_hotplug_port);
                AVInputImplementation::_instance->AVInputHotplug(hdmiin_hotplug_port, hdmiin_hotplug_conn ? AV_HOT_PLUG_EVENT_CONNECTED : AV_HOT_PLUG_EVENT_DISCONNECTED, HDMI);
            }
            else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG == eventId)
            {
                int compositein_hotplug_port = eventData->data.composite_in_connect.port;
                int compositein_hotplug_conn = eventData->data.composite_in_connect.isPortConnected;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG  event data:%d", compositein_hotplug_port);
                AVInputImplementation::_instance->AVInputHotplug(compositein_hotplug_port, compositein_hotplug_conn ? AV_HOT_PLUG_EVENT_CONNECTED : AV_HOT_PLUG_EVENT_DISCONNECTED, COMPOSITE);
            }
        }

        void AVInput::dsAVSignalStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (!AVInputImplementation::_instance)
                return;
            IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS == eventId)
            {
                int hdmi_in_port = eventData->data.hdmi_in_sig_status.port;
                int hdmi_in_signal_status = eventData->data.hdmi_in_sig_status.status;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS  event  port: %d, signal status: %d", hdmi_in_port, hdmi_in_signal_status);
                AVInputImplementation::_instance->AVInputSignalChange(hdmi_in_port, hdmi_in_signal_status, HDMI);
            }
            else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS == eventId)
            {
                int composite_in_port = eventData->data.composite_in_sig_status.port;
                int composite_in_signal_status = eventData->data.composite_in_sig_status.status;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS  event  port: %d, signal status: %d", composite_in_port, composite_in_signal_status);
                AVInputImplementation::_instance->AVInputSignalChange(composite_in_port, composite_in_signal_status, COMPOSITE);
            }
        }

        void AVInput::dsAVStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (!AVInputImplementation::_instance)
                return;
            IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS == eventId)
            {
                int hdmi_in_port = eventData->data.hdmi_in_status.port;
                bool hdmi_in_status = eventData->data.hdmi_in_status.isPresented;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS  event  port: %d, started: %d", hdmi_in_port, hdmi_in_status);
                AVInputImplementation::_instance->AVInputStatusChange(hdmi_in_port, hdmi_in_status, HDMI);
            }
            else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS == eventId)
            {
                int composite_in_port = eventData->data.composite_in_status.port;
                bool composite_in_status = eventData->data.composite_in_status.isPresented;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS  event  port: %d, started: %d", composite_in_port, composite_in_status);
                AVInputImplementation::_instance->AVInputStatusChange(composite_in_port, composite_in_status, COMPOSITE);
            }
        }

        void AVInput::dsAVVideoModeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (!AVInputImplementation::_instance)
                return;

            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE == eventId)
            {
                IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                int hdmi_in_port = eventData->data.hdmi_in_video_mode.port;
                dsVideoPortResolution_t resolution = {};
                resolution.pixelResolution = eventData->data.hdmi_in_video_mode.resolution.pixelResolution;
                resolution.interlaced = eventData->data.hdmi_in_video_mode.resolution.interlaced;
                resolution.frameRate = eventData->data.hdmi_in_video_mode.resolution.frameRate;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE  event  port: %d, pixelResolution: %d, interlaced : %d, frameRate: %d \n", hdmi_in_port, resolution.pixelResolution, resolution.interlaced, resolution.frameRate);
                AVInputImplementation::_instance->AVInputVideoModeUpdate(hdmi_in_port, resolution, HDMI);
            }
            else if (IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE == eventId)
            {
                IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                int composite_in_port = eventData->data.composite_in_video_mode.port;
                dsVideoPortResolution_t resolution = {};
                resolution.pixelResolution = eventData->data.composite_in_video_mode.resolution.pixelResolution;
                resolution.interlaced = eventData->data.composite_in_video_mode.resolution.interlaced;
                resolution.frameRate = eventData->data.composite_in_video_mode.resolution.frameRate;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE  event  port: %d, pixelResolution: %d, interlaced : %d, frameRate: %d \n", composite_in_port, resolution.pixelResolution, resolution.interlaced, resolution.frameRate);
                AVInputImplementation::_instance->AVInputVideoModeUpdate(composite_in_port, resolution, COMPOSITE);
            }
        }

        void AVInput::dsAVGameFeatureStatusEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (!AVInputImplementation::_instance)
                return;

            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS == eventId)
            {
                IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                int hdmi_in_port = eventData->data.hdmi_in_allm_mode.port;
                bool allm_mode = eventData->data.hdmi_in_allm_mode.allm_mode;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS  event  port: %d, ALLM Mode: %d", hdmi_in_port, allm_mode);

                AVInputImplementation::_instance->AVInputALLMChange(hdmi_in_port, allm_mode);
            }
            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS == eventId)
            {
                IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                int hdmi_in_port = eventData->data.hdmi_in_vrr_mode.port;
                dsVRRType_t new_vrrType = eventData->data.hdmi_in_vrr_mode.vrr_type;
                LOGWARN("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS  event  port: %d, VRR Type: %d", hdmi_in_port, new_vrrType);

                if (new_vrrType == dsVRR_NONE)
                {
                    if (AVInputImplementation::_instance->m_currentVrrType != dsVRR_NONE)
                    {
                        AVInputImplementation::_instance->AVInputVRRChange(hdmi_in_port, AVInputImplementation::_instance->m_currentVrrType, false);
                    }
                }
                else
                {
                    if (AVInputImplementation::_instance->m_currentVrrType != dsVRR_NONE)
                    {
                        AVInputImplementation::_instance->AVInputVRRChange(hdmi_in_port, AVInputImplementation::_instance->m_currentVrrType, false);
                    }
                    AVInputImplementation::_instance->AVInputVRRChange(hdmi_in_port, new_vrrType, true);
                }
                AVInputImplementation::_instance->m_currentVrrType = new_vrrType;
            }
        }

    } // namespace Plugin
} // namespace WPEFramework

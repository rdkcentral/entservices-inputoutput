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
            RegisterAll();
        }

        AVInput::~AVInput()
        {
            SYSLOG(Logging::Shutdown, (string(_T("AVInput Destructor"))));
            UnregisterAll();
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
                DeinitializeIARM(); // <pca> TODO: Do we want to do this here/at all? </pca>s

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

    void AVInput::RegisterAll()
    {
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_NUMBER_OF_INPUTS), &AVInput::endpoint_numberOfInputs, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_CURRENT_VIDEO_MODE), &AVInput::endpoint_currentVideoMode, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_CONTENT_PROTECTED), &AVInput::endpoint_contentProtected, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GET_INPUT_DEVICES), &AVInput::getInputDevicesWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_WRITE_EDID), &AVInput::writeEDIDWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_READ_EDID), &AVInput::readEDIDWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_READ_RAWSPD), &AVInput::getRawSPDWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_READ_SPD), &AVInput::getSPDWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_SET_EDID_VERSION), &AVInput::setEdidVersionWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GET_EDID_VERSION), &AVInput::getEdidVersionWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_SET_MIXER_LEVELS), &AVInput::setMixerLevels, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_SET_EDID_ALLM_SUPPORT), &AVInput::setEdid2AllmSupportWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GET_EDID_ALLM_SUPPORT), &AVInput::getEdid2AllmSupportWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_SET_VRR_SUPPORT), &AVInput::setVRRSupportWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GET_VRR_SUPPORT), &AVInput::getVRRSupportWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GET_VRR_FRAME_RATE), &AVInput::getVRRFrameRateWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GET_HDMI_COMPATIBILITY_VERSION), &AVInput::getHdmiVersionWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_START_INPUT), &AVInput::startInput, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_STOP_INPUT), &AVInput::stopInput, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_SCALE_INPUT), &AVInput::setVideoRectangleWrapper, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_SUPPORTED_GAME_FEATURES), &AVInput::getSupportedGameFeatures, this);
        Register<JsonObject, JsonObject>(_T(AVINPUT_METHOD_GAME_FEATURE_STATUS), &AVInput::getGameFeatureStatusWrapper, this);
        m_primVolume = DEFAULT_PRIM_VOL_LEVEL;
        m_inputVolume = DEFAULT_INPUT_VOL_LEVEL;
        m_currentVrrType = dsVRR_NONE;
    }

    void AVInput::UnregisterAll()
    {
        Unregister(_T(AVINPUT_METHOD_NUMBER_OF_INPUTS));
        Unregister(_T(AVINPUT_METHOD_CURRENT_VIDEO_MODE));
        Unregister(_T(AVINPUT_METHOD_CONTENT_PROTECTED));
        Unregister(_T(AVINPUT_METHOD_GET_INPUT_DEVICES));
        Unregister(_T(AVINPUT_METHOD_WRITE_EDID));
        Unregister(_T(AVINPUT_METHOD_READ_EDID));
        Unregister(_T(AVINPUT_METHOD_READ_RAWSPD));
        Unregister(_T(AVINPUT_METHOD_READ_SPD));
        Unregister(_T(AVINPUT_METHOD_SET_VRR_SUPPORT));
        Unregister(_T(AVINPUT_METHOD_GET_VRR_SUPPORT));
        Unregister(_T(AVINPUT_METHOD_GET_VRR_FRAME_RATE));
        Unregister(_T(AVINPUT_METHOD_SET_EDID_VERSION));
        Unregister(_T(AVINPUT_METHOD_GET_EDID_VERSION));
        Unregister(_T(AVINPUT_METHOD_START_INPUT));
        Unregister(_T(AVINPUT_METHOD_STOP_INPUT));
        Unregister(_T(AVINPUT_METHOD_SCALE_INPUT));
        Unregister(_T(AVINPUT_METHOD_SUPPORTED_GAME_FEATURES));
        Unregister(_T(AVINPUT_METHOD_GAME_FEATURE_STATUS));
    }

} // namespace Plugin
} // namespace WPEFramework

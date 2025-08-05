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
#include <time.h>
#include <fstream>

#include "UtilsJsonRpc.h"

#define STR_ALLM "ALLM"
#define VRR_TYPE_HDMI "VRR-HDMI"
#define VRR_TYPE_FREESYNC "VRR-FREESYNC"
#define VRR_TYPE_FREESYNC_PREMIUM "VRR-FREESYNC-PREMIUM"
#define VRR_TYPE_FREESYNC_PREMIUM_PRO "VRR-FREESYNC-PREMIUM-PRO"

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

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(AVInputImplementation, 1, 0);
        AVInputImplementation *AVInputImplementation::_instance = nullptr;

        AVInputImplementation::AVInputImplementation() : _adminLock(), _service(nullptr)
        {
            LOGINFO("Create AVInputImplementation Instance");
            AVInputImplementation::_instance = this;
        }

        AVInputImplementation::~AVInputImplementation()
        {
            AVInputImplementation::_instance = nullptr;
            _service = nullptr;
        }

        Core::hresult AVInputImplementation::Register(Exchange::IAVInput::INotification *notification)
        {
            ASSERT(nullptr != notification);

            _adminLock.Lock();

            if (std::find(_avInputNotification.begin(), _avInputNotification.end(), notification) == _avInputNotification.end())
            {
                _avInputNotification.push_back(notification);
                notification->AddRef();
            }
            else
            {
                LOGERR("same notification is registered already");
            }

            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        Core::hresult AVInputImplementation::Unregister(Exchange::IAVInput::INotification *notification)
        {
            Core::hresult status = Core::ERROR_GENERAL;

            ASSERT(nullptr != notification);

            _adminLock.Lock();

            auto itr = std::find(_avInputNotification.begin(), _avInputNotification.end(), notification);
            if (itr != _avInputNotification.end())
            {
                (*itr)->Release();
                _avInputNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("notification not found");
            }

            _adminLock.Unlock();

            return status;
        }

        void AVInputImplementation::dispatchEvent(Event event, const JsonValue &params)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
        }

        void AVInputImplementation::Dispatch(Event event, const JsonValue params)
        {
            _adminLock.Lock();

            std::list<Exchange::IAVInput::INotification *>::const_iterator index(_avInputNotification.begin());

            switch (event)
            {
            case ON_AVINPUT_DEVICES_CHANGED:
            {
                // <pca> debug
                string devices = params.String();

                printf("*** _DEBUG: printf: ON_AVINPUT_DEVICES_CHANGED: devices=%s\n", devices.c_str());
                LOGINFO("*** _DEBUG: ON_AVINPUT_DEVICES_CHANGED: devices=%s\n", devices.c_str());

                while (index != _avInputNotification.end())
                {
                    (*index)->OnDevicesChanged(devices);
                    ++index;
                }
                // </pca>
                break;
            }
            case ON_AVINPUT_SIGNAL_CHANGED:
            {
                uint8_t id = params.Object()["id"].Number();
                string locator = params.Object()["locator"].String();
                string status = params.Object()["signalStatus"].String();
                InputSignalInfo inputSignalInfo = {id, locator, status};

                while (index != _avInputNotification.end())
                {
                    (*index)->OnSignalChanged(inputSignalInfo);
                    ++index;
                }
                break;
            }
            case ON_AVINPUT_STATUS_CHANGED:
            {
                uint8_t id = params.Object()["id"].Number();
                string locator = params.Object()["locator"].String();
                string status = params.Object()["signalStatus"].String();
                InputSignalInfo inputSignalInfo = {id, locator, status};

                while (index != _avInputNotification.end())
                {
                    (*index)->OnInputStatusChanged(inputSignalInfo);
                    ++index;
                }
                break;
            }
            case ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE:
            {
                uint8_t id = params.Object()["id"].Number();
                string locator = params.Object()["locator"].String();
                uint32_t width = params.Object()["width"].Number();
                uint32_t height = params.Object()["height"].Number();
                bool progressive = params.Object()["progressive"].Boolean();
                uint32_t frameRateN = params.Object()["frameRateN"].Number();
                uint32_t frameRateD = params.Object()["frameRateD"].Number();
                InputVideoMode videoMode = {id, locator, width, height, progressive, frameRateN, frameRateD};

                while (index != _avInputNotification.end())
                {
                    (*index)->VideoStreamInfoUpdate(videoMode);
                    ++index;
                }
                break;
            }
            case ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE:
            {
                uint8_t id = params.Object()["id"].Number();
                string gameFeature = params.Object()["gameFeature"].String();
                bool allmMode = params.Object()["allmMode"].Boolean();
                GameFeatureStatus status = {id, gameFeature, allmMode};

                while (index != _avInputNotification.end())
                {
                    (*index)->GameFeatureStatusUpdate(status);
                    ++index;
                }
                break;
            }
            case ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE:
            {
                int contentType = params.Number();
                while (index != _avInputNotification.end())
                {
                    (*index)->HdmiContentTypeUpdate(contentType);
                    ++index;
                }
                break;
            }

            default:
            {
                LOGWARN("Event[%u] not handled", event);
                break;
            }
            }
            _adminLock.Unlock();
        }

        void setResponseArray(JsonObject &response, const char *key, const vector<string> &items)
        {
            JsonArray arr;
            for (auto &i : items)
                arr.Add(JsonValue(i));

            response[key] = arr;

            string json;
            response.ToString(json);
            LOGINFO("%s: result json %s\n", __FUNCTION__, json.c_str());
        }

        uint32_t AVInputImplementation::endpoint_numberOfInputs(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            uint32_t count;
            Core::hresult ret = NumberOfInputs(count);

            if (ret == Core::ERROR_NONE)
            {
                response[_T("numberOfInputs")] = count;
            }

            returnResponse(ret == Core::ERROR_NONE);
        }

        uint32_t AVInputImplementation::endpoint_currentVideoMode(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string mode;
            string message;

            Core::hresult ret = CurrentVideoMode(mode, message);
            if (ret == Core::ERROR_NONE)
            {
                response[_T("currentVideoMode")] = mode;
            }

            returnResponse(ret == Core::ERROR_NONE);
        }

        uint32_t AVInputImplementation::endpoint_contentProtected(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            // "Ths is the way it's done in Service Manager"
            response[_T("isContentProtected")] = true;

            returnResponse(true);
        }

        Core::hresult NumberOfInputs(uint32_t &inputCount)
        {
            Core::hresult ret = Core::ERROR_NONE;

            try
            {
                inputCount = device::HdmiInput::getInstance().getNumberOfInputs();
            }
            catch (...)
            {
                LOGERR("Exception caught");
                ret = Core::ERROR_GENERAL;
            }

            return ret;
        }

        Core::hresult AVInputImplementation::CurrentVideoMode(string &currentVideoMode, string &message)
        {
            Core::hresult ret = Core::ERROR_NONE;

            try
            {
                currentVideoMode = device::HdmiInput::getInstance().getCurrentVideoMode();
                // TODO: How is message set?
            }
            catch (...)
            {
                LOGERR("Exception caught");
                ret = Core::ERROR_GENERAL;
            }

            return ret;
        }

        uint32_t AVInputImplementation::StartInput(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string sPortId = parameters["portId"].String();
            string sType = parameters["typeOfInput"].String();
            bool audioMix = parameters["requestAudioMix"].Boolean();
            int portId = 0;
            int iType = 0;
            planeType = 0; // planeType = 0 -  primary, 1 - secondary video plane type
            bool topMostPlane = parameters["topMost"].Boolean();
            LOGINFO("topMost value in thunder: %d\n", topMostPlane);
            if (parameters.HasLabel("portId") && parameters.HasLabel("typeOfInput"))
            {
                try
                {
                    portId = stoi(sPortId);
                    iType = getTypeOfInput(sType);
                    if (parameters.HasLabel("plane"))
                    {
                        string sPlaneType = parameters["plane"].String();
                        planeType = stoi(sPlaneType);
                        if (!(planeType == 0 || planeType == 1)) // planeType has to be primary(0) or secondary(1)
                        {
                            LOGWARN("planeType is invalid\n");
                            returnResponse(false);
                        }
                    }
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            try
            {
                if (iType == HDMI)
                {
                    device::HdmiInput::getInstance().selectPort(portId, audioMix, planeType, topMostPlane);
                }
                else if (iType == COMPOSITE)
                {
                    device::CompositeInput::getInstance().selectPort(portId);
                }
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(portId));
                returnResponse(false);
            }
            returnResponse(true);
        }

        uint32_t AVInputImplementation::StopInput(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string sType = parameters["typeOfInput"].String();
            int iType = 0;

            if (parameters.HasLabel("typeOfInput"))
                try
                {
                    iType = getTypeOfInput(sType);
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            try
            {
                planeType = -1;
                if (isAudioBalanceSet)
                {
                    device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, MAX_PRIM_VOL_LEVEL);
                    device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, DEFAULT_INPUT_VOL_LEVEL);
                    isAudioBalanceSet = false;
                }
                if (iType == HDMI)
                {
                    device::HdmiInput::getInstance().selectPort(-1);
                }
                else if (iType == COMPOSITE)
                {
                    device::CompositeInput::getInstance().selectPort(-1);
                }
            }
            catch (const device::Exception &err)
            {
                LOGWARN("AVInputService::stopInput Failed");
                returnResponse(false);
            }
            returnResponse(true);
        }

        uint32_t AVInputImplementation::setVideoRectangleWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            bool result = true;
            if (!parameters.HasLabel("x") && !parameters.HasLabel("y"))
            {
                result = false;
                LOGWARN("please specif coordinates (x,y)");
            }

            if (!parameters.HasLabel("w") && !parameters.HasLabel("h"))
            {
                result = false;
                LOGWARN("please specify window width and height (w,h)");
            }

            if (!parameters.HasLabel("typeOfInput"))
            {
                result = false;
                LOGWARN("please specify type of input HDMI/COMPOSITE");
            }

            if (result)
            {
                int x = 0;
                int y = 0;
                int w = 0;
                int h = 0;
                int t = 0;
                string sType;

                try
                {
                    if (parameters.HasLabel("x"))
                    {
                        x = parameters["x"].Number();
                    }
                    if (parameters.HasLabel("y"))
                    {
                        y = parameters["y"].Number();
                    }
                    if (parameters.HasLabel("w"))
                    {
                        w = parameters["w"].Number();
                    }
                    if (parameters.HasLabel("h"))
                    {
                        h = parameters["h"].Number();
                    }
                    if (parameters.HasLabel("typeOfInput"))
                    {
                        sType = parameters["typeOfInput"].String();
                        t = getTypeOfInput(sType);
                    }
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }

                if (Core::ERROR_NONE != SetVideoRectangle(x, y, w, h, t))
                {
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

            try
            {
                if (type == HDMI)
                {
                    device::HdmiInput::getInstance().scaleVideo(x, y, width, height);
                }
                else
                {
                    device::CompositeInput::getInstance().scaleVideo(x, y, width, height);
                }
            }
            catch (const device::Exception &err)
            {
                ret = Core::ERROR_GENERAL;
            }

            return ret;
        }

        JsonArray devicesToJson(Exchange::IAVInput::IInputDeviceIterator *devices)
        {
            JsonArray deviceArray;
            if (devices != nullptr)
            {
                WPEFramework::Exchange::IAVInput::InputDevice device;

                devices->Reset(0);

                while (devices->Next(device))
                {
                    JsonObject obj;
                    obj["id"] = device.id;
                    obj["locator"] = device.locator;
                    obj["connected"] = device.connected;
                    deviceArray.Add(obj);
                }
            }
            return deviceArray;
        }

        uint32_t AVInputImplementation::getInputDevicesWrapper(const JsonObject &parameters, JsonObject &response)
        {
            Exchange::IAVInput::IInputDeviceIterator *devices = nullptr;
            Core::hresult result;

            LOGINFOMETHOD();

            if (parameters.HasLabel("typeOfInput"))
            {
                string sType = parameters["typeOfInput"].String();
                int iType = 0;

                try
                {
                    iType = getTypeOfInput(sType);
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }

                result = GetInputDevices(iType, devices);
            }
            else
            {
                std::list<WPEFramework::Exchange::IAVInput::InputDevice> hdmiDevices;
                result = getInputDevices(HDMI, hdmiDevices);

                if (Core::ERROR_NONE == result)
                {
                    std::list<WPEFramework::Exchange::IAVInput::InputDevice> compositeDevices;
                    result = getInputDevices(COMPOSITE, compositeDevices);

                    if (Core::ERROR_NONE == result)
                    {
                        std::list<WPEFramework::Exchange::IAVInput::InputDevice> combinedDevices = hdmiDevices;
                        combinedDevices.insert(combinedDevices.end(), compositeDevices.begin(), compositeDevices.end());
                        devices = Core::Service<RPC::IteratorType<Exchange::IAVInput::IInputDeviceIterator>>::Create<Exchange::IAVInput::IInputDeviceIterator>(combinedDevices);
                    }
                }
            }

            if (devices != nullptr && Core::ERROR_NONE == result)
            {
                response["devices"] = devicesToJson(devices);
                devices->Release();
            }

            returnResponse(Core::ERROR_NONE == result);
        }

        uint32_t AVInputImplementation::writeEDIDWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string sPortId = parameters["portId"].String();
            int portId = 0;
            std::string message;

            if (parameters.HasLabel("portId") && parameters.HasLabel("message"))
            {
                portId = stoi(sPortId);
                message = parameters["message"].String();
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            returnResponse(Core::ERROR_NONE == WriteEDID(portId, message));
        }

        uint32_t AVInputImplementation::readEDIDWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string sPortId = parameters["portId"].String();
            int portId = 0;
            try
            {
                portId = stoi(sPortId);
            }
            catch (...)
            {
                LOGWARN("Invalid Arguments");
                returnResponse(false);
            }

            string edid;

            Core::hresult result = ReadEDID(portId, edid);
            if (Core::ERROR_NONE != result || edid.empty())
            {
                returnResponse(false);
            }
            else
            {
                response["EDID"] = edid;
                returnResponse(true);
            }
        }

        Core::hresult getInputDevices(int type, std::list<WPEFramework::Exchange::IAVInput::InputDevice> devices)
        {
            Core::hresult result = Core::ERROR_NONE;

            try
            {
                int num = 0;
                if (type == HDMI)
                {
                    num = device::HdmiInput::getInstance().getNumberOfInputs();
                }
                else if (type == COMPOSITE)
                {
                    num = device::CompositeInput::getInstance().getNumberOfInputs();
                }
                if (num > 0)
                {
                    int i = 0;
                    for (i = 0; i < num; i++)
                    {
                        // Input ID is aleays 0-indexed, continuous number starting 0
                        WPEFramework::Exchange::IAVInput::InputDevice inputDevice;

                        inputDevice.id = i;
                        std::stringstream locator;
                        if (type == HDMI)
                        {
                            locator << "hdmiin://localhost/deviceid/" << i;
                            inputDevice.connected = device::HdmiInput::getInstance().isPortConnected(i);
                        }
                        else if (type == COMPOSITE)
                        {
                            locator << "cvbsin://localhost/deviceid/" << i;
                            inputDevice.connected = device::CompositeInput::getInstance().isPortConnected(i);
                        }
                        inputDevice.locator = locator.str();
                        LOGWARN("AVInputService::getInputDevices id %d, locator=[%s], connected=[%d]", i, inputDevice.locator.c_str(), inputDevice.connected);
                        devices.push_back(inputDevice);
                    }
                }
            }
            catch (const std::exception &e)
            {
                LOGWARN("AVInputService::getInputDevices Failed");
                result = Core::ERROR_GENERAL;
            }

            return result;
        }

        Core::hresult GetInputDevices(int type, Exchange::IAVInput::IInputDeviceIterator *&devices)
        {
            std::list<WPEFramework::Exchange::IAVInput::InputDevice> list;

            Core::hresult result = getInputDevices(type, list);

            if (Core::ERROR_NONE == result)
            {
                devices = Core::Service<RPC::IteratorType<Exchange::IAVInput::IInputDeviceIterator>>::Create<Exchange::IAVInput::IInputDeviceIterator>(list);
            }
            else
            {
                devices = nullptr;
            }

            return result;
        }

        Core::hresult WriteEDID(int id, const string &edid)
        {
            // <pca> TODO: This wasn't implemented in the original code, do we want to implement it? </pca>
            return Core::ERROR_NONE;
        }

        Core::hresult ReadEDID(int id, string &edid)
        {
            vector<uint8_t> edidVec({'u', 'n', 'k', 'n', 'o', 'w', 'n'});

            try
            {
                vector<uint8_t> edidVec2;
                device::HdmiInput::getInstance().getEDIDBytesInfo(id, edidVec2);
                edidVec = edidVec2; // edidVec must be "unknown" unless we successfully get to this line

                // convert to base64
                uint16_t size = min(edidVec.size(), (size_t)numeric_limits<uint16_t>::max());

                LOGWARN("AVInputImplementation::readEDID size:%d edidVec.size:%zu", size, edidVec.size());
                if (edidVec.size() > (size_t)numeric_limits<uint16_t>::max())
                {
                    LOGERR("Size too large to use ToString base64 wpe api");
                    return Core::ERROR_GENERAL;
                }
                Core::ToString((uint8_t *)&edidVec[0], size, true, edid);
            }
            catch (const device::Exception &err)
            {
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

            Exchange::IAVInput::IInputDeviceIterator *devices;

            Core::hresult result = GetInputDevices(type, devices);
            if (Core::ERROR_NONE != result)
            {
                LOGERR("AVInputHotplug [%d, %d, %d]: Failed to get devices", input, connect, type);
                return;
            }

            JsonObject params;
            params["devices"] = devicesToJson(devices);
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

            JsonObject params;
            params["id"] = port;
            std::stringstream locator;
            if (type == HDMI)
            {
                locator << "hdmiin://localhost/deviceid/" << port;
            }
            else
            {
                locator << "cvbsin://localhost/deviceid/" << port;
            }
            params["locator"] = locator.str();
            /* values of dsHdmiInSignalStatus_t and dsCompInSignalStatus_t are same
           Hence used only HDMI macro for case statement */
            switch (signalStatus)
            {
            case dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL:
                params["signalStatus"] = "noSignal";
                break;

            case dsHDMI_IN_SIGNAL_STATUS_UNSTABLE:
                params["signalStatus"] = "unstableSignal";
                break;

            case dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED:
                params["signalStatus"] = "notSupportedSignal";
                break;

            case dsHDMI_IN_SIGNAL_STATUS_STABLE:
                params["signalStatus"] = "stableSignal";
                break;

            default:
                params["signalStatus"] = "none";
                break;
            }
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

            JsonObject params;
            params["id"] = port;
            std::stringstream locator;
            if (type == HDMI)
            {
                locator << "hdmiin://localhost/deviceid/" << port;
            }
            else if (type == COMPOSITE)
            {
                locator << "cvbsin://localhost/deviceid/" << port;
            }
            params["locator"] = locator.str();

            if (isPresented)
            {
                params["status"] = "started";
            }
            else
            {
                params["status"] = "stopped";
            }
            params["plane"] = planeType;
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
            LOGWARN("AVInputVideoModeUpdate [%d]", port);

            JsonObject params;
            params["id"] = port;
            std::stringstream locator;
            if (type == HDMI)
            {

                locator << "hdmiin://localhost/deviceid/" << port;
                switch (resolution.pixelResolution)
                {

                case dsVIDEO_PIXELRES_720x480:
                    params["width"] = 720;
                    params["height"] = 480;
                    break;

                case dsVIDEO_PIXELRES_720x576:
                    params["width"] = 720;
                    params["height"] = 576;
                    break;

                case dsVIDEO_PIXELRES_1280x720:
                    params["width"] = 1280;
                    params["height"] = 720;
                    break;

                case dsVIDEO_PIXELRES_1920x1080:
                    params["width"] = 1920;
                    params["height"] = 1080;
                    break;

                case dsVIDEO_PIXELRES_3840x2160:
                    params["width"] = 3840;
                    params["height"] = 2160;
                    break;

                case dsVIDEO_PIXELRES_4096x2160:
                    params["width"] = 4096;
                    params["height"] = 2160;
                    break;

                default:
                    params["width"] = 1920;
                    params["height"] = 1080;
                    break;
                }
                params["progressive"] = (!resolution.interlaced);
            }
            else if (type == COMPOSITE)
            {
                locator << "cvbsin://localhost/deviceid/" << port;
                switch (resolution.pixelResolution)
                {
                case dsVIDEO_PIXELRES_720x480:
                    params["width"] = 720;
                    params["height"] = 480;
                    break;
                case dsVIDEO_PIXELRES_720x576:
                    params["width"] = 720;
                    params["height"] = 576;
                    break;
                default:
                    params["width"] = 720;
                    params["height"] = 576;
                    break;
                }

                params["progressive"] = false;
            }

            params["locator"] = locator.str();
            switch (resolution.frameRate)
            {
            case dsVIDEO_FRAMERATE_24:
                params["frameRateN"] = 24000;
                params["frameRateD"] = 1000;
                break;

            case dsVIDEO_FRAMERATE_25:
                params["frameRateN"] = 25000;
                params["frameRateD"] = 1000;
                break;

            case dsVIDEO_FRAMERATE_30:
                params["frameRateN"] = 30000;
                params["frameRateD"] = 1000;
                break;

            case dsVIDEO_FRAMERATE_50:
                params["frameRateN"] = 50000;
                params["frameRateD"] = 1000;
                break;

            case dsVIDEO_FRAMERATE_60:
                params["frameRateN"] = 60000;
                params["frameRateD"] = 1000;
                break;

            case dsVIDEO_FRAMERATE_23dot98:
                params["frameRateN"] = 24000;
                params["frameRateD"] = 1001;
                break;

            case dsVIDEO_FRAMERATE_29dot97:
                params["frameRateN"] = 30000;
                params["frameRateD"] = 1001;
                break;

            case dsVIDEO_FRAMERATE_59dot94:
                params["frameRateN"] = 60000;
                params["frameRateD"] = 1001;
                break;
            case dsVIDEO_FRAMERATE_100:
                params["frameRateN"] = 100000;
                params["frameRateD"] = 1000;
                break;
            case dsVIDEO_FRAMERATE_119dot88:
                params["frameRateN"] = 120000;
                params["frameRateD"] = 1001;
                break;
            case dsVIDEO_FRAMERATE_120:
                params["frameRateN"] = 120000;
                params["frameRateD"] = 1000;
                break;
            case dsVIDEO_FRAMERATE_200:
                params["frameRateN"] = 200000;
                params["frameRateD"] = 1000;
                break;
            case dsVIDEO_FRAMERATE_239dot76:
                params["frameRateN"] = 240000;
                params["frameRateD"] = 1001;
                break;
            case dsVIDEO_FRAMERATE_240:
                params["frameRateN"] = 240000;
                params["frameRateD"] = 100;
                break;
            default:
                params["frameRateN"] = 60000;
                params["frameRateD"] = 1000;
                break;
            }

            dispatchEvent(ON_AVINPUT_VIDEO_STREAM_INFO_UPDATE, params);
        }

        void AVInputImplementation::hdmiInputAviContentTypeChange(int port, int content_type)
        {
            JsonObject params;
            params["id"] = port;
            params["aviContentType"] = content_type;
            dispatchEvent(ON_AVINPUT_AVI_CONTENT_TYPE_UPDATE, params);
        }

        void AVInputImplementation::AVInputALLMChange(int port, bool allm_mode)
        {
            JsonObject params;
            params["id"] = port;
            params["gameFeature"] = STR_ALLM;
            params["mode"] = allm_mode;

            dispatchEvent(ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE, params);
        }

        void AVInputImplementation::AVInputVRRChange(int port, dsVRRType_t vrr_type, bool vrr_mode)
        {
            JsonObject params;
            switch (vrr_type)
            {
            case dsVRR_HDMI_VRR:
                params["id"] = port;
                params["gameFeature"] = VRR_TYPE_HDMI;
                params["mode"] = vrr_mode;
                break;
            case dsVRR_AMD_FREESYNC:
                params["id"] = port;
                params["gameFeature"] = VRR_TYPE_FREESYNC;
                params["mode"] = vrr_mode;
                break;
            case dsVRR_AMD_FREESYNC_PREMIUM:
                params["id"] = port;
                params["gameFeature"] = VRR_TYPE_FREESYNC_PREMIUM;
                params["mode"] = vrr_mode;
                break;
            case dsVRR_AMD_FREESYNC_PREMIUM_PRO:
                params["id"] = port;
                params["gameFeature"] = VRR_TYPE_FREESYNC_PREMIUM_PRO;
                params["mode"] = vrr_mode;
                break;
            default:
                break;
            }
            dispatchEvent(ON_AVINPUT_GAME_FEATURE_STATUS_UPDATE, params);
        }

        uint32_t AVInputImplementation::GetSupportedGameFeatures(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();
            vector<string> supportedFeatures;
            try
            {
                device::HdmiInput::getInstance().getSupportedGameFeatures(supportedFeatures);
                for (size_t i = 0; i < supportedFeatures.size(); i++)
                {
                    LOGINFO("Supported Game Feature [%zu]:  %s\n", i, supportedFeatures.at(i).c_str());
                }
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION0();
            }

            if (supportedFeatures.empty())
            {
                returnResponse(false);
            }
            else
            {
                setResponseArray(response, "supportedGameFeatures", supportedFeatures);
                returnResponse(true);
            }
        }

        uint32_t AVInputImplementation::getGameFeatureStatusWrapper(const JsonObject &parameters, JsonObject &response)
        {
            string sGameFeature = "";
            string sPortId = parameters["portId"].String();
            int portId = 0;

            LOGINFOMETHOD();
            if (parameters.HasLabel("portId") && parameters.HasLabel("gameFeature"))
            {
                try
                {
                    portId = stoi(sPortId);
                    sGameFeature = parameters["gameFeature"].String();
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            if (strcmp(sGameFeature.c_str(), STR_ALLM) == 0)
            {
                bool allm = getALLMStatus(portId);
                LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper ALLM MODE:%d", allm);
                response["mode"] = allm;
            }
            else if (strcmp(sGameFeature.c_str(), VRR_TYPE_HDMI) == 0)
            {
                bool hdmi_vrr = false;
                dsHdmiInVrrStatus_t vrrStatus;
                getVRRStatus(portId, &vrrStatus);
                if (vrrStatus.vrrType == dsVRR_HDMI_VRR)
                    hdmi_vrr = true;
                LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper HDMI VRR MODE:%d", hdmi_vrr);
                response["mode"] = hdmi_vrr;
            }
            else if (strcmp(sGameFeature.c_str(), VRR_TYPE_FREESYNC) == 0)
            {
                bool freesync = false;
                dsHdmiInVrrStatus_t vrrStatus;
                getVRRStatus(portId, &vrrStatus);
                if (vrrStatus.vrrType == dsVRR_AMD_FREESYNC)
                    freesync = true;
                LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper FREESYNC MODE:%d", freesync);
                response["mode"] = freesync;
            }
            else if (strcmp(sGameFeature.c_str(), VRR_TYPE_FREESYNC_PREMIUM) == 0)
            {
                bool freesync_premium = false;
                dsHdmiInVrrStatus_t vrrStatus;
                getVRRStatus(portId, &vrrStatus);
                if (vrrStatus.vrrType == dsVRR_AMD_FREESYNC_PREMIUM)
                    freesync_premium = true;
                LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper FREESYNC PREMIUM MODE:%d", freesync_premium);
                response["mode"] = freesync_premium;
            }
            else if (strcmp(sGameFeature.c_str(), VRR_TYPE_FREESYNC_PREMIUM_PRO) == 0)
            {
                bool freesync_premium_pro = false;
                dsHdmiInVrrStatus_t vrrStatus;
                getVRRStatus(portId, &vrrStatus);
                if (vrrStatus.vrrType == dsVRR_AMD_FREESYNC_PREMIUM_PRO)
                    freesync_premium_pro = true;
                LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper FREESYNC PREMIUM PRO MODE:%d", freesync_premium_pro);
                response["mode"] = freesync_premium_pro;
            }
            else
            {
                LOGWARN("AVInputImplementation::getGameFeatureStatusWrapper Mode is not supported. Supported mode: ALLM, VRR-HDMI, VRR-FREESYNC-PREMIUM");
                returnResponse(false);
            }
            returnResponse(true);
        }

        bool AVInputImplementation::getALLMStatus(int iPort)
        {
            bool allm = false;

            try
            {
                device::HdmiInput::getInstance().getHdmiALLMStatus(iPort, &allm);
                LOGWARN("AVInputImplementation::getALLMStatus ALLM MODE: %d", allm);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
            }
            return allm;
        }

        bool AVInputImplementation::getVRRStatus(int iPort, dsHdmiInVrrStatus_t *vrrStatus)
        {
            bool ret = true;
            try
            {
                device::HdmiInput::getInstance().getVRRStatus(iPort, vrrStatus);
                LOGWARN("AVInputImplementation::getVRRStatus VRR TYPE: %d, VRR FRAMERATE: %f", vrrStatus->vrrType, vrrStatus->vrrAmdfreesyncFramerate_Hz);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
                ret = false;
            }
            return ret;
        }

        uint32_t AVInputImplementation::getRawSPDWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string sPortId = parameters["portId"].String();
            int portId = 0;
            if (parameters.HasLabel("portId"))
            {
                try
                {
                    portId = stoi(sPortId);
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            string spdInfo = GetRawSPD(portId);
            response["HDMISPD"] = spdInfo;
            if (spdInfo.empty())
            {
                returnResponse(false);
            }
            else
            {
                returnResponse(true);
            }
        }

        uint32_t AVInputImplementation::getSPDWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            string sPortId = parameters["portId"].String();
            int portId = 0;
            if (parameters.HasLabel("portId"))
            {
                try
                {
                    portId = stoi(sPortId);
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            string spdInfo;
            Core::hresult ret = GetSPD(portId, spdInfo);
            response["HDMISPD"] = spdInfo;
            if (spdInfo.empty() || Core::ERROR_NONE != ret)
            {
                returnResponse(false);
            }
            else
            {
                returnResponse(true);
            }
        }

        std::string AVInputImplementation::GetRawSPD(int iPort)
        {
            LOGINFO("AVInputImplementation::getSPDInfo");
            vector<uint8_t> spdVect({'u', 'n', 'k', 'n', 'o', 'w', 'n'});
            std::string spdbase64 = "";
            try
            {
                LOGWARN("AVInputImplementation::getSPDInfo");
                vector<uint8_t> spdVect2;
                device::HdmiInput::getInstance().getHDMISPDInfo(iPort, spdVect2);
                spdVect = spdVect2; // edidVec must be "unknown" unless we successfully get to this line

                // convert to base64
                uint16_t size = min(spdVect.size(), (size_t)numeric_limits<uint16_t>::max());

                LOGWARN("AVInputImplementation::getSPD size:%d spdVec.size:%zu", size, spdVect.size());

                if (spdVect.size() > (size_t)numeric_limits<uint16_t>::max())
                {
                    LOGERR("Size too large to use ToString base64 wpe api");
                    return spdbase64;
                }

                LOGINFO("------------getSPD: ");
                for (size_t itr = 0; itr < spdVect.size(); itr++)
                {
                    LOGINFO("%02X ", spdVect[itr]);
                }
                Core::ToString((uint8_t *)&spdVect[0], size, false, spdbase64);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
            }
            return spdbase64;
        }

        Core::hresult GetSPD(int id, string &spd)
        {
            vector<uint8_t> spdVect({'u', 'n', 'k', 'n', 'o', 'w', 'n'});

            LOGINFO("AVInputImplementation::getSPDInfo");

            try
            {
                LOGWARN("AVInputImplementation::getSPDInfo");
                vector<uint8_t> spdVect2;
                device::HdmiInput::getInstance().getHDMISPDInfo(id, spdVect2);
                spdVect = spdVect2; // edidVec must be "unknown" unless we successfully get to this line

                // convert to base64
                uint16_t size = min(spdVect.size(), (size_t)numeric_limits<uint16_t>::max());

                LOGWARN("AVInputImplementation::getSPD size:%d spdVec.size:%zu", size, spdVect.size());

                if (spdVect.size() > (size_t)numeric_limits<uint16_t>::max())
                {
                    LOGERR("Size too large to use ToString base64 wpe api");
                    return Core::ERROR_GENERAL;
                }

                LOGINFO("------------getSPD: ");
                for (size_t itr = 0; itr < spdVect.size(); itr++)
                {
                    LOGINFO("%02X ", spdVect[itr]);
                }

                if (spdVect.size() > 0)
                {
                    struct dsSpd_infoframe_st pre;
                    memcpy(&pre, spdVect.data(), sizeof(struct dsSpd_infoframe_st));

                    char str[200] = {0};
                    snprintf(str, sizeof(str), "Packet Type:%02X,Version:%u,Length:%u,vendor name:%s,product des:%s,source info:%02X",
                             pre.pkttype, pre.version, pre.length, pre.vendor_name, pre.product_des, pre.source_info);
                    spd = str;
                }
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(id));
                return Core::ERROR_GENERAL;
            }

            return Core::ERROR_NONE;
        }

        uint32_t AVInputImplementation::SetMixerLevels(const JsonObject &parameters, JsonObject &response)
        {
            returnIfParamNotFound(parameters, "primaryVolume");
            returnIfParamNotFound(parameters, "inputVolume");

            int primVol = 0, inputVol = 0;
            try
            {
                primVol = parameters["primaryVolume"].Number();
                inputVol = parameters["inputVolume"].Number();
            }
            catch (...)
            {
                LOGERR("Incompatible params passed !!!\n");
                response["success"] = false;
                returnResponse(false);
            }

            if ((primVol >= 0) && (inputVol >= 0))
            {
                m_primVolume = primVol;
                m_inputVolume = inputVol;
            }
            else
            {
                LOGERR("Incompatible params passed !!!\n");
                response["success"] = false;
                returnResponse(false);
            }
            if (m_primVolume > MAX_PRIM_VOL_LEVEL)
            {
                LOGWARN("Primary Volume greater than limit. Set to MAX_PRIM_VOL_LEVEL(100) !!!\n");
                m_primVolume = MAX_PRIM_VOL_LEVEL;
            }
            if (m_inputVolume > DEFAULT_INPUT_VOL_LEVEL)
            {
                LOGWARN("INPUT Volume greater than limit. Set to DEFAULT_INPUT_VOL_LEVEL(100) !!!\n");
                m_inputVolume = DEFAULT_INPUT_VOL_LEVEL;
            }

            LOGINFO("GLOBAL primary Volume=%d input Volume=%d \n", m_primVolume, m_inputVolume);

            try
            {

                device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, primVol);
                device::Host::getInstance().setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, inputVol);
            }
            catch (...)
            {
                LOGWARN("Not setting SoC volume !!!\n");
                returnResponse(false);
            }
            isAudioBalanceSet = true;
            returnResponse(true);
        }

        int setEdid2AllmSupport(int portId, bool allmSupport)
        {
            bool ret = true;
            try
            {
                device::HdmiInput::getInstance().setEdid2AllmSupport(portId, allmSupport);
                LOGWARN("AVInput -  allmsupport:%d", allmSupport);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(portId));
                ret = false;
            }
            return ret;
        }

        uint32_t AVInputImplementation::setEdid2AllmSupportWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            returnIfParamNotFound(parameters, "portId");
            returnIfParamNotFound(parameters, "allmSupport");

            int portId = 0;
            string sPortId = parameters["portId"].String();
            bool allmSupport = parameters["allmSupport"].Boolean();

            try
            {
                portId = stoi(sPortId);
            }
            catch (const std::exception &err)
            {
                LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
                returnResponse(false);
            }

            bool result = setEdid2AllmSupport(portId, allmSupport);
            if (result == true)
            {
                returnResponse(true);
            }
            else
            {
                returnResponse(false);
            }
        }

        bool GetEdid2AllmSupport(int portId, bool &allmSupportValue)
        {
            bool ret = true;
            try
            {
                device::HdmiInput::getInstance().getEdid2AllmSupport(portId, &allmSupportValue);
                LOGINFO("AVInput - getEdid2AllmSupport:%d", allmSupportValue);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(portId));
                ret = false;
            }
            return ret;
        }

        uint32_t AVInputImplementation::getEdid2AllmSupportWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();
            string sPortId = parameters["portId"].String();

            int portId = 0;
            bool allmSupport = true;
            returnIfParamNotFound(parameters, "portId");

            try
            {
                portId = stoi(sPortId);
            }
            catch (const std::exception &err)
            {
                LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
                returnResponse(false);
            }

            bool result = GetEdid2AllmSupport(portId, allmSupport);
            if (result == true)
            {
                response["allmSupport"] = allmSupport;
                returnResponse(true);
            }
            else
            {
                returnResponse(false);
            }
        }

        Core::hresult GetVRRSupport(int portId, bool &vrrSupport)
        {
            Core::hresult ret = Core::ERROR_NONE;

            try
            {
                device::HdmiInput::getInstance().getVRRSupport(portId, &vrrSupport);
                LOGINFO("AVInput - getVRRSupport:%d", vrrSupport);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(portId));
                ret = Core::ERROR_GENERAL;
            }
            return ret;
        }

        uint32_t AVInputImplementation::getVRRSupportWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();
            returnIfParamNotFound(parameters, "portId");
            string sPortId = parameters["portId"].String();

            int portId = 0;
            bool vrrSupport = true;

            try
            {
                portId = stoi(sPortId);
            }
            catch (const std::exception &err)
            {
                LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
                returnResponse(false);
            }

            Core::hresult result = GetVRRSupport(portId, vrrSupport);

            if (Core::ERROR_NONE == result)
            {
                response["vrrSupport"] = vrrSupport;
            }

            returnResponse(Core::ERROR_NONE == result);
        }

        Core::hresult SetVRRSupport(int id, bool vrrSupport)
        {
            Core::hresult ret = Core::ERROR_NONE;
            try
            {
                device::HdmiInput::getInstance().setVRRSupport(id, vrrSupport);
                LOGWARN("AVInput -  vrrSupport:%d", vrrSupport);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(id));
                ret = Core::ERROR_GENERAL;
            }
            return ret;
        }

        uint32_t AVInputImplementation::setVRRSupportWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();

            returnIfParamNotFound(parameters, "portId");
            returnIfParamNotFound(parameters, "vrrSupport");

            int portId = 0;
            string sPortId = parameters["portId"].String();
            bool vrrSupport = parameters["vrrSupport"].Boolean();

            try
            {
                portId = stoi(sPortId);
            }
            catch (const std::exception &err)
            {
                LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
                returnResponse(false);
            }

            returnResponse(Core::ERROR_NONE == SetVRRSupport(portId, vrrSupport));
        }

        uint32_t AVInputImplementation::getVRRFrameRateWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();
            returnIfParamNotFound(parameters, "portId");
            string sPortId = parameters["portId"].String();

            int portId = 0;
            dsHdmiInVrrStatus_t vrrStatus;
            vrrStatus.vrrAmdfreesyncFramerate_Hz = 0;

            try
            {
                portId = stoi(sPortId);
            }
            catch (const std::exception &err)
            {
                LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
                returnResponse(false);
            }

            bool result = getVRRStatus(portId, &vrrStatus);
            if (result == true)
            {
                response["currentVRRVideoFrameRate"] = vrrStatus.vrrAmdfreesyncFramerate_Hz;
                returnResponse(true);
            }
            else
            {
                returnResponse(false);
            }
        }

        // uint32_t AVInputImplementation::setEdidVersionWrapper(const JsonObject &parameters, JsonObject &response)
        // {
        //     LOGINFOMETHOD();
        //     string sPortId = parameters["portId"].String();
        //     int portId = 0;
        //     string sVersion = "";
        //     if (parameters.HasLabel("portId") && parameters.HasLabel("edidVersion"))
        //     {
        //         try
        //         {
        //             portId = stoi(sPortId);
        //             sVersion = parameters["edidVersion"].String();
        //         }
        //         catch (...)
        //         {
        //             LOGWARN("Invalid Arguments");
        //             returnResponse(false);
        //         }
        //     }
        //     else
        //     {
        //         LOGWARN("Required parameters are not passed");
        //         returnResponse(false);
        //     }

        //     int edidVer = -1;
        //     if (strcmp(sVersion.c_str(), "HDMI1.4") == 0)
        //     {
        //         edidVer = HDMI_EDID_VER_14;
        //     }
        //     else if (strcmp(sVersion.c_str(), "HDMI2.0") == 0)
        //     {
        //         edidVer = HDMI_EDID_VER_20;
        //     }

        //     if (edidVer < 0)
        //     {
        //         returnResponse(false);
        //     }
        //     bool result = setEdidVersion(portId, edidVer);
        //     if (result == false)
        //     {
        //         returnResponse(false);
        //     }
        //     else
        //     {
        //         returnResponse(true);
        //     }
        // }
        uint32_t AVInputImplementation::setEdidVersionWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();
            string sPortId = parameters["portId"].String();
            int portId = 0;
            string sVersion = "";
            if (parameters.HasLabel("portId") && parameters.HasLabel("edidVersion"))
            {
                try
                {
                    portId = stoi(sPortId);
                    sVersion = parameters["edidVersion"].String();
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            returnResponse(Core::ERROR_NONE == SetEdidVersion(portId, sVersion));
        }

        uint32_t AVInputImplementation::getHdmiVersionWrapper(const JsonObject &parameters, JsonObject &response)
        {
            LOGINFOMETHOD();
            returnIfParamNotFound(parameters, "portId");
            string sPortId = parameters["portId"].String();
            int portId = 0;

            try
            {
                portId = stoi(sPortId);
            }
            catch (const std::exception &err)
            {
                LOGWARN("sPortId invalid paramater: %s ", sPortId.c_str());
                returnResponse(false);
            }

            dsHdmiMaxCapabilityVersion_t hdmiCapVersion = HDMI_COMPATIBILITY_VERSION_14;

            try
            {
                device::HdmiInput::getInstance().getHdmiVersion(portId, &(hdmiCapVersion));
                LOGWARN("AVInputImplementation::getHdmiVersion Hdmi Version:%d", hdmiCapVersion);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(portId));
                returnResponse(false);
            }

            switch ((int)hdmiCapVersion)
            {
            case HDMI_COMPATIBILITY_VERSION_14:
                response["HdmiCapabilityVersion"] = "1.4";
                break;
            case HDMI_COMPATIBILITY_VERSION_20:
                response["HdmiCapabilityVersion"] = "2.0";
                break;
            case HDMI_COMPATIBILITY_VERSION_21:
                response["HdmiCapabilityVersion"] = "2.1";
                break;
            }

            if (hdmiCapVersion == HDMI_COMPATIBILITY_VERSION_MAX)
            {
                returnResponse(false);
            }
            else
            {
                returnResponse(true);
            }
        }

        // int AVInputImplementation::setEdidVersion(int iPort, int iEdidVer)
        // {
        //     bool ret = true;
        //     try
        //     {
        //         device::HdmiInput::getInstance().setEdidVersion(iPort, iEdidVer);
        //         LOGWARN("AVInputImplementation::setEdidVersion EDID Version:%d", iEdidVer);
        //     }
        //     catch (const device::Exception &err)
        //     {
        //         LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
        //         ret = false;
        //     }
        //     return ret;
        // }
        Core::hresult SetEdidVersion(int id, const string &version)
        {
            Core::hresult ret = Core::ERROR_NONE;
            int edidVer = -1;

            if (strcmp(version.c_str(), "HDMI1.4") == 0)
            {
                edidVer = HDMI_EDID_VER_14;
            }
            else if (strcmp(version.c_str(), "HDMI2.0") == 0)
            {
                edidVer = HDMI_EDID_VER_20;
            }
            else
            {
                LOGERR("Invalid EDID Version: %s", version.c_str());
                return Core::ERROR_GENERAL;
            }

            try
            {
                device::HdmiInput::getInstance().setEdidVersion(id, edidVer);
                LOGWARN("AVInputImplementation::setEdidVersion EDID Version: %s", version.c_str());
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(id));
                ret = Core::ERROR_GENERAL;
            }

            return ret;
        }

        // uint32_t AVInputImplementation::getEdidVersionWrapper(const JsonObject &parameters, JsonObject &response)
        // {
        //     string sPortId = parameters["portId"].String();
        //     int portId = 0;

        //     LOGINFOMETHOD();
        //     if (parameters.HasLabel("portId"))
        //     {
        //         try
        //         {
        //             portId = stoi(sPortId);
        //         }
        //         catch (...)
        //         {
        //             LOGWARN("Invalid Arguments");
        //             returnResponse(false);
        //         }
        //     }
        //     else
        //     {
        //         LOGWARN("Required parameters are not passed");
        //         returnResponse(false);
        //     }

        //     int edidVer = getEdidVersion(portId);
        //     switch (edidVer)
        //     {
        //     case HDMI_EDID_VER_14:
        //         response["edidVersion"] = "HDMI1.4";
        //         break;
        //     case HDMI_EDID_VER_20:
        //         response["edidVersion"] = "HDMI2.0";
        //         break;
        //     }

        //     if (edidVer < 0)
        //     {
        //         returnResponse(false);
        //     }
        //     else
        //     {
        //         returnResponse(true);
        //     }
        // }
        uint32_t AVInputImplementation::getEdidVersionWrapper(const JsonObject &parameters, JsonObject &response)
        {
            string sPortId = parameters["portId"].String();
            int portId = 0;

            LOGINFOMETHOD();
            if (parameters.HasLabel("portId"))
            {
                try
                {
                    portId = stoi(sPortId);
                }
                catch (...)
                {
                    LOGWARN("Invalid Arguments");
                    returnResponse(false);
                }
            }
            else
            {
                LOGWARN("Required parameters are not passed");
                returnResponse(false);
            }

            string version;
            Core::hresult ret = GetEdidVersion(portId, version);
            if (Core::ERROR_NONE == ret)
            {
                response["edidVersion"] = version;
                returnResponse(true);
            }
            else
            {
                LOGERR("Failed to get EDID version for port %d", portId);
                returnResponse(false);
            }
        }

        // int AVInputImplementation::getEdidVersion(int iPort)
        // {
        //     int edidVersion = -1;

        //     try
        //     {
        //         device::HdmiInput::getInstance().getEdidVersion(iPort, &edidVersion);
        //         LOGWARN("AVInputImplementation::getEdidVersion EDID Version:%d", edidVersion);
        //     }
        //     catch (const device::Exception &err)
        //     {
        //         LOG_DEVICE_EXCEPTION1(std::to_string(iPort));
        //     }
        //     return edidVersion;
        // }
        Core::hresult GetEdidVersion(int id, string &version)
        {
            Core::hresult ret = Core::ERROR_NONE;
            int edidVersion = -1;

            try
            {
                device::HdmiInput::getInstance().getEdidVersion(id, &edidVersion);
                LOGWARN("AVInputImplementation::getEdidVersion EDID Version:%d", edidVersion);
            }
            catch (const device::Exception &err)
            {
                LOG_DEVICE_EXCEPTION1(std::to_string(id));
                return Core::ERROR_GENERAL;
            }

            switch (edidVersion)
            {
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

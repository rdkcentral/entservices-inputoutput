/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
*/

#include <string>
#include "AVOutput.h"
#include "UtilsIarm.h"
#include "UtilsSearchRDKProfile.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(AVOutput,1, 0);

    AVOutput::AVOutput()
    {
        LOGINFO("CTOR\n");
    }

    AVOutput::~AVOutput()
    {
    }

    const std::string AVOutput::Initialize(PluginHost::IShell* service)
    {
        LOGINFO("Entry\n");

        if (service == nullptr)
        {
            LOGERR("Service is nullptr");
            return std::string("No service.");
        }

        profileType = searchRdkProfile();

        if (profileType == STB || profileType == NOT_FOUND)
        {
            LOGINFO("Invalid profile type for TV \n");
            return std::string("Not supported");
        }

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        DEVICE_TYPE::Initialize();

        LOGINFO("Exit\n");
        return std::string("");
    }

    void AVOutput::Deinitialize(PluginHost::IShell* service)
    {

        profileType = searchRdkProfile();

        if (profileType == STB || profileType == NOT_FOUND)
        {
            LOGINFO("Invalid profile type for TV\n");
            return ;
        }

        LOGINFO();

	DEVICE_TYPE::Deinitialize();
    }

} //namespace WPEFramework

} //namespace Plugin

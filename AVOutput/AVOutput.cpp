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

        // FIX(Manual Analysis Issue #AVOutput-1): Logic Error - Cache profileType to ensure atomic check and use
        profileType = searchRdkProfile();

        if (profileType == STB || profileType == NOT_FOUND)
        {
            LOGINFO("Invalid profile type for TV \n");
            return (std::string("Not supported"));
        }

	ASSERT(service != nullptr);
        // FIX(Manual Analysis Issue #AVOutput-2): Integer Truncation - Validate WebPrefix length before casting to uint8_t
        size_t webPrefixLen = service->WebPrefix().length();
        if (webPrefixLen > 255) {
            LOGERR("WebPrefix length %zu exceeds uint8_t max, truncating to 255\n", webPrefixLen);
            _skipURL = 255;
        } else {
            _skipURL = static_cast<uint8_t>(webPrefixLen);
        }

        // FIX(Manual Analysis Issue #AVOutput-3): Exception Handling - Wrap DEVICE_TYPE::Initialize in try-catch
        try {
            DEVICE_TYPE::Initialize();
        } catch (const std::exception& e) {
            LOGERR("DEVICE_TYPE::Initialize failed: %s\n", e.what());
            return std::string("Initialization failed: ") + e.what();
        } catch (...) {
            LOGERR("DEVICE_TYPE::Initialize failed with unknown exception\n");
            return std::string("Initialization failed with unknown error");
        }

        LOGINFO("Exit\n");
            return (service != nullptr ? _T("") : _T("No service."));
    }

    void AVOutput::Deinitialize(PluginHost::IShell* service)
    {
        // FIX(Manual Analysis Issue #AVOutput-4): Code Quality - Remove redundant profile check, always cleanup resources
        LOGINFO();

	DEVICE_TYPE::Deinitialize();
    }

} //namespace WPEFramework

} //namespace Plugin

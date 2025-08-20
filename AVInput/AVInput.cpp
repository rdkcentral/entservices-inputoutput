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
            }
            else
            {
                SYSLOG(Logging::Startup, (_T("AVInput::Initialize: Failed to initialize AVInput plugin")));
                message = _T("AVInput plugin could not be initialized");
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

        void AVInput::Deactivated(RPC::IRemoteConnection *connection)
        {
            if (connection->Id() == _connectionId)
            {
                ASSERT(nullptr != _service);
                Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }

    } // namespace Plugin
} // namespace WPEFramework

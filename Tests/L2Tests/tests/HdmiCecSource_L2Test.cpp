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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <condition_variable>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <interfaces/IHdmiCecSource.h>
// Used to change the power state for events
#include <interfaces/IPowerManager.h>

#define EVNT_TIMEOUT (5000)
#define HDMICECSOURCE_CALLSIGN _T("org.rdk.HdmiCecSource.1")
#define HDMICECSOURCE_L2TEST_CALLSIGN _T("L2tests.1")

#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using HdmiCecSourceSuccess = WPEFramework::Exchange::IHdmiCecSource::HdmiCecSourceSuccess;
using HdmiCecSourceDevice = WPEFramework::Exchange::IHdmiCecSource::HdmiCecSourceDevices;
using IHdmiCecSourceDeviceListIterator = WPEFramework::Exchange::IHdmiCecSource::IHdmiCecSourceDeviceListIterator;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

namespace {
class AsyncHandlerMock {
public:
    virtual ~AsyncHandlerMock() = default;
    virtual void onActiveSourceStatusUpdated(bool status) = 0;
    virtual void onDeviceAdded(int logicalAddress) = 0;
    virtual void onDeviceRemoved(int logicalAddress) = 0;
    virtual void onDeviceInfoUpdated(int logicalAddress) = 0;
    virtual void standbyMessageReceived(int logicalAddress) = 0;
    virtual void onKeyReleaseEvent(int logicalAddress) = 0;
    virtual void onKeyPressEvent(int logicalAddress, int keyCode) = 0;
};

class MockAsyncHandler : public AsyncHandlerMock {
public:
    MOCK_METHOD(void, onActiveSourceStatusUpdated, (bool status), (override));
    MOCK_METHOD(void, onDeviceAdded, (int logicalAddress), (override));
    MOCK_METHOD(void, onDeviceRemoved, (int logicalAddress), (override));
    MOCK_METHOD(void, onDeviceInfoUpdated, (int logicalAddress), (override));
    MOCK_METHOD(void, standbyMessageReceived, (int logicalAddress), (override));
    MOCK_METHOD(void, onKeyReleaseEvent, (int logicalAddress), (override));
    MOCK_METHOD(void, onKeyPressEvent, (int logicalAddress, int keyCode), (override));
};
}

// Event flags for different CEC events
typedef enum : uint32_t {
    ON_ACTIVE_SOURCE_STATUS_UPDATED = 0x00000001,
    ON_DEVICE_ADDED = 0x00000002,
    ON_DEVICE_REMOVED = 0x00000004,
    ON_DEVICE_INFO_UPDATED = 0x00000008,
    STANDBY_MESSAGE_RECEIVED = 0x00000010,
    ON_KEY_RELEASE_EVENT = 0x00000020,
    ON_KEY_PRESS_EVENT = 0x00000040,
    HDMICECSOURCE_STATUS_INVALID = 0x00000000
} HdmiCecSourceL2test_async_events_t;

// Notification handler for HdmiCecSource events
class HdmiCecSourceNotificationHandler : public Exchange::IHdmiCecSource::INotification {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;

    BEGIN_INTERFACE_MAP(Notification)
    INTERFACE_ENTRY(Exchange::IHdmiCecSource::INotification)
    END_INTERFACE_MAP

public:
    HdmiCecSourceNotificationHandler()
        : m_event_signalled(HDMICECSOURCE_STATUS_INVALID)
        , m_activeSourceStatus(false)
        , m_logicalAddress(0)
        , m_keyCode(0)
    {
    }

    ~HdmiCecSourceNotificationHandler() override = default;

    void OnActiveSourceStatusUpdated(const bool status) override
    {
        TEST_LOG("OnActiveSourceStatusUpdated event received, status: %d", status);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_activeSourceStatus = status;
        m_event_signalled |= ON_ACTIVE_SOURCE_STATUS_UPDATED;
        m_condition_variable.notify_one();
    }

    void OnDeviceAdded(const int logicalAddress) override
    {
        TEST_LOG("OnDeviceAdded event received, logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logicalAddress = logicalAddress;
        m_event_signalled |= ON_DEVICE_ADDED;
        m_condition_variable.notify_one();
    }

    void OnDeviceRemoved(const int logicalAddress) override
    {
        TEST_LOG("OnDeviceRemoved event received, logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logicalAddress = logicalAddress;
        m_event_signalled |= ON_DEVICE_REMOVED;
        m_condition_variable.notify_one();
    }

    void OnDeviceInfoUpdated(const int logicalAddress) override
    {
        TEST_LOG("OnDeviceInfoUpdated event received, logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logicalAddress = logicalAddress;
        m_event_signalled |= ON_DEVICE_INFO_UPDATED;
        m_condition_variable.notify_one();
    }

    void StandbyMessageReceived(const int logicalAddress) override
    {
        TEST_LOG("StandbyMessageReceived event received, logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logicalAddress = logicalAddress;
        m_event_signalled |= STANDBY_MESSAGE_RECEIVED;
        m_condition_variable.notify_one();
    }

    void OnKeyReleaseEvent(const int logicalAddress) override
    {
        TEST_LOG("OnKeyReleaseEvent event received, logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logicalAddress = logicalAddress;
        m_event_signalled |= ON_KEY_RELEASE_EVENT;
        m_condition_variable.notify_one();
    }

    void OnKeyPressEvent(const int logicalAddress, const int keyCode) override
    {
        TEST_LOG("OnKeyPressEvent event received, logicalAddress: %d, keyCode: %d", logicalAddress, keyCode);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logicalAddress = logicalAddress;
        m_keyCode = keyCode;
        m_event_signalled |= ON_KEY_PRESS_EVENT;
        m_condition_variable.notify_one();
    }

    uint32_t WaitForEvent(uint32_t timeout_ms, HdmiCecSourceL2test_async_events_t expected_status)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        auto timeout = now + std::chrono::milliseconds(timeout_ms);
        uint32_t signalled = HDMICECSOURCE_STATUS_INVALID;

        while (!(m_event_signalled & expected_status)) {
            if (m_condition_variable.wait_until(lock, timeout) == std::cv_status::timeout) {
                TEST_LOG("Timeout waiting for event: 0x%08X", expected_status);
                return HDMICECSOURCE_STATUS_INVALID;
            }
        }

        signalled = m_event_signalled & expected_status;
        m_event_signalled = HDMICECSOURCE_STATUS_INVALID;
        return signalled;
    }

    void ResetEvent()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled = HDMICECSOURCE_STATUS_INVALID;
    }

    bool GetActiveSourceStatus() const { return m_activeSourceStatus; }
    int GetLogicalAddress() const { return m_logicalAddress; }
    int GetKeyCode() const { return m_keyCode; }

private:
    bool m_activeSourceStatus;
    int m_logicalAddress;
    int m_keyCode;
};

class AsyncHandlerMock_HdmiCecSource {
public:
    AsyncHandlerMock_HdmiCecSource()
    {
        m_asyncHandlerMock = new NiceMock<MockAsyncHandler>;
    }

    virtual ~AsyncHandlerMock_HdmiCecSource()
    {
        delete m_asyncHandlerMock;
    }

    MockAsyncHandler& mock() { return *m_asyncHandlerMock; }

private:
    MockAsyncHandler* m_asyncHandlerMock;
};

class HdmiCecSource_L2Test : public L2TestMocks {
protected:
    HdmiCecSource_L2Test();
    virtual ~HdmiCecSource_L2Test() override;

public:
    uint32_t CreateHdmiCecSourceInterfaceObject();
    uint32_t WaitForRequestStatus(uint32_t timeout_ms, HdmiCecSourceL2test_async_events_t expected_status);
    void onActiveSourceStatusUpdated(const JsonObject& message);
    void onDeviceAdded(const JsonObject& message);
    void onDeviceInfoUpdated(const JsonObject& message);
    void onDeviceRemoved(const JsonObject& message);
    void standbyMessageReceived(const JsonObject& message);
    void onKeyReleaseEvent(const JsonObject& message);
    void onKeyPressEvent(const JsonObject& message);

protected:
    Exchange::IHdmiCecSource* m_cecSourcePlugin = nullptr;
    PluginHost::IShell* m_controller_cecSource = nullptr;
    Core::Sink<HdmiCecSourceNotificationHandler> m_notificationHandler;
    IARM_EventHandler_t dsHdmiEventHandler = nullptr;
    IARM_EventHandler_t powerEventHandler = nullptr;
    FrameListener* registeredListener = nullptr;
    std::vector<FrameListener*> listeners;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> HdmiCecSource_Engine;
    Core::ProxyType<RPC::CommunicatorClient> HdmiCecSource_Client;

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled = HDMICECSOURCE_STATUS_INVALID;
};

HdmiCecSource_L2Test::HdmiCecSource_L2Test()
    : L2TestMocks()
{
    TEST_LOG("HdmiCecSource_L2Test Constructor");

    // Mock IARM Bus initialization
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Init(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(IARM_RESULT_SUCCESS));

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Connect())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(IARM_RESULT_SUCCESS));

    // Mock IARM Event Registration to capture event handlers
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Invoke(
            [this](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if (strcmp(ownerName, IARM_BUS_DSMGR_NAME) == 0) {
                    if (eventId == IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG) {
                        dsHdmiEventHandler = handler;
                        TEST_LOG("Captured HDMI HotPlug Event Handler");
                    }
                } else if (strcmp(ownerName, IARM_BUS_PWRMGR_NAME) == 0) {
                    if (eventId == IARM_BUS_PWRMGR_EVENT_MODECHANGED) {
                        powerEventHandler = handler;
                        TEST_LOG("Captured Power Manager Event Handler");
                    }
                }
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_UnRegisterEventHandler(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(IARM_RESULT_SUCCESS));

    // Activate the service with retry logic
    uint32_t status = Core::ERROR_GENERAL;
    int retry_count = 0;
    const int max_retries = 10;

    while (status != Core::ERROR_NONE && retry_count < max_retries) {
        status = ActivateService("org.rdk.HdmiCecSource");
        if (status != Core::ERROR_NONE) {
            TEST_LOG("ActivateService attempt %d/%d returned: %d (%s)",
                     retry_count + 1, max_retries, status, Core::ErrorToString(status));
            retry_count++;
            if (retry_count < max_retries) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        } else {
            TEST_LOG("ActivateService succeeded on attempt %d", retry_count + 1);
        }
    }

    EXPECT_EQ(status, Core::ERROR_NONE);
}

HdmiCecSource_L2Test::~HdmiCecSource_L2Test()
{
    TEST_LOG("HdmiCecSource_L2Test Destructor");

    uint32_t status = DeactivateService("org.rdk.HdmiCecSource");
    EXPECT_EQ(status, Core::ERROR_NONE);

    if (HdmiCecSource_Client.IsValid()) {
        HdmiCecSource_Client.Release();
    }

    if (HdmiCecSource_Engine.IsValid()) {
        HdmiCecSource_Engine.Release();
    }

    TEST_LOG("HdmiCecSource_L2Test cleanup complete");
}

uint32_t HdmiCecSource_L2Test::CreateHdmiCecSourceInterfaceObject()
{
    uint32_t return_value = Core::ERROR_GENERAL;

    TEST_LOG("Creating HdmiCecSource_Engine");
    HdmiCecSource_Engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    HdmiCecSource_Client = Core::ProxyType<RPC::CommunicatorClient>::Create(
        Core::NodeId("/tmp/communicator"),
        Core::ProxyType<Core::IIPCServer>(HdmiCecSource_Engine));

    TEST_LOG("Creating HdmiCecSource_Engine Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    HdmiCecSource_Engine->Announcements(HdmiCecSource_Client->Announcement());
#endif

    if (!HdmiCecSource_Client.IsValid()) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        m_controller_cecSource = HdmiCecSource_Client->Open<PluginHost::IShell>(
            _T("org.rdk.HdmiCecSource"), ~0, 3000);
        if (m_controller_cecSource) {
            m_cecSourcePlugin = m_controller_cecSource->QueryInterface<Exchange::IHdmiCecSource>();
            if (m_cecSourcePlugin) {
                m_cecSourcePlugin->Register(&m_notificationHandler);
                return_value = Core::ERROR_NONE;
                TEST_LOG("Successfully created HdmiCecSource Plugin Interface");
            } else {
                TEST_LOG("Failed to get IHdmiCecSource interface");
            }
        } else {
            TEST_LOG("Failed to get HdmiCecSource Plugin Interface");
        }
    }
    return return_value;
}

uint32_t HdmiCecSource_L2Test::WaitForRequestStatus(uint32_t timeout_ms, HdmiCecSourceL2test_async_events_t expected_status)
{
    return m_notificationHandler.WaitForEvent(timeout_ms, expected_status);
}

void HdmiCecSource_L2Test::onActiveSourceStatusUpdated(const JsonObject& message)
{
    TEST_LOG("onActiveSourceStatusUpdated JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_ACTIVE_SOURCE_STATUS_UPDATED;
    m_condition_variable.notify_one();
}

void HdmiCecSource_L2Test::onDeviceAdded(const JsonObject& message)
{
    TEST_LOG("onDeviceAdded JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_DEVICE_ADDED;
    m_condition_variable.notify_one();
}

void HdmiCecSource_L2Test::onDeviceInfoUpdated(const JsonObject& message)
{
    TEST_LOG("onDeviceInfoUpdated JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_DEVICE_INFO_UPDATED;
    m_condition_variable.notify_one();
}

void HdmiCecSource_L2Test::onDeviceRemoved(const JsonObject& message)
{
    TEST_LOG("onDeviceRemoved JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_DEVICE_REMOVED;
    m_condition_variable.notify_one();
}

void HdmiCecSource_L2Test::standbyMessageReceived(const JsonObject& message)
{
    TEST_LOG("standbyMessageReceived JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= STANDBY_MESSAGE_RECEIVED;
    m_condition_variable.notify_one();
}

void HdmiCecSource_L2Test::onKeyReleaseEvent(const JsonObject& message)
{
    TEST_LOG("onKeyReleaseEvent JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_KEY_RELEASE_EVENT;
    m_condition_variable.notify_one();
}

void HdmiCecSource_L2Test::onKeyPressEvent(const JsonObject& message)
{
    TEST_LOG("onKeyPressEvent JSON-RPC event received");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_KEY_PRESS_EVENT;
    m_condition_variable.notify_one();
}

/*******************************************************************************************************************
 * Test Functions
 * *****************************************************************************************************************/

/**
 * @brief Test GetActiveSourceStatus API via COM-RPC
 *
 * This test verifies that the GetActiveSourceStatus API returns the correct status
 * and success flag using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetActiveSourceStatus_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing GetActiveSourceStatus via COM-RPC");

                // Declare output parameters
                bool isActiveSource = false;
                bool success = false;

                // Call the API
                uint32_t result = m_cecSourcePlugin->GetActiveSourceStatus(isActiveSource, success);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(success);

                // Log and validate output
                TEST_LOG("  isActiveSource: %d", isActiveSource);
                TEST_LOG("  success: %d", success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test GetActiveSourceStatus API via JSON-RPC
 *
 * This test verifies that the getActiveSourceStatus API returns the correct status
 * using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetActiveSourceStatus_JSONRPC)
{
    TEST_LOG("Testing getActiveSourceStatus via JSON-RPC");

    JsonObject params;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "getActiveSourceStatus", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    }

    // Validate status field
    EXPECT_TRUE(result.HasLabel("status"));
    if (result.HasLabel("status")) {
        bool activeSourceStatus = result["status"].Boolean();
        TEST_LOG("  status: %d", activeSourceStatus);
    }
}

/**
 * @brief Test SetEnabled API via COM-RPC
 *
 * This test verifies that the SetEnabled API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SetEnabled_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing SetEnabled via COM-RPC");

                // Declare output parameters
                HdmiCecSourceSuccess setResult;

                // Call the API
                uint32_t result = m_cecSourcePlugin->SetEnabled(true, setResult);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(setResult.success);

                // Log output
                TEST_LOG("  success: %d", setResult.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SetOSDName API via COM-RPC
 *
 * This test verifies that the SetOSDName API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SetOSDName_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing SetOSDName via COM-RPC");

                // Declare output parameters
                string testOSDName = "TestSTB";
                HdmiCecSourceSuccess setResult;

                // Call the API
                uint32_t result = m_cecSourcePlugin->SetOSDName(testOSDName, setResult);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(setResult.success);

                // Log output
                TEST_LOG("  osdName set to: %s", testOSDName.c_str());
                TEST_LOG("  success: %d", setResult.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SetOSDName API via JSON-RPC
 *
 * This test verifies that the setOSDName API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SetOSDName_JSONRPC)
{
    TEST_LOG("Testing setOSDName via JSON-RPC");

    JsonObject params;
    params["name"] = "TestSTB";
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "setOSDName", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    }
}

/**
 * @brief Test GetOSDName API via COM-RPC
 *
 * This test verifies that the GetOSDName API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetOSDName_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing GetOSDName via COM-RPC");

                // Declare output parameters
                string osdName;
                bool success = false;

                // Call the API
                uint32_t result = m_cecSourcePlugin->GetOSDName(osdName, success);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(success);

                // Log and validate output
                TEST_LOG("  osdName: %s", osdName.c_str());
                TEST_LOG("  success: %d", success);
                EXPECT_FALSE(osdName.empty());

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test GetOSDName API via JSON-RPC
 *
 * This test verifies that the getOSDName API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetOSDName_JSONRPC)
{
    TEST_LOG("Testing getOSDName via JSON-RPC");

    JsonObject params;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "getOSDName", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    }

    // Validate name field
    EXPECT_TRUE(result.HasLabel("name"));
    if (result.HasLabel("name")) {
        string osdName = result["name"].String();
        TEST_LOG("  name: %s", osdName.c_str());
        EXPECT_FALSE(osdName.empty());
    }
}

/**
 * @brief Test SetVendorId API via COM-RPC
 *
 * This test verifies that the SetVendorId API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SetVendorId_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing SetVendorId via COM-RPC");

                // Declare output parameters
                string testVendorId = "0019FB";
                HdmiCecSourceSuccess setResult;

                // Call the API
                uint32_t result = m_cecSourcePlugin->SetVendorId(testVendorId, setResult);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(setResult.success);

                // Log output
                TEST_LOG("  vendorId set to: %s", testVendorId.c_str());
                TEST_LOG("  success: %d", setResult.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }SetOTPEnabled API via COM-RPC
 *
 * This test verifies that the SetOTPEnabled API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SetOTPEnabled_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing SetOTPEnabled via COM-RPC");

                // Declare output parameters
                HdmiCecSourceSuccess setResult;

                // Call the API
                uint32_t result = m_cecSourcePlugin->SetOTPEnabled(true, setResult);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(setResult.success);

                // Log output
                TEST_LOG("  success: %d", setResult.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SetOTPEnabled API via JSON-RPC
 *
 * This test verifies that the setOTPEnabled API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SetOTPEnabled_JSONRPC)
{
    TEST_LOG("Testing setOTPEnabled via JSON-RPC");

    JsonObject params;
    params["enabled"] = true;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "setOTPEnabled", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    }
}

/**
 * @brief Test GetOTPEnabled API via COM-RPC
 *
 * This test verifies that the GetOTPEnabled API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetOTPEnabled_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing GetOTPEnabled via COM-RPC");

                // Declare output parameters
                bool enabled = false;
                bool success = false;

                // Call the API
                uint32_t result = m_cecSourcePlugin->GetOTPEnabled(enabled, success);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(success);

                // Log and validate output
                TEST_LOG("  enabled: %d", enabled);
                TEST_LOG("  success: %d", success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test GetOTPEnabled API via JSON-RPC
 *
 * This test verifies that the getOTPEnabled API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetOTPEnabled_JSONRPC)
{
    TEST_LOG("Testing getOTPEnabled via JSON-RPC");

    JsonObject params;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "getOTPEnabled", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    }

    // Validate enabled field
    EXPECT_TRUE(result.HasLabel("enabled"));
    if (result.HasLabel("enabled")) {
        bool enabled = result["enabled"].Boolean();
        TEST_LOG("  enabled: %d", enabled);
    }
}

/**
 * @brief Test SendStandbyMessage API via COM-RPC
 *
 * This test verifies that the SendStandbyMessage API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SendStandbyMessage_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing SendStandbyMessage via COM-RPC");

                // Declare output parameters
                HdmiCecSourceSuccess result;

                // Call the API
                uint32_t retval = m_cecSourcePlugin->SendStandbyMessage(result);

                // Validate result
                EXPECT_EQ(retval, Core::ERROR_NONE);
                if (retval != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(retval) + " (" + std::string(Core::ErrorToString(retval)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(result.success);

                // Log output
                TEST_LOG("  success: %d", result.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SendStandbyMessage API via JSON-RPC
 *
 * This test verifies that the sendStandbyMessage API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SendStandbyMessage_JSONRPC)
{
    TEST_LOG("Testing sendStandbyMessage via JSON-RPC");

    JsonObject params;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "sendStandbyMessage", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());   }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    } via COM-RPC
 *
 * This test verifies that the SendKeyPressEvent API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SendKeyPressEvent_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing SendKeyPressEvent via COM-RPC");

                // Declare input/output parameters
                uint32_t logicalAddress = 0; // TV logical address
                uint32_t keyCode = 0x00; // Select key code
                HdmiCecSourceSuccess result;

                // Call the API
                uint32_t retval = m_cecSourcePlugin->SendKeyPressEvent(logicalAddress, keyCode, result);

                // Validate result
                EXPECT_EQ(retval, Core::ERROR_NONE);
                if (retval != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(retval) + " (" + std::string(Core::ErrorToString(retval)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(result.success);

                // Log output
                TEST_LOG("  logicalAddress: %d", logicalAddress);
                TEST_LOG("  keyCode: %d", keyCode);
                TEST_LOG("  success: %d", result.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SendKeyPressEvent API via JSON-RPC
 *
 * This test verifies that the sendKeyPressEvent API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, SendKeyPressEvent_JSONRPC)
{
    TEST_LOG("Testing sendKeyPressEvent via JSON-RPC");

    JsonObject params;
    params["logicalAddress"] = 0; // TV logical address
    params["keyCode"] = 0x00; // Select key code
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "sendKeyPressEvent", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());XPECT_FALSE(vendorId.empty()); Test GetEnabled API via JSON-RPC
 *
 * This test verifies that the getEnabled API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetEnabled_JSONRPC)
{ via COM-RPC
 *
 * This test verifies that the GetDeviceList API returns the correct device information using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetDeviceList_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing GetDeviceList via COM-RPC");

                // Declare output parameters
                uint32_t numberOfDevices = 0;
                IHdmiCecSourceDeviceListIterator* deviceList = nullptr;
                bool success = false;

                // Call the API
                uint32_t result = m_cecSourcePlugin->GetDeviceList(numberOfDevices, deviceList, success);

                // Validate result
                EXPECT_EQ(result, Core::ERROR_NONE);
                if (result != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(result) + " (" + std::string(Core::ErrorToString(result)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(success);

                // Log and validate output
                TEST_LOG("  numberOfDevices: %d", numberOfDevices);
                TEST_LOG("  success: %d", success);

                if (deviceList != nullptr) {
                    HdmiCecSourceDevice device;
                    uint32_t deviceCount = 0;
                    while (deviceList->Next(device)) {
                        TEST_LOG("  Device[%d]: logicalAddress=%d, vendorID=%s, osdName=%s",
                                 deviceCount++, device.logicalAddress, device.vendorID.c_str(), device.osdName.c_str());
                        EXPECT_FALSE(device.vendorID.empty());
                        EXPECT_FALSE(device.osdName.empty());
                    }
                    EXPECT_EQ(deviceCount, numberOfDevices);
                    deviceList->Release();
                }

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test GetDeviceList API via JSON-RPC
 *
 * This test verifies that the getDeviceList API returns the correct device information using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, GetDeviceList_JSONRPC)
{
    TEST_LOG("Testing getDeviceList via JSON-RPC");

    JsonObject params;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "getDeviceList", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    }

    // Validate numberofdevices field
    EXPECT_TRUE(result.HasLabel("numberofdevices"));
    if (result.HasLabel("numberofdevices")) {
        uint32_t numberOfDevices = result["numberofdevices"].Number();
        TEST_LOG("  numberofdevices: %d", numberOfDevices);
    }

    // Validate deviceList array
    EXPECT_TRUE(result.HasLabel("deviceList"));
    if (result.HasLabel("deviceList")) {
        JsonArray deviceList = result["deviceList"].Array();
        TEST_LOG("  deviceList length: %d", deviceList.Length());

        for (uint32_t i = 0; i < deviceList.Length(); i++) {
            JsonObject device = deviceList[i].Object();

            EXPECT_TRUE(device.HasLabel("logicalAddress"));
            if (device.HasLabel("logicalAddress")) {
                uint32_t logicalAddress = device["logicalAddress"].Number();
                TEST_LOG("    Device[%d].logicalAddress: %d", i, logicalAddress);
            }

            EXPECT_TRUE(device.HasLabel("vendorID"));
            if (device.HasLabel("vendorID")) {
                string vendorID = device["vendorID"].String();
                TEST_LOG("    Device[%d].vendorID: %s", i, vendorID.c_str());
                EXPECT_FALSE(vendorID.empty());
            }

            EXPECT_TRUE(device.HasLabel("osdName"));
            if (device.HasLabel("osdName")) {
                string osdName = device["osdName"].String();
                TEST_LOG("    Device[%d].osdName: %s", i, osdName.c_str());
                EXPECT_FALSE(osdName.empty());
            }
                string testOSDName = "TestSTB";
                HdmiCecSourceSuccess setResult;
                uint32_t result = m_cecSourcePlugin->SetOSDName(testOSDName, setResult);
                EXPECT_EQ(result, Core::ERROR_NONE);
                EXPECT_TRUE(setResult.success);
 via COM-RPC
 *
 * This test verifies that the PerformOTPAction API works correctly using COM-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, PerformOTPAction_COMRPC)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                TEST_LOG("Testing PerformOTPAction via COM-RPC");

                // Declare output parameters
                HdmiCecSourceSuccess result;

                // Call the API
                uint32_t retval = m_cecSourcePlugin->PerformOTPAction(result);

                // Validate result
                EXPECT_EQ(retval, Core::ERROR_NONE);
                if (retval != Core::ERROR_NONE) {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(retval) + " (" + std::string(Core::ErrorToString(retval)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                EXPECT_TRUE(result.success);

                // Log output
                TEST_LOG("  success: %d", result.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test PerformOTPAction API via JSON-RPC
 *
 * This test verifies that the performOTPAction API works correctly using JSON-RPC interface.
 */
TEST_F(HdmiCecSource_L2Test, PerformOTPAction_JSONRPC)
{
    TEST_LOG("Testing performOTPAction via JSON-RPC");

    JsonObject params;
    JsonObject result;

    uint32_t status = InvokeServiceMethod("org.rdk.HdmiCecSource.1", "performOTPAction", params, result);

    EXPECT_EQ(status, Core::ERROR_NONE);

    // Validate success field
    EXPECT_TRUE(result.HasLabel("success"));
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
        TEST_LOG("  success: %d", result["success"].Boolean());
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                // Set Vendor ID
                string testVendorId = "0019FB";
                HdmiCecSourceSuccess setResult;
                uint32_t result = m_cecSourcePlugin->SetVendorId(testVendorId, setResult);
                EXPECT_EQ(result, Core::ERROR_NONE);
                EXPECT_TRUE(setResult.success);

                // Get Vendor ID
                string vendorId;
                bool success = false;
                result = m_cecSourcePlugin->GetVendorId(vendorId, success);
                EXPECT_EQ(result, Core::ERROR_NONE);
                EXPECT_TRUE(success);
                EXPECT_EQ(vendorId, testVendorId);
                TEST_LOG("GetVendorId: vendorId=%s, success=%d", vendorId.c_str(), success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test GetOTPEnabled/SetOTPEnabled APIs
 *
 * This test verifies that the SetOTPEnabled and GetOTPEnabled APIs work correctly.
 */
TEST_F(HdmiCecSource_L2Test, SetGetOTPEnabled)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                // Set OTP enabled to true
                HdmiCecSourceSuccess setResult;
                uint32_t result = m_cecSourcePlugin->SetOTPEnabled(true, setResult);
                EXPECT_EQ(result, Core::ERROR_NONE);
                EXPECT_TRUE(setResult.success);

                // Get OTP enabled status
                bool enabled = false;
                bool success = false;
                result = m_cecSourcePlugin->GetOTPEnabled(enabled, success);
                EXPECT_EQ(result, Core::ERROR_NONE);
                EXPECT_TRUE(success);
                EXPECT_TRUE(enabled);
                TEST_LOG("GetOTPEnabled: enabled=%d, success=%d", enabled, success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SendStandbyMessage API
 *
 * This test verifies that the SendStandbyMessage API works correctly.
 */
TEST_F(HdmiCecSource_L2Test, SendStandbyMessage)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                HdmiCecSourceSuccess result;
                uint32_t retval = m_cecSourcePlugin->SendStandbyMessage(result);

                EXPECT_EQ(retval, Core::ERROR_NONE);
                EXPECT_TRUE(result.success);
                TEST_LOG("SendStandbyMessage: success=%d", result.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test SendKeyPressEvent API
 *
 * This test verifies that the SendKeyPressEvent API works correctly.
 */
TEST_F(HdmiCecSource_L2Test, SendKeyPressEvent)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                uint32_t logicalAddress = 0; // TV logical address
                uint32_t keyCode = 0x00; // Select key code
                HdmiCecSourceSuccess result;
                uint32_t retval = m_cecSourcePlugin->SendKeyPressEvent(logicalAddress, keyCode, result);

                EXPECT_EQ(retval, Core::ERROR_NONE);
                EXPECT_TRUE(result.success);
                TEST_LOG("SendKeyPressEvent: logicalAddress=%d, keyCode=%d, success=%d",
                         logicalAddress, keyCode, result.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test GetDeviceList API
 *
 * This test verifies that the GetDeviceList API returns the correct device information.
 */
TEST_F(HdmiCecSource_L2Test, GetDeviceList)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                uint32_t numberOfDevices = 0;
                IHdmiCecSourceDeviceListIterator* deviceList = nullptr;
                bool success = false;

                uint32_t result = m_cecSourcePlugin->GetDeviceList(numberOfDevices, deviceList, success);

                EXPECT_EQ(result, Core::ERROR_NONE);
                EXPECT_TRUE(success);
                TEST_LOG("GetDeviceList: numberOfDevices=%d, success=%d", numberOfDevices, success);

                if (deviceList != nullptr) {
                    HdmiCecSourceDevice device;
                    while (deviceList->Next(device)) {
                        TEST_LOG("Device: logicalAddress=%d, vendorID=%s, osdName=%s",
                                 device.logicalAddress, device.vendorID.c_str(), device.osdName.c_str());
                    }
                    deviceList->Release();
                }

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test PerformOTPAction API
 *
 * This test verifies that the PerformOTPAction API works correctly.
 */
TEST_F(HdmiCecSource_L2Test, PerformOTPAction)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                HdmiCecSourceSuccess result;
                uint32_t retval = m_cecSourcePlugin->PerformOTPAction(result);

                EXPECT_EQ(retval, Core::ERROR_NONE);
                EXPECT_TRUE(result.success);
                TEST_LOG("PerformOTPAction: success=%d", result.success);

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test OnActiveSourceStatusUpdated event
 *
 * This test verifies that the OnActiveSourceStatusUpdated event is received correctly.
 */
TEST_F(HdmiCecSource_L2Test, OnActiveSourceStatusUpdatedEvent)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                // Simulate active source status change
                m_notificationHandler.OnActiveSourceStatusUpdated(true);

                uint32_t status = WaitForRequestStatus(EVNT_TIMEOUT, ON_ACTIVE_SOURCE_STATUS_UPDATED);
                EXPECT_EQ(status, ON_ACTIVE_SOURCE_STATUS_UPDATED);
                EXPECT_TRUE(m_notificationHandler.GetActiveSourceStatus());
                TEST_LOG("OnActiveSourceStatusUpdated event verified");

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

/**
 * @brief Test OnDeviceAdded event
 *
 * This test verifies that the OnDeviceAdded event is received correctly.
 */
TEST_F(HdmiCecSource_L2Test, OnDeviceAddedEvent)
{
    if (CreateHdmiCecSourceInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSource_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSource != nullptr);
        if (m_controller_cecSource) {
            EXPECT_TRUE(m_cecSourcePlugin != nullptr);
            if (m_cecSourcePlugin) {
                // Simulate device added event
                int testLogicalAddress = 4;
                m_notificationHandler.OnDeviceAdded(testLogicalAddress);

                uint32_t status = WaitForRequestStatus(EVNT_TIMEOUT, ON_DEVICE_ADDED);
                EXPECT_EQ(status, ON_DEVICE_ADDED);
                EXPECT_EQ(m_notificationHandler.GetLogicalAddress(), testLogicalAddress);
                TEST_LOG("OnDeviceAdded event verified");

                m_cecSourcePlugin->Unregister(&m_notificationHandler);
                m_cecSourcePlugin->Release();
            } else {
                TEST_LOG("m_cecSourcePlugin is NULL");
            }
            m_controller_cecSource->Release();
        } else {
            TEST_LOG("m_controller_cecSource is NULL");
        }
    }
}

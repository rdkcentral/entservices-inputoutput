/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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
#include <interfaces/IHdcpProfile.h>
#include <interfaces/IPowerManager.h>

#define EVNT_TIMEOUT (5000)
#define HDCPPROFILE_CALLSIGN _T("org.rdk.HdcpProfile.1")
#define HDCPPROFILE_L2TEST_CALLSIGN _T("L2tests.1")

#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using HDCPStatus = WPEFramework::Exchange::IHdcpProfile::HDCPStatus;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

namespace {
    /**
     * @brief Listener for JSON-RPC notifications
     */
    class JSONRPCDirectLink : public Core::JSONRPC::ILocalDispatcher::ICallback {
    public:
        JSONRPCDirectLink()
            : _messages()
        {
        }

        void Message(const Core::JSONRPC::Context& context, const string& message) override
        {
            if (!message.empty()) {
                Core::JSONRPC::Message jsonrpcMessage;
                jsonrpcMessage.FromString(message);
                _messages.push_back(jsonrpcMessage);
            }
        }

        std::vector<Core::JSONRPC::Message> GetMessages()
        {
            return _messages;
        }

    private:
        std::vector<Core::JSONRPC::Message> _messages;
    };
}

// Event flags for different HDCP events
typedef enum : uint32_t {
    ON_DISPLAY_CONNECTION_CHANGED = 0x00000001,
    HDCPPROFILE_STATUS_INVALID = 0x00000000
} HdcpProfileL2test_async_events_t;

// Notification handler for HdcpProfile events
class HdcpProfileNotificationHandler : public Exchange::IHdcpProfile::INotification {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;

    BEGIN_INTERFACE_MAP(Notification)
    INTERFACE_ENTRY(Exchange::IHdcpProfile::INotification)
    END_INTERFACE_MAP

public:
    HdcpProfileNotificationHandler()
        : m_event_signalled(HDCPPROFILE_STATUS_INVALID)
    {
    }

    void OnDisplayConnectionChanged(const HDCPStatus& hdcpStatus) override
    {
        TEST_LOG("OnDisplayConnectionChanged notification received");
        TEST_LOG("  isConnected: %d", hdcpStatus.isConnected);
        TEST_LOG("  isHDCPCompliant: %d", hdcpStatus.isHDCPCompliant);
        TEST_LOG("  isHDCPEnabled: %d", hdcpStatus.isHDCPEnabled);
        TEST_LOG("  hdcpReason: %d", hdcpStatus.hdcpReason);
        TEST_LOG("  supportedHDCPVersion: %s", hdcpStatus.supportedHDCPVersion.c_str());
        TEST_LOG("  receiverHDCPVersion: %s", hdcpStatus.receiverHDCPVersion.c_str());
        TEST_LOG("  currentHDCPVersion: %s", hdcpStatus.currentHDCPVersion.c_str());

        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_DISPLAY_CONNECTION_CHANGED;
        m_lastHdcpStatus = hdcpStatus;
        m_condition_variable.notify_one();
    }

    uint32_t WaitForEvent(uint32_t timeout_ms, HdcpProfileL2test_async_events_t expected_status)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        
        if (m_condition_variable.wait_until(lock, now + std::chrono::milliseconds(timeout_ms), 
            [this, expected_status]() { return (m_event_signalled & expected_status) != 0; })) {
            return m_event_signalled;
        }
        
        TEST_LOG("Timeout waiting for event 0x%08X, got 0x%08X", expected_status, m_event_signalled);
        return HDCPPROFILE_STATUS_INVALID;
    }

    void ResetEvent()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled = HDCPPROFILE_STATUS_INVALID;
    }

    HDCPStatus GetLastHdcpStatus()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_lastHdcpStatus;
    }

private:
    HDCPStatus m_lastHdcpStatus;
};

class AsyncHandlerMock_HdcpProfile {
public:
    AsyncHandlerMock_HdcpProfile()
    {
    }

    MOCK_METHOD(void, onDisplayConnectionChanged, (const JsonObject& parameters));
};

class HdcpProfile_L2Test : public L2TestMocks {
protected:
    HdcpProfile_L2Test()
        : L2TestMocks()
    {
        TEST_LOG("HdcpProfile_L2Test Constructor");
    }

public:
    uint32_t CreateHdcpProfileInterfaceObject();
    uint32_t WaitForRequestStatus(uint32_t timeout_ms, HdcpProfileL2test_async_events_t expected_status);
    void onDisplayConnectionChanged(const JsonObject& message);

protected:
    Exchange::IHdcpProfile* m_hdcpProfilePlugin = nullptr;
    PluginHost::IShell* m_controller_hdcpProfile = nullptr;
    Core::Sink<HdcpProfileNotificationHandler> m_notificationHandler;
    IARM_EventHandler_t dsHdmiEventHandler = nullptr;
    IARM_EventHandler_t powerEventHandler = nullptr;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> HdcpProfile_Engine;
    Core::ProxyType<RPC::CommunicatorClient> HdcpProfile_Client;

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled = HDCPPROFILE_STATUS_INVALID;
};

HdcpProfile_L2Test::HdcpProfile_L2Test()
    : L2TestMocks()
{
    TEST_LOG("Initializing HdcpProfile L2 Test Environment");

    // Initialize Device Manager mocks
    device::Manager::Initialize();

    // Mock IARM Bus initialization
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Init(::testing::_))
        .WillByDefault(::testing::Return(IARM_RESULT_SUCCESS));

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Connect())
        .WillByDefault(::testing::Return(IARM_RESULT_SUCCESS));

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Init(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(IARM_RESULT_SUCCESS));

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Connect())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(IARM_RESULT_SUCCESS));

    // Mock IARM event registration
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Invoke(
            [this](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if (strcmp(ownerName, IARM_BUS_DSMGR_NAME) == 0) {
                    if (eventId == IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG) {
                        dsHdmiEventHandler = handler;
                    }
                } else if (strcmp(ownerName, IARM_BUS_PWRMGR_NAME) == 0) {
                    powerEventHandler = handler;
                }
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_UnRegisterEventHandler(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(IARM_RESULT_SUCCESS));

    // Mock IARM Bus calls
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                TEST_LOG("IARM_Bus_Call: %s.%s", ownerName, methodName);
                return IARM_RESULT_SUCCESS;
            }));

    // Setup VideoOutputPort mocks
    ON_CALL(*p_videoOutputPortMock, getType())
        .WillByDefault(::testing::Return(device::VideoOutputPortType::kHDMI));

    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    EXPECT_CALL(*p_videoOutputPortMock, getType())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(device::VideoOutputPortType::kHDMI));

    EXPECT_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(true));

    EXPECT_CALL(*p_videoOutputPortMock, getHDCPStatus())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(dsHDCP_STATUS_AUTHENTICATED));

    EXPECT_CALL(*p_videoOutputPortMock, getHdcpProtocolVersion())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(dsHDCP_VERSION_2X));

    EXPECT_CALL(*p_videoOutputPortMock, getHdcpReceiverProtocol())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(dsHDCP_VERSION_2X));

    EXPECT_CALL(*p_videoOutputPortMock, getHdcpCurrentProtocol())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(dsHDCP_VERSION_2X));

    // Setup Host singleton mocks
    ON_CALL(*p_hostMock, getVideoOutputPort(::testing::_))
        .WillByDefault(::testing::Return(device::VideoOutputPort::getInstance()));

    EXPECT_CALL(*p_hostMock, getVideoOutputPort(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(device::VideoOutputPort::getInstance()));

    // Create COM-RPC infrastructure
    HdcpProfile_Engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    HdcpProfile_Client = Core::ProxyType<RPC::CommunicatorClient>::Create(
        Core::NodeId("/tmp/communicator"),
        Core::ProxyType<Core::IIPCServer>(HdcpProfile_Engine));

    // Activate the plugin
    CreateHdcpProfileInterfaceObject();
}

HdcpProfile_L2Test::~HdcpProfile_L2Test()
{
    TEST_LOG("HdcpProfile_L2Test Destructor");

    if (m_hdcpProfilePlugin != nullptr) {
        m_hdcpProfilePlugin->Unregister(&m_notificationHandler);
        m_hdcpProfilePlugin->Release();
        m_hdcpProfilePlugin = nullptr;
    }

    if (m_controller_hdcpProfile != nullptr) {
        PluginHost::IShell* controller = m_controller_hdcpProfile;
        controller->Release();
        m_controller_hdcpProfile = nullptr;
    }

    if (HdcpProfile_Client.IsValid()) {
        HdcpProfile_Client.Release();
    }

    if (HdcpProfile_Engine.IsValid()) {
        HdcpProfile_Engine.Release();
    }

    device::Manager::DeInitialize();
    TEST_LOG("HdcpProfile_L2Test Destructor - Cleanup Complete");
}

void HdcpProfile_L2Test::onDisplayConnectionChanged(const JsonObject& message)
{
    TEST_LOG("JSON-RPC onDisplayConnectionChanged event received");
    
    std::unique_lock<std::mutex> lock(m_mutex);
    m_event_signalled |= ON_DISPLAY_CONNECTION_CHANGED;
    m_condition_variable.notify_one();
}

uint32_t HdcpProfile_L2Test::WaitForRequestStatus(uint32_t timeout_ms, HdcpProfileL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    
    if (m_condition_variable.wait_until(lock, now + std::chrono::milliseconds(timeout_ms),
        [this, expected_status]() { return (m_event_signalled & expected_status) != 0; })) {
        return m_event_signalled;
    }
    
    TEST_LOG("Timeout waiting for event 0x%08X, got 0x%08X", expected_status, m_event_signalled);
    return HDCPPROFILE_STATUS_INVALID;
}

MATCHER_P(MatchRequest, data, "")
{
    bool match = true;
    std::string expected;
    std::string actual;

    data.ToString(expected);
    arg.ToString(actual);
    TEST_LOG(" rec = %s, arg = %s", expected.c_str(), actual.c_str());
    EXPECT_STREQ(expected.c_str(), actual.c_str());

    return match;
}

uint32_t HdcpProfile_L2Test::CreateHdcpProfileInterfaceObject()
{
    TEST_LOG("Creating HdcpProfile interface object");
    
    m_controller_hdcpProfile = m_controller->QueryInterfaceByCallsign<PluginHost::IShell>(HDCPPROFILE_CALLSIGN);
    
    if (m_controller_hdcpProfile == nullptr) {
        TEST_LOG("Failed to get controller interface");
        return Core::ERROR_GENERAL;
    }

#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    m_controller_hdcpProfile->Activate(PluginHost::IShell::REQUESTED);
#endif

    string result;
    if ((result = m_controller_hdcpProfile->Instantiate(3000, HDCPPROFILE_CALLSIGN, _T("HdcpProfileImplementation"), 0, _T(""))) == _T("")) {
        m_hdcpProfilePlugin = m_controller_hdcpProfile->QueryInterface<Exchange::IHdcpProfile>();
        if (m_hdcpProfilePlugin != nullptr) {
            m_hdcpProfilePlugin->Register(&m_notificationHandler);
            TEST_LOG("HdcpProfile interface created successfully");
            return Core::ERROR_NONE;
        }
    }
    
    TEST_LOG("Failed to instantiate HdcpProfile plugin: %s", result.c_str());
    return Core::ERROR_GENERAL;
}

// ============================= COM-RPC Test Cases =============================

// Test case to validate GetHDCPStatus using COM-RPC
TEST_F(HdcpProfile_L2Test, GetHDCPStatus_COMRPC)
{
    TEST_LOG("Testing GetHDCPStatus via COM-RPC");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    HDCPStatus hdcpStatus;
    bool success = false;
    
    uint32_t result = m_hdcpProfilePlugin->GetHDCPStatus(hdcpStatus, success);
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
    
    TEST_LOG("HDCP Status:");
    TEST_LOG("  isConnected: %d", hdcpStatus.isConnected);
    TEST_LOG("  isHDCPCompliant: %d", hdcpStatus.isHDCPCompliant);
    TEST_LOG("  isHDCPEnabled: %d", hdcpStatus.isHDCPEnabled);
    TEST_LOG("  hdcpReason: %d", hdcpStatus.hdcpReason);
    TEST_LOG("  supportedHDCPVersion: %s", hdcpStatus.supportedHDCPVersion.c_str());
    TEST_LOG("  receiverHDCPVersion: %s", hdcpStatus.receiverHDCPVersion.c_str());
    TEST_LOG("  currentHDCPVersion: %s", hdcpStatus.currentHDCPVersion.c_str());
    
    // Validate basic expectations
    EXPECT_TRUE(hdcpStatus.isConnected);
    EXPECT_FALSE(hdcpStatus.supportedHDCPVersion.empty());
}

// Test case to validate GetSettopHDCPSupport using COM-RPC
TEST_F(HdcpProfile_L2Test, GetSettopHDCPSupport_COMRPC)
{
    TEST_LOG("Testing GetSettopHDCPSupport via COM-RPC");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    string supportedHDCPVersion;
    bool isHDCPSupported = false;
    bool success = false;
    
    uint32_t result = m_hdcpProfilePlugin->GetSettopHDCPSupport(supportedHDCPVersion, isHDCPSupported, success);
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
    EXPECT_TRUE(isHDCPSupported);
    EXPECT_FALSE(supportedHDCPVersion.empty());
    
    TEST_LOG("Settop HDCP Support:");
    TEST_LOG("  isHDCPSupported: %d", isHDCPSupported);
    TEST_LOG("  supportedHDCPVersion: %s", supportedHDCPVersion.c_str());
}

// Test case to validate OnDisplayConnectionChanged event using COM-RPC
TEST_F(HdcpProfile_L2Test, OnDisplayConnectionChanged_Event_COMRPC)
{
    TEST_LOG("Testing OnDisplayConnectionChanged event via COM-RPC");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    m_notificationHandler.ResetEvent();
    
    // Simulate HDMI hotplug event
    if (dsHdmiEventHandler != nullptr) {
        IARM_Bus_DSMgr_EventData_t eventData;
        eventData.data.hdmi_hpd.event = dsDISPLAY_EVENT_CONNECTED;
        TEST_LOG("Triggering HDMI hotplug event");
        dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, &eventData, sizeof(eventData));
    }
    
    // Wait for the event notification
    uint32_t status = m_notificationHandler.WaitForEvent(EVNT_TIMEOUT, ON_DISPLAY_CONNECTION_CHANGED);
    EXPECT_NE(status, HDCPPROFILE_STATUS_INVALID);
    EXPECT_TRUE((status & ON_DISPLAY_CONNECTION_CHANGED) != 0);
    
    // Validate the received status
    HDCPStatus hdcpStatus = m_notificationHandler.GetLastHdcpStatus();
    TEST_LOG("Event received with status:");
    TEST_LOG("  isConnected: %d", hdcpStatus.isConnected);
    TEST_LOG("  isHDCPCompliant: %d", hdcpStatus.isHDCPCompliant);
    TEST_LOG("  isHDCPEnabled: %d", hdcpStatus.isHDCPEnabled);
}

// Test case to validate HDMI disconnect event using COM-RPC
TEST_F(HdcpProfile_L2Test, OnDisplayDisconnection_Event_COMRPC)
{
    TEST_LOG("Testing display disconnection event via COM-RPC");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    // First, simulate connection
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));
    
    m_notificationHandler.ResetEvent();
    
    // Now simulate disconnection
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(false));
    
    if (dsHdmiEventHandler != nullptr) {
        IARM_Bus_DSMgr_EventData_t eventData;
        eventData.data.hdmi_hpd.event = dsDISPLAY_EVENT_DISCONNECTED;
        TEST_LOG("Triggering HDMI disconnect event");
        dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, &eventData, sizeof(eventData));
    }
    
    // Wait for the event notification
    uint32_t status = m_notificationHandler.WaitForEvent(EVNT_TIMEOUT, ON_DISPLAY_CONNECTION_CHANGED);
    EXPECT_NE(status, HDCPPROFILE_STATUS_INVALID);
    
    HDCPStatus hdcpStatus = m_notificationHandler.GetLastHdcpStatus();
    EXPECT_FALSE(hdcpStatus.isConnected);
    TEST_LOG("Display disconnected successfully");
}

// ============================= JSON-RPC Test Cases =============================

// Test case to validate getHDCPStatus using JSON-RPC
TEST_F(HdcpProfile_L2Test, GetHDCPStatus_JSONRPC)
{
    TEST_LOG("Testing getHDCPStatus via JSON-RPC");
    
    ASSERT_NE(m_controller_hdcpProfile, nullptr);
    
    JsonObject params;
    JsonObject result;
    
    uint32_t status = InvokeServiceMethod("org.rdk.HdcpProfile.1", "getHDCPStatus", params, result);
    
    EXPECT_EQ(status, Core::ERROR_NONE);
    
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
    }
    
    if (result.HasLabel("isConnected")) {
        TEST_LOG("  isConnected: %d", result["isConnected"].Boolean());
        EXPECT_TRUE(result["isConnected"].Boolean());
    }
    
    if (result.HasLabel("isHDCPCompliant")) {
        TEST_LOG("  isHDCPCompliant: %d", result["isHDCPCompliant"].Boolean());
    }
    
    if (result.HasLabel("isHDCPEnabled")) {
        TEST_LOG("  isHDCPEnabled: %d", result["isHDCPEnabled"].Boolean());
    }
    
    if (result.HasLabel("hdcpReason")) {
        TEST_LOG("  hdcpReason: %d", result["hdcpReason"].Number());
    }
    
    if (result.HasLabel("supportedHDCPVersion")) {
        string version = result["supportedHDCPVersion"].String();
        TEST_LOG("  supportedHDCPVersion: %s", version.c_str());
        EXPECT_FALSE(version.empty());
    }
    
    if (result.HasLabel("receiverHDCPVersion")) {
        TEST_LOG("  receiverHDCPVersion: %s", result["receiverHDCPVersion"].String().c_str());
    }
    
    if (result.HasLabel("currentHDCPVersion")) {
        TEST_LOG("  currentHDCPVersion: %s", result["currentHDCPVersion"].String().c_str());
    }
}

// Test case to validate getSettopHDCPSupport using JSON-RPC
TEST_F(HdcpProfile_L2Test, GetSettopHDCPSupport_JSONRPC)
{
    TEST_LOG("Testing getSettopHDCPSupport via JSON-RPC");
    
    ASSERT_NE(m_controller_hdcpProfile, nullptr);
    
    JsonObject params;
    JsonObject result;
    
    uint32_t status = InvokeServiceMethod("org.rdk.HdcpProfile.1", "getSettopHDCPSupport", params, result);
    
    EXPECT_EQ(status, Core::ERROR_NONE);
    
    if (result.HasLabel("success")) {
        EXPECT_TRUE(result["success"].Boolean());
    }
    
    if (result.HasLabel("isHDCPSupported")) {
        bool isSupported = result["isHDCPSupported"].Boolean();
        TEST_LOG("  isHDCPSupported: %d", isSupported);
        EXPECT_TRUE(isSupported);
    }
    
    if (result.HasLabel("supportedHDCPVersion")) {
        string version = result["supportedHDCPVersion"].String();
        TEST_LOG("  supportedHDCPVersion: %s", version.c_str());
        EXPECT_FALSE(version.empty());
    }
}

// Test case to validate onDisplayConnectionChanged event using JSON-RPC
TEST_F(HdcpProfile_L2Test, OnDisplayConnectionChanged_Event_JSONRPC)
{
    TEST_LOG("Testing onDisplayConnectionChanged event via JSON-RPC");
    
    ASSERT_NE(m_controller_hdcpProfile, nullptr);
    
    // Subscribe to the event
    status_t status = Core::ERROR_GENERAL;
    std::function<void(const JsonObject&)> method = [this](const JsonObject& parameters) {
        onDisplayConnectionChanged(parameters);
    };
    
    status = SubscribeToEvent(_T("org.rdk.HdcpProfile.1"), _T("onDisplayConnectionChanged"), method);
    EXPECT_EQ(status, Core::ERROR_NONE);
    
    m_event_signalled = HDCPPROFILE_STATUS_INVALID;
    
    // Trigger HDMI hotplug event
    if (dsHdmiEventHandler != nullptr) {
        IARM_Bus_DSMgr_EventData_t eventData;
        eventData.data.hdmi_hpd.event = dsDISPLAY_EVENT_CONNECTED;
        TEST_LOG("Triggering HDMI hotplug event for JSON-RPC");
        dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, &eventData, sizeof(eventData));
    }
    
    // Wait for the event
    uint32_t eventStatus = WaitForRequestStatus(EVNT_TIMEOUT, ON_DISPLAY_CONNECTION_CHANGED);
    EXPECT_NE(eventStatus, HDCPPROFILE_STATUS_INVALID);
    EXPECT_TRUE((eventStatus & ON_DISPLAY_CONNECTION_CHANGED) != 0);
    
    // Unsubscribe from the event
    UnsubscribeFromEvent(_T("org.rdk.HdcpProfile.1"), _T("onDisplayConnectionChanged"));
    
    TEST_LOG("Successfully received onDisplayConnectionChanged event via JSON-RPC");
}

// Test case to validate multiple status queries
TEST_F(HdcpProfile_L2Test, MultipleStatusQueries_COMRPC)
{
    TEST_LOG("Testing multiple HDCP status queries");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    for (int i = 0; i < 5; i++) {
        HDCPStatus hdcpStatus;
        bool success = false;
        
        uint32_t result = m_hdcpProfilePlugin->GetHDCPStatus(hdcpStatus, success);
        
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_TRUE(success);
        EXPECT_TRUE(hdcpStatus.isConnected);
        
        TEST_LOG("Query %d - Status retrieved successfully", i + 1);
        
        // Small delay between queries
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Test case to validate HDCP version consistency
TEST_F(HdcpProfile_L2Test, HDCPVersionConsistency_COMRPC)
{
    TEST_LOG("Testing HDCP version consistency");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    // Get status
    HDCPStatus hdcpStatus;
    bool success = false;
    uint32_t result = m_hdcpProfilePlugin->GetHDCPStatus(hdcpStatus, success);
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
    
    // Get settop support
    string supportedVersion;
    bool isSupported = false;
    result = m_hdcpProfilePlugin->GetSettopHDCPSupport(supportedVersion, isSupported, success);
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
    
    // Verify that supportedVersion matches between both calls
    EXPECT_EQ(hdcpStatus.supportedHDCPVersion, supportedVersion);
    TEST_LOG("Version consistency verified: %s", supportedVersion.c_str());
}

// Test case to validate behavior when display is not connected
TEST_F(HdcpProfile_L2Test, StatusWhenDisplayNotConnected_COMRPC)
{
    TEST_LOG("Testing status when display is not connected");
    
    ASSERT_NE(m_hdcpProfilePlugin, nullptr);
    
    // Mock display as disconnected
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(false));
    
    HDCPStatus hdcpStatus;
    bool success = false;
    
    uint32_t result = m_hdcpProfilePlugin->GetHDCPStatus(hdcpStatus, success);
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
    EXPECT_FALSE(hdcpStatus.isConnected);
    
    TEST_LOG("Status correctly reflects disconnected display");
}

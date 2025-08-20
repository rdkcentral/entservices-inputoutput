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
#include <interfaces/IHdmiCecSink.h>

#define EVNT_TIMEOUT (5000)
#define HDMICECSINK_CALLSIGN _T("org.rdk.HdmiCecSink.1")
#define HDMICECSINKTEST_CALLSIGN _T("L2tests.1")

#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using HdmiCecSinkSuccess = WPEFramework::Exchange::IHdmiCecSink::HdmiCecSinkSuccess;
using HdmiCecSinkDevice = WPEFramework::Exchange::IHdmiCecSink::HdmiCecSinkDevice;
using HdmiCecSinkActivePath = WPEFramework::Exchange::IHdmiCecSink::HdmiCecSinkActivePath;
using IHdmiCecSinkDeviceListIterator = WPEFramework::Exchange::IHdmiCecSink::IHdmiCecSinkDeviceListIterator;
using IHdmiCecSinkActivePathIterator = WPEFramework::Exchange::IHdmiCecSink::IHdmiCecSinkActivePathIterator;

namespace {
static void removeFile(const char* fileName)
{
    if (std::remove(fileName) != 0) {
        printf("File %s failed to remove\n", fileName);
        perror("Error deleting file");
    } else {
        printf("File %s successfully deleted\n", fileName);
    }
}

static void createFile(const char* fileName, const char* fileContent)
{
    removeFile(fileName);

    std::ofstream fileContentStream(fileName);
    fileContentStream << fileContent;
    fileContentStream << "\n";
    fileContentStream.close();
}
}

// Event flags for different CEC events
typedef enum : uint32_t {
    ON_ACTIVE_SOURCE_CHANGE = 0x00000001,
    ON_DEVICE_ADDED = 0x00000002,
    ON_DEVICE_REMOVED = 0x00000004,
    ON_DEVICE_INFO_UPDATED = 0x00000008,
    ON_IMAGE_VIEW_ON = 0x00000010,
    ON_TEXT_VIEW_ON = 0x00000020,
    ON_INACTIVE_SOURCE = 0x00000040,
    ON_WAKEUP_FROM_STANDBY = 0x00000080,
    ARC_INITIATION_EVENT = 0x00000100,
    ARC_TERMINATION_EVENT = 0x00000200,
    REPORT_AUDIO_DEVICE_CONNECTED = 0x00000400,
    REPORT_AUDIO_STATUS = 0x00000800,
    REPORT_FEATURE_ABORT = 0x00001000,
    REPORT_CEC_ENABLED = 0x00002000,
    SET_SYSTEM_AUDIO_MODE = 0x00004000,
    SHORT_AUDIO_DESCRIPTOR = 0x00008000,
    STANDBY_MESSAGE_RECEIVED = 0x00010000,
    REPORT_AUDIO_DEVICE_POWER_STATUS = 0x00020000,
    HDMICECSINK_STATUS_INVALID = 0x00000000
} HdmiCecSinkL2test_async_events_t;

// Notification handler for HdmiCecSink events
class HdmiCecSinkNotificationHandler : public Exchange::IHdmiCecSink::INotification {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;

    BEGIN_INTERFACE_MAP(Notification)
    INTERFACE_ENTRY(Exchange::IHdmiCecSink::INotification)
    END_INTERFACE_MAP

public:
    HdmiCecSinkNotificationHandler() {}
    ~HdmiCecSinkNotificationHandler() {}

    // Event handlers with data storage for validation
    void ArcInitiationEvent(const string status) override
    {
        TEST_LOG("ArcInitiationEvent triggered with status: %s", status.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ARC_INITIATION_EVENT;
        m_condition_variable.notify_one();
    }

    void ArcTerminationEvent(const string status) override
    {
        TEST_LOG("ArcTerminationEvent triggered with status: %s", status.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ARC_TERMINATION_EVENT;
        m_condition_variable.notify_one();
    }

    void OnActiveSourceChange(const int logicalAddress, const string physicalAddress) override
    {
        TEST_LOG("OnActiveSourceChange event: logicalAddress=%d, physicalAddress=%s", logicalAddress, physicalAddress.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_ACTIVE_SOURCE_CHANGE;
        m_condition_variable.notify_one();
    }

    void OnDeviceAdded(const int logicalAddress) override
    {
        TEST_LOG("OnDeviceAdded triggered - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_DEVICE_ADDED;
        m_condition_variable.notify_one();
    }

    void OnDeviceInfoUpdated(const int logicalAddress) override
    {
        TEST_LOG("OnDeviceInfoUpdated triggered - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_DEVICE_INFO_UPDATED;
        m_condition_variable.notify_one();
    }

    void OnDeviceRemoved(const int logicalAddress) override
    {
        TEST_LOG("OnDeviceRemoved triggered - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_DEVICE_REMOVED;
        m_condition_variable.notify_one();
    }

    void OnImageViewOnMsg(const int logicalAddress) override
    {
        TEST_LOG("OnImageViewOnMsg triggered - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_IMAGE_VIEW_ON;
        m_condition_variable.notify_one();
    }

    void OnInActiveSource(const int logicalAddress, const string physicalAddress) override
    {
        TEST_LOG("OnInActiveSource triggered - logicalAddress: %d, physicalAddress: %s",
            logicalAddress, physicalAddress.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_INACTIVE_SOURCE;
        m_condition_variable.notify_one();
    }

    void OnTextViewOnMsg(const int logicalAddress) override
    {
        TEST_LOG("OnTextViewOnMsg triggered - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_TEXT_VIEW_ON;
        m_condition_variable.notify_one();
    }

    void OnWakeupFromStandby(const int logicalAddress) override
    {
        TEST_LOG("OnWakeupFromStandby triggered - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= ON_WAKEUP_FROM_STANDBY;
        m_condition_variable.notify_one();
    }

    void ReportAudioDeviceConnectedStatus(const string status, const string audioDeviceConnected) override
    {
        TEST_LOG("ReportAudioDeviceConnectedStatus - status: %s, connected: %s",
            status.c_str(), audioDeviceConnected.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= REPORT_AUDIO_DEVICE_CONNECTED;
        m_condition_variable.notify_one();
    }

    void ReportAudioStatusEvent(const int muteStatus, const int volumeLevel) override
    {
        TEST_LOG("ReportAudioStatusEvent - muteStatus: %d, volumeLevel: %d", muteStatus, volumeLevel);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= REPORT_AUDIO_STATUS;
        m_condition_variable.notify_one();
    }

    void ReportFeatureAbortEvent(const int logicalAddress, const int opcode, const int FeatureAbortReason) override
    {
        TEST_LOG("ReportFeatureAbortEvent - logicalAddress: %d, opcode: %d, reason: %d",
            logicalAddress, opcode, FeatureAbortReason);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= REPORT_FEATURE_ABORT;
        m_condition_variable.notify_one();
    }

    void ReportCecEnabledEvent(const string cecEnable) override
    {
        TEST_LOG("ReportCecEnabledEvent - cecEnable: %s", cecEnable.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= REPORT_CEC_ENABLED;
        m_condition_variable.notify_one();
    }

    void SetSystemAudioModeEvent(const string audioMode) override
    {
        TEST_LOG("SetSystemAudioModeEvent - audioMode: %s", audioMode.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= SET_SYSTEM_AUDIO_MODE;
        m_condition_variable.notify_one();
    }

    void ShortAudiodescriptorEvent(const string& jsonresponse) override
    {
        TEST_LOG("ShortAudiodescriptorEvent - jsonResponse: %s", jsonresponse.c_str());
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= SHORT_AUDIO_DESCRIPTOR;
        m_condition_variable.notify_one();
    }

    void StandbyMessageReceived(const int logicalAddress) override
    {
        TEST_LOG("StandbyMessageReceived - logicalAddress: %d", logicalAddress);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= STANDBY_MESSAGE_RECEIVED;
        m_condition_variable.notify_one();
    }

    void ReportAudioDevicePowerStatus(const int powerStatus) override
    {
        TEST_LOG("ReportAudioDevicePowerStatus - powerStatus: %d", powerStatus);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_event_signalled |= REPORT_AUDIO_DEVICE_POWER_STATUS;
        m_condition_variable.notify_one();
    }

    uint32_t WaitForRequestStatus(uint32_t timeout_ms, HdmiCecSinkL2test_async_events_t expected_status)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        std::chrono::milliseconds timeout(timeout_ms);
        uint32_t signalled = HDMICECSINK_STATUS_INVALID;

        while (!(expected_status & m_event_signalled)) {
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
                TEST_LOG("Timeout waiting for request status event");
                break;
            }
        }
        signalled = m_event_signalled;
        return signalled;
    }
};

class AsyncHandlerMock_HdmiCecSink {
public:
    AsyncHandlerMock_HdmiCecSink()
    {
    }

    MOCK_METHOD(void, ArcInitiationEvent, (const JsonObject& message));
    MOCK_METHOD(void, ArcTerminationEvent, (const JsonObject& message));
    MOCK_METHOD(void, OnActiveSourceChange, (const JsonObject& message));
    MOCK_METHOD(void, OnDeviceAdded, (const JsonObject& message));
    MOCK_METHOD(void, OnDeviceInfoUpdated, (const JsonObject& message));
    MOCK_METHOD(void, OnDeviceRemoved, (const JsonObject& message));
    MOCK_METHOD(void, OnImageViewOnMsg, (const JsonObject& message));
    MOCK_METHOD(void, OnInActiveSource, (const JsonObject& message));
    MOCK_METHOD(void, OnTextViewOnMsg, (const JsonObject& message));
    MOCK_METHOD(void, OnWakeupFromStandby, (const JsonObject& message));
    MOCK_METHOD(void, ReportAudioDeviceConnectedStatus, (const JsonObject& message));
    MOCK_METHOD(void, ReportAudioStatusEvent, (const JsonObject& message));
    MOCK_METHOD(void, ReportFeatureAbortEvent, (const JsonObject& message));
    MOCK_METHOD(void, ReportCecEnabledEvent, (const JsonObject& message));
    MOCK_METHOD(void, SetSystemAudioModeEvent, (const JsonObject& message));
    MOCK_METHOD(void, ShortAudiodescriptorEvent, (const JsonObject& message));
    MOCK_METHOD(void, StandbyMessageReceived, (const JsonObject& message));
    MOCK_METHOD(void, ReportAudioDevicePowerStatus, (const JsonObject& message));
};

class HdmiCecSink_L2Test : public L2TestMocks {
protected:
    HdmiCecSink_L2Test();
    virtual ~HdmiCecSink_L2Test() override;

public:
    uint32_t CreateHdmiCecSinkInterfaceObject();
    uint32_t WaitForRequestStatus(uint32_t timeout_ms, HdmiCecSinkL2test_async_events_t expected_status);
    void ArcInitiationEvent(const JsonObject& message);
    void ArcTerminationEvent(const JsonObject& message);
    void OnActiveSourceChange(const JsonObject& message);
    void OnDeviceAdded(const JsonObject& message);
    void OnDeviceInfoUpdated(const JsonObject& message);
    void OnDeviceRemoved(const JsonObject& message);
    void OnImageViewOnMsg(const JsonObject& message);
    void OnInActiveSource(const JsonObject& message);
    void OnTextViewOnMsg(const JsonObject& message);
    void OnWakeupFromStandby(const JsonObject& message);
    void ReportAudioDeviceConnectedStatus(const JsonObject& message);
    void ReportAudioStatusEvent(const JsonObject& message);
    void ReportFeatureAbortEvent(const JsonObject& message);
    void ReportCecEnabledEvent(const JsonObject& message);
    void SetSystemAudioModeEvent(const JsonObject& message);
    void ShortAudiodescriptorEvent(const JsonObject& message);
    void StandbyMessageReceived(const JsonObject& message);
    void ReportAudioDevicePowerStatus(const JsonObject& message);

protected:
    Exchange::IHdmiCecSink* m_cecSinkPlugin = nullptr;
    PluginHost::IShell* m_controller_cecSink = nullptr;
    Core::Sink<HdmiCecSinkNotificationHandler> m_notificationHandler;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> HdmiCecSink_Engine;
    Core::ProxyType<RPC::CommunicatorClient> HdmiCecSink_Client;

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled = HDMICECSINK_STATUS_INVALID;
};

HdmiCecSink_L2Test::HdmiCecSink_L2Test()
    : L2TestMocks()
{
    uint32_t status = Core::ERROR_GENERAL;
    m_event_signalled = HDMICECSINK_STATUS_INVALID;

    removeFile("/etc/device.properties");
    createFile("/etc/device.properties", "RDK_PROFILE=TV");

    ON_CALL(*p_connectionImplMock, poll(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const LogicalAddress& from, const Throw_e& doThrow) {
                throw CECNoAckException();
            }));

    EXPECT_CALL(*p_libCCECImplMock, getPhysicalAddress(::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](uint32_t* physAddress) {
                *physAddress = (uint32_t)0x12345678;
            }));

    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const DataBlock&>(::testing::_)))
        .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
        .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG)) {
                    EXPECT_TRUE(handler != nullptr);
                    dsHdmiEventHandler = handler;
                }
                return IARM_RESULT_SUCCESS;
            }));

    ON_CALL(*p_connectionImplMock, open())
        .WillByDefault(::testing::Return());

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
        .WillByDefault(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                if (strcmp(methodName, IARM_BUS_PWRMGR_API_GetPowerState) == 0) {
                    auto* param = static_cast<IARM_Bus_PWRMgr_GetPowerState_Param_t*>(arg);
                    param->curState = IARM_BUS_PWRMGR_POWERSTATE_ON;
                }
                if (strcmp(methodName, IARM_BUS_DSMGR_API_dsHdmiInGetNumberOfInputs) == 0) {
                    auto* param = static_cast<dsHdmiInGetNumberOfInputsParam_t*>(arg);
                    param->result = dsERR_NONE;
                    param->numHdmiInputs = 3;
                }
                if (strcmp(methodName, IARM_BUS_DSMGR_API_dsHdmiInGetStatus) == 0) {
                    auto* param = static_cast<dsHdmiInGetStatusParam_t*>(arg);
                    param->result = dsERR_NONE;
                    param->status.isPortConnected[1] = 1;
                }
                if (strcmp(methodName, IARM_BUS_DSMGR_API_dsGetHDMIARCPortId) == 0) {
                    auto* param = static_cast<dsGetHDMIARCPortIdParam_t*>(arg);
                    param->portId = 1;
                }
                return IARM_RESULT_SUCCESS;
            });
    /* Activate plugin in constructor */
    status = ActivateService("org.rdk.HdmiCecSink");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

HdmiCecSink_L2Test::~HdmiCecSink_L2Test()
{
    uint32_t status = Core::ERROR_GENERAL;
    removeFile("/etc/device.properties");

    status = DeactivateService("org.rdk.HdmiCecSink");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

void HdmiCecSink_L2Test::ArcInitiationEvent(const JsonObject& message)
{
    TEST_LOG("arcInitiation event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("arcInitiation received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ARC_INITIATION_EVENT;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ArcTerminationEvent(const JsonObject& message)
{
    TEST_LOG("arcTermination event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("arcTermination received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ARC_TERMINATION_EVENT;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnActiveSourceChange(const JsonObject& message)
{
    TEST_LOG("OnActiveSourceChange event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnActiveSourceChange received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_ACTIVE_SOURCE_CHANGE;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnDeviceAdded(const JsonObject& message)
{
    TEST_LOG("OnDeviceAdded event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnDeviceAdded received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_DEVICE_ADDED;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnDeviceInfoUpdated(const JsonObject& message)
{
    TEST_LOG("OnDeviceInfoUpdated event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnDeviceInfoUpdated received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_DEVICE_INFO_UPDATED;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnDeviceRemoved(const JsonObject& message)
{
    TEST_LOG("OnDeviceRemoved event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnDeviceRemoved received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_DEVICE_REMOVED;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnImageViewOnMsg(const JsonObject& message)
{
    TEST_LOG("OnImageViewOnMsg event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnImageViewOnMsg received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_IMAGE_VIEW_ON;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnInActiveSource(const JsonObject& message)
{
    TEST_LOG("OnInActiveSource event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnInActiveSource received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_INACTIVE_SOURCE;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnTextViewOnMsg(const JsonObject& message)
{
    TEST_LOG("OnTextViewOnMsg event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnTextViewOnMsg received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_TEXT_VIEW_ON;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::OnWakeupFromStandby(const JsonObject& message)
{
    TEST_LOG("OnWakeupFromStandby event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("OnWakeupFromStandby received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= ON_WAKEUP_FROM_STANDBY;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ReportAudioDeviceConnectedStatus(const JsonObject& message)
{
    TEST_LOG("ReportAudioDeviceConnectedStatus event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("ReportAudioDeviceConnectedStatus received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= REPORT_AUDIO_DEVICE_CONNECTED;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ReportAudioStatusEvent(const JsonObject& message)
{
    TEST_LOG("ReportAudioStatusEvent event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("ReportAudioStatusEvent received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= REPORT_AUDIO_STATUS;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ReportFeatureAbortEvent(const JsonObject& message)
{
    TEST_LOG("ReportFeatureAbortEvent event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("ReportFeatureAbortEvent received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= REPORT_FEATURE_ABORT;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ReportCecEnabledEvent(const JsonObject& message)
{
    TEST_LOG("ReportCecEnabledEvent event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("ReportCecEnabledEvent received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= REPORT_CEC_ENABLED;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::SetSystemAudioModeEvent(const JsonObject& message)
{
    TEST_LOG("SetSystemAudioModeEvent event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("SetSystemAudioModeEvent received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= SET_SYSTEM_AUDIO_MODE;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ShortAudiodescriptorEvent(const JsonObject& message)
{
    TEST_LOG("ShortAudiodescriptorEvent event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("ShortAudiodescriptorEvent received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= SHORT_AUDIO_DESCRIPTOR;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::StandbyMessageReceived(const JsonObject& message)
{
    TEST_LOG("StandbyMessageReceived event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("StandbyMessageReceived received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= STANDBY_MESSAGE_RECEIVED;
    m_condition_variable.notify_one();
}

void HdmiCecSink_L2Test::ReportAudioDevicePowerStatus(const JsonObject& message)
{
    TEST_LOG("ReportAudioDevicePowerStatus event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("ReportAudioDevicePowerStatus received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= REPORT_AUDIO_DEVICE_POWER_STATUS;
    m_condition_variable.notify_one();
}

HdmiCecSink_L2Test::WaitForRequestStatus(uint32_t timeout_ms, HdmiCecSinkL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    uint32_t signalled = HDMICECSINK_STATUS_INVALID;

    while (!(expected_status & m_event_signalled)) {
        if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("Timeout waiting for request status event");
            break;
        }
    }
    signalled = m_event_signalled;
    return signalled;
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

uint32_t HdmiCecSink_L2Test::CreateHdmiCecSinkInterfaceObject()
{
    uint32_t return_value = Core::ERROR_GENERAL;

    TEST_LOG("Creating HdmiCecSink_Engine");
    HdmiCecSink_Engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    HdmiCecSink_Client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(HdmiCecSink_Engine));

    TEST_LOG("Creating HdmiCecSink_Engine Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    HdmiCecSink_Engine->Announcements(mHdmiCecSink_Client->Announcement());
#endif
    if (!HdmiCecSink_Client.IsValid()) {
        TEST_LOG("Invalid HdmiCecSink_Client");
    } else {
        m_controller_cecSink = HdmiCecSink_Client->Open<PluginHost::IShell>(_T("org.rdk.HdmiCecSink"), ~0, 3000);
        if (m_controller_cecSink) {
            m_cecSinkPlugin = m_controller_cecSink->QueryInterface<Exchange::IHdmiCecSink>();
            return_value = Core::ERROR_NONE;
        }
    }
    return return_value;
}

TEST_F(HdmiCecSink_L2Test, Test)
{
    if (CreateHdmiCecSinkInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid HdmiCecSink_Client");
    } else {
        EXPECT_TRUE(m_controller_cecSink != nullptr);
        if (m_controller_cecSink) {
            EXPECT_TRUE(m_cecSinkPlugin != nullptr);
            if (m_cecSinkPlugin) {
                m_cecSinkPlugin->AddRef();

                m_cecSinkPlugin->Release();
            } else {
                TEST_LOG("m_cecSinkPlugin is NULL");
            }
        } else {
            TEST_LOG("m_controller_cecSink is NULL");
        }
    }
}
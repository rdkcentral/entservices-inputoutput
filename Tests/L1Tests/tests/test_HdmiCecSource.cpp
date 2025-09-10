/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <fstream>
#include <string>

#include "HdmiCecSourceImplementation.h"
#include "HdmiCec.h"
#include "HdmiCecSource.h"
#include "PowerManagerMock.h"
#include "FactoriesImplementation.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "devicesettings.h"
#include "HdmiCecMock.h"
#include "DisplayMock.h"
#include "VideoOutputPortMock.h"
#include "HostMock.h"
#include "ManagerMock.h"
#include "ThunderPortability.h"
#include "COMLinkMock.h"
#include "HdmiCecSourceMock.h"
#include "WorkerPoolImplementation.h"
#include "WrapsMock.h"

#define JSON_TIMEOUT   (1000)

using namespace WPEFramework;
using ::testing::NiceMock;

namespace
{
	static void removeFile(const char* fileName)
	{
		if (std::remove(fileName) != 0)
		{
			printf("File %s failed to remove\n", fileName);
			perror("Error deleting file");
		}
		else
		{
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

typedef enum : uint32_t {
    HdmiCecSource_OnDeviceAdded = 0x00000001,
    HdmiCecSource_OnDeviceRemoved = 0x00000002,
    HdmiCecSource_OnDeviceInfoUpdated = 0x00000004,
    HdmiCecSource_OnActiveSourceStatusUpdated = 0x00000008,
    HdmiCecSource_StandbyMessageReceived = 0x00000010,
    HdmiCecSource_OnKeyReleaseEvent = 0x00000020,
    HdmiCecSource_OnKeyPressEvent = 0x00000040,
} HdmiCecSourceEventType_t;


class NotificationHandler : public Exchange::IHdmiCecSource::INotification {
    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;
        bool m_OnDeviceAdded_signalled =false;
        bool m_onDeviceRemoved_signalled =false;
        bool m_OnDeviceInfoUpdated_signalled =false;
        bool m_OnActiveSourceStatusUpdated_signalled = false;
        bool m_StandbyMessageReceived_signalled = false;
        bool m_OnKeyReleaseEvent=false;
        bool m_OnKeyPressEvent=false;


        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IHdmiCecSource::INotification)
        END_INTERFACE_MAP

    public:
        NotificationHandler(){}
        ~NotificationHandler(){}

        void OnDeviceAdded(const int logicalAddress) override
        {
            TEST_LOG("OnDeviceAdded event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("LogicalAddress: %d\n", logicalAddress);
            m_event_signalled |= HdmiCecSource_OnDeviceAdded;
            m_OnDeviceAdded_signalled = true;
            m_condition_variable.notify_one();
            

        }
        void OnDeviceRemoved(const int logicalAddress) override
        {
            TEST_LOG("OnDeviceRemoved event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("LogicalAddress: %d\n", logicalAddress);
            m_event_signalled |= HdmiCecSource_OnDeviceRemoved;
            m_onDeviceRemoved_signalled = true;
            m_condition_variable.notify_one();
        }
        void OnDeviceInfoUpdated(const int logicalAddress) override
        {
            TEST_LOG("OnDeviceInfoUpdated event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("LogicalAddress: %d\n", logicalAddress);
            m_event_signalled |= HdmiCecSource_OnDeviceInfoUpdated;
            m_OnDeviceInfoUpdated_signalled = true;
            m_condition_variable.notify_one();
        }
        void OnActiveSourceStatusUpdated(const bool status) override
        {
            TEST_LOG("OnActiveSourceStatusUpdated event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("status: %d\n", status);
            m_event_signalled |= HdmiCecSource_OnActiveSourceStatusUpdated;
            m_OnActiveSourceStatusUpdated_signalled = true;
            m_condition_variable.notify_one();
        }
        void StandbyMessageReceived(const int logicalAddress) override
        {
            TEST_LOG("StandbyMessageReceived event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("LogicalAddress: %d\n", logicalAddress);
            m_event_signalled |= HdmiCecSource_StandbyMessageReceived;
            m_StandbyMessageReceived_signalled = true;
            m_condition_variable.notify_one();
        }
        void OnKeyReleaseEvent(const int logicalAddress) override
        {
            TEST_LOG("OnKeyReleaseEvent event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("LogicalAddress: %d\n", logicalAddress);
            m_event_signalled |= HdmiCecSource_OnKeyReleaseEvent;
            m_OnKeyReleaseEvent = true;
            m_condition_variable.notify_one();
        }
        void OnKeyPressEvent(const int logicalAddress, const int keyCode) override
        {
            TEST_LOG("OnKeyPressEvent event trigger ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("LogicalAddress: %d\n", logicalAddress);
            TEST_LOG("KeyCode: %d\n", keyCode);
            m_event_signalled |= HdmiCecSource_OnKeyPressEvent;
            m_OnKeyPressEvent = true;
            m_condition_variable.notify_one();
        }

        bool WaitForRequestStatus(uint32_t timeout_ms, HdmiCecSourceEventType_t expected_status)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::system_clock::now();
            std::chrono::milliseconds timeout(timeout_ms);
            bool signalled = false;

            while (!(expected_status & m_event_signalled))
            {
              if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
              {
                 TEST_LOG("Timeout waiting for request status event");
                 break;
              }
            }

            switch(m_event_signalled)
            {
                case HdmiCecSource_OnDeviceAdded:
                    signalled = m_OnDeviceAdded_signalled;
                    break;
                case HdmiCecSource_OnDeviceRemoved:
                    signalled = m_onDeviceRemoved_signalled;
                    break;
                case HdmiCecSource_OnDeviceInfoUpdated:
                    signalled = m_OnDeviceInfoUpdated_signalled;
                    break;
                case HdmiCecSource_OnActiveSourceStatusUpdated:
                    signalled = m_OnActiveSourceStatusUpdated_signalled;
                    break;
                case HdmiCecSource_StandbyMessageReceived:
                    signalled = m_StandbyMessageReceived_signalled;
                    break;
                case HdmiCecSource_OnKeyReleaseEvent:
                    signalled = m_OnKeyReleaseEvent;
                    break;
                case HdmiCecSource_OnKeyPressEvent:
                    signalled = m_OnKeyPressEvent;
                    break;
                default:
                    signalled = false;
                    break;
            }
                

            signalled = m_event_signalled;
            return signalled;
        }
    };


class HdmiCecSourceTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::HdmiCecSource> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    IARM_EventHandler_t cecMgrEventHandler;
    IARM_EventHandler_t dsHdmiEventHandler;
    IARM_EventHandler_t pwrMgrEventHandler;
    ManagerImplMock   *p_managerImplMock = nullptr ;
    HostImplMock      *p_hostImplMock = nullptr ;
    VideoOutputPortMock      *p_videoOutputPortMock = nullptr ;
    DisplayMock      *p_displayMock = nullptr ;
    LibCCECImplMock      *p_libCCECImplMock = nullptr ;
    ConnectionImplMock      *p_connectionImplMock = nullptr ;
    MessageEncoderMock      *p_messageEncoderMock = nullptr ;
    WrapsImplMock *p_wrapsImplMock = nullptr;
    ServiceMock  *p_serviceMock  = nullptr;
    HdmiCecSourceMock       *p_hdmiCecSourceMock = nullptr;
    testing::NiceMock<COMLinkMock> comLinkMock;
    testing::NiceMock<ServiceMock> service;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    Core::ProxyType<Plugin::HdmiCecSourceImplementation> HdmiCecSourceImplementationImpl;
    Exchange::IHdmiCecSource::INotification *HdmiCecSourceNotification = nullptr;

    HdmiCecSourceTest()
        : plugin(Core::ProxyType<Plugin::HdmiCecSource>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
        , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
            2, Core::Thread::DefaultStackSize(), 16))
    {
        p_iarmBusImplMock  = new testing::NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        p_managerImplMock  = new testing::NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        p_hostImplMock  = new testing::NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        p_videoOutputPortMock  = new testing::NiceMock <VideoOutputPortMock>;
        device::VideoOutputPort::setImpl(p_videoOutputPortMock);

        p_displayMock  = new testing::NiceMock <DisplayMock>;
        device::Display::setImpl(p_displayMock);

        p_libCCECImplMock  = new testing::NiceMock <LibCCECImplMock>;
        LibCCEC::setImpl(p_libCCECImplMock);

        p_connectionImplMock  = new testing::NiceMock <ConnectionImplMock>;
        Connection::setImpl(p_connectionImplMock);

        p_messageEncoderMock  = new testing::NiceMock <MessageEncoderMock>;
        MessageEncoder::setImpl(p_messageEncoderMock);

        p_serviceMock = new testing::NiceMock <ServiceMock>;

        p_hdmiCecSourceMock = new NiceMock <HdmiCecSourceMock>;

        p_wrapsImplMock = new NiceMock <WrapsImplMock>;

        Wraps::setImpl(p_wrapsImplMock);

        ON_CALL(*p_hdmiCecSourceMock, Register(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](Exchange::IHdmiCecSource::INotification *notification){
                HdmiCecSourceNotification = notification;
                return Core::ERROR_NONE;;
            }));


        ON_CALL(service, COMLink())
            .WillByDefault(::testing::Invoke(
                  [this]() {
                        TEST_LOG("Pass created comLinkMock: %p ", &comLinkMock);
                        return &comLinkMock;
                    }));

        //OnCall required for intialize to run properly
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const DataBlock&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));

        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(device::Display::getInstance()));

        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));

        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(device::VideoOutputPort::getInstance()));

        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](std::vector<uint8_t> &edidVec2) {
                    edidVec2 = std::vector<uint8_t>({ 't', 'e', 's', 't' });
                }));
        //Set enabled needs to be
        ON_CALL(*p_libCCECImplMock, getLogicalAddress(::testing::_))
            .WillByDefault(::testing::Return(0));

        ON_CALL(*p_connectionImplMock, open())
            .WillByDefault(::testing::Return());
        ON_CALL(*p_connectionImplMock, addFrameListener(::testing::_))
            .WillByDefault(::testing::Return());
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                   if ((string(IARM_BUS_CECMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_CECMGR_EVENT_DAEMON_INITIALIZED)) {
                        EXPECT_TRUE(handler != nullptr);
                        cecMgrEventHandler = handler;
                    }
                        if ((string(IARM_BUS_CECMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_CECMGR_EVENT_STATUS_UPDATED)) {
                        EXPECT_TRUE(handler != nullptr);
                        cecMgrEventHandler = handler;
                    }
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiEventHandler = handler;
                    }
                if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_MODECHANGED)) {
                        EXPECT_TRUE(handler != nullptr);
                        pwrMgrEventHandler = handler;
                    }

                    return IARM_RESULT_SUCCESS;
                }));
        
    }
    virtual ~HdmiCecSourceTest() override
    {
        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr)
        {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }
        device::Host::setImpl(nullptr);
        if (p_hostImplMock != nullptr)
        {
            delete p_hostImplMock;
            p_hostImplMock = nullptr;
        }
        device::VideoOutputPort::setImpl(nullptr);
        if (p_videoOutputPortMock != nullptr)
        {
            delete p_videoOutputPortMock;
            p_videoOutputPortMock = nullptr;
        }
        device::Display::setImpl(nullptr);
        if (p_displayMock != nullptr)
        {
            delete p_displayMock;
            p_displayMock = nullptr;
        }
        LibCCEC::setImpl(nullptr);
        if (p_libCCECImplMock != nullptr)
        {
            delete p_libCCECImplMock;
            p_libCCECImplMock = nullptr;
        }
        Connection::setImpl(nullptr);
        if (p_connectionImplMock != nullptr)
        {
            delete p_connectionImplMock;
            p_connectionImplMock = nullptr;
        }
        MessageEncoder::setImpl(nullptr);
        if (p_messageEncoderMock != nullptr)
        {
            delete p_messageEncoderMock;
            p_messageEncoderMock = nullptr;
        }

        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();

        if (p_serviceMock != nullptr)
        {
            delete p_serviceMock;
            p_serviceMock = nullptr;
        }

        if (p_hdmiCecSourceMock != nullptr)
        {
            delete p_hdmiCecSourceMock;
            p_hdmiCecSourceMock = nullptr;
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr)
        {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }
    }
};

class HdmiCecSourceInitializedTest : public HdmiCecSourceTest {
protected:
    HdmiCecSourceInitializedTest()
        : HdmiCecSourceTest()
    {
        system("ls -lh /etc/");
        removeFile("/etc/device.properties");
        system("ls -lh /etc/");
        createFile("/etc/device.properties", "RDK_PROFILE=STB");
        system("ls -lh /etc/");
        EXPECT_EQ(string(""), plugin->Initialize(&service));
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));
    }
    virtual ~HdmiCecSourceInitializedTest() override
    {
            int lCounter = 0;
            while ((Plugin::HdmiCecSourceImplementation::_instance->deviceList[0].m_isOSDNameUpdated) && (lCounter < (2*10))) { //sleep for 2sec.
                        usleep (100 * 1000); //sleep for 100 milli sec
                        lCounter ++;
                }
            EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
            EXPECT_EQ(response, string("{\"success\":true}"));
            plugin->Deinitialize(&service);
	    removeFile("/etc/device.properties");
	    
    }
};
class HdmiCecSourceInitializedEventTest : public HdmiCecSourceInitializedTest {
protected:
    
    FactoriesImplementation factoriesImplementation;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::JSONRPC::Message message;

    HdmiCecSourceInitializedEventTest()
        : HdmiCecSourceInitializedTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
            plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);
    }

    virtual ~HdmiCecSourceInitializedEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();
        PluginHost::IFactories::Assign(nullptr);

    }
};

TEST_F(HdmiCecSourceInitializedTest, setEnabled_EnablesCecSuccessfully)
{
    // Test setEnabled with enabled=true (CEC is already enabled in fixture setup)
    string response;
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
    
    // Verify successful response
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Verify that the enable state can be retrieved
    string getEnabledResponse;
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T("{}"), getEnabledResponse));
    EXPECT_THAT(getEnabledResponse, ::testing::HasSubstr("\"enabled\":true"));
    
    // Test setEnabled with enabled=false to verify disable functionality
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Verify CEC is now disabled
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T("{}"), getEnabledResponse));
    EXPECT_THAT(getEnabledResponse, ::testing::HasSubstr("\"enabled\":false"));
    
    // Re-enable for clean teardown
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

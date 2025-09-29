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

TEST_F(HdmiCecSourceInitializedTest, RegisteredMethods)
{

    removeFile("/etc/device.properties");
    createFile("/etc/device.properties", "RDK_PROFILE=STB");

    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getActiveSourceStatus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getDeviceList")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getOSDName")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getOTPEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getVendorId")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("performOTPAction")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendKeyPressEvent")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendStandbyMessage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setOSDName")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setOTPEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVendorId")));

    removeFile("/etc/device.properties");

}

TEST_F(HdmiCecSourceInitializedTest, getEnabledTrue)
{
    //Get enabled just checks if CEC is on, which is a global variable.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T(""), response));
    EXPECT_EQ(response, string("{\"enabled\":true,\"success\":true}"));

}

TEST_F(HdmiCecSourceInitializedTest, getActiveSourceStatusTrue)
{
    //SetsOTP to on.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));

    //Sets Activesource to true
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("performOTPAction"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));


    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, string("{\"status\":true,\"success\":true}"));


}

TEST_F(HdmiCecSourceInitializedTest, getDeviceList)
{
    int iCounter = 0;
    //Checking to see if one of the values has been filled in (as the rest get filled in at the same time, and waiting if its not.
    while ((!Plugin::HdmiCecSourceImplementation::_instance->deviceList[0].m_isOSDNameUpdated) && (iCounter < (2*10))) { //sleep for 2sec.
                usleep (100 * 1000); //sleep for 100 milli sec
                iCounter ++;
        }

    const char* val = "TEST";
    OSDName name = OSDName(val);
    SetOSDName osdName = SetOSDName(name);

    Header header;
    header.from = LogicalAddress(1); //specifies with logicalAddress in the deviceList we're using

    VendorID vendor(1,2,3);
    DeviceVendorID vendorid(vendor);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());

    proc.process(osdName, header); //calls the process that sets osdName for LogicalAddress = 1
    proc.process(vendorid, header); //calls the process that sets vendorID for LogicalAddress = 1

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T(""), response));

    EXPECT_EQ(response, string(_T("{\"numberofdevices\":14,\"deviceList\":[{\"logicalAddress\":1,\"vendorID\":\"123\",\"osdName\":\"TEST\"},{\"logicalAddress\":2,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":3,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":4,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":5,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":6,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":7,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":8,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":9,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":10,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":11,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":12,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":13,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":14,\"vendorID\":\"000\",\"osdName\":\"NA\"}],\"success\":true}")));
}

TEST_F(HdmiCecSourceInitializedTest, sendStandbyMessage)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendStandbyMessage"), _T("{}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, setOSDName)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\": \"CUSTOM8 Tv\"}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOSDName"), _T("{}"), response));
        EXPECT_EQ(response, string("{\"name\":\"CUSTOM8 Tv\",\"success\":true}"));

}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventVolumeUp)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_VOLUME_UP);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 65}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventVolumeDown)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_VOLUME_DOWN);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 66}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventMute)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_MUTE);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 67}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventUp)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_UP);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventDown)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_DOWN);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 2}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventLeft)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_LEFT);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 3}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventRight)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_RIGHT);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 4}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventSelect)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_SELECT);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventHome)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_HOME);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 9}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventBack)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_BACK);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 13}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber0)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_0);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 32}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber1)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_1);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 33}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber2)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_2);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 34}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber3)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_3);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 35}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber4)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_4);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 36}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber5)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_5);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 37}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber6)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_6);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 38}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber7)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_7);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 39}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber8)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_8);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 40}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventNumber9)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_NUM_9);
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 41}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}
TEST_F(HdmiCecSourceInitializedTest, getActiveSourceStatusTrueDuplicate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("performOTPAction"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, string("{\"status\":true,\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, getActiveSourceStatusFalseDuplicate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, string("{\"status\":false,\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, setVendorIdAndGetVendorIdDuplicate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\": \"0x0019FB\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVendorId"), _T(""), response));
    EXPECT_EQ(response, string("{\"vendorid\":\"0019fb\",\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, getOTPEnabledTrueDuplicate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOTPEnabled"), _T(""), response));
    EXPECT_EQ(response, string("{\"enabled\":true,\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, getOTPEnabledFalseDuplicate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOTPEnabled"), _T(""), response));
    EXPECT_EQ(response, string("{\"enabled\":false,\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, sendStandbyMessageTestDuplicate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendStandbyMessage"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, textViewOnProcess)
{
    TextViewOn msg;
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, inActiveSourceProcess)
{
    InActiveSource msg;
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, getCECVersionProcess)
{
    GetCECVersion msg;
    Header header;
    header.from = LogicalAddress(1);

    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const LogicalAddress& to, const CECFrame& frame) {
                EXPECT_EQ(to.toInt(), 1);
            }));

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, setMenuLanguageProcess)
{
    Language lang("eng");
    SetMenuLanguage msg(lang);
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, setOSDStringProcess)
{
    DisplayControl display = DisplayControl::DEFAULT_TIME;
    OSDString osdString = OSDString("Test String");
    SetOSDString msg(display, osdString);
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, getMenuLanguageProcess)
{
    GetMenuLanguage msg;
    Header header;
    header.from = LogicalAddress(1);

    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const LogicalAddress& to, const CECFrame& frame) {
                EXPECT_EQ(to.toInt(), 1);
            }));

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, routingInformationProcessFixed)
{
    RoutingInformation msg;
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, reportPhysicalAddressProcessFixed)
{
    PhysicalAddress physAddr(0x0F, 0x0F, 0x0F, 0x0F);
    ReportPhysicalAddress msg(physAddr, DeviceType::TUNER);
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, featureAbortProcess)
{
    OpCode opcode(0x83);
    AbortReason reason = INVALID_OPERAND;
    FeatureAbort msg(opcode, reason);
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, abortProcessFixed)
{
    OpCode opcode(0x83);
    Abort msg(opcode);
    Header header;
    header.from = LogicalAddress(1);

    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const LogicalAddress& to, const CECFrame& frame) {
                EXPECT_EQ(to.toInt(), 1);
            }));

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, pollingProcess)
{
    Polling msg;
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, standbyProcess)
{
    Standby msg;
    Header header;
    header.from = LogicalAddress(0);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, activeSourceProcess)
{
    PhysicalAddress physAddr(0x0F, 0x0F, 0x0F, 0x0F);
    ActiveSource msg(physAddr);
    Header header;
    header.from = LogicalAddress(1);

    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedEventTest, onDeviceAddedNotification)
{
    Core::Sink<NotificationHandler> notification;
    HdmiCecSourceImplementationImpl = Core::ProxyType<Plugin::HdmiCecSourceImplementation>::Create();
    HdmiCecSourceImplementationImpl->Register(&notification);

    HdmiCecSourceImplementationImpl->addDevice(1);

    EXPECT_TRUE(notification.WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnDeviceAdded));

    HdmiCecSourceImplementationImpl->Unregister(&notification);
}

TEST_F(HdmiCecSourceInitializedEventTest, onDeviceRemovedNotification)
{
    Core::Sink<NotificationHandler> notification;
    HdmiCecSourceImplementationImpl = Core::ProxyType<Plugin::HdmiCecSourceImplementation>::Create();
    HdmiCecSourceImplementationImpl->Register(&notification);

    HdmiCecSourceImplementationImpl->addDevice(1);
    HdmiCecSourceImplementationImpl->removeDevice(1);

    EXPECT_TRUE(notification.WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnDeviceRemoved));

    HdmiCecSourceImplementationImpl->Unregister(&notification);
}

TEST_F(HdmiCecSourceInitializedEventTest, onActiveSourceStatusUpdatedNotification)
{
    Core::Sink<NotificationHandler> notification;
    HdmiCecSourceImplementationImpl = Core::ProxyType<Plugin::HdmiCecSourceImplementation>::Create();
    HdmiCecSourceImplementationImpl->Register(&notification);

    if (HdmiCecSourceNotification) {
        HdmiCecSourceNotification->OnActiveSourceStatusUpdated(true);
    }

    EXPECT_TRUE(notification.WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnActiveSourceStatusUpdated));

    HdmiCecSourceImplementationImpl->Unregister(&notification);
}

TEST_F(HdmiCecSourceInitializedEventTest, standbyMessageReceivedNotification)
{
    Core::Sink<NotificationHandler> notification;
    HdmiCecSourceImplementationImpl = Core::ProxyType<Plugin::HdmiCecSourceImplementation>::Create();
    HdmiCecSourceImplementationImpl->Register(&notification);

    if (HdmiCecSourceNotification) {
        HdmiCecSourceNotification->StandbyMessageReceived(1);
    }

    EXPECT_TRUE(notification.WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_StandbyMessageReceived));

    HdmiCecSourceImplementationImpl->Unregister(&notification);
}

TEST_F(HdmiCecSourceInitializedEventTest, onKeyPressEventNotification)
{
    Core::Sink<NotificationHandler> notification;
    HdmiCecSourceImplementationImpl = Core::ProxyType<Plugin::HdmiCecSourceImplementation>::Create();
    HdmiCecSourceImplementationImpl->Register(&notification);

    if (HdmiCecSourceNotification) {
        HdmiCecSourceNotification->OnKeyPressEvent(1, 0x01);
    }

    EXPECT_TRUE(notification.WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnKeyPressEvent));

    HdmiCecSourceImplementationImpl->Unregister(&notification);
}

TEST_F(HdmiCecSourceInitializedEventTest, onKeyReleaseEventNotification)
{
    Core::Sink<NotificationHandler> notification;
    HdmiCecSourceImplementationImpl = Core::ProxyType<Plugin::HdmiCecSourceImplementation>::Create();
    HdmiCecSourceImplementationImpl->Register(&notification);

    if (HdmiCecSourceNotification) {
        HdmiCecSourceNotification->OnKeyReleaseEvent(1);
    }

    EXPECT_TRUE(notification.WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnKeyReleaseEvent));

    HdmiCecSourceImplementationImpl->Unregister(&notification);
}

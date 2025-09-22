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

        // Default encode handlers for all CEC message types used by the implementation to avoid runtime_error
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const ImageViewOn&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const TextViewOn&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const ActiveSource&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const RequestActiveSource&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const CECVersion&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const GiveOSDName&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const GivePhysicalAddress&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const GiveDeviceVendorID&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const RoutingChange&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const RoutingInformation&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const SetStreamPath&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const ReportPhysicalAddress&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const DeviceVendorID&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const GiveDevicePowerStatus&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const ReportPowerStatus&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlReleased&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const Abort&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const SetOSDName&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const Standby&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        // ...existing code...
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

TEST_F(HdmiCecSourceInitializedTest, getActiveSourceStatusFalse)
{
    //ActiveSource is a local variable, no mocked functions to check.
    //Active source is false by default.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, string("{\"status\":false,\"success\":true}"));
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

TEST_F(HdmiCecSourceInitializedTest, sendKeyPressEventUp)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
            .WillByDefault(::testing::Invoke(
            [](const UserControlPressed& m) -> CECFrame&  {
                EXPECT_EQ(m.uiCommand.toInt(),UICommand::UI_COMMAND_VOLUME_UP );
                return CECFrame::getInstance();
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 65}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSourceInitializedTest, GetInformation)
{
    EXPECT_EQ("This HdmiCecSource PLugin Facilitates the HDMI CEC Source Control", plugin->Information());
}

TEST_F(HdmiCecSourceInitializedTest, activeSourceProcess)
{
    int iCounter = 0;
    while ((!Plugin::HdmiCecSourceImplementation::_instance->deviceList[0].m_isOSDNameUpdated) && (iCounter < (2*10))) { //sleep for 2sec.
        usleep (100 * 1000); //sleep for 100 milli sec
        iCounter ++;
}


    Header header;
    header.from = LogicalAddress(1); //specifies with logicalAddress in the deviceList we're using

    PhysicalAddress physicalAddress(0x0F,0x0F,0x0F,0x0F);
    PhysicalAddress physicalAddress2(1,2,3,4);
    ActiveSource activeSource(physicalAddress);
    ActiveSource activeSource2(physicalAddress2);


    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(activeSource2, header);
    proc.process(activeSource, header); 

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T(""), response));

    EXPECT_EQ(response, string(_T("{\"numberofdevices\":14,\"deviceList\":[{\"logicalAddress\":1,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":2,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":3,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":4,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":5,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":6,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":7,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":8,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":9,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":10,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":11,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":12,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":13,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":14,\"vendorID\":\"000\",\"osdName\":\"NA\"}],\"success\":true}")));


}

TEST_F(HdmiCecSourceInitializedEventTest, textViewOnProcess){
    int iCounter = 0;
    while ((!Plugin::HdmiCecSourceImplementation::_instance->deviceList[0].m_isOSDNameUpdated) && (iCounter < (2*10))) { //sleep for 2sec.
        usleep (100 * 1000); //sleep for 100 milli sec
        iCounter ++;
}
    

    Header header;
    header.from = LogicalAddress(1); //specifies with logicalAddress in the deviceList we're using

    TextViewOn textViewOn;


    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(textViewOn, header); 

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T(""), response));

    EXPECT_EQ(response, string(_T("{\"numberofdevices\":14,\"deviceList\":[{\"logicalAddress\":1,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":2,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":3,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":4,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":5,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":6,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":7,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":8,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":9,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":10,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":11,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":12,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":13,\"vendorID\":\"000\",\"osdName\":\"NA\"},{\"logicalAddress\":14,\"vendorID\":\"000\",\"osdName\":\"NA\"}],\"success\":true}")));

}

TEST_F(HdmiCecSourceInitializedEventTest, hdmiEventHandler)
{
    int iCounter = 0;
    while ((!Plugin::HdmiCecSourceImplementation::_instance->deviceList[0].m_isOSDNameUpdated) && (iCounter < (2*10))) { //sleep for 2sec.
        usleep (100 * 1000); //sleep for 100 milli sec
        iCounter ++;
    }

    ASSERT_TRUE(dsHdmiEventHandler != nullptr);
    EXPECT_CALL(*p_hostImplMock, getDefaultVideoPortName())
    .Times(1)
        .WillOnce(::testing::Return("TEST"));


    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_hpd.event = 0;

    EVENT_SUBSCRIBE(0, _T("onHdmiHotPlug"), _T("client.events.onHdmiHotPlug"), message);

    dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onHdmiHotPlug"), _T("client.events.onHdmiHotPlug"), message);
}


TEST_F(HdmiCecSourceInitializedEventTest, powerModeChanged)
{
    EXPECT_CALL(*p_libCCECImplMock, getLogicalAddress(::testing::_))
    .WillRepeatedly(::testing::Invoke(
        [&](int devType) {
           EXPECT_EQ(devType, 1);
           return 0;
        }));

        Plugin::HdmiCecSourceImplementation::_instance->onPowerModeChanged(WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF, WPEFramework::Exchange::IPowerManager::POWER_STATE_ON);


}


TEST_F(HdmiCecSourceInitializedTest, ExistsAllMethodsAgain)
{
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
}

TEST_F(HdmiCecSourceInitializedTest, SetEnabledMalformed)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": \"true\"}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, SetEnabledMissingParam)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, GetEnabledInvoke)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::HasSubstr("\"success\":true"));
}

TEST_F(HdmiCecSourceInitializedTest, SetOTPEnabledTrue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": true}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOTPEnabled"), _T("{}"), response));
    EXPECT_EQ(response, "{\"enabled\":true,\"success\":true}");
}

TEST_F(HdmiCecSourceInitializedTest, SetOTPEnabledFalse)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOTPEnabled"), _T("{}"), response));
    EXPECT_EQ(response, "{\"enabled\":false,\"success\":true}");
}

TEST_F(HdmiCecSourceInitializedTest, SetOTPEnabledMalformed)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": 123}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, GetOTPEnabledInvoke)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOTPEnabled"), _T(""), response));
    EXPECT_THAT(response, ::testing::HasSubstr("\"success\":true"));
}

TEST_F(HdmiCecSourceInitializedTest, SetOSDNameEdgeCases)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\": \"\"}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOSDName"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::HasSubstr("\"success\":true"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\": \"A\"}"), response));
    EXPECT_EQ(response, "{\"success\":true}");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\": \"12345678901234567890\"}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
}

TEST_F(HdmiCecSourceInitializedTest, SetOSDNameMalformed)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"wrong\": \"ABC\"}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, VendorIdSetGetValid)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\": \"0x0019FB\"}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVendorId"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::HasSubstr("\"success\":true"));
}

TEST_F(HdmiCecSourceInitializedTest, VendorIdMalformed)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\": \"XYZ123\"}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, PerformOTPActionInvoke)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("performOTPAction"), _T("{}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, GetActiveSourceStatusFalseInvoke)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T("{}"), response));
    EXPECT_EQ(response, "{\"status\":false,\"success\":true}");
}

TEST_F(HdmiCecSourceInitializedTest, GetActiveSourceStatusTrueAfterOTP)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOTPEnabled"), _T("{\"enabled\": true}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("performOTPAction"), _T("{}"), response));

    Header header;
    header.from = LogicalAddress(2);
    PhysicalAddress phys(0x0F,0x0F,0x0F,0x0F);
    ActiveSource activeSource(phys);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(activeSource, header);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::HasSubstr("\"success\":true"));
}

TEST_F(HdmiCecSourceInitializedTest, SendStandbyMessageInvoke)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendStandbyMessage"), _T("{}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
}

TEST_F(HdmiCecSourceInitializedTest, SendKeyPressEventVolumeDown)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
        .WillByDefault(::testing::Invoke([](const UserControlPressed& m)->CECFrame& {
            EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_VOLUME_DOWN);
            return CECFrame::getInstance();
        }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 66}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
}

TEST_F(HdmiCecSourceInitializedTest, SendKeyPressEventMute)
{
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
        .WillByDefault(::testing::Invoke([](const UserControlPressed& m)->CECFrame& {
            EXPECT_EQ(m.uiCommand.toInt(), UICommand::UI_COMMAND_MUTE);
            return CECFrame::getInstance();
        }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":0,\"keyCode\":67}"), response));
    EXPECT_EQ(response, "{\"success\":true}");
}

#define KEY_EVENT_TEST(NAME, CODE, EXPECT_ENUM) \
TEST_F(HdmiCecSourceInitializedTest, SendKeyPressEvent##NAME) { \
    ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_))) \
        .WillByDefault(::testing::Invoke([](const UserControlPressed& m)->CECFrame& { \
            EXPECT_EQ(m.uiCommand.toInt(), EXPECT_ENUM); \
            return CECFrame::getInstance(); \
        })); \
    std::stringstream ss; \
    ss << "{\"logicalAddress\":0,\"keyCode\":" << CODE << "}"; \
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), Core::ToString(ss.str()), response)); \
    EXPECT_EQ(response, "{\"success\":true}"); \
}

KEY_EVENT_TEST(Up, 1, UICommand::UI_COMMAND_UP)
KEY_EVENT_TEST(Down, 2, UICommand::UI_COMMAND_DOWN)
KEY_EVENT_TEST(Left, 3, UICommand::UI_COMMAND_LEFT)
KEY_EVENT_TEST(Right, 4, UICommand::UI_COMMAND_RIGHT)
KEY_EVENT_TEST(Select, 0, UICommand::UI_COMMAND_SELECT)
KEY_EVENT_TEST(Home, 9, UICommand::UI_COMMAND_HOME)
KEY_EVENT_TEST(Back, 13, UICommand::UI_COMMAND_BACK)
KEY_EVENT_TEST(Number0, 32, UICommand::UI_COMMAND_NUM_0)
KEY_EVENT_TEST(Number1, 33, UICommand::UI_COMMAND_NUM_1)
KEY_EVENT_TEST(Number2, 34, UICommand::UI_COMMAND_NUM_2)
KEY_EVENT_TEST(Number3, 35, UICommand::UI_COMMAND_NUM_3)
KEY_EVENT_TEST(Number4, 36, UICommand::UI_COMMAND_NUM_4)
KEY_EVENT_TEST(Number5, 37, UICommand::UI_COMMAND_NUM_5)
KEY_EVENT_TEST(Number6, 38, UICommand::UI_COMMAND_NUM_6)
KEY_EVENT_TEST(Number7, 39, UICommand::UI_COMMAND_NUM_7)
KEY_EVENT_TEST(Number8, 40, UICommand::UI_COMMAND_NUM_8)
KEY_EVENT_TEST(Number9, 41, UICommand::UI_COMMAND_NUM_9)

TEST_F(HdmiCecSourceInitializedTest, SendKeyPressEventMalformed)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":0}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, DeviceListAfterAddAndVendorUpdate)
{
    Header header;
    header.from = LogicalAddress(4);
    VendorID vid(0x01,0x02,0x03);
    DeviceVendorID dvid(vid);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(dvid, header);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::HasSubstr("\"success\":true"));
}

TEST_F(HdmiCecSourceInitializedTest, ImageViewOnProcess)
{
    Header header;
    header.from = LogicalAddress(5);
    ImageViewOn msg;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T("{}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, RequestActiveSourceProcess)
{
    Header header;
    header.from = LogicalAddress(6);
    RequestActiveSource msg;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(msg, header);
}

TEST_F(HdmiCecSourceInitializedTest, CecVersionProcess)
{
    Header header;
    header.from = LogicalAddress(7);
    CECVersion ver(Version::V_1_4);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ver, header);
}

TEST_F(HdmiCecSourceInitializedTest, GiveOSDNameProcess)
{
    Header header;
    header.from = LogicalAddress(8);
    GiveOSDName give;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(give, header);
}

TEST_F(HdmiCecSourceInitializedTest, GivePhysicalAddressProcess)
{
    Header header;
    header.from = LogicalAddress(9);
    GivePhysicalAddress gpa;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(gpa, header);
}

TEST_F(HdmiCecSourceInitializedTest, GiveDeviceVendorIDProcess)
{
    Header header;
    header.from = LogicalAddress(10);
    GiveDeviceVendorID gvid;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(gvid, header);
}

TEST_F(HdmiCecSourceInitializedTest, RoutingChangeProcess)
{
    Header header;
    header.from = LogicalAddress(11);
    PhysicalAddress from(1,0,0,0);
    PhysicalAddress to(0x0F,0x0F,0x0F,0x0F);
    RoutingChange rc(from, to);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(rc, header);
}

TEST_F(HdmiCecSourceInitializedTest, RoutingInformationProcess)
{
    Header header;
    header.from = LogicalAddress(12);
    RoutingInformation ri;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ri, header);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T("{}"), response));
}

TEST_F(HdmiCecSourceInitializedTest, SetStreamPathProcess)
{
    Header header;
    header.from = LogicalAddress(13);
    PhysicalAddress to(0x0F,0x0F,0x0F,0x0F);
    SetStreamPath ssp(to);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ssp, header);
}

TEST_F(HdmiCecSourceInitializedTest, ReportPhysicalAddressProcess)
{
    Header header;
    header.from = LogicalAddress(14);
    PhysicalAddress pa(1,2,3,0);
    ReportPhysicalAddress rpa(pa, header.from.toInt());
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(rpa, header);
}

TEST_F(HdmiCecSourceInitializedTest, DeviceVendorIDProcess)
{
    Header header;
    header.from = LogicalAddress(3);
    VendorID v(0x11,0x22,0x33);
    DeviceVendorID dvid(v);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(dvid, header);
}

TEST_F(HdmiCecSourceInitializedTest, GiveDevicePowerStatusProcess)
{
    Header header;
    header.from = LogicalAddress(2);
    GiveDevicePowerStatus gdp;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(gdp, header);
}

TEST_F(HdmiCecSourceInitializedTest, ReportPowerStatusProcess)
{
    Header header;
    header.from = LogicalAddress(0);
    ReportPowerStatus rps(PowerStatus(1));
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(rps, header);
}

TEST_F(HdmiCecSourceInitializedTest, UserControlPressedProcess)
{
    Header header;
    header.from = LogicalAddress(1);
    UserControlPressed ucp(UICommand::UI_COMMAND_VOLUME_UP);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ucp, header);
}

TEST_F(HdmiCecSourceInitializedTest, UserControlReleasedProcess)
{
    Header header;
    header.from = LogicalAddress(1);
    UserControlReleased ucr;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ucr, header);
}

TEST_F(HdmiCecSourceInitializedTest, AbortProcess)
{
    Header header;
    header.from = LogicalAddress(4);
    Abort abortMsg;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(abortMsg, header);
}

TEST_F(HdmiCecSourceInitializedTest, SetOSDNameProcessEvent)
{
    Core::ProxyType<NotificationHandler> notif = Core::ProxyType<NotificationHandler>::Create();
    ASSERT_NE(nullptr, Plugin::HdmiCecSourceImplementation::_instance);
    Plugin::HdmiCecSourceImplementation::_instance->Register(notif.operator->());
    Header header;
    header.from = LogicalAddress(5);
    OSDName newName("EVENT_OSD");
    SetOSDName setName(newName);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(setName, header);
    notif->WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnDeviceInfoUpdated);
    Plugin::HdmiCecSourceImplementation::_instance->Unregister(notif.operator->());
}

TEST_F(HdmiCecSourceInitializedTest, StandbyMessageProcessEvent)
{
    Core::ProxyType<NotificationHandler> notif = Core::ProxyType<NotificationHandler>::Create();
    ASSERT_NE(nullptr, Plugin::HdmiCecSourceImplementation::_instance);
    Plugin::HdmiCecSourceImplementation::_instance->Register(notif.operator->());
    Header header;
    header.from = LogicalAddress(2);
    Standby standby;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(standby, header);
    notif->WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_StandbyMessageReceived);
    Plugin::HdmiCecSourceImplementation::_instance->Unregister(notif.operator->());
}

TEST_F(HdmiCecSourceInitializedTest, ActiveSourceEventNotification)
{
    Core::ProxyType<NotificationHandler> notif = Core::ProxyType<NotificationHandler>::Create();
    ASSERT_NE(nullptr, Plugin::HdmiCecSourceImplementation::_instance);
    Plugin::HdmiCecSourceImplementation::_instance->Register(notif.operator->());
    Header header;
    header.from = LogicalAddress(2);
    PhysicalAddress pa(0x0F,0x0F,0x0F,0x0F);
    ActiveSource as(pa);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(as, header);
    notif->WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnActiveSourceStatusUpdated);
    Plugin::HdmiCecSourceImplementation::_instance->Unregister(notif.operator->());
}

TEST_F(HdmiCecSourceInitializedTest, KeyPressEventNotification)
{
    Core::ProxyType<NotificationHandler> notif = Core::ProxyType<NotificationHandler>::Create();
    ASSERT_NE(nullptr, Plugin::HdmiCecSourceImplementation::_instance);
    Plugin::HdmiCecSourceImplementation::_instance->Register(notif.operator->());
    Header header;
    header.from = LogicalAddress(3);
    UserControlPressed ucp(UICommand::UI_COMMAND_NUM_5);
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ucp, header);
    notif->WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnKeyPressEvent);
    Plugin::HdmiCecSourceImplementation::_instance->Unregister(notif.operator->());
}

TEST_F(HdmiCecSourceInitializedTest, KeyReleaseEventNotification)
{
    Core::ProxyType<NotificationHandler> notif = Core::ProxyType<NotificationHandler>::Create();
    ASSERT_NE(nullptr, Plugin::HdmiCecSourceImplementation::_instance);
    Plugin::HdmiCecSourceImplementation::_instance->Register(notif.operator->());
    Header header;
    header.from = LogicalAddress(3);
    UserControlReleased ucr;
    Plugin::HdmiCecSourceProcessor proc(Connection::getInstance());
    proc.process(ucr, header);
    notif->WaitForRequestStatus(JSON_TIMEOUT, HdmiCecSource_OnKeyReleaseEvent);
    Plugin::HdmiCecSourceImplementation::_instance->Unregister(notif.operator->());
}
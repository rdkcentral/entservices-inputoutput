/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
**/

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <string>


#include "HdmiCecSink.h"
#include "HdmiCecSinkImplementation.h"
#include "HdmiCecSinkMock.h"
#include "FactoriesImplementation.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "devicesettings.h"
#include "HdmiCec.h"
#include "HdmiCecMock.h"
#include "WrapsMock.h"
#include "RfcApiMock.h"
#include "ThunderPortability.h"
#include "PowerManagerMock.h"
#include "WorkerPoolImplementation.h"
#include "COMLinkMock.h"
#include "ManagerMock.h"
#include "HostMock.h"
#include "HdmiInputMock.h"

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

class HdmiCecSinkInitializeTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::HdmiCecSink> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    IARM_EventHandler_t dsHdmiEventHandler;
    Core::ProxyType<Plugin::HdmiCecSinkImplementation> pluginImpl;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    NiceMock<ServiceMock> service;
    PLUGINHOST_DISPATCHER* dispatcher;
    NiceMock<COMLinkMock> comLinkMock;
    Core::JSONRPC::Message message;
    string response;

    HdmiCecSinkInitializeTest()
        : plugin(Core::ProxyType<Plugin::HdmiCecSink>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
		, dsHdmiEventHandler(nullptr)
        , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
              2, Core::Thread::DefaultStackSize(), 16))
		, dispatcher(nullptr)
    {
    }

    virtual ~HdmiCecSinkInitializeTest() override
    {
        plugin.Release();
    }
};

class HdmiCecSinkDsTest : public HdmiCecSinkInitializeTest {
protected:
    IarmBusImplMock         *p_iarmBusImplMock = nullptr ;
    ManagerImplMock         *p_managerImplMock = nullptr ;
    HostImplMock            *p_hostImplMock = nullptr ;
    HdmiInputImplMock       *p_hdmiInputImplMock = nullptr;
    ConnectionImplMock      *p_connectionImplMock = nullptr ;
    MessageEncoderMock      *p_messageEncoderMock = nullptr ;
    LibCCECImplMock         *p_libCCECImplMock = nullptr ;
    RfcApiImplMock   *p_rfcApiImplMock = nullptr ;
    WrapsImplMock  *p_wrapsImplMock   = nullptr ;
    NiceMock<RfcApiImplMock> rfcApiImplMock;
    NiceMock<WrapsImplMock> wrapsImplMock;
    string response;

    HdmiCecSinkDsTest(): HdmiCecSinkInitializeTest()
    {
        createFile("/etc/device.properties", "RDK_PROFILE=TV");
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        p_managerImplMock  = new NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        p_hostImplMock      = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        p_hdmiInputImplMock  = new NiceMock <HdmiInputImplMock>;
        device::HdmiInput::setImpl(p_hdmiInputImplMock);

        p_libCCECImplMock  = new testing::NiceMock <LibCCECImplMock>;
        LibCCEC::setImpl(p_libCCECImplMock);

        p_messageEncoderMock  = new testing::NiceMock <MessageEncoderMock>;
        MessageEncoder::setImpl(p_messageEncoderMock);

        p_connectionImplMock  = new testing::NiceMock <ConnectionImplMock>;
        Connection::setImpl(p_connectionImplMock);

        p_rfcApiImplMock  = new testing::NiceMock <RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);

        p_wrapsImplMock  = new testing::NiceMock <WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock); /*Set up mock for fopen;
                                                      to use the mock implementation/the default behavior of the fopen function from Wraps class.*/

        ON_CALL(*p_connectionImplMock, poll(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const LogicalAddress &from, const Throw_e &doThrow) {
                throw CECNoAckException();
                }));

        EXPECT_CALL(*p_libCCECImplMock, getPhysicalAddress(::testing::_))
            .WillRepeatedly(::testing::Invoke(
                [&](uint32_t *physAddress) {
                    *physAddress = (uint32_t)0x12345678;
                }));

        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const DataBlock&>(::testing::_)))
            .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));
        ON_CALL(*p_messageEncoderMock, encode(::testing::Matcher<const UserControlPressed&>(::testing::_)))
           .WillByDefault(::testing::ReturnRef(CECFrame::getInstance()));

        EXPECT_CALL(*p_managerImplMock, Initialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        ON_CALL(*p_connectionImplMock, open())
            .WillByDefault(::testing::Return());

        EXPECT_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
            .WillRepeatedly(::testing::Return(3));

        ON_CALL(*p_hdmiInputImplMock, isPortConnected(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](int8_t port) {
                    return port == 1? true : false;
                }));

        ON_CALL(*p_hdmiInputImplMock, getHDMIARCPortId(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](int &portId) {
                    portId = 1;
                    return dsERR_NONE;
                }));

        ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) {
                    pluginImpl = Core::ProxyType<Plugin::HdmiCecSinkImplementation>::Create();
                    return &pluginImpl;
                }));

        Core::IWorkerPool::Assign(&(*workerPool));
        workerPool->Run();

        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
           plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);

        EXPECT_EQ(string(""), plugin->Initialize(&service));
    }
    virtual ~HdmiCecSinkDsTest() override {

        plugin->Deinitialize(&service);

        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();
        dispatcher->Deactivate();
        dispatcher->Release();
        PluginHost::IFactories::Assign(nullptr);

        removeFile("/etc/device.properties");

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
        device::HdmiInput::setImpl(nullptr);
        if (p_hdmiInputImplMock != nullptr)
        {
            delete p_hdmiInputImplMock;
            p_hdmiInputImplMock = nullptr;
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

        RfcApi::setImpl(nullptr);
        if (p_rfcApiImplMock != nullptr)
        {
            delete p_rfcApiImplMock;
            p_rfcApiImplMock = nullptr;
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr)
        {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }
    }
};

class HdmiCecSinkInitializedEventTest : public HdmiCecSinkDsTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::JSONRPC::Message message;

    HdmiCecSinkInitializedEventTest(): HdmiCecSinkDsTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
           plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);
    }
    virtual ~HdmiCecSinkInitializedEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();
        PluginHost::IFactories::Assign(nullptr);
    }
};

class HdmiCecSinkInitializedEventDsTest : public HdmiCecSinkInitializedEventTest {
protected:
    HdmiCecSinkInitializedEventDsTest(): HdmiCecSinkInitializedEventTest()
    {
    }
    virtual ~HdmiCecSinkInitializedEventDsTest() override
    {
    }
};

TEST_F(HdmiCecSinkDsTest, RegisteredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setOSDName")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVendorId")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getVendorId")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setActivePath")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setRoutingChange")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getDeviceList")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getActiveSource")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setActiveSource")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getActiveRoute")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setMenuLanguage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("requestActiveSource")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setupARCRouting")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("requestShortAudioDescriptor")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendStandbyMessage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendAudioDevicePowerOnMessage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendKeyPressEvent")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendGetAudioStatusMessage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getAudioDeviceConnectedStatus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("requestAudioDevicePowerStatus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendUserControlPressed")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendUserControlReleased")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setLatencyInfo")));

}

TEST_F(HdmiCecSinkDsTest, setOSDNameParamMissing)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, getOSDName)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\":\"CECTEST\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getOSDName"), _T("{}"), response));
    EXPECT_EQ(response,  string("{\"name\":\"CECTEST\",\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setVendorIdParamMissing)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, getVendorId)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\":\"0x0019FF\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVendorId"), _T("{}"), response));
    EXPECT_EQ(response,  string("{\"vendorid\":\"019ff\",\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setActivePathMissingParam)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActivePath"), _T("{}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setActivePath)
{

    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](const LogicalAddress &to, const CECFrame &frame, int timeout) {
               EXPECT_EQ(to.toInt(), LogicalAddress::BROADCAST);
               EXPECT_GT(timeout, 0);
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActivePath"), _T("{\"activePath\":\"2.0.0.0\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setRoutingChangeInvalidParam)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setRoutingChange"), _T("{\"oldPort\":\"HDMI0\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":false}"));

}

TEST_F(HdmiCecSinkDsTest, setRoutingChange)
{

    std::this_thread::sleep_for(std::chrono::seconds(30));

    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](const LogicalAddress &to, const CECFrame &frame, int timeout) {
                EXPECT_EQ(to.toInt(), LogicalAddress::BROADCAST);
                EXPECT_GT(timeout, 0);
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setRoutingChange"), _T("{\"oldPort\":\"HDMI0\",\"newPort\":\"TV\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setMenuLanguageInvalidParam)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setMenuLanguage"), _T("{\"language\":""}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setMenuLanguage)
{

    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](const LogicalAddress &to, const CECFrame &frame, int timeout) {
                EXPECT_LE(to.toInt(), LogicalAddress::BROADCAST);
                EXPECT_GT(timeout, 0);
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setMenuLanguage"), _T("{\"language\":\"english\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setupARCRoutingInvalidParam)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setupARCRouting"), _T("{}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, setupARCRouting)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setupARCRouting"), _T("{\"enabled\":\"true\"}"), response));
    EXPECT_EQ(response,  string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEventMissingParam)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": }"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\": 0, \"keyCode\": 65}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

}

TEST_F(HdmiCecSinkInitializedEventDsTest, onHdmiOutputHDCPStatusEvent)
{

    EVENT_SUBSCRIBE(0, _T("onDevicesChanged"), _T("client.events.onDevicesChanged"), message);
    Plugin::HdmiCecSinkImplementation::_instance->OnHdmiInEventHotPlug(dsHDMI_IN_PORT_1, true);
    EVENT_UNSUBSCRIBE(0, _T("onDevicesChanged"), _T("client.events.onDevicesChanged"), message);

}

TEST_F(HdmiCecSinkInitializedEventDsTest, powerModeChange)
{
    // ASSERT_TRUE(pwrMgrModeChangeEventHandler != nullptr);

    IARM_Bus_PWRMgr_EventData_t eventData;
    eventData.data.state.newState =IARM_BUS_PWRMGR_POWERSTATE_ON;
    eventData.data.state.curState =IARM_BUS_PWRMGR_POWERSTATE_STANDBY;

    (void) eventData;

    // pwrMgrModeChangeEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData , 0);
}

TEST_F(HdmiCecSinkInitializedEventDsTest, DISABLED_getCecVersion)
{
    /*EXPECT_CALL(rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                EXPECT_EQ(string(pcCallerID), string("HdmiCecSink"));
                EXPECT_EQ(string(pcParameterName), string("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.HdmiCecSink.CECVersion"));
                strncpy(pstParamData->value, "1.4", sizeof(pstParamData->value));
                return WDMP_SUCCESS;
            }));*/

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getCecVersion"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"CECVersion\":\"1.4\",\"success\":true}"));

}



//Copilot Generated Code

TEST_F(HdmiCecSinkDsTest, setEnabled_ValidTrue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\":true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setEnabled_ValidFalse)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, getEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"enabled\":(true|false)"));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"success\":true"));
}

TEST_F(HdmiCecSinkDsTest, setOSDName_EmptyName)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\":\"\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setOSDName_LongName)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\":\"VERYLONGNAMETHATEXCEEDSLIMIT\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setVendorId_ValidHex)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\":\"0x123456\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setVendorId_InvalidFormat)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\":\"INVALID\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setActivePath_InvalidPath)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActivePath"), _T("{\"activePath\":\"INVALID.PATH\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setRoutingChange_ValidPorts)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setRoutingChange"), _T("{\"oldPort\":\"HDMI1\",\"newPort\":\"HDMI2\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setRoutingChange_SamePorts)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setRoutingChange"), _T("{\"oldPort\":\"HDMI0\",\"newPort\":\"HDMI0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, getDeviceList)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"numberofdevices\":"));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"success\":true"));
}

TEST_F(HdmiCecSinkDsTest, getActiveSource)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSource"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"logicalAddress\":"));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"success\":true"));
}

TEST_F(HdmiCecSinkDsTest, setActiveSource_ValidLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActiveSource"), _T("{\"logicalAddress\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setActiveSource_InvalidLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActiveSource"), _T("{\"logicalAddress\":16}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setActiveSource_MissingParam)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActiveSource"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, getActiveRoute)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveRoute"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"available\":false,\"length\":0,\"ActiveRoute\":\"\",\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, requestActiveSource)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("requestActiveSource"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, requestShortAudioDescriptor)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("requestShortAudioDescriptor"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendStandbyMessage)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendStandbyMessage"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendAudioDevicePowerOnMessage)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendAudioDevicePowerOnMessage"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_InvalidLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":16,\"keyCode\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_InvalidKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":256}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendGetAudioStatusMessage)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendGetAudioStatusMessage"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, getAudioDeviceConnectedStatus)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getAudioDeviceConnectedStatus"), _T("{}"), response));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"connected\":"));
    EXPECT_THAT(response, ::testing::ContainsRegex("\"success\":true"));
}

TEST_F(HdmiCecSinkDsTest, requestAudioDevicePowerStatus)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("requestAudioDevicePowerStatus"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, getDeviceList_ConnectionClosed)
{
    EXPECT_CALL(*p_connectionImplMock, close())
        .WillOnce(::testing::Return());
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T("{}"), response));
}

TEST_F(HdmiCecSinkDsTest, setOSDName_MaxLength)
{
    string longName(14, 'X');
    string payload = "{\"name\":\"" + longName + "\"}";
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), payload, response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setVendorId_Boundary)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\":\"0xFFFFFF\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setVendorId_MinValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\":\"0x000000\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_BoundaryKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":0,\"keyCode\":255}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_MinKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":0,\"keyCode\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setActiveSource_BoundaryLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActiveSource"), _T("{\"logicalAddress\":15}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setActiveSource_MinLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setActiveSource"), _T("{\"logicalAddress\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setRoutingChange_InvalidPortFormat)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setRoutingChange"), _T("{\"oldPort\":\"INVALID_PORT\",\"newPort\":\"HDMI0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

TEST_F(HdmiCecSinkDsTest, setMenuLanguage_SpecialCharacters)
{    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setMenuLanguage"), _T("{\"language\":\"ñäöü\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, MalformedJSON_setEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\":}"), response));
}

TEST_F(HdmiCecSinkDsTest, MalformedJSON_setOSDName)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setOSDName"), _T("{\"name\":"), response));
}

TEST_F(HdmiCecSinkDsTest, MalformedJSON_setVendorId)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVendorId"), _T("{\"vendorid\""), response));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_VolumeDown)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":66}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Mute)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":67}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Down)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":2}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Left)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":3}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Right)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":4}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Home)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":9}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Back)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":13}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number0)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":32}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number1)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":33}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number2)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":34}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number3)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":35}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number4)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":36}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number5)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":37}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number6)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":38}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number7)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":39}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number8)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":40}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_Number9)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":41}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendKeyPressEvent_NullConnection)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendKeyPressEvent"), _T("{\"logicalAddress\":1,\"keyCode\":65}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_VolumeUp)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":65}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Select)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Up)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Down)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":2}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Left)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":3}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Right)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":4}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Home)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":9}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Back)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":13}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number0)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":32}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number1)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":33}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number2)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":34}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number3)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":35}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number4)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":36}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number5)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":37}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number6)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":38}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number7)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":39}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number8)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":40}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Number9)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":41}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_VolumeDown)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":66}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_Mute)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":67}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_InvalidLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":16,\"keyCode\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_InvalidKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":4,\"keyCode\":256}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_MissingParams)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_BoundaryKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":0,\"keyCode\":255}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_MinKeyCode)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":0,\"keyCode\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_NegativeValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":-1,\"keyCode\":-1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_StringValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":\"invalid\",\"keyCode\":\"test\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlPressed_MalformedJSON)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlPressed"), _T("{\"logicalAddress\":"), response));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_VolumeUp)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":65}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Select)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Up)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Down)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":2}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Left)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":3}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Right)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":4}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Home)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":9}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Back)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":13}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number0)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":32}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number1)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":33}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number2)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":34}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number3)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":35}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number4)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":36}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number5)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":37}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number6)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":38}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number7)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":39}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number8)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":40}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Number9)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":41}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_VolumeDown)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":66}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_Mute)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":67}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_InvalidLogicalAddress)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":16,\"keyCode\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_InvalidKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":4,\"keyCode\":256}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_MissingParams)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_BoundaryKeyCode)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":0,\"keyCode\":255}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_MinKeyCode)
{
    EXPECT_CALL(*p_connectionImplMock, sendTo(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return());

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":0,\"keyCode\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_NegativeValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":-1,\"keyCode\":-1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_StringValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":\"invalid\",\"keyCode\":\"test\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, sendUserControlReleased_MalformedJSON)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendUserControlReleased"), _T("{\"logicalAddress\":"), response));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_ValidParameters)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"20\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"10\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_MinValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"0\",\"lowLatencyMode\":\"0\",\"audioOutputCompensated\":\"0\",\"audioOutputDelay\":\"0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_MaxValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"255\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"255\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_LowLatencyModeEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"15\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"5\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_LowLatencyModeDisabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"50\",\"lowLatencyMode\":\"0\",\"audioOutputCompensated\":\"0\",\"audioOutputDelay\":\"25\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_AudioOutputCompensatedEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"30\",\"lowLatencyMode\":\"0\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"15\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_AudioOutputCompensatedDisabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"40\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"0\",\"audioOutputDelay\":\"20\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_HighVideoLatency)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"100\",\"lowLatencyMode\":\"0\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"50\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_HighAudioDelay)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"25\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"200\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_AllParametersEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"35\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"30\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_AllParametersDisabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"45\",\"lowLatencyMode\":\"0\",\"audioOutputCompensated\":\"0\",\"audioOutputDelay\":\"35\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_NegativeValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"-10\",\"lowLatencyMode\":\"-1\",\"audioOutputCompensated\":\"-1\",\"audioOutputDelay\":\"-5\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_LargeValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"1000\",\"lowLatencyMode\":\"5\",\"audioOutputCompensated\":\"10\",\"audioOutputDelay\":\"2000\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_FloatValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"20.5\",\"lowLatencyMode\":\"1.2\",\"audioOutputCompensated\":\"0.8\",\"audioOutputDelay\":\"10.7\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_ZeroStringValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"0\",\"lowLatencyMode\":\"0\",\"audioOutputCompensated\":\"0\",\"audioOutputDelay\":\"0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_OneStringValues)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"1\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"1\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_ExtraParameters)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"20\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"10\",\"extraParam\":\"ignored\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecSinkDsTest, setLatencyInfo_DuplicateParameters)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setLatencyInfo"), _T("{\"videoLatency\":\"20\",\"videoLatency\":\"30\",\"lowLatencyMode\":\"1\",\"audioOutputCompensated\":\"1\",\"audioOutputDelay\":\"10\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

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

#include "HdcpProfile.h"

#include "FactoriesImplementation.h"
#include "HostMock.h"
#include "ManagerMock.h"
#include "VideoOutputPortConfigMock.h"
#include "VideoOutputPortMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "dsMgr.h"
#include "dsDisplay.h"
#include "ThunderPortability.h"
#include "PowerManagerMock.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include "COMLinkMock.h"
#include "WrapsMock.h"
#include "IarmBusMock.h"
#include "WorkerPoolImplementation.h"
#include "HdcpProfileImplementation.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class HDCPProfileTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::HdcpProfile> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    Core::JSONRPC::Message message;
    string response;

    WrapsImplMock *p_wrapsImplMock = nullptr;
    IarmBusImplMock  *p_iarmBusImplMock   = nullptr;
    Core::ProxyType<Plugin::HdcpProfileImplementation> hdcpProfileImpl;

    NiceMock<COMLinkMock> comLinkMock;
    NiceMock<ServiceMock> service;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    
    NiceMock<FactoriesImplementation> factoriesImplementation;

    HDCPProfileTest()
        : plugin(Core::ProxyType<Plugin::HdcpProfile>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
        , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(2, Core::Thread::DefaultStackSize(), 16))
    {
        p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        printf("Pass created wrapsImplMock: %p ", p_wrapsImplMock);
        Wraps::setImpl(p_wrapsImplMock);
        
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
    	IarmBus::setImpl(p_iarmBusImplMock);

        
        ON_CALL(service, COMLink())
        .WillByDefault(::testing::Invoke(
              [this]() {
                    TEST_LOG("Pass created comLinkMock: %p ", &comLinkMock);
                    return &comLinkMock;
                }));


        #ifdef USE_THUNDER_R4
            ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_))
                .WillByDefault(::testing::Invoke(
                        [&](const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) {
                            hdcpProfileImpl = Core::ProxyType<Plugin::HdcpProfileImplementation>::Create();
                            TEST_LOG("Pass created hdcpProfileImpl: %p ", &hdcpProfileImpl);
                            return &hdcpProfileImpl;
                    }));
         #else
            ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                 .WillByDefault(::testing::Return(hdcpProfileImpl));
         #endif /*USE_THUNDER_R4 */
        
                PluginHost::IFactories::Assign(&factoriesImplementation);
        
                Core::IWorkerPool::Assign(&(*workerPool));
                workerPool->Run();
        
                dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
                plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
                dispatcher->Activate(&service);
        
                EXPECT_EQ(string(""), plugin->Initialize(&service));

    }
    virtual ~HDCPProfileTest() override
    {
        TEST_LOG("HdcpProfileTest Destructor");

        plugin->Deinitialize(&service);

        dispatcher->Deactivate();
        dispatcher->Release();

        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();
    
        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr)
        {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }
        PluginHost::IFactories::Assign(nullptr);
        IarmBus::setImpl(nullptr);
         if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
        
    }
};

class HDCPProfileDsTest : public HDCPProfileTest {
protected:
    HostImplMock             *p_hostImplMock = nullptr ;
    VideoOutputPortConfigImplMock      *p_videoOutputPortConfigImplMock = nullptr ;
    VideoOutputPortMock                *p_videoOutputPortMock = nullptr ;

    HDCPProfileDsTest()
        : HDCPProfileTest()
    {
        p_hostImplMock  = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);
        p_videoOutputPortConfigImplMock  = new NiceMock <VideoOutputPortConfigImplMock>;
        device::VideoOutputPortConfig::setImpl(p_videoOutputPortConfigImplMock);
        p_videoOutputPortMock  = new NiceMock <VideoOutputPortMock>;
        device::VideoOutputPort::setImpl(p_videoOutputPortMock);
    }
    virtual ~HDCPProfileDsTest() override
    {
        device::VideoOutputPort::setImpl(nullptr);
        if (p_videoOutputPortMock != nullptr)
        {
            delete p_videoOutputPortMock;
            p_videoOutputPortMock = nullptr;
        }
        device::VideoOutputPortConfig::setImpl(nullptr);
        if (p_videoOutputPortConfigImplMock != nullptr)
        {
            delete p_videoOutputPortConfigImplMock;
            p_videoOutputPortConfigImplMock = nullptr;
        }
        device::Host::setImpl(nullptr);
        if (p_hostImplMock != nullptr)
        {
            delete p_hostImplMock;
            p_hostImplMock = nullptr;
        }
    }
};

class HDCPProfileEventTest : public HDCPProfileDsTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::JSONRPC::Message message;

    HDCPProfileEventTest()
        : HDCPProfileDsTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
            plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);
    }

    virtual ~HDCPProfileEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);
    }
};

class HDCPProfileEventIarmTest : public HDCPProfileEventTest {
protected:
    ManagerImplMock   *p_managerImplMock = nullptr ;
    IARM_EventHandler_t dsHdmiEventHandler;

    HDCPProfileEventIarmTest()
        : HDCPProfileEventTest()
    {
        p_managerImplMock  = new NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        EXPECT_CALL(*p_managerImplMock, Initialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG)) {
                        dsHdmiEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDCP_STATUS)) {
                         dsHdmiEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));

        EXPECT_EQ(string(""), plugin->Initialize(&service));
    }

    virtual ~HDCPProfileEventIarmTest() override
    {
        plugin->Deinitialize(&service);
        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr)
        {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }
    }
};

TEST_F(HDCPProfileTest, RegisteredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getHDCPStatus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getSettopHDCPSupport")));
}

TEST_F(HDCPProfileDsTest, getHDCPStatus_isConnected_false)
{
    device::VideoOutputPort videoOutputPort;

    string videoPort(_T("HDMI0"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPStatus())
        .WillByDefault(::testing::Return(dsHDCP_STATUS_UNPOWERED));
    ON_CALL(*p_videoOutputPortMock, isContentProtected())
        .WillByDefault(::testing::Return(0));
    ON_CALL(*p_videoOutputPortMock, getHDCPReceiverProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_MAX));
    ON_CALL(*p_videoOutputPortMock, getHDCPCurrentProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_MAX));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getHDCPStatus"), _T(""), response));
    EXPECT_THAT(response, ::testing::MatchesRegex(_T("\\{"
                                                     "\"HDCPStatus\":"
                                                     "\\{"
                                                     "\"isConnected\":false,"
                                                     "\"isHDCPCompliant\":false,"
                                                     "\"isHDCPEnabled\":false,"
                                                     "\"hdcpReason\":0,"
                                                     "\"supportedHDCPVersion\":\"[1-2]+.[1-4]\","
                                                     "\"receiverHDCPVersion\":\"[1-2]+.[1-4]\","
                                                     "\"currentHDCPVersion\":\"[1-2]+.[1-4]\""
                                                     "\\},"
                                                     "\"success\":true"
                                                     "\\}")));
}

TEST_F(HDCPProfileDsTest, getHDCPStatus_isConnected_true)
{
    NiceMock<VideoOutputPortMock> videoOutputPortMock;
    device::VideoOutputPort videoOutputPort;

    string videoPort(_T("HDMI0"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPStatus())
        .WillByDefault(::testing::Return(dsHDCP_STATUS_AUTHENTICATED));
    ON_CALL(*p_videoOutputPortMock, isContentProtected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getHDCPReceiverProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPCurrentProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getHDCPStatus"), _T(""), response));
    EXPECT_THAT(response, ::testing::MatchesRegex(_T("\\{"
                                                     "\"HDCPStatus\":"
                                                     "\\{"
                                                     "\"isConnected\":true,"
                                                     "\"isHDCPCompliant\":true,"
                                                     "\"isHDCPEnabled\":true,"
                                                     "\"hdcpReason\":2,"
                                                     "\"supportedHDCPVersion\":\"[1-2]+.[1-4]\","
                                                     "\"receiverHDCPVersion\":\"[1-2]+.[1-4]\","
                                                     "\"currentHDCPVersion\":\"[1-2]+.[1-4]\""
                                                     "\\},"
                                                     "\"success\":true"
                                                     "\\}")));
}

TEST_F(HDCPProfileDsTest, getSettopHDCPSupport_Hdcp_v1x)
{
    NiceMock<VideoOutputPortMock> videoOutputPortMock;
    device::VideoOutputPort videoOutputPort;

    string videoPort(_T("HDMI0"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_1X));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getSettopHDCPSupport"), _T(""), response));
    EXPECT_THAT(response, ::testing::MatchesRegex(_T("\\{"
                                                     "\"supportedHDCPVersion\":\"[1-2]+.[1-4]\","
                                                     "\"isHDCPSupported\":true,"
                                                     "\"success\":true"
                                                     "\\}")));
}

TEST_F(HDCPProfileDsTest, getSettopHDCPSupport_Hdcp_v2x)
{
    NiceMock<VideoOutputPortMock> videoOutputPortMock;
    device::VideoOutputPort videoOutputPort;

    string videoPort(_T("HDMI0"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getSettopHDCPSupport"), _T(""), response));
    EXPECT_THAT(response, ::testing::MatchesRegex(_T("\\{"
                                                     "\"supportedHDCPVersion\":\"[1-2]+.[1-4]\","
                                                     "\"isHDCPSupported\":true,"
                                                     "\"success\":true"
                                                     "\\}")));
}

#if 0

TEST_F(HDCPProfileEventIarmTest, onDisplayConnectionChanged)
{
    ASSERT_TRUE(dsHdmiEventHandler != nullptr);

    Core::Event onDisplayConnectionChanged(false, true);

    NiceMock<VideoOutputPortMock> videoOutputPortMock;
    device::VideoOutputPort videoOutputPort;

    string videoPort(_T("HDMI0"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPStatus())
        .WillByDefault(::testing::Return(dsHDCP_STATUS_AUTHENTICATED));
    ON_CALL(*p_videoOutputPortMock, isContentProtected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getHDCPReceiverProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPCurrentProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));

                //EXPECT_EQ(text, string(_T("")));
                EXPECT_THAT(text, ::testing::MatchesRegex(_T("\\{"
                "\"jsonrpc\":\"2.0\","
                "\"method\":\"client.events.onDisplayConnectionChanged\","
                "\"params\":"
                "\\{\"HDCPStatus\":"
                "\\{"
                "\"isConnected\":true,"
                "\"isHDCPCompliant\":true,"
                "\"isHDCPEnabled\":true,"
                "\"hdcpReason\":2,"
                "\"supportedHDCPVersion\":\"2.2\","
                "\"receiverHDCPVersion\":\"2.2\","
                "\"currentHDCPVersion\":\"2.2\""
                "\\}"
                "\\}"
                "\\}")));

                onDisplayConnectionChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onDisplayConnectionChanged"), _T("client.events"), message);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_hpd.event = dsDISPLAY_EVENT_CONNECTED;
    dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onDisplayConnectionChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onDisplayConnectionChanged"), _T("client.events"), message);
}

#endif
TEST_F(HDCPProfileEventIarmTest, onHdmiOutputHDCPStatusEvent)
{
    ASSERT_TRUE(dsHdmiEventHandler != nullptr);

    Core::Event onDisplayConnectionChanged(false, true);

    NiceMock<VideoOutputPortMock> videoOutputPortMock;
    device::VideoOutputPort videoOutputPort;

    string videoPort(_T("HDMI0"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPStatus())
        .WillByDefault(::testing::Return(dsHDCP_STATUS_AUTHENTICATED));
    ON_CALL(*p_videoOutputPortMock, isContentProtected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getHDCPReceiverProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
    ON_CALL(*p_videoOutputPortMock, getHDCPCurrentProtocol())
        .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));

      EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));

                EXPECT_THAT(text, ::testing::MatchesRegex(_T("\\{"
                "\"jsonrpc\":\"2.0\","
                "\"method\":\"client.events.onDisplayConnectionChanged\","
                "\"params\":"
                "\\{\"HDCPStatus\":"
                "\\{"
                "\"isConnected\":true,"
                "\"isHDCPCompliant\":true,"
                "\"isHDCPEnabled\":true,"
                "\"hdcpReason\":2,"
                "\"supportedHDCPVersion\":\"2.2\","
                "\"receiverHDCPVersion\":\"2.2\","
                "\"currentHDCPVersion\":\"2.2\""
                "\\}"
                "\\}"
                "\\}")));

                onDisplayConnectionChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onDisplayConnectionChanged"), _T("client.events"), message);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_hdcp.hdcpStatus = dsDISPLAY_HDCPPROTOCOL_CHANGE;
    dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDCP_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onDisplayConnectionChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onDisplayConnectionChanged"), _T("client.events"), message);
}

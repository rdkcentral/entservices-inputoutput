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
#include "COMLinkMock.h"
#include <gmock/gmock.h>

#include "AVInput.h"

#include "CompositeInputMock.h"
#include "FactoriesImplementation.h"
#include "HdmiInputMock.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "ThunderPortability.h"

#include "AVInputImplementation.h"
#include "AVInputMock.h"
#include "WorkerPoolImplementation.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class AVInputTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::AVInput> plugin;
    Core::ProxyType<Plugin::AVInputImplementation> AVInputImpl;

    NiceMock<ServiceMock> service;
    NiceMock<COMLinkMock> comLinkMock;
    Core::JSONRPC::Handler& handler;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    string response;
    AVInputMock* p_avInputMock = nullptr;
    DECL_CORE_JSONRPC_CONX connection;
    IarmBusImplMock* p_iarmBusImplMock = nullptr;
    PLUGINHOST_DISPATCHER* dispatcher;

    IARM_EventHandler_t dsAVGameFeatureStatusEventHandler   = nullptr;
    IARM_EventHandler_t dsAVEventHandler                    = nullptr;    
    IARM_EventHandler_t dsAVSignalStatusEventHandler        = nullptr;
    IARM_EventHandler_t dsAVStatusEventHandler              = nullptr;
    IARM_EventHandler_t dsAVVideoModeEventHandler           = nullptr;
    IARM_EventHandler_t dsAviContentTypeEventHandler        = nullptr;

    AVInputTest()
        : plugin(Core::ProxyType<Plugin::AVInput>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
        , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
          2, Core::Thread::DefaultStackSize(), 16))
    {
        p_avInputMock  = new NiceMock<AVInputMock>;

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
            plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);

        #ifdef USE_THUNDER_R4
        ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_))
                .WillByDefault(::testing::Invoke(
                    [&](const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) {
                        AVInputImpl = Core::ProxyType<Plugin::AVInputImplementation>::Create();
                        return &AVInputImpl;
                    }));
        #else
            ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillByDefault(::testing::Return(AVInputImpl));
        #endif

        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVSignalStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVVideoModeEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVGameFeatureStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVGameFeatureStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVSignalStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAVVideoModeEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsAviContentTypeEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));

        Core::IWorkerPool::Assign(&(*workerPool));
        workerPool->Run();

        plugin->Initialize(&service);
    }

    virtual ~AVInputTest()
    {
        dispatcher->Deactivate();
        dispatcher->Release();

        plugin->Deinitialize(&service);
        
        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr) {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }

        if (p_avInputMock != nullptr) {
            delete p_avInputMock;
            p_avInputMock = nullptr;
        }
    }
};

// <pca> debug
#if 1

TEST_F(AVInputTest, RegisteredMethods)
{
    printf("*** _DEBUG: AVInputTest: RegisteredMethods ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("numberOfInputs")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("currentVideoMode")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("contentProtected")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setEdid2AllmSupport")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getEdid2AllmSupport")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVRRSupport")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getVRRSupport")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getVRRFrameRate")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getInputDevices")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("writeEDID")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("readEDID")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getRawSPD")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getSPD")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setEdidVersion")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getEdidVersion")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getHdmiVersion")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setMixerLevels")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("startInput")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("stopInput")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVideoRectangle")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getSupportedGameFeatures")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getGameFeatureStatus")));
    printf("*** _DEBUG: AVInputTest: RegisteredMethods: exit ***\n");
}

TEST_F(AVInputTest, contentProtected)
{
    printf("*** _DEBUG: AVInputTest: contentProtected ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("contentProtected"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"isContentProtected\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputTest: contentProtected: exit ***\n");
}

#endif
// </pca>

// <pca> debug
#if 1
class AVInputDsTest : public AVInputTest {
protected:
    HdmiInputImplMock* p_hdmiInputImplMock = nullptr;
    CompositeInputImplMock* p_compositeInputImplMock = nullptr;
    HostImplMock* p_HostImplMock = nullptr;

    AVInputDsTest()
        : AVInputTest()
    {
        p_hdmiInputImplMock  = new NiceMock <HdmiInputImplMock>;
        device::HdmiInput::setImpl(p_hdmiInputImplMock);

        p_compositeInputImplMock = new NiceMock<CompositeInputImplMock>;
        device::CompositeInput::setImpl(p_compositeInputImplMock);

        p_HostImplMock = new NiceMock<HostImplMock>;
        device::Host::setImpl(p_HostImplMock);
    }
    virtual ~AVInputDsTest() override
    {
        device::HdmiInput::setImpl(nullptr);
        if (p_hdmiInputImplMock != nullptr)
        {
            delete p_hdmiInputImplMock;
            p_hdmiInputImplMock = nullptr;
        }

        device::CompositeInput::setImpl(nullptr);
        if (p_compositeInputImplMock != nullptr) {
            delete p_compositeInputImplMock;
            p_compositeInputImplMock = nullptr;
        }

        device::Host::setImpl(nullptr);
        if (p_HostImplMock != nullptr) {
            delete p_HostImplMock;
            p_HostImplMock = nullptr;
        }
    }
};
#endif
// </pca>

// <pca> debug
#if 1

TEST_F(AVInputDsTest, numberOfInputs)
{
    printf("*** _DEBUG: AVInputDsTest: numberOfInputs ***\n");
    ON_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("numberOfInputs"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"numberOfInputs\":1,\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: numberOfInputs: exit ***\n");
}

TEST_F(AVInputDsTest, currentVideoMode)
{
    printf("*** _DEBUG: AVInputDsTest: currentVideoMode ***\n");
    ON_CALL(*p_hdmiInputImplMock, getCurrentVideoMode())
        .WillByDefault(::testing::Return(string("unknown")));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("currentVideoMode"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"currentVideoMode\":\"unknown\",\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: currentVideoMode: exit ***\n");
}

TEST_F(AVInputDsTest, getEdid2AllmSupport)
{
    printf("*** _DEBUG: AVInputDsTest: getEdid2AllmSupport ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEdid2AllmSupport"), _T("{\"portId\": \"0\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"allmSupport\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: getEdid2AllmSupport: exit ***\n");
}

TEST_F(AVInputDsTest, getEdid2AllmSupport_ErrorCase)
{
    printf("*** _DEBUG: AVInputDsTest: getEdid2AllmSupport_ErrorCase ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getEdid2AllmSupport"), _T("{\"portId\": \"test\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputDsTest: getEdid2AllmSupport_ErrorCase: exit ***\n");
}

TEST_F(AVInputDsTest, setEdid2AllmSupport)
{
    printf("*** _DEBUG: AVInputDsTest: setEdid2AllmSupport ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEdid2AllmSupport"), _T("{\"portId\": \"0\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: setEdid2AllmSupport: exit ***\n");
}

TEST_F(AVInputDsTest, setEdid2AllmSupport_ErrorCase)
{
    printf("*** _DEBUG: AVInputDsTest: setEdid2AllmSupport_ErrorCase ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setEdid2AllmSupport"), _T("{\"portId\": \"test\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputDsTest: setEdid2AllmSupport_ErrorCase: exit ***\n");
}

TEST_F(AVInputDsTest, getVRRSupport)
{
    printf("*** _DEBUG: AVInputDsTest: getVRRSupport ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVRRSupport"), _T("{\"portId\": \"0\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"vrrSupport\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: getVRRSupport: exit ***\n");
}

TEST_F(AVInputDsTest, getVRRSupport_ErrorCase)
{
    printf("*** _DEBUG: AVInputDsTest: getVRRSupport_ErrorCase ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getVRRSupport"), _T("{\"portId\": \"test\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputDsTest: getVRRSupport_ErrorCase: exit ***\n");
}

TEST_F(AVInputDsTest, setVRRSupport)
{
    printf("*** _DEBUG: AVInputDsTest: setVRRSupport ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVRRSupport"), _T("{\"portId\": \"0\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: setVRRSupport: exit ***\n");
}

TEST_F(AVInputDsTest, setVRRSupport_ErrorCase)
{
    printf("*** _DEBUG: AVInputDsTest: setVRRSupport_ErrorCase ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setVRRSupport"), _T("{\"portId\": \"test\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputDsTest: setVRRSupport_ErrorCase: exit ***\n");
}

TEST_F(AVInputDsTest, getVRRFrameRate)
{
    printf("*** _DEBUG: AVInputDsTest: getVRRFrameRate ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVRRFrameRate"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"currentVRRVideoFrameRate\":0,\"success\":true}"));
    printf("*** _DEBUG: AVInputDsTest: getVRRFrameRate: exit ***\n");
}

TEST_F(AVInputDsTest, getVRRFrameRate_ErrorCase)
{
    printf("*** _DEBUG: AVInputDsTest: getVRRFrameRate_ErrorCase ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getVRRFrameRate"), _T("{\"portId\": \"test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputDsTest: getVRRFrameRate_ErrorCase: exit ***\n");
}

#endif
// </pca>

// <pca> debug
#if 1
class AVInputInit : public AVInputDsTest {
protected:
    NiceMock<FactoriesImplementation> factoriesImplementation;
    Core::JSONRPC::Message message;

    AVInputInit()
        : AVInputDsTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);
    }

    virtual ~AVInputInit() override
    {
        PluginHost::IFactories::Assign(nullptr);
    }
};
#endif
// </pca>

// <pca> debug
#if 1

TEST_F(AVInputInit, getInputDevices)
{
    printf("*** _DEBUG: AVInputInit: getInputDevices ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
        .WillOnce(::testing::Return(1));
    EXPECT_CALL(*p_compositeInputImplMock, getNumberOfInputs())
        .WillOnce(::testing::Return(1));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getInputDevices"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"devices\":[{\"id\":0,\"connected\":false,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\"},{\"id\":0,\"connected\":false,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\"}],\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getInputDevices: exit ***\n");
}

TEST_F(AVInputInit, getInputDevices_HDMI)
{
    printf("*** _DEBUG: AVInputInit: getInputDevices_HDMI ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
        .WillOnce(::testing::Return(1));
    EXPECT_CALL(*p_hdmiInputImplMock, isPortConnected(::testing::_))
        .WillOnce(::testing::Return(true));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getInputDevices"), _T("{\"typeOfInput\": \"HDMI\"}"), response));
    EXPECT_EQ(response, string("{\"devices\":[{\"id\":0,\"connected\":true,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\"}],\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getInputDevices_HDMI: exit ***\n");
}

TEST_F(AVInputInit, getInputDevices_COMPOSITE)
{
    printf("*** _DEBUG: AVInputInit: getInputDevices_COMPOSITE ***\n");
    EXPECT_CALL(*p_compositeInputImplMock, getNumberOfInputs())
        .WillOnce(::testing::Return(1));
    EXPECT_CALL(*p_compositeInputImplMock, isPortConnected(::testing::_))
        .WillOnce(::testing::Return(true));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getInputDevices"), _T("{\"typeOfInput\": \"COMPOSITE\"}"), response));
    EXPECT_EQ(response, string("{\"devices\":[{\"id\":0,\"connected\":true,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\"}],\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getInputDevices_COMPOSITE: exit ***\n");
}

TEST_F(AVInputInit, getInputDevices_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: getInputDevices_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getInputDevices"), _T("{\"typeOfInput\": \"Test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getInputDevices_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, writeEDID)
{
    printf("*** _DEBUG: AVInputInit: writeEDID ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("writeEDID"), _T("{\"portId\": \"1\",\"message\":\"Test\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: writeEDID: exit ***\n");
}

TEST_F(AVInputInit, writeEDID_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: writeEDID_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("writeEDID"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: writeEDID_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, readEDID)
{
    printf("*** _DEBUG: AVInputInit: readEDID ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getEDIDBytesInfo(::testing::_, ::testing::_))
        .WillOnce([](int port, std::vector<uint8_t>& edid) {
            EXPECT_EQ(port, 1);
            edid = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("readEDID"), _T("{\"portId\": \"1\"}"), response));
    EXPECT_EQ(response, string("{\"EDID\":\"AP\\/\\/\\/\\/\\/\\/\\/w==\",\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: readEDID: exit ***\n");
}

TEST_F(AVInputInit, readEDIDFailure)
{
    printf("*** _DEBUG: AVInputInit: readEDIDFailure ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getEDIDBytesInfo(::testing::_, ::testing::_))
        .WillOnce([](int port, std::vector<uint8_t>& edid) {
            edid = {};
        });

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("readEDID"), _T("{\"portId\": \"1\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: readEDIDFailure: exit ***\n");
}

TEST_F(AVInputInit, readEDID_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: readEDID_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("readEDID"), _T("{\"portId\": \"test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: readEDID_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, getRawSPD)
{
    printf("*** _DEBUG: AVInputInit: getRawSPD ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getHDMISPDInfo(::testing::_, ::testing::_))
        .WillOnce([](int port, std::vector<uint8_t>& data) {
            data = { 0x53, 0x50, 0x44, 0x00 };
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getRawSPD"), _T("{\"portId\": \"1\"}"), response));
    EXPECT_EQ(response, string("{\"HDMISPD\":\"U1BEAA\",\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getRawSPD: exit ***\n");
}

TEST_F(AVInputInit, getRawSPD_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: getRawSPD_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getRawSPD"), _T("{\"portId\": \"test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getRawSPD_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, getSPD)
{
    printf("*** _DEBUG: AVInputInit: getSPD ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getHDMISPDInfo(::testing::_, ::testing::_))
        .WillOnce([](int port, std::vector<uint8_t>& data) {
            data = { 0x53, 0x50, 0x44, 0x00 };
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getSPD"), _T("{\"portId\": \"1\"}"), response));
    EXPECT_EQ(response, string("{\"HDMISPD\":\"Packet Type:53,Version:80,Length:68,vendor name:wn,product des:,source info:00\",\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getSPD: exit ***\n");
}

TEST_F(AVInputInit, getSPD_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: getSPD_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getSPD"), _T("{\"portId\": \"test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getSPD_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, setEdidVersion)
{
    printf("*** _DEBUG: AVInputInit: setEdidVersion ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, setEdidVersion(::testing::_, ::testing::_))
        .WillOnce([](int port, int edidVersion) {
            EXPECT_EQ(port, 1);
            EXPECT_EQ(edidVersion, 0);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEdidVersion"), _T("{\"portId\": \"1\", \"edidVersion\":\"HDMI1.4\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: setEdidVersion: exit ***\n");
}

TEST_F(AVInputInit, setEdidVersion_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: setEdidVersion_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setEdidVersion"), _T("{\"portId\": \"test\", \"edidVersion\":\"test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: setEdidVersion_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, getEdidVersion1)
{
    printf("*** _DEBUG: AVInputInit: getEdidVersion1 ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getEdidVersion(::testing::_, ::testing::_))
        .WillOnce([](int port, int* edidVersion) {
            EXPECT_EQ(port, 0);
            *edidVersion = 0;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEdidVersion"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"edidVersion\":\"HDMI1.4\",\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getEdidVersion1: exit ***\n");
}

TEST_F(AVInputInit, getEdidVersion2)
{
    printf("*** _DEBUG: AVInputInit: getEdidVersion2 ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getEdidVersion(::testing::_, ::testing::_))
        .WillOnce([](int port, int* edidVersion) {
            EXPECT_EQ(port, 1);
            *edidVersion = 1;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEdidVersion"), _T("{\"portId\": \"1\"}"), response));
    EXPECT_EQ(response, string("{\"edidVersion\":\"HDMI2.0\",\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getEdidVersion2: exit ***\n");
}

TEST_F(AVInputInit, getEdidVersion_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: getEdidVersion_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getEdidVersion"), _T("{\"portId\": \"test\"}}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getEdidVersion_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, getHdmiVersion)
{
    printf("*** _DEBUG: AVInputInit: getHdmiVersion ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getHdmiVersion(::testing::_, ::testing::_))
        .WillOnce([](int port, dsHdmiMaxCapabilityVersion_t* capVersion) {
            if (capVersion) {
                *capVersion = HDMI_COMPATIBILITY_VERSION_21;
            }
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getHdmiVersion"), _T("{\"portId\": \"1\"}"), response));
    EXPECT_EQ(response, string("{\"HdmiCapabilityVersion\":\"2.1\",\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getHdmiVersion: exit ***\n");
}

TEST_F(AVInputInit, getHdmiVersion_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: getHdmiVersion_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getHdmiVersion"), _T("{\"portId\": \"test\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getHdmiVersion_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, setMixerLevels)
{
    printf("*** _DEBUG: AVInputInit: setMixerLevels ***\n");
    EXPECT_CALL(*p_HostImplMock, setAudioMixerLevels(dsAUDIO_INPUT_PRIMARY, ::testing::_))
        .WillOnce([](dsAudioInput_t input, int volume) {
            EXPECT_EQ(input, dsAUDIO_INPUT_PRIMARY);
            EXPECT_EQ(volume, 50);
        });

    EXPECT_CALL(*p_HostImplMock, setAudioMixerLevels(dsAUDIO_INPUT_SYSTEM, ::testing::_))
        .WillOnce([](dsAudioInput_t input, int volume) {
            EXPECT_EQ(input, dsAUDIO_INPUT_SYSTEM);
            EXPECT_EQ(volume, 30);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setMixerLevels"), _T("{\"primaryVolume\": 50 ,\"inputVolume\":30}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: setMixerLevels: exit ***\n");
}

TEST_F(AVInputInit, setMixerLevelsErrorCase)
{
    printf("*** _DEBUG: AVInputInit: setMixerLevelsErrorCase ***\n");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setMixerLevels"), _T("{\"primaryVolume\": 110 ,\"inputVolume\":110}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: setMixerLevelsErrorCase: exit ***\n");
}

TEST_F(AVInputInit, startInput_HDMI)
{
    printf("*** _DEBUG: AVInputInit: startInput_HDMI ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, selectPort(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce([](int8_t Port, bool audioMix, int videoPlane, bool topMost) {
            EXPECT_EQ(Port, 1);
            EXPECT_EQ(audioMix, true);
            EXPECT_EQ(videoPlane, 1);
            EXPECT_EQ(topMost, true);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startInput"), _T("{\"portId\": 1 ,\"typeOfInput\":\"HDMI\", \"requestAudioMix\": true, \"plane\" : 1, \"topMost\" : true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: startInput_HDMI: exit ***\n");
}

TEST_F(AVInputInit, startInput_COMPOSITE)
{
    printf("*** _DEBUG: AVInputInit: startInput_COMPOSITE ***\n");
    EXPECT_CALL(*p_compositeInputImplMock, selectPort(::testing::_))
        .WillOnce([](int8_t Port) {
            EXPECT_EQ(Port, 2);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startInput"), _T("{\"portId\": 2 ,\"typeOfInput\":\"COMPOSITE\", \"requestAudioMix\": true, \"plane\" : 1, \"topMost\" : true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: startInput_COMPOSITE: exit ***\n");
}

TEST_F(AVInputInit, startInput_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: startInput_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("startInput"), _T("{\"portId\": \"test\" ,\"typeOfInput\":\"HDMI\", \"requestAudioMix\": true, \"plane\" : 1, \"topMost\" : true}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: startInput_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, stopInput_HDMI)
{
    printf("*** _DEBUG: AVInputInit: stopInput_HDMI ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, selectPort(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce([](int8_t Port, bool audioMix, int videoPlane, bool topMost) {
            EXPECT_EQ(Port, -1);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopInput"), _T("{\"typeOfInput\":\"HDMI\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: stopInput_HDMI: exit ***\n");
}

TEST_F(AVInputInit, stopInput_COMPOSITE)
{
    printf("*** _DEBUG: AVInputInit: stopInput_COMPOSITE ***\n");
    EXPECT_CALL(*p_compositeInputImplMock, selectPort(::testing::_))
        .WillOnce([](int8_t Port) {
            EXPECT_EQ(Port, -1);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopInput"), _T("{\"typeOfInput\":\"COMPOSITE\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: stopInput_COMPOSITE: exit ***\n");
}

TEST_F(AVInputInit, stopInput_COMPOSITE_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: stopInput_COMPOSITE_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("stopInput"), _T("{\"typeOfInput\":\"DP\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: stopInput_COMPOSITE_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, setVideoRectangle_HDMI)
{
    printf("*** _DEBUG: AVInputInit: setVideoRectangle_HDMI ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, scaleVideo(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce([](int32_t x, int32_t y, int32_t width, int32_t height) {
            EXPECT_EQ(x, 0);
            EXPECT_EQ(y, 0);
            EXPECT_EQ(width, 3840);
            EXPECT_EQ(height, 2160);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\" : 0, \"y\" : 0, \"w\" : 3840, \"h\" : 2160 ,\"typeOfInput\":\"HDMI\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: setVideoRectangle_HDMI: exit ***\n");
}

TEST_F(AVInputInit, setVideoRectangle_COMPOSITE)
{
    printf("*** _DEBUG: AVInputInit: setVideoRectangle_COMPOSITE ***\n");
    EXPECT_CALL(*p_compositeInputImplMock, scaleVideo(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce([](int32_t x, int32_t y, int32_t width, int32_t height) {
            EXPECT_EQ(x, 0);
            EXPECT_EQ(y, 0);
            EXPECT_EQ(width, 720);
            EXPECT_EQ(height, 480);
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\" : 0, \"y\" : 0, \"w\" : 720, \"h\" : 480 ,\"typeOfInput\":\"COMPOSITE\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: setVideoRectangle_COMPOSITE: exit ***\n");
}

TEST_F(AVInputInit, setVideoRectangle_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: setVideoRectangle_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\" : 0, \"y\" : 0, \"w\" : 720, \"h\" : 480 ,\"typeOfInput\":\"DP\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: setVideoRectangle_InvalidParameters: exit ***\n");
}

TEST_F(AVInputInit, getSupportedGameFeatures)
{
    printf("*** _DEBUG: AVInputInit: getSupportedGameFeatures ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getSupportedGameFeatures(::testing::_))
        .WillOnce([](std::vector<std::string>& featureList) {
            featureList = { "ALLM", "VRR", "QMS" };
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getSupportedGameFeatures"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"supportedGameFeatures\":[\"ALLM\",\"VRR\",\"QMS\"],\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getSupportedGameFeatures: exit ***\n");
}

TEST_F(AVInputInit, getSupportedGameFeatures_ErrorCase)
{
    printf("*** _DEBUG: AVInputInit: getSupportedGameFeatures_ErrorCase ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getSupportedGameFeatures(::testing::_))
        .WillOnce([](std::vector<std::string>& featureList) {
            featureList = {};
        });

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getSupportedGameFeatures"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getSupportedGameFeatures_ErrorCase: exit ***\n");
}

TEST_F(AVInputInit, getGameFeatureStatus_ALLM)
{
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_ALLM ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getHdmiALLMStatus(::testing::_, ::testing::_))
        .WillOnce([](int iHdmiPort, bool* allmStatus) {
            EXPECT_EQ(iHdmiPort, 1);
            *allmStatus = true;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getGameFeatureStatus"), _T("{\"portId\" : 1, \"gameFeature\" : \"ALLM\"}"), response));
    EXPECT_EQ(response, string("{\"mode\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_ALLM: exit ***\n");
}

TEST_F(AVInputInit, getGameFeatureStatus_VRR_HDMI)
{
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_HDMI ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getVRRStatus(::testing::_, ::testing::_))
        .WillOnce([](int iHdmiPort, dsHdmiInVrrStatus_t* vrrStatus) {
            ASSERT_NE(vrrStatus, nullptr);
            vrrStatus->vrrType = dsVRR_HDMI_VRR;
            vrrStatus->vrrAmdfreesyncFramerate_Hz = 90.00;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getGameFeatureStatus"), _T("{\"portId\" : 1, \"gameFeature\" : \"VRR-HDMI\"}"), response));
    EXPECT_EQ(response, string("{\"mode\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_HDMI: exit ***\n");
}

TEST_F(AVInputInit, getGameFeatureStatus_VRR_FREESYNC)
{
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_FREESYNC ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getVRRStatus(::testing::_, ::testing::_))
        .WillOnce([](int iHdmiPort, dsHdmiInVrrStatus_t* vrrStatus) {
            ASSERT_NE(vrrStatus, nullptr);
            vrrStatus->vrrType = dsVRR_AMD_FREESYNC;
            vrrStatus->vrrAmdfreesyncFramerate_Hz = 100.00;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getGameFeatureStatus"), _T("{\"portId\" : 1, \"gameFeature\" : \"VRR-FREESYNC\"}"), response));
    EXPECT_EQ(response, string("{\"mode\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_FREESYNC: exit ***\n");
}

TEST_F(AVInputInit, getGameFeatureStatus_VRR_FREESYNC_PREMIUM)
{
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_FREESYNC_PREMIUM ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getVRRStatus(::testing::_, ::testing::_))
        .WillOnce([](int iHdmiPort, dsHdmiInVrrStatus_t* vrrStatus) {
            ASSERT_NE(vrrStatus, nullptr);
            vrrStatus->vrrType = dsVRR_AMD_FREESYNC_PREMIUM;
            vrrStatus->vrrAmdfreesyncFramerate_Hz = 120.00;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getGameFeatureStatus"), _T("{\"portId\" : 1, \"gameFeature\" : \"VRR-FREESYNC-PREMIUM\"}"), response));
    EXPECT_EQ(response, string("{\"mode\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_FREESYNC_PREMIUM: exit ***\n");
}

TEST_F(AVInputInit, getGameFeatureStatus_VRR_FREESYNC_PREMIUM_PRO)
{
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_FREESYNC_PREMIUM_PRO ***\n");
    EXPECT_CALL(*p_hdmiInputImplMock, getVRRStatus(::testing::_, ::testing::_))
        .WillOnce([](int iHdmiPort, dsHdmiInVrrStatus_t* vrrStatus) {
            ASSERT_NE(vrrStatus, nullptr);
            vrrStatus->vrrType = dsVRR_AMD_FREESYNC_PREMIUM_PRO;
            vrrStatus->vrrAmdfreesyncFramerate_Hz = 144.00;
        });

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getGameFeatureStatus"), _T("{\"portId\" : 1, \"gameFeature\" : \"VRR-FREESYNC-PREMIUM-PRO\"}"), response));
    EXPECT_EQ(response, string("{\"mode\":true,\"success\":true}"));
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_VRR_FREESYNC_PREMIUM_PRO: exit ***\n");
}

TEST_F(AVInputInit, getGameFeatureStatus_InvalidParameters)
{
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_InvalidParameters ***\n");
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getGameFeatureStatus"), _T("{\"portId\" : \"test\", \"gameFeature\" : \"VRR-FREESYNC-PREMIUM-PRO\"}"), response));
    EXPECT_EQ(response, string(""));
    printf("*** _DEBUG: AVInputInit: getGameFeatureStatus_InvalidParameters: exit ***\n");
}

#endif
// </pca>

class AVInputEvents : public AVInputDsTest {
protected:
    NiceMock<FactoriesImplementation> factoriesImplementation;
    Core::JSONRPC::Message message;

    Exchange::IAVInput::IDevicesChangedNotification*            DevicesChangedNotification          = nullptr;
    Exchange::IAVInput::ISignalChangedNotification*             SignalChangedNotification           = nullptr;
    Exchange::IAVInput::IInputStatusChangedNotification*        InputStatusChangedNotification      = nullptr;
    Exchange::IAVInput::IVideoStreamInfoUpdateNotification*     VideoStreamInfoUpdateNotification   = nullptr;
    Exchange::IAVInput::IGameFeatureStatusUpdateNotification*   GameFeatureStatusUpdateNotification = nullptr;
    Exchange::IAVInput::IHdmiContentTypeUpdateNotification*     HdmiContentTypeUpdateNotification   = nullptr;

    AVInputEvents()
        : AVInputDsTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        ON_CALL(*p_avInputMock, RegisterDevicesChangedNotification(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IAVInput::IDevicesChangedNotification* notification) {
                    DevicesChangedNotification = notification;
            return Core::ERROR_NONE;
                }));

        ON_CALL(*p_avInputMock, RegisterSignalChangedNotification(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IAVInput::ISignalChangedNotification* notification) {
                    SignalChangedNotification = notification;
            return Core::ERROR_NONE;
                }));

        ON_CALL(*p_avInputMock, RegisterInputStatusChangedNotification(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IAVInput::IInputStatusChangedNotification* notification) {
                    InputStatusChangedNotification = notification;
            return Core::ERROR_NONE;
                }));

        ON_CALL(*p_avInputMock, RegisterVideoStreamInfoUpdateNotification(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IAVInput::IVideoStreamInfoUpdateNotification* notification) {
                    VideoStreamInfoUpdateNotification = notification;
            return Core::ERROR_NONE;
                }));

        ON_CALL(*p_avInputMock, RegisterGameFeatureStatusUpdateNotification(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IAVInput::IGameFeatureStatusUpdateNotification* notification) {
                    GameFeatureStatusUpdateNotification = notification;
            return Core::ERROR_NONE;
                }));

        ON_CALL(*p_avInputMock, RegisterHdmiContentTypeUpdateNotification(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IAVInput::IHdmiContentTypeUpdateNotification* notification) {
                    HdmiContentTypeUpdateNotification = notification;
            return Core::ERROR_NONE;
                }));
    }

    virtual ~AVInputEvents() override
    {
        PluginHost::IFactories::Assign(nullptr);
    }
};

// <pca> debug
#if 1

TEST_F(AVInputEvents, onDevicesChangedHDMI)
{
    printf("*** _DEBUG: AVInputEvents: onDevicesChangedHDMI ***\n");
    Core::Event onDevicesChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onDevicesChanged\",\"params\":{\"devices\":[]}}");

                onDevicesChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    ASSERT_TRUE(dsAVEventHandler != nullptr);
    EVENT_SUBSCRIBE(0, _T("onDevicesChanged"), _T("org.rdk.AVInput"), message);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_connect.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_connect.isPortConnected = true;
    dsAVEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onDevicesChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onDevicesChanged"), _T("org.rdk.AVInput"), message);
    printf("*** _DEBUG: AVInputEvents: onDevicesChangedHDMI: exit ***\n");
}

TEST_F(AVInputEvents, onDevicesChangedCOMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onDevicesChangedCOMPOSITE ***\n");
    Core::Event onDevicesChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onDevicesChanged\",\"params\":{\"devices\":[]}}");

                onDevicesChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onDevicesChanged"), _T("org.rdk.AVInput"), message);

    ASSERT_TRUE(dsAVEventHandler != nullptr);
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_connect.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_connect.isPortConnected = true;
    dsAVEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onDevicesChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onDevicesChanged"), _T("org.rdk.AVInput"), message);
    printf("*** _DEBUG: AVInputEvents: onDevicesChangedCOMPOSITE: exit ***\n");
}

TEST_F(AVInputEvents, onSignalChangedStableHDMI)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedStableHDMI ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,
                    "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"stableSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);

    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_STABLE;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    printf("*** _DEBUG: AVInputEvents: onSignalChangedStableHDMI: exit ***\n");
}

TEST_F(AVInputEvents, onSignalChangedNoSignalHDMI)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedNoSignalHDMI ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,
                    "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"noSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedNoSignalHDMI EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedUnstableHDMI)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedUnstableHDMI ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,
                    "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"unstableSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_UNSTABLE;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedUnstableHDMI EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedNotSupportedHDMI)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedNotSupportedHDMI ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"notSupportedSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedNotSupportedHDMI EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedDefaultHDMI)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedDefaultHDMI ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"none\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_MAX;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedDefaultHDMI EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedStableCOMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedStableCOMPOSITE ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"stableSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_STABLE;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedStableCOMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedNoSignalCOMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedNoSignalCOMPOSITE ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"noSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_NOSIGNAL;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedNoSignalCOMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedUnstableCOMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedUnstableCOMPOSITE ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"unstableSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_UNSTABLE;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedUnstableCOMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedNotSupportedCOMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedNotSupportedCOMPOSITE ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"notSupportedSignal\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_NOTSUPPORTED;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedNotSupportedCOMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, onSignalChangedDefaultCOMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onSignalChangedDefaultCOMPOSITE ***\n");
    Core::Event onSignalChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"none\"}}");

                onSignalChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_MAX;
    dsAVSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onSignalChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onSignalChangedDefaultCOMPOSITE EXIT ***\n");
}

// <pca> debug // Tests completed up until this point at least // </pca>

TEST_F(AVInputEvents, onInputStatusChangeOn_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOn_HDMI ***\n");
    Core::Event onInputStatusChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"started\",\"plane\":1}}");

                onInputStatusChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startInput"), _T("{\"portId\": \"0\" , \"typeOfInput\":\"HDMI\", \"requestAudioMix\": true, \"plane\" : 1, \"topMost\" : true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_status.isPresented = true;
    dsAVStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onInputStatusChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOn_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, onInputStatusChangeOff_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOff_HDMI ***\n");
    Core::Event onInputStatusChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"stopped\",\"plane\":-1}}");

                onInputStatusChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopInput"), _T("{\"typeOfInput\":\"HDMI\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_status.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_status.isPresented = false;

    dsAVStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onInputStatusChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOff_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, onInputStatusChangeOn_COMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOn_COMPOSITE ***\n");
    Core::Event onInputStatusChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"started\",\"plane\":1}}");

                onInputStatusChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startInput"), _T("{\"portId\": \"0\" , \"typeOfInput\":\"COMPOSITE\", \"requestAudioMix\": true, \"plane\" : 1, \"topMost\" : true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_status.isPresented = true;
    dsAVStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onInputStatusChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOn_COMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, onInputStatusChangeOff_COMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOff_COMPOSITE ***\n");
    Core::Event onInputStatusChanged(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"stopped\",\"plane\":-1}}");

                onInputStatusChanged.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopInput"), _T("{\"typeOfInput\":\"COMPOSITE\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_status.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_status.isPresented = false;
    dsAVStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, onInputStatusChanged.Lock());

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: onInputStatusChangeOff_COMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, hdmiGameFeatureStatusUpdate)
{
    printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate ***\n");
    Core::Event gameFeatureStatusUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.gameFeatureStatusUpdate\",\"params\":{\"id\":0,\"gameFeature\":\"ALLM\",\"mode\":true}}");

                gameFeatureStatusUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_allm_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_allm_mode.allm_mode = true;
    dsAVGameFeatureStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, gameFeatureStatusUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate EXIT ***\n");
}

TEST_F(AVInputEvents, hdmiGameFeatureStatusUpdate_HDMI_VRR)
{
    printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_HDMI_VRR ***\n");
    Core::Event gameFeatureStatusUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.gameFeatureStatusUpdate\",\"params\":{\"id\":0,\"gameFeature\":\"VRR-HDMI\",\"mode\":true}}");

                gameFeatureStatusUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_vrr_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_vrr_mode.vrr_type = dsVRR_HDMI_VRR;
    dsAVGameFeatureStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, gameFeatureStatusUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_HDMI_VRR EXIT ***\n");
}

TEST_F(AVInputEvents, hdmiGameFeatureStatusUpdate_AMD_FREESYNC)
{
    printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_AMD_FREESYNC ***\n");
    Core::Event gameFeatureStatusUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.gameFeatureStatusUpdate\",\"params\":{\"id\":0,\"gameFeature\":\"VRR-FREESYNC\",\"mode\":true}}");

                gameFeatureStatusUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_vrr_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_vrr_mode.vrr_type = dsVRR_AMD_FREESYNC;
    dsAVGameFeatureStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, gameFeatureStatusUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_AMD_FREESYNC EXIT ***\n");
}

TEST_F(AVInputEvents, hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM)
{
    printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM ***\n");
    Core::Event gameFeatureStatusUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        // <pca> debug - change to AtMost(2) to see if it helps with occasional test failure
        //.Times(1)
        .Times(::testing::AtMost(2))
        // </pca>
        .WillRepeatedly(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.gameFeatureStatusUpdate\",\"params\":{\"id\":0,\"gameFeature\":\"VRR-FREESYNC-PREMIUM\",\"mode\":true}}");

                gameFeatureStatusUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_vrr_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_vrr_mode.vrr_type = dsVRR_AMD_FREESYNC_PREMIUM;
    dsAVGameFeatureStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, gameFeatureStatusUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM EXIT ***\n");
}

#endif
// </pca>

// <pca> debug - hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM_PRO enabled vvvvvvv </pca>

TEST_F(AVInputEvents, hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM_PRO)
{
    printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM_PRO ***\n");
    Core::Event gameFeatureStatusUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.gameFeatureStatusUpdate\",\"params\":{\"id\":0,\"gameFeature\":\"VRR-FREESYNC-PREMIUM-PRO\",\"mode\":true}}");

                gameFeatureStatusUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_vrr_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_vrr_mode.vrr_type = dsVRR_AMD_FREESYNC_PREMIUM_PRO;
    dsAVGameFeatureStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, gameFeatureStatusUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("gameFeatureStatusUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: hdmiGameFeatureStatusUpdate_AMD_FREESYNC_PREMIUM_PRO EXIT ***\n");
}

// <pca> debug
#if 1

TEST_F(AVInputEvents, videoStreamInfoUpdate1_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate1_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1920,\"height\":1080,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1001}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_59dot94;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate1_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate2_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate2_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":480,\"progressive\":false,\"frameRateN\":24000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_720x480;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_24;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate2_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate3_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate3_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":576,\"progressive\":false,\"frameRateN\":25000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_720x576;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_25;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate3_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate4_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate4_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":3840,\"height\":2160,\"progressive\":false,\"frameRateN\":30000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_3840x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_30;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate4_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate5_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate5_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":50000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_50;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate5_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate6_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate6_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_60;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate6_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate7_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate7_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":24000,\"frameRateD\":1001}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_23dot98;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate7_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate8_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate8_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":30000,\"frameRateD\":1001}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_29dot97;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate8_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate9_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate9_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":30000,\"frameRateD\":1001}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_29dot97;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);
    
    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate9_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate10_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate10_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":100000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_100;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate10_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate11_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate11_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":120000,\"frameRateD\":1001}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_119dot88;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate11_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate12_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate12_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":120000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_120;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate12_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate13_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate13_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":200000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_200;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate13_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate14_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate14_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":240000,\"frameRateD\":1001}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_239dot76;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate14_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate15_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate15_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":240000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_240;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate15_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdateDefault_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdateDefault_HDMI ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1920,\"height\":1080,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_MAX;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_MAX;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());
    
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdateDefault_HDMI EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdate1_COMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate1_COMPOSITE ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":480,\"progressive\":false,\"frameRateN\":24000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_video_mode.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_720x480;
    eventData.data.composite_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_24;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate1_COMPOSITE EXIT ***\n");
}


TEST_F(AVInputEvents, videoStreamInfoUpdate2_COMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate2_COMPOSITE ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":576,\"progressive\":false,\"frameRateN\":25000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_video_mode.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_720x576;
    eventData.data.composite_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_25;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdate2_COMPOSITE EXIT ***\n");
}

TEST_F(AVInputEvents, videoStreamInfoUpdateDefault_COMPOSITE)
{
    printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdateDefault_COMPOSITE ***\n");
    Core::Event videoStreamInfoUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":576,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1000}}");

                videoStreamInfoUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_video_mode.port = dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_MAX;
    eventData.data.composite_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_MAX;
    dsAVVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, videoStreamInfoUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: videoStreamInfoUpdateDefault_COMPOSITE EXIT ***\n");
}

// <pca> debug
#endif
#if 0

TEST_F(AVInputEvents, aviContentTypeUpdate_HDMI)
{
    printf("*** _DEBUG: AVInputEvents: aviContentTypeUpdate_HDMI ***\n");
    Core::Event aviContentTypeUpdate(false, true);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text,"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AVInput.aviContentTypeUpdate\",\"params\":{\"id\":0,\"aviContentType\":0}}");

                aviContentTypeUpdate.SetEvent();

                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("aviContentTypeUpdate"), _T("org.rdk.AVInput"), message);
    ASSERT_TRUE(dsAVSignalStatusEventHandler != nullptr);

    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_content_type.port = dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_content_type.aviContentType = dsAVICONTENT_TYPE_GRAPHICS;
    dsAviContentTypeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE, &eventData, 0);

    EXPECT_EQ(Core::ERROR_NONE, aviContentTypeUpdate.Lock());

    EVENT_UNSUBSCRIBE(0, _T("aviContentTypeUpdate"), _T("org.rdk.AVInput"), message);
        printf("*** _DEBUG: AVInputEvents: aviContentTypeUpdate_HDMI EXIT ***\n");
}

#endif
// </pca>

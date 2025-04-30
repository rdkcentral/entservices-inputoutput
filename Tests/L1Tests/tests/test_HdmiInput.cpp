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

#include "HdmiInput.h"

#include "FactoriesImplementation.h"

#include "HdmiInputMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"

#include "dsMgr.h"
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class HdmiInputTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::HdmiInput> plugin;
    Core::JSONRPC::Handler& handler;
    Core::JSONRPC::Handler& handlerV2;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    HdmiInputTest()
        : plugin(Core::ProxyType<Plugin::HdmiInput>::Create())
        , handler(*(plugin))
        , handlerV2(*(plugin->GetHandler(2)))
        , INIT_CONX(1, 0)
    {
    }
    virtual ~HdmiInputTest() = default;
};

class HdmiInputDsTest : public HdmiInputTest {
protected:
    HdmiInputImplMock   *p_hdmiInputImplMock = nullptr ;

    HdmiInputDsTest()
        : HdmiInputTest()
    {
        p_hdmiInputImplMock  = new NiceMock <HdmiInputImplMock>;
        device::HdmiInput::setImpl(p_hdmiInputImplMock);
    }
    virtual ~HdmiInputDsTest() override
    {
        device::HdmiInput::setImpl(nullptr);
        if (p_hdmiInputImplMock != nullptr)
        {
            delete p_hdmiInputImplMock;
            p_hdmiInputImplMock = nullptr;
        }
    }
};

class HdmiInputInitializedTest : public HdmiInputTest {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    IARM_EventHandler_t dsHdmiEventHandler;
    IARM_EventHandler_t dsHdmiStatusEventHandler;
    IARM_EventHandler_t dsHdmiSignalStatusEventHandler;
    IARM_EventHandler_t dsHdmiVideoModeEventHandler;
    IARM_EventHandler_t dsHdmiGameFeatureStatusEventHandler;

    // NiceMock<ServiceMock> service;
    ServiceMock service;

    HdmiInputInitializedTest()
        : HdmiInputTest()
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        EXPECT_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke(
                [&](const uint32_t, const string& name) -> void* {
                    return nullptr;
                }));
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiEventHandler = handler;
                    }
                   if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiStatusEventHandler = handler;
                   }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiSignalStatusEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiVideoModeEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiGameFeatureStatusEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));

        EXPECT_EQ(string(""), plugin->Initialize(&service));
    }
    virtual ~HdmiInputInitializedTest() override
    {
        plugin->Deinitialize(&service);

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
    }
};


class HdmiInputInitializedEventTest : public HdmiInputInitializedTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::JSONRPC::Message message;

    HdmiInputInitializedEventTest()
        : HdmiInputInitializedTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
            plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);
    }

    virtual ~HdmiInputInitializedEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);
    }
};

class HdmiInputInitializedEventDsTest : public HdmiInputInitializedEventTest {
protected:
    HdmiInputImplMock   *p_hdmiInputImplMock = nullptr ;

    HdmiInputInitializedEventDsTest()
        : HdmiInputInitializedEventTest()
    {
        p_hdmiInputImplMock  = new NiceMock <HdmiInputImplMock>;
        device::HdmiInput::setImpl(p_hdmiInputImplMock);
    }

    virtual ~HdmiInputInitializedEventDsTest() override
    {
        device::HdmiInput::setImpl(nullptr);
        if (p_hdmiInputImplMock != nullptr)
        {
            delete p_hdmiInputImplMock;
            p_hdmiInputImplMock = nullptr;
        }
    }
};

TEST_F(HdmiInputTest, RegisteredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getHDMIInputDevices")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("writeEDID")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("readEDID")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getRawHDMISPD")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getHDMISPD")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("setEdidVersion")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getEdidVersion")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("startHdmiInput")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("stopHdmiInput")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVideoRectangle")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getSupportedGameFeatures")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getHdmiGameFeatureStatus")));

    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getHDMIInputDevices")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("writeEDID")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("readEDID")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("startHdmiInput")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("stopHdmiInput")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("setVideoRectangle")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getSupportedGameFeatures")));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getHdmiGameFeatureStatus")));
}

TEST_F(HdmiInputDsTest, getHDMIInputDevices)
{

    ON_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*p_hdmiInputImplMock, isPortConnected(::testing::_))
        .WillByDefault(::testing::Return(true));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getHDMIInputDevices"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"devices\":[{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"connected\":\"true\"}],\"success\":true}"));
}


TEST_F(HdmiInputDsTest, writeEDIDEmpty)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("writeEDID"), _T("{\"message\": \"message\"}"), response));
    EXPECT_EQ(response, string(""));
}


TEST_F(HdmiInputDsTest, writeEDID)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("writeEDID"), _T("{\"deviceId\": 0, \"message\": \"message\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiInputDsTest, writeEDIDInvalid)
{
      ON_CALL(*p_hdmiInputImplMock, getEDIDBytesInfo(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, std::vector<uint8_t> &edidVec2) {
                edidVec2 = std::vector<uint8_t>({ 't', 'e', 's', 't' });
            }));
   EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("readEDID"), _T("{\"deviceId\": \"b\"}"), response));
   EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, readEDID)
{
    ON_CALL(*p_hdmiInputImplMock, getEDIDBytesInfo(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, std::vector<uint8_t> &edidVec2) {
                edidVec2 = std::vector<uint8_t>({ 't', 'e', 's', 't' });
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("readEDID"), _T("{\"deviceId\": 0}"), response));
    EXPECT_EQ(response, string("{\"EDID\":\"dGVzdA==\",\"success\":true}"));
}

TEST_F(HdmiInputDsTest, getRawHDMISPD)
{
    ON_CALL(*p_hdmiInputImplMock, getHDMISPDInfo(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, std::vector<uint8_t>& edidVec2) {
                edidVec2 = { 't', 'e', 's', 't' };
            }));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("getRawHDMISPD"), _T("{\"portId\":0}"), response));
    EXPECT_EQ(response, string("{\"HDMISPD\":\"dGVzdA\",\"success\":true}"));
}
TEST_F(HdmiInputDsTest, getRawHDMISPDInvalid)
{
    ON_CALL(*p_hdmiInputImplMock, getHDMISPDInfo(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, std::vector<uint8_t>& edidVec2) {
                edidVec2 = { 't', 'e', 's', 't' };
            }));
    EXPECT_EQ(Core::ERROR_GENERAL, handlerV2.Invoke(connection, _T("getRawHDMISPD"), _T("{\"portId\":\"b\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, getHDMISPD)
{
    ON_CALL(*p_hdmiInputImplMock, getHDMISPDInfo(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, std::vector<uint8_t>& edidVec2) {
                edidVec2 = {'0','1','2','n', 'p', '1','2','3','4','5','6','7',0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',0,'q','r'};
            }));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("getHDMISPD"), _T("{\"portId\":0}"), response));
    EXPECT_EQ(response, string("{\"HDMISPD\":\"Packet Type:30,Version:49,Length:50,vendor name:1234567,product des:abcdefghijklmno,source info:71\",\"success\":true}"));
}
TEST_F(HdmiInputDsTest, getHDMISPDInvalid)
{
    ON_CALL(*p_hdmiInputImplMock, getHDMISPDInfo(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, std::vector<uint8_t>& edidVec2) {
                edidVec2 = {'0','1','2','n', 'p', '0'};
            }));
    EXPECT_EQ(Core::ERROR_GENERAL, handlerV2.Invoke(connection, _T("getHDMISPD"), _T("{\"portId\":\"b\"}"), response));
    EXPECT_EQ(response, string(""));
}


TEST_F(HdmiInputDsTest, setEdidVersionInvalid)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handlerV2.Invoke(connection, _T("setEdidVersion"), _T("{\"portId\": \"b\", \"edidVersion\":\"HDMI1.4\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, setEdidVersion14)
{
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("setEdidVersion"), _T("{\"portId\": \"0\", \"edidVersion\":\"HDMI1.4\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiInputDsTest, setEdidVersion20)
{
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("setEdidVersion"), _T("{\"portId\": \"0\", \"edidVersion\":\"HDMI2.0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}
TEST_F(HdmiInputDsTest, setEdidVersionEmpty)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handlerV2.Invoke(connection, _T("setEdidVersion"), _T("{\"portId\": \"0\", \"edidVersion\":\"\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, getEdidVersionInvalid)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handlerV2.Invoke(connection, _T("getEdidVersion"), _T("{\"portId\": \"b\", \"edidVersion\":\"HDMI1.4\"}"), response));
    EXPECT_EQ(response, string(""));
}
TEST_F(HdmiInputDsTest, getEdidVersionVer14)
{
    ON_CALL(*p_hdmiInputImplMock, getEdidVersion(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iPort, int *edidVersion) {
                *edidVersion = 0;
            }));
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("getEdidVersion"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"edidVersion\":\"HDMI1.4\",\"success\":true}"));
}
TEST_F(HdmiInputDsTest, getEdidVersionVer20)
{
    ON_CALL(*p_hdmiInputImplMock, getEdidVersion(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iPort, int *edidVersion) {
                *edidVersion = 1;
            })); 
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("getEdidVersion"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"edidVersion\":\"HDMI2.0\",\"success\":true}"));
}

TEST_F(HdmiInputDsTest, startHdmiInputInvalid)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("startHdmiInput"), _T("{\"portId\": \"b\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, startHdmiInput)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startHdmiInput"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}


TEST_F(HdmiInputDsTest, stopHdmiInput)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopHdmiInput"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiInputDsTest, setVideoRectangleInvalid)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\": \"b\",\"y\": 0,\"w\": 1920,\"h\": 1080}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, setVideoRectangle)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\": 0,\"y\": 0,\"w\": 1920,\"h\": 1080}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}


TEST_F(HdmiInputDsTest, getSupportedGameFeatures)
{
    ON_CALL(*p_hdmiInputImplMock, getSupportedGameFeatures(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](std::vector<std::string> &supportedFeatures) {
                supportedFeatures = {"ALLM"};
            })); 
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getSupportedGameFeatures"), _T("{\"supportedGameFeatures\": \"ALLM\"}"), response));
    EXPECT_EQ(response, string("{\"supportedGameFeatures\":[\"ALLM\"],\"success\":true}"));
}


TEST_F(HdmiInputDsTest, getHdmiGameFeatureStatusInvalidPort)
{
    ON_CALL(*p_hdmiInputImplMock, getHdmiALLMStatus(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, bool *allm) {
                *allm = true;
            }));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getHdmiGameFeatureStatus"), _T("{\"portId\": \"b\",\"gameFeature\": \"ALLM\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputDsTest, getHdmiGameFeatureStatus)
{
    ON_CALL(*p_hdmiInputImplMock, getHdmiALLMStatus(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, bool *allm) {
                *allm = true;
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getHdmiGameFeatureStatus"), _T("{\"portId\": \"0\",\"gameFeature\": \"ALLM\"}"), response));
    EXPECT_EQ(response, string("{\"mode\":true,\"success\":true}"));
}
TEST_F(HdmiInputDsTest, getHdmiGameFeatureStatusInvalidFeature)
{
    ON_CALL(*p_hdmiInputImplMock, getHdmiALLMStatus(::testing::_,::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int iport, bool *allm) {
                *allm = true;
            }));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getHdmiGameFeatureStatus"), _T("{\"portId\": \"0\",\"gameFeature\": \"Invalid\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(HdmiInputInitializedEventDsTest, onDevicesChanged)
{
   ASSERT_TRUE(dsHdmiEventHandler != nullptr);
    ON_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*p_hdmiInputImplMock, isPortConnected(::testing::_))
        .WillByDefault(::testing::Return(true));

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onDevicesChanged.onDevicesChanged\",\"params\":{\"devices\":[{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"connected\":\"true\"}]}}")));

                return Core::ERROR_NONE;
            }));


    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_connect.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_connect.isPortConnected = true;

    EVENT_SUBSCRIBE(0, _T("onDevicesChanged"), _T("client.events.onDevicesChanged"), message);

    dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onDevicesChanged"), _T("client.events.onDevicesChanged"), message);
}

TEST_F(HdmiInputInitializedEventDsTest, onInputStatusChangeOn)
{
   ASSERT_TRUE(dsHdmiStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onInputStatusChanged.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"started\",\"plane\":0}}")));
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startHdmiInput"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_status.isPresented = true;

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);

    dsHdmiStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, onInputStatusChangeOff)
{
   ASSERT_TRUE(dsHdmiStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onInputStatusChanged.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"stopped\",\"plane\":-1}}")));
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopHdmiInput"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_status.isPresented = false;

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);

    dsHdmiStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, onSignalChangedStable)
{
   ASSERT_TRUE(dsHdmiSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"stableSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_STABLE;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsHdmiSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, onSignalChangedNoSignal)
{
   ASSERT_TRUE(dsHdmiSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"noSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsHdmiSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, onSignalChangedUnstable)
{
   ASSERT_TRUE(dsHdmiSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"unstableSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_UNSTABLE;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsHdmiSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, onSignalChangedNotSupported)
{
   ASSERT_TRUE(dsHdmiSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"notSupportedSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsHdmiSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

}
TEST_F(HdmiInputInitializedEventDsTest, onSignalChangedDefault)
{
   ASSERT_TRUE(dsHdmiSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"none\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_sig_status.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_sig_status.status = dsHDMI_IN_SIGNAL_STATUS_MAX;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsHdmiSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}

TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate1)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1920,\"height\":1080,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1001}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_59dot94;

    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);

    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate2)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":480,\"progressive\":false,\"frameRateN\":24000,\"frameRateD\":1000}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_720x480;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_24;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);

    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate3)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":720,\"height\":576,\"progressive\":false,\"frameRateN\":25000,\"frameRateD\":1000}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_720x576;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_25;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);

}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate4)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":3840,\"height\":2160,\"progressive\":false,\"frameRateN\":30000,\"frameRateD\":1000}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_3840x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_30;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);

}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate5)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":50000,\"frameRateD\":1000}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_50;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate6)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1000}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_60;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate7)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":24000,\"frameRateD\":1001}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_23dot98;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate8)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":4096,\"height\":2160,\"progressive\":false,\"frameRateN\":30000,\"frameRateD\":1001}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_4096x2160;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_29dot97;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdate9)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1280,\"height\":720,\"progressive\":false,\"frameRateN\":30000,\"frameRateD\":1001}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_1280x720;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate = dsVIDEO_FRAMERATE_29dot97;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, videoStreamInfoUpdateDefault)
{
   ASSERT_TRUE(dsHdmiVideoModeEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.videoStreamInfoUpdate.videoStreamInfoUpdate\",\"params\":{\"id\":0,\"locator\":\"hdmiin:\\/\\/localhost\\/deviceid\\/0\",\"width\":1920,\"height\":1080,\"progressive\":false,\"frameRateN\":60000,\"frameRateD\":1000}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_video_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_video_mode.resolution.pixelResolution = dsVIDEO_PIXELRES_MAX;
    eventData.data.hdmi_in_video_mode.resolution.interlaced = true;
    eventData.data.hdmi_in_video_mode.resolution.frameRate= dsVIDEO_FRAMERATE_MAX;
    EVENT_SUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
    dsHdmiVideoModeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, &eventData , 0);
    EVENT_UNSUBSCRIBE(0, _T("videoStreamInfoUpdate"), _T("client.events.videoStreamInfoUpdate"), message);
}
TEST_F(HdmiInputInitializedEventDsTest, hdmiGameFeatureStatusUpdate)
{
   ASSERT_TRUE(dsHdmiGameFeatureStatusEventHandler != nullptr);

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.hdmiGameFeatureStatusUpdate.hdmiGameFeatureStatusUpdate\",\"params\":{\"id\":0,\"gameFeature\":\"ALLM\",\"mode\":true}}")));

                return Core::ERROR_NONE;
            }));


    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_allm_mode.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_allm_mode.allm_mode = true;
    EVENT_SUBSCRIBE(0, _T("hdmiGameFeatureStatusUpdate"), _T("client.events.hdmiGameFeatureStatusUpdate"), message);

    dsHdmiGameFeatureStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("hdmiGameFeatureStatusUpdate"), _T("client.events.hdmiGameFeatureStatusUpdate"), message);
}

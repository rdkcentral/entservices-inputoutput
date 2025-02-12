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
#include "CompositeInput.h"
#include "FactoriesImplementation.h"
#include "CompositeInputMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "ThunderPortability.h"

using namespace WPEFramework;
using ::testing::NiceMock;
using ::testing::Eq;

class CompositeInputTest : public ::testing::Test {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    Core::ProxyType<Plugin::CompositeInput> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    CompositeInputTest()
        : plugin(Core::ProxyType<Plugin::CompositeInput>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
    {
    }
    virtual ~CompositeInputTest()
    {
    }
};

class CompositeInputDsTest : public CompositeInputTest {
protected:
    CompositeInputImplMock   *p_compositeInputImplMock = nullptr ;

    CompositeInputDsTest()
        : CompositeInputTest()
    {
        p_compositeInputImplMock  = new NiceMock <CompositeInputImplMock>;
        device::CompositeInput::setImpl(p_compositeInputImplMock);
    }
    virtual ~CompositeInputDsTest() override
    {
        device::CompositeInput::setImpl(nullptr);
        if (p_compositeInputImplMock != nullptr)
        {
            delete p_compositeInputImplMock;
            p_compositeInputImplMock = nullptr;
        }
    }
};

class CompositeInputInitializedTest : public CompositeInputTest {
protected:
    NiceMock<IarmBusImplMock> iarmBusImplMock;
    IARM_EventHandler_t dsCompositeEventHandler;
    IARM_EventHandler_t dsCompositeStatusEventHandler;
    IARM_EventHandler_t dsCompositeSignalStatusEventHandler;

 CompositeInputInitializedTest()
        : CompositeInputTest()
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsCompositeEventHandler = handler;
                    }
                   if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsCompositeStatusEventHandler = handler;
                   }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsCompositeSignalStatusEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));

        EXPECT_EQ(string(""), plugin->Initialize(nullptr));
    }
    virtual ~CompositeInputInitializedTest() override
    {
        plugin->Deinitialize(nullptr);

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
    }
};


class CompositeInputInitializedEventTest : public CompositeInputInitializedTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::JSONRPC::Message message;

    CompositeInputInitializedEventTest()
        : CompositeInputInitializedTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
            plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));

        dispatcher->Activate(&service);
    }

    virtual ~CompositeInputInitializedEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);
    }
};

class CompositeInputInitializedEventDsTest : public CompositeInputInitializedEventTest {
protected:
    CompositeInputImplMock   *p_compositeInputImplMock = nullptr ;

    CompositeInputInitializedEventDsTest()
        : CompositeInputInitializedEventTest()
    {
        p_compositeInputImplMock  = new NiceMock <CompositeInputImplMock>;
        device::CompositeInput::setImpl(p_compositeInputImplMock);
    }

    virtual ~CompositeInputInitializedEventDsTest() override
    {
        device::CompositeInput::setImpl(nullptr);
        if (p_compositeInputImplMock != nullptr)
        {
            delete p_compositeInputImplMock;
            p_compositeInputImplMock = nullptr;
        }
    }
};

TEST_F(CompositeInputTest, RegisteredMethods)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getCompositeInputDevices")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("startCompositeInput")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("stopCompositeInput")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVideoRectangle")));
}

TEST_F(CompositeInputDsTest, getCompositeInputDevices)
{
    ON_CALL(*p_compositeInputImplMock, getNumberOfInputs())
            .WillByDefault(::testing::Return(1));
    ON_CALL(*p_compositeInputImplMock, isPortConnected(::testing::_))
            .WillByDefault(::testing::Return(true));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getCompositeInputDevices"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"devices\":[{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"connected\":\"true\"}],\"success\":true}"));
}

TEST_F(CompositeInputDsTest, startCompositeInputInvalid)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("startCompositeInput"), _T("{\"portId\": \"b\"}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(CompositeInputDsTest, startCompositeInput)
{
    EXPECT_CALL(*p_compositeInputImplMock, selectPort(::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
        [](int8_t Port) {

        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startCompositeInput"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(CompositeInputDsTest, setVideoRectangleInvalid)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\": \"b\",\"y\": 0,\"w\": 1920,\"h\": 1080}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(CompositeInputDsTest, setVideoRectangle)
{
     EXPECT_CALL(*p_compositeInputImplMock, scaleVideo(::testing::_,::testing::_,::testing::_,::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
            [](int32_t x, int32_t y, int32_t width, int32_t height) {
            
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVideoRectangle"), _T("{\"x\": 0,\"y\": 0,\"w\": 1920,\"h\": 1080}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(CompositeInputInitializedEventDsTest, onDevicesChanged)
{
   ASSERT_TRUE(dsCompositeEventHandler != nullptr);
    ON_CALL(*p_compositeInputImplMock, getNumberOfInputs())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*p_compositeInputImplMock, isPortConnected(::testing::_))
        .WillByDefault(::testing::Return(true));

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onDevicesChanged.onDevicesChanged\",\"params\":{\"devices\":[{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"connected\":\"true\"}]}}")));

                return Core::ERROR_NONE;
            }));


    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_connect.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_connect.isPortConnected = true;

    EVENT_SUBSCRIBE(0, _T("onDevicesChanged"), _T("client.events.onDevicesChanged"), message);

    dsCompositeEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onDevicesChanged"), _T("client.events.onDevicesChanged"), message);
}

TEST_F(CompositeInputInitializedEventDsTest, onInputStatusChangeOn)
{
   ASSERT_TRUE(dsCompositeStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onInputStatusChanged.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"started\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_status.isPresented = true;

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);

    dsCompositeStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS , &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);
}
TEST_F(CompositeInputInitializedEventDsTest, onInputStatusChangeOff)
{
   ASSERT_TRUE(dsCompositeStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onInputStatusChanged.onInputStatusChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"status\":\"stopped\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_status.isPresented = false;

    EVENT_SUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);

    dsCompositeStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onInputStatusChanged"), _T("client.events.onInputStatusChanged"), message);
}
TEST_F(CompositeInputInitializedEventDsTest, onSignalChangedStable)
{
   ASSERT_TRUE(dsCompositeSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"stableSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_STABLE;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsCompositeSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS , &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(CompositeInputInitializedEventDsTest, onSignalChangedNoSignal)
{
   ASSERT_TRUE(dsCompositeSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"noSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_NOSIGNAL;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsCompositeSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(CompositeInputInitializedEventDsTest, onSignalChangedUnstable)
{
   ASSERT_TRUE(dsCompositeSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"unstableSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_UNSTABLE;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsCompositeSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(CompositeInputInitializedEventDsTest, onSignalChangedNotSupported)
{
   ASSERT_TRUE(dsCompositeSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"notSupportedSignal\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_NOTSUPPORTED;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsCompositeSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}
TEST_F(CompositeInputInitializedEventDsTest, onSignalChangedDefault)
{
   ASSERT_TRUE(dsCompositeSignalStatusEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{\"jsonrpc\":\"2.0\",\"method\":\"client.events.onSignalChanged.onSignalChanged\",\"params\":{\"id\":0,\"locator\":\"cvbsin:\\/\\/localhost\\/deviceid\\/0\",\"signalStatus\":\"none\"}}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.composite_in_sig_status.port =dsCOMPOSITE_IN_PORT_0;
    eventData.data.composite_in_sig_status.status = dsCOMP_IN_SIGNAL_STATUS_MAX;

    EVENT_SUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);

    dsCompositeSignalStatusEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS, &eventData , 0);

    EVENT_UNSUBSCRIBE(0, _T("onSignalChanged"), _T("client.events.onSignalChanged"), message);
}

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

using namespace WPEFramework;

using ::testing::NiceMock;

class AVInputTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::AVInput> plugin;
    Core::ProxyType<Plugin::AVInputImplementation> AVInputImpl;

    NiceMock<ServiceMock> service;
    NiceMock<COMLinkMock> comLinkMock;

    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    AVInputMock* p_avInputMock = nullptr;

    IarmBusImplMock* p_iarmBusImplMock = nullptr;

    AVInputTest()
        : plugin(Core::ProxyType<Plugin::AVInput>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
    {
        p_avInputMock  = new NiceMock<AVInputMock>;

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

        plugin->Initialize(&service);

    }
    virtual ~AVInputTest()
    {
        plugin->Deinitialize(&service);

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr) {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
    }
};

TEST_F(AVInputTest, RegisteredMethods)
{
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
}

TEST_F(AVInputTest, contentProtected)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("contentProtected"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"isContentProtected\":true,\"success\":true}"));
}

class AVInputDsTest : public AVInputTest {
protected:
    HdmiInputImplMock* p_hdmiInputImplMock = nullptr;
    CompositeInputImplMock* p_compositeInputImplMock = nullptr;
    HostImplMock* p_HostImplMock = nullptr;
    IARM_EventHandler_t dsAVGameFeatureStatusEventHandler;
    IARM_EventHandler_t dsAVEventHandler;
    IARM_EventHandler_t dsAVSignalStatusEventHandler;
    IARM_EventHandler_t dsAVStatusEventHandler;
    IARM_EventHandler_t dsAVVideoModeEventHandler;
    IARM_EventHandler_t dsAviContentTypeEventHandler;

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

TEST_F(AVInputDsTest, numberOfInputs)
{
    ON_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("numberOfInputs"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"numberOfInputs\":1,\"success\":true}"));
}

TEST_F(AVInputDsTest, currentVideoMode)
{
    ON_CALL(*p_hdmiInputImplMock, getCurrentVideoMode())
        .WillByDefault(::testing::Return(string("unknown")));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("currentVideoMode"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"currentVideoMode\":\"unknown\",\"success\":true}"));
}

TEST_F(AVInputDsTest, getEdid2AllmSupport)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEdid2AllmSupport"), _T("{\"portId\": \"0\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"allmSupport\":true,\"success\":true}"));
}

TEST_F(AVInputDsTest, getEdid2AllmSupport_ErrorCase)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getEdid2AllmSupport"), _T("{\"portId\": \"test\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(AVInputDsTest, setEdid2AllmSupport)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEdid2AllmSupport"), _T("{\"portId\": \"0\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(AVInputDsTest, setEdid2AllmSupport_ErrorCase)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setEdid2AllmSupport"), _T("{\"portId\": \"test\",\"allmSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(AVInputDsTest, getVRRSupport)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVRRSupport"), _T("{\"portId\": \"0\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"vrrSupport\":true,\"success\":true}"));
}

TEST_F(AVInputDsTest, getVRRSupport_ErrorCase)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getVRRSupport"), _T("{\"portId\": \"test\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(AVInputDsTest, setVRRSupport)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setVRRSupport"), _T("{\"portId\": \"0\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(AVInputDsTest, setVRRSupport_ErrorCase)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setVRRSupport"), _T("{\"portId\": \"test\",\"vrrSupport\":true}"), response));
    EXPECT_EQ(response, string(""));
}

TEST_F(AVInputDsTest, getVRRFrameRate)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getVRRFrameRate"), _T("{\"portId\": \"0\"}"), response));
    EXPECT_EQ(response, string("{\"currentVRRVideoFrameRate\":0,\"success\":true}"));
}

TEST_F(AVInputDsTest, getVRRFrameRate_ErrorCase)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getVRRFrameRate"), _T("{\"portId\": \"test\"}"), response));
    EXPECT_EQ(response, string(""));
}



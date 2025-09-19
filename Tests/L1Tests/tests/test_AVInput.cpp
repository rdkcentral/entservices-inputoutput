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

    NiceMock<COMLinkMock> comLinkMock;

    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    AVInputMock* p_avInputMock = nullptr;

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

        plugin->Initialize(&service);

    }
    virtual ~AVInputTest()
    {
        plugin->Deinitialize(&service);
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


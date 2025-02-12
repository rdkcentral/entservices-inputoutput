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

#include "Implementation/DeviceInfo.h"

#include "IarmBusMock.h"

#include <fstream>
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class DeviceInfoTest : public ::testing::Test {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    Core::ProxyType<Plugin::DeviceInfoImplementation> deviceInfoImplementation;
    Exchange::IDeviceInfo* interface;

    DeviceInfoTest()
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        deviceInfoImplementation = Core::ProxyType<Plugin::DeviceInfoImplementation>::Create();

        interface = static_cast<Exchange::IDeviceInfo*>(
            deviceInfoImplementation->QueryInterface(Exchange::IDeviceInfo::ID));
    }
    virtual ~DeviceInfoTest()
    {
        interface->Release();
        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
    }

    virtual void SetUp()
    {
        ASSERT_TRUE(interface != nullptr);
    }

    virtual void TearDown()
    {
        ASSERT_TRUE(interface != nullptr);
    }
};

TEST_F(DeviceInfoTest, Make)
{
    std::ofstream file("/etc/device.properties");
    file << "MFG_NAME=CUSTOM4";
    file.close();

    string make;
    EXPECT_EQ(Core::ERROR_NONE, interface->Make(make));
    EXPECT_EQ(make, _T("CUSTOM4"));
}

TEST_F(DeviceInfoTest, Model)
{
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=\"CUSTOM4 CUSTOM9\"";
    file.close();

    string model;
    EXPECT_EQ(Core::ERROR_NONE, interface->Model(model));
    EXPECT_EQ(model, _T("CUSTOM4 CUSTOM9"));
}

TEST_F(DeviceInfoTest, DeviceType)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=IpStb";
    file.close();

    string deviceType;
    EXPECT_EQ(Core::ERROR_NONE, interface->DeviceType(deviceType));
    EXPECT_EQ(deviceType, _T("IpStb"));
}

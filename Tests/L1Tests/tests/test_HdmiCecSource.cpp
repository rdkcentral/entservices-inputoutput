
#include <gtest/gtest.h>
#include "HdmiCecSourceImplementation.h"
#include "mocks.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;
using namespace ::testing;

// Helper matchers
MATCHER_P(LA_EQ, expected, "") { return arg.toInt() == expected; }
MATCHER_P(UCP_CMD, expected, "") { return arg.uiCommand.toInt() == expected; }

class HdmiCecSourceTest : public ::testing::Test {
protected:
    HdmiCecSourceImplementation* plugin;
    Connection* p_connectionImplMock;
    MessageEncoder* p_messageEncoderMock;
    HdmiCecSourceProcessor* m_processor;
    // Mock service and other necessary objects
    // For simplicity, we assume these are initialized in a real test setup
    Core::JSONRPC::Handler& handler;
    Core::JSONRPC::Connection connection;
    string response;
    string message;

    HdmiCecSourceTest()
        : handler(*(new Core::JSONRPC::Handler(
              {
                  _T("SetEnabled"),
                  _T("GetEnabled"),
                  _T("SetOTPEnabled"),
                  _T("GetOTPEnabled"),
                  _T("SetOSDName"),
                  _T("GetOSDName"),
                  _T("SetVendorId"),
                  _T("GetVendorId"),
                  _T("PerformOTPAction"),
                  _T("SendStandbyMessage"),
                  _T("getActiveSourceStatus"),
                  _T("SendKeyPressEvent"),
                  _T("GetDeviceList")
              }
          )))
    {
        // Initialization of mocks and plugin instance would go here
        // plugin = new HdmiCecSourceImplementation();
        // p_connectionImplMock = new MockConnection();
        // p_messageEncoderMock = new MockMessageEncoder();
        // plugin->smConnection = p_connectionImplMock;
        // m_processor = new HdmiCecSourceProcessor(*p_connectionImplMock);
        // HdmiCecSourceImplementation::_instance = plugin;
    }

    virtual ~HdmiCecSourceTest() {
        // delete plugin;
        // delete p_connectionImplMock;
        // delete p_messageEncoderMock;
        // delete m_processor;
        // delete &handler;
    }
};

// Mock Notification Sink for C++ notifications
class MockNotificationSink : public Exchange::IHdmiCecSource::INotification {
public:
    MOCK_METHOD(void, OnDeviceAdded, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnDeviceRemoved, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnDeviceInfoUpdated, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnActiveSourceStatusUpdated, (const bool isActiveSource), (override));
    MOCK_METHOD(void, StandbyMessageReceived, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnKeyPressEvent, (const int logicalAddress, const int keyCode), (override));
    MOCK_METHOD(void, OnKeyReleaseEvent, (const int logicalAddress), (override));

    // IUnknown dummy implementations
    void AddRef() const override {}
    uint32_t Release() const override { return 1; }
    Core::hresult QueryInterface(const uint32_t, void**) override { return Core::ERROR_UNAVAILABLE; }
};


/*******************************************************************************************************************
 ******************************************** Existence Tests ******************************************************
 *******************************************************************************************************************/

TEST_F(HdmiCecSourceTest, SetEnabledExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("SetEnabled")));
}

TEST_F(HdmiCecSourceTest, GetEnabledExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("GetEnabled")));
}

TEST_F(HdmiCecSourceTest, SetOTPEnabledExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("SetOTPEnabled")));
}

TEST_F(HdmiCecSourceTest, GetOTPEnabledExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("GetOTPEnabled")));
}

TEST_F(HdmiCecSourceTest, SetOSDNameExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("SetOSDName")));
}

TEST_F(HdmiCecSourceTest, GetOSDNameExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("GetOSDName")));
}

TEST_F(HdmiCecSourceTest, SetVendorIdExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("SetVendorId")));
}

TEST_F(HdmiCecSourceTest, GetVendorIdExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("GetVendorId")));
}

TEST_F(HdmiCecSourceTest, PerformOTPActionExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("PerformOTPAction")));
}

TEST_F(HdmiCecSourceTest, SendStandbyMessageExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("SendStandbyMessage")));
}

TEST_F(HdmiCecSourceTest, GetActiveSourceStatusExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getActiveSourceStatus")));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("SendKeyPressEvent")));
}

TEST_F(HdmiCecSourceTest, GetDeviceListExists) {
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("GetDeviceList")));
}

/*******************************************************************************************************************
 ******************************************** Invoke Tests *********************************************************
 *******************************************************************************************************************/

TEST_F(HdmiCecSourceTest, SetAndGetEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetEnabled"), _T("{"enabled":true}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("GetEnabled"), _T(""), response));
    EXPECT_EQ(response, _T("{"enabled":true,"success":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetEnabled"), _T("{"enabled":false}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("GetEnabled"), _T(""), response));
    EXPECT_EQ(response, _T("{"enabled":false,"success":true}"));
}

TEST_F(HdmiCecSourceTest, SetAndGetOTPEnabled)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetOTPEnabled"), _T("{"enabled":true}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("GetOTPEnabled"), _T(""), response));
    EXPECT_EQ(response, _T("{"enabled":true,"success":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetOTPEnabled"), _T("{"enabled":false}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("GetOTPEnabled"), _T(""), response));
    EXPECT_EQ(response, _T("{"enabled":false,"success":true}"));
}

TEST_F(HdmiCecSourceTest, SetAndGetOSDName)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetOSDName"), _T("{"name":"MyTestDevice"}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("GetOSDName"), _T(""), response));
    EXPECT_THAT(response, HasSubstr(_T(""name":"MyTestDevice"")));
}

TEST_F(HdmiCecSourceTest, setAndGetVendorId)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetVendorId"), R"({"vendorid":"0x0019FB"})", response));
    EXPECT_EQ(response, R"({"success":true})");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("GetVendorId"), "", response));
    EXPECT_THAT(response, HasSubstr(R"("vendorid":"0019fb")"));
    EXPECT_THAT(response, HasSubstr(R"("success":true)"));
}

TEST_F(HdmiCecSourceTest, SendStandbyMessage)
{
    plugin->cecEnableStatus = true;
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const Standby&>(_)))
        .WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::BROADCAST), _, _))
        .Times(1);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendStandbyMessage"), _T("{}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
}

TEST_F(HdmiCecSourceTest, getActiveSourceStatus)
{
    // Initially false
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, _T("{"isActiveSource":false,"success":true}"));

    // Set true via OTP action
    plugin->cecEnableStatus = true;
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SetOTPEnabled"), _T("{"enabled":true}"), response));
    EXPECT_CALL(*p_connectionImplMock, sendTo(_, _, _)).Times(AtLeast(3));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("PerformOTPAction"), _T(""), response));
    EXPECT_EQ(response, _T("{"success":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, _T("{"isActiveSource":true,"success":true}"));
}

/*******************************************************************************************************************
 **************************************** SendKeyPressEvent Tests **************************************************
 *******************************************************************************************************************/

TEST_F(HdmiCecSourceTest, SendKeyPressEventVolumeUp)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_VOLUME_UP)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":65}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventVolumeDown)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_VOLUME_DOWN)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":66}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventMute)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_MUTE)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":67}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventUp)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_UP)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":1}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventDown)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_DOWN)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":2}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventLeft)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_LEFT)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":3}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventRight)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_RIGHT)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":4}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventSelect)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_SELECT)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":0}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventHome)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_HOME)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":9}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventBack)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_BACK)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":13}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber0)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_0)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":32}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber1)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_1)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":33}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber2)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_2)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":34}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber3)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_3)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":35}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber4)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_4)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":36}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber5)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_5)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":37}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber6)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_6)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":38}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber7)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_7)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":39}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber8)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_8)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":40}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(HdmiCecSourceTest, SendKeyPressEventNumber9)
{
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlPressed&>(UCP_CMD(UICommand::UI_COMMAND_NUM_9)))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const UserControlReleased&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(0), _, _)).Times(2);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("SendKeyPressEvent"), _T("{"logicalAddress":0, "keyCode":41}"), response));
    EXPECT_EQ(response, _T("{"success":true}"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/*******************************************************************************************************************
 **************************************** CEC Message Processor Tests **********************************************
 *******************************************************************************************************************/

TEST_F(HdmiCecSourceTest, imageViewOnProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::UNREGISTERED);
    ImageViewOn msg;
    EXPECT_CALL(*plugin, addDevice(LogicalAddress::TV)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, requestActiveSourceProccess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    RequestActiveSource msg;
    // Set device as active source
    isDeviceActiveSource = true;
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const ActiveSource&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::BROADCAST), _, _)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, CecVersionProcess)
{
    Header header(LogicalAddress::PLAYBACK_1, LogicalAddress::UNREGISTERED);
    CECVersion msg(Version::V_1_4);
    EXPECT_CALL(*plugin, addDevice(LogicalAddress::PLAYBACK_1)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, giveOSDNameProcess)
{
    Header header(LogicalAddress::TV, logicalAddress);
    GiveOSDName msg;
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const SetOSDName&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::TV), _, _)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, givePhysicalAddressProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    GivePhysicalAddress msg;
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const ReportPhysicalAddress&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::BROADCAST), _, _)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, giveDeviceVendorIdProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    GiveDeviceVendorID msg;
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const DeviceVendorID&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::BROADCAST), _, _)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, routingChangeProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    RoutingChange msg(PhysicalAddress(0x1000), physical_addr);
    EXPECT_CALL(*plugin, sendActiveSourceEvent()).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, routingInformationProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    RoutingInformation msg(physical_addr);
    EXPECT_CALL(*plugin, sendActiveSourceEvent()).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, setStreamPathProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    SetStreamPath msg(physical_addr);
    EXPECT_CALL(*plugin, sendActiveSourceEvent()).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, reportPhysicalAddressProcess)
{
    Header header(LogicalAddress::PLAYBACK_1, LogicalAddress::BROADCAST);
    ReportPhysicalAddress msg(PhysicalAddress(0x2000), LogicalAddress::PLAYBACK_1);
    EXPECT_CALL(*plugin, addDevice(LogicalAddress::PLAYBACK_1)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, deviceVendorIDProcess)
{
    Header header(LogicalAddress::TV, LogicalAddress::BROADCAST);
    DeviceVendorID msg(VendorID(0xAA, 0xBB, 0xCC));
    plugin->deviceList[LogicalAddress::TV].m_deviceInfoStatus = 1; // Mark as present
    EXPECT_CALL(*plugin, sendDeviceUpdateInfo(LogicalAddress::TV)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, GiveDevicePowerStatusProcess)
{
    Header header(LogicalAddress::TV, logicalAddress);
    GiveDevicePowerStatus msg;
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const ReportPowerStatus&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::TV), _, _)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, reportPowerStatusProcess)
{
    Header header(LogicalAddress::TV, logicalAddress);
    ReportPowerStatus msg(PowerStatus::ON);
    EXPECT_CALL(*plugin, addDevice(LogicalAddress::TV)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, userControlPressedProcess)
{
    Header header(LogicalAddress::TV, logicalAddress);
    UserControlPressed msg(UICommand::UI_COMMAND_PLAY);
    EXPECT_CALL(*plugin, SendKeyPressMsgEvent(LogicalAddress::TV, UICommand::UI_COMMAND_PLAY)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, userControlReleasedProcess)
{
    Header header(LogicalAddress::TV, logicalAddress);
    UserControlReleased msg;
    EXPECT_CALL(*plugin, SendKeyReleaseMsgEvent(LogicalAddress::TV)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, abortProcess)
{
    Header header(LogicalAddress::TV, logicalAddress);
    Abort msg(OpCode::ABORT);
    EXPECT_CALL(*p_messageEncoderMock, encode(Matcher<const FeatureAbort&>(_))).WillOnce(Return(CECFrame()));
    EXPECT_CALL(*p_connectionImplMock, sendTo(LA_EQ(LogicalAddress::TV), _, _)).Times(1);
    m_processor->process(msg, header);
}

TEST_F(HdmiCecSourceTest, setOSDNameProcess)
{
    MockNotificationSink notificationSink;
    plugin->Register(&notificationSink);
    plugin->deviceList[LogicalAddress::TV].m_deviceInfoStatus = 1; // Mark as present

    Header header(LogicalAddress::TV, logicalAddress);
    SetOSDName msg(OSDName("NewTVName"));

    EXPECT_CALL(notificationSink, OnDeviceInfoUpdated(LogicalAddress::TV)).Times(1);
    m_processor->process(msg, header);

    plugin->Unregister(&notificationSink);
}

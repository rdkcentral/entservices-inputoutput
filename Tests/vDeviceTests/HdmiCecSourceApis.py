send_standby_message = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.sendStandbyMessage"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


get_device_list = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.getDeviceList"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


get_enabled = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.getEnabled"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


get_osd_name = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.getOSDName"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


get_otp_enabled = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.getOTPEnabled"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


get_vendor_id = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.getVendorId"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)



perform_otp_action = (
    'curl --max-time 8 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,"method":"org.rdk.HdmiCecSource.performOTPAction"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


send_key_press_event = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.sendKeyPressEvent",'
    '"params":{"logicalAddress":0,"keyCode":65}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


set_enabled_false = (
    'curl --max-time 8 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.setEnabled",'
    '"params":{"enabled":false}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


set_enabled_true = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.setEnabled",'
    '"params":{"enabled":true}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)

set_osd_name = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.setOSDName",'
    '"params":{"name":"Sky TV"}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


set_otp_enabled_true = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.setOTPEnabled",'
    '"params":{"enabled":true}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


set_vendor_id = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.setVendorId",'
    '"params":{"vendorid":"0x0019FB"}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)


set_otp_enabled_false = (
    'curl --max-time 5 '
    '--header "Content-Type: application/json" '
    '--request POST '
    '-d \'{"jsonrpc":"2.0","id":42,'
    '"method":"org.rdk.HdmiCecSource.setOTPEnabled",'
    '"params":{"enabled":false}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)

get_led_state = (
    'curl --max-time 5 '
    '--header "Content-Type: text/plain;" '
    '--request POST '
    '--data-binary \'{"jsonrpc": 2.0, "id": 0, "method": "org.rdk.LEDControl.getLEDState"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)

get_supported_led_states = (
    'curl --max-time 5 '
    '--header "Content-Type: text/plain;" '
    '--request POST '
    '--data-binary \'{"jsonrpc": 2.0, "id": 1, "method": "org.rdk.LEDControl.getSupportedLEDStates"}\' '
    'http://127.0.0.1:9998/jsonrpc'
)

set_led_state = (
    'curl --max-time 5 '
    '--header "Content-Type: text/plain;" '
    '--request POST '
    '--data-binary \'{"jsonrpc": 2.0, "id": 2, "method": "org.rdk.LEDControl.setLEDState", "params": {"state": "FACTORY_RESET"}}\' '
    'http://127.0.0.1:9998/jsonrpc'
)

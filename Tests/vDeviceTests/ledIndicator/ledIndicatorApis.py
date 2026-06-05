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


def make_set_led_state(state_name):
    '''Return a curl command string to set the LED state to the given state name.
    Valid state_name values: ACTIVE, STANDBY, WPS_CONNECTING, WPS_CONNECTED,
    WPS_ERROR, FACTORY_RESET, USB_UPGRADE, DOWNLOAD_ERROR
    '''
    return (
        'curl --max-time 5 '
        '--header "Content-Type: text/plain;" '
        '--request POST '
        f'--data-binary \'{{"jsonrpc": 2.0, "id": 2, "method": "org.rdk.LEDControl.setLEDState", "params": {{"state": "{state_name}"}}}}\' '
        'http://127.0.0.1:9998/jsonrpc'
    )


# Convenience pre-built commands for each mappable state
set_led_state_active        = make_set_led_state("ACTIVE")
set_led_state_standby       = make_set_led_state("STANDBY")
set_led_state_wps_connecting = make_set_led_state("WPS_CONNECTING")
set_led_state_wps_connected  = make_set_led_state("WPS_CONNECTED")
set_led_state_wps_error      = make_set_led_state("WPS_ERROR")
set_led_state_factory_reset  = make_set_led_state("FACTORY_RESET")
set_led_state_usb_upgrade    = make_set_led_state("USB_UPGRADE")
set_led_state_download_error = make_set_led_state("DOWNLOAD_ERROR")

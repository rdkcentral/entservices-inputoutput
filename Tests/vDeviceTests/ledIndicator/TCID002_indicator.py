import json
from utils import (
    send_curl_command,
    log_info,
    log_success,
    log_error,
    log_warning
)
import ledIndicatorApis


def run_test():
    expected_output_response = {
    "jsonrpc": 2.0,
    "id": 1,
    "result": {
        "supportedLEDStates": [
            "['ACTIVE', 'STANDBY', 'WPS_CONNECTING', 'WPS_CONNECTED', 'WPS_ERROR', 'FACTORY_RESET', 'USB_UPGRADE', 'DOWNLOAD_ERROR']"
        ],
        "success": True
    }
}

    log_info("Executing the curl command get supported let states - Returns the list of LED states that are actually supported by the platform at runtime. Possible values include NONE, ACTIVE, STANDBY, WPS_CONNECTING, WPS_CONNECTED, WPS_ERROR, FACTORY_RESET, USB_UPGRADE and DOWNLOAD_ERROR")

    curl_response = send_curl_command(
        ledIndicatorApis.get_supported_led_states
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        if json.loads(curl_response) == expected_output_response:
            log_success("TCID002 Passed ✅")
            return True
        else:
            log_error("TCID002 Failed ❌")
            return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID002 Failed ❌")
        return False

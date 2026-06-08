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
    valid_states = {
        "NONE",
        "ACTIVE",
        "STANDBY",
        "WPS_CONNECTING",
        "WPS_CONNECTED",
        "WPS_ERROR",
        "FACTORY_RESET",
        "USB_UPGRADE",
        "DOWNLOAD_ERROR",
    }


    log_info("Executing the curl command get led states - Retrieves current state of the LED. e.g. {“state”:”WPS_CONNECTING”}")

    curl_response = send_curl_command(
        ledIndicatorApis.get_led_state
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        response_json = json.loads(curl_response)
        state = response_json.get("result", {}).get("state")
        if isinstance(state, str) and state in valid_states:
            log_success("TCID001 Passed ✅")
            return True

        log_error(f"Unexpected LED state: {state}")
        log_error("TCID001 Failed ❌")
        return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID001 Failed ❌")
        return False

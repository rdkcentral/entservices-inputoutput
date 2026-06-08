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
    required_states = {
        "ACTIVE",
        "STANDBY",
        "WPS_CONNECTING",
        "WPS_CONNECTED",
        "WPS_ERROR",
        "FACTORY_RESET",
        "USB_UPGRADE",
        "DOWNLOAD_ERROR",
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
        response_json = json.loads(curl_response)
        result = response_json.get("result", {})
        supported_states = result.get("supportedLEDStates")
        success = result.get("success")

        if isinstance(supported_states, list) and success is True:
            states_set = set(supported_states)
            missing_states = required_states - states_set
            if not missing_states:
                log_success("TCID002 Passed ✅")
                return True
            log_error(f"Missing expected states: {sorted(missing_states)}")
        else:
            log_error("Invalid getSupportedLEDStates response shape")

        log_error("TCID002 Failed ❌")
        return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID002 Failed ❌")
        return False

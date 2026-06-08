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
    log_info("Executing the curl command get supported let states - Returns the list of LED states that are actually supported by the platform at runtime. Possible values include NONE, ACTIVE, STANDBY, WPS_CONNECTING, WPS_CONNECTED, WPS_ERROR, FACTORY_RESET, USB_UPGRADE and DOWNLOAD_ERROR")

    curl_response = send_curl_command(
        ledIndicatorApis.set_led_state
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        response_json = json.loads(curl_response)
        result = response_json.get("result")
        if result is True:
            log_success("TCID003 Passed ✅")
            return True

        log_error(f"Unexpected setLEDState result: {result}")
        log_error("TCID003 Failed ❌")
        return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID003 Failed ❌")
        return False

import json, time
from utils import (
    send_curl_command,
    log_info,
    log_success,
    log_error,
    log_warning
)
import HdmiCecSourceApis


def run_test():
    log_info("Executing the curl command perform OTP Action")

    time.sleep(3)
    curl_response = send_curl_command(
        HdmiCecSourceApis.perform_otp_action
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        devices_response = send_curl_command(HdmiCecSourceApis.get_device_list)
        device_count = -1
        if devices_response:
            try:
                dbody = json.loads(devices_response)
                device_count = dbody.get("result", {}).get("numberofdevices", -1)
            except json.JSONDecodeError:
                device_count = -1

        body = json.loads(curl_response)
        success = body.get("result", {}).get("success") is True
        expected_runtime_error = (
            body.get("error", {}).get("message") == "ERROR_GENERAL"
            and device_count == 0
        )

        if success or expected_runtime_error:
            log_success("TCID006 Passed ✅")
            return True

        log_error("TCID006 Failed ❌")
        return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID006 Failed ❌")
        return False

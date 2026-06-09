import json
from utils import (
    send_curl_command,
    log_info,
    log_success,
    log_error,
    log_warning
)
import HdmiCecSourceApis


def run_test():
    # Deterministic precondition for validation.
    send_curl_command(HdmiCecSourceApis.set_otp_enabled_true)

    log_info("Executing the curl command get OTP enabled")

    curl_response = send_curl_command(
        HdmiCecSourceApis.get_otp_enabled
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        body = json.loads(curl_response)
        result = body.get("result", {})
        if result.get("success") is True and result.get("enabled") is True:
            log_success("TCID004 Passed ✅")
            return True

        log_error("TCID004 Failed ❌")
        return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID004 Failed ❌")
        return False

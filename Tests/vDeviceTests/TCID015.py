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
    expected_output_response = {
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "name": "Sky TV",
        "success": True
        }
    }

    log_info("Executing the curl command get OSD Name")

    curl_response = send_curl_command(
        HdmiCecSourceApis.get_osd_name
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        if json.loads(curl_response) == expected_output_response:
            log_success("TCID015 Passed ✅")
            return True
        else:
            log_error("TCID015 Failed ❌")
            return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID015 Failed ❌")
        return False

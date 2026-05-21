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
            "numberofdevices": 5,
            "deviceList": [
                {
                    "logicalAddress": 1,
                    "vendorID": "0099",
                    "osdName": "CECAdapter"
                },
                {
                    "logicalAddress": 4,
                    "vendorID": "0e036",
                    "osdName": "PIONEER"
                },
                {
                    "logicalAddress": 5,
                    "vendorID": "0a0de",
                    "osdName": "YAMAHA"
                },
                {
                    "logicalAddress": 6,
                    "vendorID": "01a11",
                    "osdName": "GOOGLE"
                },
                {
                    "logicalAddress": 7,
                    "vendorID": "00a1",
                    "osdName": "IPSTB"
                },
                {
                    "logicalAddress": 8,
                    "vendorID": "000",
                    "osdName": "Kishore"
                }
            ],
            "success": True
        }
    }
    log_info("Executing the curl command get device list")

    curl_response = send_curl_command(
        HdmiCecSourceApis.get_device_list
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        actual_output_response = json.loads(curl_response)
        expected_normalized = json.dumps(expected_output_response, sort_keys=True)
        actual_normalized = json.dumps(actual_output_response, sort_keys=True)

        if actual_normalized == expected_normalized:
            log_success("TCID002 Passed ✅")
            return True
        else:
            log_warning(
                f"Expected: {json.dumps(expected_output_response, indent=2, sort_keys=True)}"
            )
            log_warning(
                f"Actual  : {json.dumps(actual_output_response, indent=2, sort_keys=True)}"
            )
            log_error("TCID002 Failed ❌")
            return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID002 Failed ❌")
        return False

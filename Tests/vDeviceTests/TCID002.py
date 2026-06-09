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
        result = actual_output_response.get("result", {})
        has_success = result.get("success") is True
        has_count = isinstance(result.get("numberofdevices"), int)

        count = result.get("numberofdevices")
        device_list = result.get("deviceList")
        if count == 0:
            list_consistent = (device_list is None) or (
                isinstance(device_list, list) and len(device_list) == 0
            )
        else:
            # Some targets report count excluding placeholder/NA devices, so
            # the list length can be greater than numberofdevices.
            list_consistent = isinstance(device_list, list) and len(device_list) >= count

        entries_valid = True
        if isinstance(device_list, list):
            for dev in device_list:
                if not isinstance(dev, dict):
                    entries_valid = False
                    break
                if not isinstance(dev.get("logicalAddress"), int):
                    entries_valid = False
                    break

        if has_success and has_count and list_consistent and entries_valid:
            log_success("TCID002 Passed ✅")
            return True

        log_warning(
            f"Actual  : {json.dumps(actual_output_response, indent=2, sort_keys=True)}"
        )
        log_error("TCID002 Failed ❌")
        return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID002 Failed ❌")
        return False

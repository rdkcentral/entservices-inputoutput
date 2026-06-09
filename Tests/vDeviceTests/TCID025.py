import json
import time
from utils import (
    send_curl_command,
    send_vcomponent_command,
    HDMICEC_CMD_BASE,
    log_info,
    log_success,
    log_error,
    log_warning,
)
import HdmiCecSourceApis


def _post_hdmicec(yaml_file):
    http_code, body = send_vcomponent_command(f"{HDMICEC_CMD_BASE}/{yaml_file}")
    log_info(f"  vComponent POST {yaml_file}: HTTP {http_code}  {body}")
    return http_code == 200


def run_test():
    # Legacy intent: sending events simulation.
    before = send_curl_command(HdmiCecSourceApis.get_device_list)

    ok1 = _post_hdmicec("hdmicec_device_config_add_network.yaml")
    time.sleep(1)
    ok2 = _post_hdmicec("hdmicec_device_status.yaml")
    time.sleep(1)

    if not (ok1 and ok2):
        log_error("✖ required vComponent emulation posts failed")
        return False

    response = send_curl_command(HdmiCecSourceApis.get_device_list)
    if not response:
        log_error("✖ getDeviceList curl command not sent")
        return False

    log_warning(f"Response: {response}")
    try:
        before_count = None
        if before:
            bbody = json.loads(before)
            before_count = bbody.get("result", {}).get("numberofdevices")

        body = json.loads(response)
        result = body.get("result", {})
        after_count = result.get("numberofdevices")
        count_valid = isinstance(after_count, int)
        if isinstance(before_count, int):
            count_valid = count_valid and after_count >= before_count

        if "error" not in body and result.get("success") is True and count_valid:
            log_success("TCID025 Passed")
            return True
    except json.JSONDecodeError:
        pass

    log_error("TCID025 Failed")
    return False

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
    # Legacy intent: abort/feature-abort process emulation.
    ok1 = _post_hdmicec("hdmicec_device_cec_message_userdef.yaml")
    time.sleep(1)
    ok2 = _post_hdmicec("hdmicec_device_bus_status.yaml")
    time.sleep(1)

    if not (ok1 and ok2):
        log_error("✖ required vComponent emulation posts failed")
        return False

    response = send_curl_command(HdmiCecSourceApis.send_standby_message)
    if not response:
        log_error("✖ standby curl command not sent")
        return False

    log_warning(f"Response: {response}")
    try:
        body = json.loads(response)
        ok = isinstance(body.get("result"), dict) and body["result"].get("success") is True
        if ok:
            log_success("TCID024 Passed")
            return True
    except json.JSONDecodeError:
        pass

    log_error("TCID024 Failed")
    return False

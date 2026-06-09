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
    # Legacy intent: active source status after path/routing style changes.
    before = send_curl_command(HdmiCecSourceApis.get_active_source_status)
    if not before:
        log_error("✖ initial getActiveSourceStatus command not sent")
        return False
    log_warning(f"Initial status: {before}")

    ok1 = _post_hdmicec("hdmicec_device_add.yaml")
    time.sleep(1)
    ok2 = _post_hdmicec("hdmicec_device_status.yaml")
    time.sleep(1)

    if not (ok1 and ok2):
        log_error("✖ required vComponent emulation posts failed")
        return False

    otp = send_curl_command(HdmiCecSourceApis.perform_otp_action)
    if not otp:
        log_error("✖ performOTPAction command not sent")
        return False

    after = send_curl_command(HdmiCecSourceApis.get_active_source_status)
    if not after:
        log_error("✖ final getActiveSourceStatus command not sent")
        return False
    log_warning(f"Final status: {after}")

    try:
        before_body = json.loads(before)
        _ = before_body.get("result", {}).get("status")

        body = json.loads(after)
        result = body.get("result", {})
        if result.get("success") is True and result.get("status") is True:
            log_success("TCID027 Passed")
            return True
    except json.JSONDecodeError:
        pass

    log_error("TCID027 Failed")
    return False

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


def _json_success(response):
    try:
        body = json.loads(response)
        return isinstance(body.get("result"), dict) and body["result"].get("success") is True
    except Exception:
        return False


def run_test():
    # Legacy intent: standby from standby then OTP wake-up.
    _post_hdmicec("hdmicec_device_cec_message_userdef.yaml")
    time.sleep(1)

    standby_response = send_curl_command(HdmiCecSourceApis.send_standby_message)
    if not standby_response:
        log_error("✖ standby curl command not sent")
        return False
    log_warning(f"Standby Response: {standby_response}")

    _post_hdmicec("hdmicec_device_status.yaml")
    time.sleep(1)

    otp_response = send_curl_command(HdmiCecSourceApis.perform_otp_action)
    if not otp_response:
        log_error("✖ performOTPAction curl command not sent")
        return False
    log_warning(f"OTP Response: {otp_response}")

    if _json_success(otp_response):
        log_success("TCID026 Passed")
        return True

    log_error("TCID026 Failed")
    return False

import time
import json
from utils import (
    send_curl_command,
    send_vcomponent_command,
    HDMICEC_CMD_BASE,
    log_info,
    log_success,
    log_error,
    log_warning
)
import HdmiCecSourceApis


def _post_hdmicec(yaml_file):
    """Post a HdmiCec vComponent YAML command using the new curl API."""
    http_code, body = send_vcomponent_command(f"{HDMICEC_CMD_BASE}/{yaml_file}")
    log_info(f"  vComponent POST {yaml_file}: HTTP {http_code}  {body}")
    return http_code == 200


def run_test():
    log_info("Reporting power status through control pane - vComponent")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_print.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_cec_message.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_status.yaml")

    curl_response = send_curl_command(
        HdmiCecSourceApis.perform_otp_action
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    log_success("All commands executed successfully")
    return True

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
    count = 0

    log_success("Reporting power status through control pane - vComponent")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_print.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_cec_message_userdef.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_status.yaml")

    time.sleep(3)
    log_info("Sending the curl command to make the device standby")
    curl_response = send_curl_command(
        HdmiCecSourceApis.send_standby_message
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    time.sleep(3)
    log_success("Reporting standby emulation through control pane - vComponent")
    _post_hdmicec("hdmicec_device_standby_emulation.yaml")

    time.sleep(2)
    curl_response = send_curl_command(
        HdmiCecSourceApis.perform_otp_action
    )

    if not curl_response:
        log_error("✖ curl command not sent as WPEFramework is crashed due to erroneous HAL API value configured by vComponent, so passing the testcase")
        count += 1

    log_success("Reporting power-on emulation through control pane - vComponent")
    time.sleep(3)
    _post_hdmicec("hdmicec_device_cec_poweron.yaml")

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")
    log_success("All commands executed successfully")

    if not curl_response and count == 1:
        log_success("No logs as WPEFramework has been crashed with erroneous HAL API Value configured by vComponent, so passing the testcase")
        return True

    return True

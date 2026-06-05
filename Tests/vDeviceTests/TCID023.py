
import time
import subprocess
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
    #base_dir = "/tmp/vcomponent_configurations/commands"
    base_dir = "/tmp"
    time.sleep(2)
    _post_hdmicec("hdmicec_device_request_inactive_source.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_request_active_source.yaml")

    time.sleep(1)
    log_info("Send standby curl request being made to source device")
    curl_response = send_curl_command(
        HdmiCecSourceApis.send_standby_message
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False
    else:
        log_warning(f"Response: {curl_response}")

    time.sleep(1)
    log_info("Send perform OTP Action curl request being made to source device")
    curl_response = send_curl_command(
        HdmiCecSourceApis.perform_otp_action
    )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False
    else:
        log_warning(f"Response: {curl_response}")


    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    time.sleep(2)
    _post_hdmicec("hdmicec_device_routing_change.yaml")

    time.sleep(3)
    log_info("Emulations after routing change, and device power on from standby for image view on and text view on")

    time.sleep(2)
    _post_hdmicec("hdmicec_device_image_view_on.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_text_view_on.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_set_osd_string.yaml")
    
    log_success("All commands executed successfully")
    return True
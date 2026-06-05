
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
    _post_hdmicec("hdmicec_device_get_menu_language.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_set_menu_language.yaml")
    time.sleep(2)
    _post_hdmicec("hdmicec_device_get_cec_version.yaml")

    for i in range(2):
        time.sleep(1)
        curl_response = send_curl_command(
            HdmiCecSourceApis.get_device_list
        )

        if not curl_response:
            log_error("✖ curl command not sent")
            return False
        else:
            log_warning(f"Response: {curl_response}")


    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    time.sleep(2)
    _post_hdmicec("hdmicec_device_remove.yaml")

    for i in range(2):
        time.sleep(1)
        curl_response = send_curl_command(
                HdmiCecSourceApis.get_device_list
            )

        if not curl_response:
            log_error("✖ curl command not sent")
            return False
        else:
            log_warning(f"Response: {curl_response}")

    
    log_success("All commands executed successfully")
    return True
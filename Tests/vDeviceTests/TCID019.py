
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
    #base_dir = "/tmp/vcomponent_configurations/commands"
    log_success("Negative scenario - Making the setEnabled driver status as FALSE")
    curl_response = send_curl_command(
            HdmiCecSourceApis.set_enabled_false
        )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False
    else:
        log_warning(f"Response: {curl_response}")

    time.sleep(2)
    log_success("Negative scenario - verifying the driver status with getEnabled")
    curl_response = send_curl_command(
            HdmiCecSourceApis.get_enabled
        )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False
    else:
        log_warning(f"Response: {curl_response}")

    log_error("Overriding the HAL API HdmICecOpen return value as negative")
    time.sleep(3)
    _post_hdmicec("hdmicec_setapi_open_fail.yaml")
    time.sleep(2)
    try:
        log_success("Negative scenario - making the driver status as TRUE using setEnabled")
        curl_response = send_curl_command(
            HdmiCecSourceApis.set_enabled_true
        )

        if not curl_response:
            log_error("✖ curl command not sent")
            return False
        else:
            log_warning(f"Response: {curl_response}")
    except:
        log_warning("WPEFramework crashed , check Thunder logs for futher details")
    
    log_error("Overriding the HAL API HdmICecOpen return value as POSITIVE as post condition")
    time.sleep(3)
    _post_hdmicec("hdmicec_setapi_open_pass.yaml")
    return True


   

import time
import subprocess
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
    #base_dir = "/tmp/vcomponent_configurations/commands"
    log_success("Negative scenario - calling getEnabled with driver status TRUE")
    curl_response = send_curl_command(
            HdmiCecSourceApis.get_enabled
        )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False
    else:
        log_warning(f"Response: {curl_response}")

    time.sleep(2)
    log_success("Negative scenario - calling getDeviceList")
    curl_response = send_curl_command(
            HdmiCecSourceApis.get_device_list
        )

    if not curl_response:
        log_error("✖ curl command not sent")
        return False
    else:
        log_warning(f"Response: {curl_response}")

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_setapi_open_fail.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_setapi_logical_fail.yaml 8080",

    ]

    base_dir = "/tmp"


    for command in commands:
        log_error("Overriding the HAL API HdmICecOpen return value as negative")
        time.sleep(3)
        log_info(f"Running command: {command} in directory: {base_dir}")
        result = subprocess.run(
            command,
            shell=True,
            check=False,
            text=True,
            cwd=base_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        log_info(f"Command output: {result.stdout.strip()}")
        log_info(f"Command error (if any): {result.stderr.strip()}")
        log_success(result.stdout.strip())

    
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
        log_success("Negative scenario - calling getDeviceList After setting the HdmiCecLogical address and HdmiCecOpen HAL APIs return value error")
        time.sleep(2)
        curl_response = send_curl_command(
                HdmiCecSourceApis.get_device_list
            )

        if not curl_response:
            log_error("✖ curl command not sent")
            return False
        else:
            log_warning(f"Response: {curl_response}")
    except:
        log_warning("Driver FAILED") 
    return True


   
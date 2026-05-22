import subprocess
import json
import time
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
    base_dir = "/tmp"


    commands = [
        "./hdmicec_post_command.sh hdmicec_device_print.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_print.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_cec_message.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_status.yaml 8080",
       
    ]

    for command in commands:
        log_info(f"Executing (cwd={base_dir}): {command}")
        try:
            result = subprocess.run(
                command,
                shell=True,
                check=True,
                capture_output=True,
                text=True,
                cwd=base_dir
            )
            log_success(result.stdout.strip())

        except subprocess.CalledProcessError as e:
            log_error(e.stderr.strip())
            return False
        
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
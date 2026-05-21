
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
    base_dir = "/tmp"


    base_dir = "/tmp"

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_get_menu_language.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_set_menu_language.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_get_cec_version.yaml 8080",
    ]

    for command in commands:
        log_info("querying the get menu language and set menu language emulation throughvComponent")
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

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_remove.yaml 8080",
    ]

    for command in commands:
        log_info("Removing a new Device from the CEC Network")
        time.sleep(2)
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

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
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_request_inactive_source.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_request_active_source.yaml 8080",
    ]

    for command in commands:
        log_info("inactive and active source emulation throughvComponent")
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

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_routing_change.yaml 8080",
    ]

    for command in commands:
        log_info("Routing change emulation through Vcomponent")
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

    time.sleep(3)
    log_info("Emulations after routing change, and device power on from standby for image view on and text view on")

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_image_view_on.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_text_view_on.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_set_osd_string.yaml 8080",

    ]

    for command in commands:
        log_info("inactive and active source emulation throughvComponent")
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
    
    log_success("All commands executed successfully")
    return True
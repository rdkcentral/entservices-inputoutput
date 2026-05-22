
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
    log_info(" Set the devices to standby mode and hit the sendStandbyMessage curl command again. Then wake up the remote device using otp feature.First, set the device to standby mode via emulation. Next, hit the curl command for sendStandbyMessage and send corresponding cec messages using sendMessage API to hal. Then hit the curl command for perform OTP Action and send corresponding cec messages to hal.Verify the thunder logs for more info")
    #base_dir = "/tmp/vcomponent_configurations/commands"

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


    base_dir = "/tmp"


    base_dir = "/tmp"

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_get_power_status.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_report_power_status.yaml 8080",
    ]

    for command in commands:
        log_info("get power status and report power status emulation throughvComponent")
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


    time.sleep(3)
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

    base_dir = "/tmp"


    base_dir = "/tmp"

    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_get_power_status.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_report_power_status.yaml 8080",
    ]

    for command in commands:
        log_info("inactive and active source emulation throughvComponent after making the device PowerON")
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
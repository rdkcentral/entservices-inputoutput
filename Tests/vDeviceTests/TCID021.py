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
    count = 0
    #base_dir = "/tmp/vcomponent_configurations/commands"
    base_dir = "/tmp"


    commands = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_print.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_cec_message_userdef.yaml 8080",
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_status.yaml 8080",
       
    ]


    log_success("Reporting power status through control pane - vComponent")
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

    commands_ = [
        "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_standby 8080",
           ]


    log_success("Reporting power status through control pane - vComponent")
    for command in commands_:
        time.sleep(3)
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

    time.sleep(2)
    curl_response = send_curl_command(
        HdmiCecSourceApis.perform_otp_action
    )

    if not curl_response:
        log_error("✖ curl command not sent as WPEFramework is crashed due to erroneous HAL API value configured by vComponent, so passing the testcase")
        count += 1
    commands_after_power_on = [
            "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/hdmicec_device_cec_poweron.yaml 8080",
        
        ]
    
    log_success("Reporting power status through control pane - vComponent")
    for command in commands_after_power_on:
        time.sleep(3)
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

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    log_success("All commands executed successfully")
    if not curl_response and count == 1: #here count is a flag to check the first scenario of wpeframework crash by negative scenario
        log_success("No logs as WPEFramework has been crashed with erroneous HAL API Value configured by vComponent , so passing the testcase")
        return True
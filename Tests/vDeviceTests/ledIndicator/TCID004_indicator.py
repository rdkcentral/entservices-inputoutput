import json
import subprocess
import time
from utils import (
    send_curl_command,
    log_info,
    log_success,
    log_error,
    log_warning
)
import ledIndicatorApis


def run_test():
    expected_output_response = {
    "jsonrpc": 2.0,
    "id": 2,
    "result": {
        "success": True
    }
}

    base_dir = "/tmp"

    log_info("Executing the curl command get supported let states - Returns the list of LED states that are actually supported by the platform at runtime. Possible values include NONE, ACTIVE, STANDBY, WPS_CONNECTING, WPS_CONNECTED, WPS_ERROR, FACTORY_RESET, USB_UPGRADE and DOWNLOAD_ERROR")

    commands = [
            "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/indicator_vcomponent_setstates_active.yaml 8080",
            "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/indicator_vcomponent_setstates_standby.yaml 8080",
            "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/indicator_vcomponent_setstates_usbcontrol.yaml 8080",
            "./hdmicec_post_command.sh /tmp/vcomponent_configurations/commands/indicator_vcomponent_setstates_wpsconnected.yaml 8080",
        ]

    for command in commands:
        log_info("setting LED states throughvComponent")
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

        curl_response = send_curl_command(
            ledIndicatorApis.set_led_state
        )

        if not curl_response:
            log_error("✖ curl command not sent")
            return False
        time.sleep(3)

    log_success("✔ curl command sent")
    log_warning(f"Response: {curl_response}")

    try:
        if json.loads(curl_response) == expected_output_response:
            log_success("TCID003 Passed ✅")
            return True
        else:
            log_error("TCID003 Failed ❌")
            return False
    except json.JSONDecodeError:
        log_error("Invalid JSON response")
        log_error("TCID003 Failed ❌")
        return False

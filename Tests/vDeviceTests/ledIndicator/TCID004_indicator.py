import json
import time
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from utils import (
    send_curl_command,
    send_vcomponent_command,
    INDICATOR_CMD_BASE,
    log_info,
    log_success,
    log_error,
    log_warning
)
import ledIndicatorApis


# Maps vComponent YAML state -> expected MW LEDControlState string returned by getLEDState
# (AIDL states IP_ACQUIRED, OFF, DEEP_SLEEP map to ACTIVE/STANDBY per implementation)
VCOMP_TO_MW_STATE = {
    "active":        "ACTIVE",
    "standby":       "STANDBY",
    "usb_upgrade":   "USB_UPGRADE",
    "wps_connected": "WPS_CONNECTED",
}


def _post_indicator_state(yaml_name):
    '''Post an indicator state YAML command via the new vComponent API.'''
    yaml_path = f"{INDICATOR_CMD_BASE}/{yaml_name}"
    log_info(f"  vComponent POST: {yaml_path}")
    http_code, body = send_vcomponent_command(yaml_path)
    log_info(f"  HTTP {http_code}  body: {body}")
    return 200 <= http_code < 300


def run_test():
    log_info("TCID004 - Set LED states via vComponent and verify via MW getLEDState")
    log_info("Scenario: Cycle through ACTIVE -> STANDBY -> USB_UPGRADE -> WPS_CONNECTED")

    # Each entry: (yaml_filename, expected_mw_state)
    steps = [
        ("indicator_set_state_active.yaml",       "ACTIVE"),
        ("indicator_set_state_standby.yaml",       "STANDBY"),
        ("indicator_set_state_usb_upgrade.yaml",   "USB_UPGRADE"),
        ("indicator_set_state_wps_connected.yaml", "WPS_CONNECTED"),
    ]

    overall_pass = True

    for yaml_file, expected_state in steps:
        log_info(f"\n-- Setting indicator to {expected_state} via vComponent --")

        if not _post_indicator_state(yaml_file):
            log_error(f"  vComponent POST failed for {yaml_file}")
            overall_pass = False
            continue

        log_success(f"  vComponent POST OK for {yaml_file}")
        time.sleep(2)

        curl_response = send_curl_command(ledIndicatorApis.get_led_state)
        if not curl_response or curl_response == "< No response from WPEFramework >":
            log_error("  getLEDState: no response from WPEFramework")
            overall_pass = False
            continue

        log_warning(f"  getLEDState response: {curl_response}")
        try:
            resp_json = json.loads(curl_response)
            actual_state = resp_json.get("result", {}).get("state", "")
            if actual_state == expected_state:
                log_success(f"  State verified: {actual_state} == {expected_state} ✔")
            else:
                log_error(f"  State mismatch: got '{actual_state}', expected '{expected_state}' ✖")
                overall_pass = False
        except json.JSONDecodeError:
            log_error("  Invalid JSON in getLEDState response")
            overall_pass = False

    if overall_pass:
        log_success("TCID004 Passed ✅")
    else:
        log_error("TCID004 Failed ❌")

    return overall_pass

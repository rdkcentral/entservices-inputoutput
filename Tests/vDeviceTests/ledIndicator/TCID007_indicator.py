"""
TCID007 - Scenario: USB Upgrade Lifecycle
Verify USB upgrade state is reported correctly and device returns to ACTIVE.
  ACTIVE -> USB_UPGRADE -> ACTIVE
Also verifies getSupportedLEDStates includes USB_UPGRADE.
"""
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


def _post(yaml_file):
    yaml_path = f"{INDICATOR_CMD_BASE}/{yaml_file}"
    log_info(f"  vComponent POST: {yaml_path}")
    http_code, body = send_vcomponent_command(yaml_path)
    log_info(f"  HTTP {http_code}  body: {body}")
    return 200 <= http_code < 300


def _get_led_state():
    resp = send_curl_command(ledIndicatorApis.get_led_state)
    if not resp or resp == "< No response from WPEFramework >":
        return None
    try:
        return json.loads(resp).get("result", {}).get("state")
    except json.JSONDecodeError:
        return None


def run_test():
    log_info("TCID007 - Scenario: USB Upgrade Lifecycle")

    # Pre-check: USB_UPGRADE must be in supported states
    log_info("\n-- Pre-check: getSupportedLEDStates contains USB_UPGRADE --")
    supp_resp = send_curl_command(ledIndicatorApis.get_supported_led_states)
    if not supp_resp or supp_resp == "< No response from WPEFramework >":
        log_error("  getSupportedLEDStates: no response")
        log_error("TCID007 Failed ❌")
        return False
    log_warning(f"  getSupportedLEDStates response: {supp_resp}")
    try:
        supp_json = json.loads(supp_resp)
        states_list = supp_json.get("result", {}).get("supportedLEDStates", [])
        if "USB_UPGRADE" not in states_list:
            log_error("  USB_UPGRADE not in supportedLEDStates")
            log_error("TCID007 Failed ❌")
            return False
        log_success("  USB_UPGRADE found in supportedLEDStates ✔")
    except json.JSONDecodeError:
        log_error("  Invalid JSON in getSupportedLEDStates response")
        log_error("TCID007 Failed ❌")
        return False

    steps = [
        ("indicator_set_state_active.yaml",      "ACTIVE"),
        ("indicator_set_state_usb_upgrade.yaml", "USB_UPGRADE"),
        ("indicator_set_state_active.yaml",      "ACTIVE"),
    ]
    for yaml_file, expected in steps:
        log_info(f"\n-- Step: set {expected} --")
        if not _post(yaml_file):
            log_error(f"  vComponent POST failed: {yaml_file}")
            log_error("TCID007 Failed ❌")
            return False
        time.sleep(2)
        actual = _get_led_state()
        log_warning(f"  getLEDState -> '{actual}'  (expected '{expected}')")
        if actual != expected:
            log_error(f"  Mismatch: got '{actual}', expected '{expected}'")
            log_error("TCID007 Failed ❌")
            return False
        log_success(f"  OK: {actual}")
    log_success("TCID007 Passed ✅")
    return True

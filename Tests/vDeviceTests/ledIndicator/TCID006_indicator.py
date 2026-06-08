"""
TCID006 - Scenario: WPS Error Recovery
Simulate WPS failure and verify device recovers back to STANDBY.
  WPS_CONNECTING -> WPS_ERROR -> STANDBY
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
    log_info("TCID006 - Scenario: WPS Error Recovery (CONNECTING -> ERROR -> STANDBY)")
    steps = [
        ("indicator_set_state_wps_connecting.yaml", "WPS_CONNECTING"),
        ("indicator_set_state_wps_error.yaml",       "WPS_ERROR"),
        ("indicator_set_state_standby.yaml",          "STANDBY"),
    ]
    for yaml_file, expected in steps:
        log_info(f"\n-- Step: set {expected} --")
        if not _post(yaml_file):
            log_error(f"  vComponent POST failed: {yaml_file}")
            log_error("TCID006 Failed ❌")
            return False
        time.sleep(2)
        actual = _get_led_state()
        log_warning(f"  getLEDState -> '{actual}'  (expected '{expected}')")
        if actual != expected:
            log_error(f"  Mismatch: got '{actual}', expected '{expected}'")
            log_error("TCID006 Failed ❌")
            return False
        log_success(f"  OK: {actual}")
    log_success("TCID006 Passed ✅")
    return True

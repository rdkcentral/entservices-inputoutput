"""
TCID011 - Scenario: IP Acquired maps to ACTIVE
Verify that AIDL IP_ACQUIRED state is surfaced as ACTIVE through the MW.
  IP_ACQUIRED (vComponent) -> getLEDState returns ACTIVE
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
    log_info("TCID011 - Scenario: IP_ACQUIRED maps to MW ACTIVE")
    log_info("  Per aidlStateToLEDControlState: IP_ACQUIRED -> LEDSTATE_ACTIVE")

    log_info("\n-- Step: set IP_ACQUIRED via vComponent --")
    if not _post("indicator_set_state_ip_acquired.yaml"):
        log_error("  vComponent POST failed")
        log_error("TCID011 Failed ❌")
        return False
    time.sleep(2)

    actual = _get_led_state()
    log_warning(f"  getLEDState -> '{actual}'  (expected 'ACTIVE')")
    if actual != "ACTIVE":
        log_error(f"  Mismatch: got '{actual}', expected 'ACTIVE'")
        log_error("TCID011 Failed ❌")
        return False

    log_success("  IP_ACQUIRED correctly mapped to ACTIVE ✔")
    log_success("TCID011 Passed ✅")
    return True

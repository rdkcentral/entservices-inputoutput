"""
TCID014 - Scenario: Full Device Lifecycle (Boot to Standby)
Simulate a complete device power lifecycle:
  BOOT -> ACTIVE (IP acquired) -> WPS_CONNECTING -> WPS_CONNECTED -> STANDBY -> DEEP_SLEEP
Each step verifies the MW getLEDState returns the correct mapped value.
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
    log_info("TCID014 - Scenario: Full Device Lifecycle")
    log_info("  BOOT->ACTIVE(IP_ACQUIRED)->WPS_CONNECTING->WPS_CONNECTED->STANDBY->DEEP_SLEEP(STANDBY)")

    # (yaml_file, expected_mw_state, description)
    # BOOT is AIDL-only, not in MW enum -> getLEDState will return NONE (unrecognised)
    # IP_ACQUIRED -> ACTIVE, DEEP_SLEEP -> STANDBY per aidlStateToLEDControlState
    steps = [
        ("indicator_set_state_ip_acquired.yaml",   "ACTIVE",        "IP acquired (network up)"),
        ("indicator_set_state_wps_connecting.yaml","WPS_CONNECTING", "WPS session start"),
        ("indicator_set_state_wps_connected.yaml", "WPS_CONNECTED",  "WPS paired"),
        ("indicator_set_state_active.yaml",         "ACTIVE",        "Normal operation"),
        ("indicator_set_state_standby.yaml",        "STANDBY",       "User standby"),
        ("indicator_set_state_deep_sleep.yaml",     "STANDBY",       "Deep sleep (maps to STANDBY)"),
    ]

    overall_pass = True
    for yaml_file, expected, description in steps:
        log_info(f"\n-- [{description}] set {yaml_file.replace('indicator_set_state_','').replace('.yaml','')} -> expect MW {expected} --")
        if not _post(yaml_file):
            log_error(f"  vComponent POST failed: {yaml_file}")
            overall_pass = False
            continue
        time.sleep(2)
        actual = _get_led_state()
        log_warning(f"  getLEDState -> '{actual}'  (expected '{expected}')")
        if actual != expected:
            log_error(f"  Mismatch: got '{actual}', expected '{expected}'")
            overall_pass = False
        else:
            log_success(f"  OK ✔")

    if overall_pass:
        log_success("TCID014 Passed ✅")
    else:
        log_error("TCID014 Failed ❌")
    return overall_pass

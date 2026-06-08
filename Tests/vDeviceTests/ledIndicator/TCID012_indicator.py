"""
TCID012 - Scenario: setLEDState via MW and verify vComponent reflects it
Use the MW setLEDState API (not vComponent) to change state, then query getLEDState.
Tests the reverse path: MW -> AIDL HAL -> vComponent.
"""
import json
import time
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from utils import (
    send_curl_command,
    log_info,
    log_success,
    log_error,
    log_warning
)
import ledIndicatorApis


def _get_led_state():
    resp = send_curl_command(ledIndicatorApis.get_led_state)
    if not resp or resp == "< No response from WPEFramework >":
        return None
    try:
        return json.loads(resp).get("result", {}).get("state")
    except json.JSONDecodeError:
        return None


def run_test():
    log_info("TCID012 - Scenario: setLEDState via MW API for each supported state")
    log_info("  Calls MW setLEDState then immediately reads back with getLEDState")

    # (curl_cmd_attr, expected_state)
    steps = [
        (ledIndicatorApis.set_led_state_active,        "ACTIVE"),
        (ledIndicatorApis.set_led_state_standby,       "STANDBY"),
        (ledIndicatorApis.set_led_state_wps_connecting, "WPS_CONNECTING"),
        (ledIndicatorApis.set_led_state_wps_connected,  "WPS_CONNECTED"),
        (ledIndicatorApis.set_led_state_wps_error,      "WPS_ERROR"),
        (ledIndicatorApis.set_led_state_factory_reset,  "FACTORY_RESET"),
        (ledIndicatorApis.set_led_state_usb_upgrade,    "USB_UPGRADE"),
        (ledIndicatorApis.set_led_state_download_error, "DOWNLOAD_ERROR"),
    ]

    overall_pass = True
    for set_cmd, expected in steps:
        log_info(f"\n-- setLEDState to {expected} --")
        set_resp = send_curl_command(set_cmd)
        log_warning(f"  setLEDState response: {set_resp}")
        if not set_resp or set_resp == "< No response from WPEFramework >":
            log_error(f"  No response for setLEDState({expected})")
            overall_pass = False
            continue
        try:
            set_json = json.loads(set_resp)
            result = set_json.get("result")
            set_ok = (result is True) or (
                isinstance(result, dict) and result.get("success", False)
            )
            if not set_ok:
                log_error(f"  setLEDState({expected}) returned success=false")
                overall_pass = False
                continue
        except json.JSONDecodeError:
            log_error(f"  Invalid JSON for setLEDState({expected})")
            overall_pass = False
            continue

        time.sleep(1)
        actual = _get_led_state()
        log_warning(f"  getLEDState -> '{actual}'  (expected '{expected}')")
        if actual != expected:
            log_error(f"  Mismatch: got '{actual}', expected '{expected}'")
            overall_pass = False
        else:
            log_success(f"  OK: {actual} ✔")

    if overall_pass:
        log_success("TCID012 Passed ✅")
    else:
        log_error("TCID012 Failed ❌")
    return overall_pass

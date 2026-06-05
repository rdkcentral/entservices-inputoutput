"""
TCID013 - Scenario: getSupportedLEDStates content validation
Verify that getSupportedLEDStates returns the expected set of states
matching the hfp-indicator_vcomponent.yaml profile.
No vComponent interaction needed - pure MW API test.
"""
import json
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


# These are the MW-level state names that should be returned.
# BOOT, DEEP_SLEEP, IP_ACQUIRED, OFF, NO_IP, WIFI_ERROR, WPS_SES_OVERLAP, PSU_FAILURE
# are AIDL-only states not mapped to MW enum, so they are excluded.
EXPECTED_STATES = {
    "ACTIVE",
    "STANDBY",
    "WPS_CONNECTING",
    "WPS_CONNECTED",
    "WPS_ERROR",
    "FACTORY_RESET",
    "USB_UPGRADE",
    "DOWNLOAD_ERROR",
}


def run_test():
    log_info("TCID013 - Scenario: getSupportedLEDStates content validation")

    resp = send_curl_command(ledIndicatorApis.get_supported_led_states)
    if not resp or resp == "< No response from WPEFramework >":
        log_error("  No response from WPEFramework")
        log_error("TCID013 Failed ❌")
        return False

    log_warning(f"  getSupportedLEDStates response: {resp}")

    try:
        resp_json = json.loads(resp)
    except json.JSONDecodeError:
        log_error("  Invalid JSON response")
        log_error("TCID013 Failed ❌")
        return False

    result = resp_json.get("result", {})
    if not result.get("success", False):
        log_error("  success field is false")
        log_error("TCID013 Failed ❌")
        return False

    returned_states = set(result.get("supportedLEDStates", []))
    log_info(f"  Returned states: {sorted(returned_states)}")

    missing = EXPECTED_STATES - returned_states
    if missing:
        log_error(f"  Missing expected states: {missing}")
        log_error("TCID013 Failed ❌")
        return False

    log_success("  All expected states present in getSupportedLEDStates ✔")
    log_success("TCID013 Passed ✅")
    return True

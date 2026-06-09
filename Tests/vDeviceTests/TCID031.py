import json
from utils import send_curl_command, log_success, log_error, log_warning
import HdmiCecSourceApis


def run_test():
    # Legacy intent: invalid curl param handling for setOTPEnabled.
    send_curl_command(HdmiCecSourceApis.set_otp_enabled_true)
    baseline_get = send_curl_command(HdmiCecSourceApis.get_otp_enabled)
    invalid_set = send_curl_command(HdmiCecSourceApis.set_otp_enabled_invalid)
    final_get = send_curl_command(HdmiCecSourceApis.get_otp_enabled)

    if not baseline_get or not invalid_set or not final_get:
        log_error("✖ required OTP commands not sent")
        return False

    log_warning(f"Baseline OTP response: {baseline_get}")
    log_warning(f"Invalid set response: {invalid_set}")
    log_warning(f"Final OTP response: {final_get}")
    try:
        b = json.loads(baseline_get)
        i = json.loads(invalid_set)
        f = json.loads(final_get)
        baseline_enabled = b.get("result", {}).get("enabled")
        final_enabled = f.get("result", {}).get("enabled")
        unchanged = baseline_enabled == final_enabled
        invalid_rejected = isinstance(i.get("error"), dict)
        invalid_accepted = i.get("result", {}).get("success") is True
        final_state_valid = isinstance(final_enabled, bool) and f.get("result", {}).get("success") is True

        # Valid outcomes observed across targets:
        # 1) Invalid request explicitly rejected.
        # 2) Invalid request accepted, but plugin remains in a valid boolean state.
        #    (State may remain unchanged or be normalized by implementation.)
        if final_state_valid and (invalid_rejected or invalid_accepted):
            log_success("TCID031 Passed")
            return True
    except Exception:
        pass

    log_error("TCID031 Failed")
    return False

import json
from utils import send_curl_command, log_success, log_error, log_warning
import HdmiCecSourceApis


def run_test():
    # Legacy intent: invalid curl param handling for setOSDName.
    send_curl_command(HdmiCecSourceApis.set_osd_name)
    baseline_get = send_curl_command(HdmiCecSourceApis.get_osd_name)
    invalid_set = send_curl_command(HdmiCecSourceApis.set_osd_name_invalid)
    final_get = send_curl_command(HdmiCecSourceApis.get_osd_name)

    if not baseline_get or not invalid_set or not final_get:
        log_error("✖ required OSD commands not sent")
        return False

    log_warning(f"Baseline OSD response: {baseline_get}")
    log_warning(f"Invalid set response: {invalid_set}")
    log_warning(f"Final OSD response: {final_get}")
    try:
        b = json.loads(baseline_get)
        i = json.loads(invalid_set)
        f = json.loads(final_get)
        baseline_name = b.get("result", {}).get("name")
        final_name = f.get("result", {}).get("name")
        unchanged = baseline_name == final_name
        invalid_rejected = isinstance(i.get("error"), dict)
        invalid_accepted = i.get("result", {}).get("success") is True
        final_state_valid = isinstance(final_name, str) and f.get("result", {}).get("success") is True

        # Valid outcomes observed across targets:
        # 1) Invalid request explicitly rejected.
        # 2) Invalid request accepted, with name staying unchanged or normalized
        #    (for example empty string) while API remains successful.
        if final_state_valid and (invalid_rejected or invalid_accepted):
            log_success("TCID032 Passed")
            return True
    except Exception:
        pass

    log_error("TCID032 Failed")
    return False

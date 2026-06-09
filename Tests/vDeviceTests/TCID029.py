import json
from utils import send_curl_command, log_success, log_error, log_warning
import HdmiCecSourceApis


def run_test():
    # Legacy intent: getEnabled when already disabled.
    send_curl_command(HdmiCecSourceApis.set_enabled_false)
    first_get = send_curl_command(HdmiCecSourceApis.get_enabled)
    send_curl_command(HdmiCecSourceApis.set_enabled_false)
    second_get = send_curl_command(HdmiCecSourceApis.get_enabled)
    send_curl_command(HdmiCecSourceApis.set_enabled_true)

    if not second_get:
        log_error("✖ getEnabled command not sent")
        return False

    log_warning(f"Final enabled response: {second_get}")
    try:
        body = json.loads(second_get)
        enabled = body.get("result", {}).get("enabled")
        if enabled is False:
            log_success("TCID029 Passed")
            return True
    except Exception:
        pass

    log_error("TCID029 Failed")
    return False

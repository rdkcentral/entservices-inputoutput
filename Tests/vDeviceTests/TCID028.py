import json
from utils import send_curl_command, log_success, log_error, log_warning
import HdmiCecSourceApis


def run_test():
    # Legacy intent: invalid curl param handling for setVendorId.
    baseline_set = send_curl_command(HdmiCecSourceApis.set_vendor_id)
    baseline_get = send_curl_command(HdmiCecSourceApis.get_vendor_id)
    invalid_set = send_curl_command(HdmiCecSourceApis.set_vendor_id_invalid)
    final_get = send_curl_command(HdmiCecSourceApis.get_vendor_id)

    if not final_get:
        log_error("✖ getVendorId command not sent")
        return False

    log_warning(f"Final vendor response: {final_get}")
    try:
        b = json.loads(baseline_get)
        f = json.loads(final_get)
        if "result" in b and "result" in f and b["result"].get("vendorid") == f["result"].get("vendorid"):
            log_success("TCID028 Passed")
            return True
    except Exception:
        pass

    log_error("TCID028 Failed")
    return False

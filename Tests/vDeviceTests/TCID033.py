import json
import time
from pathlib import Path

from utils import (
    send_curl_command,
    send_vcomponent_command,
    HDMICEC_CMD_BASE,
    log_info,
    log_success,
    log_warning,
    log_error,
)
import HdmiCecSourceApis


def _post_yaml(yaml_name):
    http_code, body = send_vcomponent_command(f"{HDMICEC_CMD_BASE}/{yaml_name}")
    log_info(f"POST {yaml_name}: HTTP {http_code} {body}")
    return http_code == 200


def _health_check():
    response = send_curl_command(HdmiCecSourceApis.get_device_list)
    if not response or response.startswith("< No response"):
        return False

    try:
        body = json.loads(response)
        return isinstance(body, dict) and "result" in body
    except json.JSONDecodeError:
        return False


def _get_device_snapshot():
    response = send_curl_command(HdmiCecSourceApis.get_device_list)
    if not response or response.startswith("< No response"):
        return None

    try:
        body = json.loads(response)
        result = body.get("result", {})
        number = result.get("numberofdevices")
        devices = result.get("deviceList", [])
        if not isinstance(devices, list):
            devices = []
        logicals = set()
        for d in devices:
            if isinstance(d, dict):
                la = d.get("logicalAddress")
                if isinstance(la, int):
                    logicals.add(la)
        return {
            "number": number if isinstance(number, int) else None,
            "logicals": logicals,
        }
    except json.JSONDecodeError:
        return None


def _get_active_source_status_ok():
    response = send_curl_command(HdmiCecSourceApis.get_active_source_status)
    if not response or response.startswith("< No response"):
        return False
    try:
        body = json.loads(response)
        result = body.get("result", {})
        return isinstance(result.get("status"), bool) and result.get("success") is True
    except json.JSONDecodeError:
        return False


def run_test():
    # Validate all process-trigger YAML files are accepted by vComponent,
    # and add observable checks for handlers that should affect plugin state.
    commands_dir = Path(__file__).resolve().parent / "vcomponent_configurations" / "hdmicec" / "commands"
    yaml_files = sorted(
        p.relative_to(commands_dir).as_posix()
        for p in commands_dir.rglob("hdmicec_process_*.yaml")
    )

    if not yaml_files:
        log_error("TCID033 Failed: no hdmicec_process_*.yaml files found")
        return False

    # Configure vcomponent network with known topology (YAMAHA at LA=5) so that
    # device-list state checks have a stable vcomponent-backed device to verify against.
    http_code, _ = send_vcomponent_command(f"{HDMICEC_CMD_BASE}/hdmicec_device_config_add_network.yaml")
    if http_code != 200:
        log_error("TCID033 Failed: configure command rejected")
        return False
    time.sleep(1)

    if not _health_check():
        log_error("TCID033 Failed: pre-check getDeviceList is not healthy")
        return False

    pre_snapshot = _get_device_snapshot()
    if pre_snapshot is None:
        log_error("TCID033 Failed: unable to capture pre device snapshot")
        return False

    failed_posts = []
    state_check_failures = []

    # These handlers can be observed by API-level side effects.
    # Maps yaml filename -> expected logical address that must appear in deviceList.
    # Using la5 variants (src=LA=5, broadcast dst) which have real vcomponent network
    # backing, so addDevice(5) sticks through the full discovery pipeline.
    # LA=4 variants are kept as posts-only checks (no state assertion) because LA=4
    # has no vcomponent network entry and the update thread NACKs during discovery.
    should_touch_device_list = {
        "la_devices/hdmicec_process_report_physical_address_la5.yaml": 5,
        "la_devices/hdmicec_process_cec_version_la5.yaml": 5,
        "la_devices/hdmicec_process_set_osd_name_la5.yaml": 5,
        "la_devices/hdmicec_process_device_vendor_id_la5.yaml": 5,
    }
    should_keep_active_status_api_healthy = {
        "hdmicec_process_routing_change.yaml",
        "hdmicec_process_routing_information.yaml",
        "hdmicec_process_set_stream_path.yaml",
        "hdmicec_process_request_active_source.yaml",
    }

    for yaml_name in yaml_files:
        if not _post_yaml(yaml_name):
            failed_posts.append(yaml_name)
            continue

        if yaml_name in should_touch_device_list:
            # Longer window for the full pipeline:
            # vcomponent AIDL callback → DriverReceiveCallback → rQueue
            # → read thread → MessageDecoder → process() → addDevice()
            time.sleep(1.5)
            expected_la = should_touch_device_list[yaml_name]
            snap = _get_device_snapshot()
            if snap is None:
                state_check_failures.append(f"{yaml_name}: snapshot unavailable")
            else:
                if expected_la not in snap["logicals"]:
                    state_check_failures.append(
                        f"{yaml_name}: logicalAddress {expected_la} not observed"
                    )
        else:
            # Short window for non-state-check YAMLs.
            time.sleep(0.2)

        if yaml_name in should_keep_active_status_api_healthy:
            if not _get_active_source_status_ok():
                state_check_failures.append(f"{yaml_name}: activeSourceStatus API unhealthy")

    if not _health_check():
        log_error("TCID033 Failed: post-check getDeviceList is not healthy")
        return False

    post_snapshot = _get_device_snapshot()
    if post_snapshot is None:
        log_error("TCID033 Failed: unable to capture post device snapshot")
        return False

    pre_num = pre_snapshot["number"] if isinstance(pre_snapshot["number"], int) else -1
    post_num = post_snapshot["number"] if isinstance(post_snapshot["number"], int) else -1
    log_info(f"Device count pre={pre_num} post={post_num}")

    if failed_posts:
        log_warning(f"Failed YAML posts: {failed_posts}")
        log_error("TCID033 Failed")
        return False

    if state_check_failures:
        log_warning(f"State-check warnings: {state_check_failures}")
        log_error("TCID033 Failed")
        return False

    log_info("Non-observable handlers (event-only/outbound-only) remain acceptance-based in this TC.")
    log_info("For strict proof, add implementation counters or parse plugin logs per handler.")

    if post_num < pre_num:
        log_warning("Post device count lower than pre-count; treating as unstable runtime")
        log_error("TCID033 Failed")
        return False

    log_success(f"TCID033 Passed ({len(yaml_files)} process YAMLs posted + observable checks)")
    return True

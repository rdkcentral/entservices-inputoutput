import importlib
import io
import sys
import time
from pathlib import Path
import os

from utils import log_error, log_info, log_success, activate_plugin, WPEFRAMEWORK_JSONRPC_URL


BASE_DIR = Path(__file__).resolve().parent
SUITES = {
    "hdmicecsource": {
        "banner": "******************** L2 SUITE - RDK - HDMI CEC SOURCE ****************************",
        "module_dir": BASE_DIR,
        "tests": [
            "TCID001",
            "TCID002",
            "TCID003",
            "TCID004",
            "TCID005",
            "TCID006",
            "TCID007",
            "TCID008",
            "TCID009",
            "TCID010",
            "TCID011",
            "TCID012",
            "TCID013",
            "TCID014",
            "TCID015",
            "TCID016",
            "TCID022",
            "TCID023",
            "TCID024",
            "TCID021",
            "TCID018",
            "TCID020",
            "TCID019",
        ],
    },
    "ledindicator": {
        "banner": "******************** L2 SUITE - RDK - LED INDICATOR ****************************",
        "module_dir": BASE_DIR / "ledIndicator",
        "tests": [
            "TCID001_indicator",
            "TCID002_indicator",
            "TCID003_indicator",
            "TCID004_indicator",
            "TCID005_indicator",
            "TCID006_indicator",
            "TCID007_indicator",
            "TCID008_indicator",
            "TCID009_indicator",
            "TCID010_indicator",
            "TCID011_indicator",
            "TCID012_indicator",
            "TCID013_indicator",
            "TCID014_indicator",
        ],
    },
}

SUITE_PLUGIN_CALLSIGNS = {
    "hdmicecsource": "org.rdk.HdmiCecSource",
    "ledindicator": "org.rdk.LEDControl",
}


def normalize_suite_name(raw_name):
    return raw_name.strip().replace("_", "").replace("-", "").lower()


def load_test_cases(suite_name):
    suite_config = SUITES[suite_name]
    module_dir = str(suite_config["module_dir"])

    if module_dir not in sys.path:
        sys.path.insert(0, module_dir)

    test_cases = []
    for module_name in suite_config["tests"]:
        module = importlib.import_module(module_name)
        test_cases.append((module_name, module.run_test))

    return suite_config["banner"], test_cases


def run_suite(suite_name):
    banner, test_cases = load_test_cases(suite_name)
    print(banner)

    auto_activate = os.environ.get("AUTO_ACTIVATE_PLUGINS", "1").lower() not in ("0", "false", "no")
    callsign = SUITE_PLUGIN_CALLSIGNS.get(suite_name)
    if auto_activate and callsign:
        log_info(f"Auto-activating plugin '{callsign}' via {WPEFRAMEWORK_JSONRPC_URL}")
        if activate_plugin(callsign):
            log_success(f"Plugin activated: {callsign}")
        else:
            log_error(f"Plugin activation failed: {callsign}")
            log_error("Check JSON-RPC endpoint reachability and plugin availability before running tests.")
            return False

    passed = 0
    failed = 0
    failed_cases = []
    original_stdout = sys.stdout

    for tc_name, tc_fn in test_cases:
        log_info(f"\n{'='*60}")
        log_info(f"Running: {tc_name}")
        log_info(f"{'='*60}")
        captured = io.StringIO()
        sys.stdout = captured
        try:
            result = tc_fn()
        except Exception as exc:
            result = False
            print(f"EXCEPTION in {tc_name}: {exc}")
        finally:
            sys.stdout = original_stdout

        output = captured.getvalue()
        print(output, end="")

        if result:
            passed += 1
            log_success(f"[PASS] {tc_name}")
        else:
            failed += 1
            failed_cases.append(tc_name)
            log_error(f"[FAIL] {tc_name}")

        time.sleep(1)

    log_info(f"\n{'='*60}")
    log_info(f"Suite Summary: {passed} passed, {failed} failed")
    if failed_cases:
        log_error(f"Failed cases: {failed_cases}")
    log_info(f"{'='*60}")
    return failed == 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python suiteManager.py <suite_name>")
        print(f"Available suites: {list(SUITES.keys())}")
        sys.exit(1)

    suite_arg = normalize_suite_name(sys.argv[1])
    matching = [k for k in SUITES if normalize_suite_name(k) == suite_arg]
    if not matching:
        log_error(f"Unknown suite '{sys.argv[1]}'. Available: {list(SUITES.keys())}")
        sys.exit(1)

    ok = run_suite(matching[0])
    sys.exit(0 if ok else 1)

import importlib
import io
import sys
import time
from pathlib import Path

from utils import log_error, log_info, log_success


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
        ],
    },
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

    passed = 0
    failed = 0
    failed_cases = []
    original_stdout = sys.stdout

    for tc_name, tc_fn in test_cases:
        log_info(f"Running {tc_name}")
        captured = io.StringIO()
        try:
            time.sleep(10)
            sys.stdout = captured
            result = tc_fn()
            sys.stdout = original_stdout
            output = captured.getvalue()
            print(output, end="")
            if result:
                passed += 1
            else:
                failed += 1
                failed_cases.append((tc_name, "Returned False", output.strip()))
        except Exception as error:
            sys.stdout = original_stdout
            output = captured.getvalue()
            print(output, end="")
            log_error(f"{tc_name} Crashed ❌ | Error: {error}")
            failed += 1
            failed_cases.append((tc_name, f"Exception: {error}", output.strip()))

    print("\n==================== L2 SUITE SUMMARY ====================")
    log_success(f"Passed : {passed}")
    log_error(f"Failed : {failed}")
    print(f"Total  : {passed + failed}")

    if failed_cases:
        print("Failed Cases :")
        for tc_name, _, _ in failed_cases:
            log_error(f"  ❌ {tc_name}")

        print("Failed Remarks :")
        for tc_name, reason, output in failed_cases:
            log_error(f"  ❌ {tc_name} | {reason}")
            if output:
                log_error(f"     Output : {output}")
    else:
        print("Failed Cases : None")

    print("==========================================================")
    return 0 if failed == 0 else 1


def main():
    if len(sys.argv) != 2:
        log_error("Usage: python3 suiteManager.py <HdmiCecSource|ledindicator>")
        return 1

    suite_name = normalize_suite_name(sys.argv[1])
    if suite_name not in SUITES:
        log_error(f"Unknown suite '{sys.argv[1]}'. Supported suites: HdmiCecSource, ledindicator")
        return 1

    return run_suite(suite_name)


if __name__ == "__main__":
    raise SystemExit(main())
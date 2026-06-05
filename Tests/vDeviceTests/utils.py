import os
import json
import subprocess
from pathlib import Path


# Base paths for vComponent YAML commands.
# Prefer testcase-local YAMLs, fallback to /etc paths, and allow env overrides.
_BASE_DIR = Path(__file__).resolve().parent
_LOCAL_INDICATOR_CMD_BASE = _BASE_DIR / "vcomponent_configurations" / "indicator" / "commands"
_LOCAL_HDMICEC_CMD_BASE = _BASE_DIR / "vcomponent_configurations" / "hdmicec" / "commands"


def _pick_existing_dir(primary, fallback):
    if primary.is_dir():
        return str(primary)
    return fallback


INDICATOR_CMD_BASE = os.environ.get("INDICATOR_CMD_BASE") or _pick_existing_dir(
    _LOCAL_INDICATOR_CMD_BASE,
    "/etc/indicator/vcomponent_configurations/commands",
)
HDMICEC_CMD_BASE = os.environ.get("HDMICEC_CMD_BASE") or _pick_existing_dir(
    _LOCAL_HDMICEC_CMD_BASE,
    "/etc/hdmicec/vcomponent_configurations/commands",
)
VCOMPONENT_API_URL = "http://127.0.0.1:8080/api/postKVP"


# ---------- ANSI COLOR CONSTANTS ----------
RESET = "\033[0m"
BOLD = "\033[1m"

RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
CYAN = "\033[96m"

# ---------- OPTIONAL LOG HELPERS ----------
def log_info(msg):
    print(f"{CYAN}{msg}{RESET}")

def log_success(msg):
    print(f"{GREEN}{BOLD}{msg}{RESET}")

def log_warning(msg):
    print(f"{YELLOW}{msg}{RESET}")

def log_error(msg):
    print(f"{RED}{BOLD}{msg}{RESET}")

def send_curl_command(curl_command):
    '''This function is used to send the curl commands to get the output response using os module'''
    output_response = ""
    try:
        # Send the curl command using os.system module
        response = os.popen(curl_command)

        # Find the line that is a valid JSON for extracting only the json response
        for line in response.readlines():
            try:
                # Try to parse the current line as JSON
                json.loads(line)
                output_response = line
                # Exit the loop as we found the JSON line
                break
            except json.JSONDecodeError:
                # If current line is not a valid JSON, just pass and continue with the next line
                pass

        # Check the output response and add a message if the obtained output response is null
        if len(output_response) < 5:
            output_response = "< No response from WPEFramework >"
    except:
        print("Inside Utils.py : Exception in send_curl_command function")
    finally:
        # Return the output json response of given curl command as a string
        return output_response


def send_vcomponent_command(yaml_file_path):
    '''Post a YAML command file to the vComponent HTTP API (new implementation).
    Uses: curl -sS -X POST -H "Content-Type: application/x-yaml"
               --data-binary @<yaml_file> http://127.0.0.1:8080/api/postKVP
    Returns (http_code: int, body: str) tuple.
    http_code 200 indicates success.
    '''
    cmd = [
        "curl", "-sS", "-w", "\n%{http_code}",
        "-X", "POST",
        "-H", "Content-Type: application/x-yaml",
        "--data-binary", f"@{yaml_file_path}",
        VCOMPONENT_API_URL,
    ]
    try:
        result = subprocess.run(
            cmd,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        lines = result.stdout.strip().rsplit("\n", 1)
        body = lines[0] if len(lines) > 1 else result.stdout.strip()
        http_code_str = lines[-1].strip() if len(lines) > 1 else "0"
        try:
            http_code = int(http_code_str)
        except ValueError:
            http_code = 0
        return http_code, body
    except Exception as exc:
        print(f"Inside Utils.py : Exception in send_vcomponent_command: {exc}")
        return 0, ""

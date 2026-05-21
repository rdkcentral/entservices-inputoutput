import os
import json



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
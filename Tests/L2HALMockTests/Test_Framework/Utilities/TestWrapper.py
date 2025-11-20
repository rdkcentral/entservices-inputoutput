#** *****************************************************************************
# *
# * If not stated otherwise in this file or this component's LICENSE file the
# * following copyright and licenses apply:
# *
# * Copyright 2024 RDK Management
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *
# http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *
#* ******************************************************************************

"""
TestWrapper module provides helper functions to automatically capture and display
WPEFramework logs when test cases fail.
"""

from Utilities import Utils, ReportGenerator

def execute_test_with_logs(tc_id, expected_response, actual_response, success_message, failure_message):
    """
    Execute a test case and automatically capture WPEFramework logs on failure.
    
    Args:
        tc_id: Test case ID
        expected_response: Expected response string
        actual_response: Actual response string
        success_message: Message to display on success
        failure_message: Message to display on failure
    
    Returns:
        tuple: (status, message) where status is 'Pass' or 'Fail'
    """
    # Compare responses
    if str(actual_response) == str(expected_response):
        status = 'Pass'
        message = success_message
    else:
        status = 'Fail'
        message = failure_message
        
        # Capture WPEFramework logs on failure
        try:
            wpe_logs = Utils.get_wpeframework_error_logs(tc_id, num_lines=100)
            if wpe_logs and "No WPEFramework logs found" not in wpe_logs:
                Utils.error_log(f"\nWPEFramework Error Logs for {tc_id}:")
                print(wpe_logs)
                message = message + "\n\nWPEFramework Error Logs captured. See above for details."
        except Exception as e:
            Utils.error_log(f"Error capturing WPEFramework logs: {str(e)}")
    
    # Log test results
    print("Testcase ID : " + tc_id)
    print("Testcase Output Response : " + str(actual_response))
    print("Testcase Status : " + status)
    print("Testcase Message : " + message)
    print("")
    
    # Update report
    if status == 'Pass':
        ReportGenerator.passed_tc_list.append(tc_id)
    else:
        ReportGenerator.failed_tc_list.append(tc_id)
        # Capture logs for failed test in report
        try:
            wpe_logs = Utils.get_wpeframework_error_logs(tc_id, num_lines=50)
            ReportGenerator.append_test_results_to_csv(tc_id, str(actual_response), status, message, wpe_logs)
        except:
            ReportGenerator.append_test_results_to_csv(tc_id, str(actual_response), status, message)
    
    return status, message


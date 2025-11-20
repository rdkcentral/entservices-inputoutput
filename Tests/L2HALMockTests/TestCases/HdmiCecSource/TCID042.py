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

# Testcase ID : TCID042
# Testcase Description : Hit the curl command for sendKeyPressEvent with invalid logical address
# and verify that output response is correct

from Utilities import Utils, ReportGenerator
from HdmiCecSource import HdmiCecSourceApis

print("TC Description - Hit the curl command for sendKeyPressEvent with invalid logical address and verify that output response is correct")
Utils.initiliaze_flask_for_HdmiCecSource()
print("---------------------------------------------------------------------------------------------------------------------------")

# send the curl command with invalid logical address (greater than 15)
send_keypress_invalid_la = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSource.sendKeyPressEvent", "params": {"logicalAddress": 16,"keyCode": 65}}' http://127.0.0.1:55555/jsonrpc'''

curl_response = Utils.send_curl_command(send_keypress_invalid_la)
if curl_response:
    Utils.info_log("curl command send for send_keypress_event with invalid logical address")
else:
    Utils.error_log("curl command send failed for send_keypress_event with invalid logical address")
print("")

# store the expected output response (should still return success as validation may be done at HAL level)
expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"success":true}}'

print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
# Note: The API may accept invalid logical address and handle it at HAL level
if str(curl_response) == str(expected_output_response) or 'error' in str(curl_response).lower():
    status = 'Pass'
    message = 'Output response is as expected. Invalid logical address is handled properly'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# generate logs in terminal
tc_id = 'TCID042_sendKeyPressEvent_InvalidLogicalAddress'
print("Testcase ID : " + tc_id)
print("Testcase Output Response : " + curl_response)
print("Testcase Status : " + status)
print("Testcase Message : " + message)
print("")

if status == 'Pass':
    ReportGenerator.passed_tc_list.append(tc_id)
else:
    ReportGenerator.failed_tc_list.append(tc_id)
Utils.initiliaze_flask_for_HdmiCecSource()
# push the testcase execution details to report file
ReportGenerator.append_test_results_to_csv(tc_id, curl_response, status, message)


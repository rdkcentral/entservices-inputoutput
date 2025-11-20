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

# Testcase ID : TCID044
# Testcase Description : Verify that sendStandbyMessage returns error when CEC is disabled

from Utilities import Utils, ReportGenerator
from HdmiCecSource import HdmiCecSourceApis

print("TC Description - Verify that sendStandbyMessage returns error when CEC is disabled")
Utils.initiliaze_flask_for_HdmiCecSource()
print("---------------------------------------------------------------------------------------------------------------------------")

# First disable CEC
set_response = Utils.send_curl_command(HdmiCecSourceApis.set_enabled_false)
if set_response:
    Utils.info_log("curl command sent for disabling CEC")
else:
    Utils.error_log("curl command failed for disabling CEC")
print("")

# store the expected output response (should return error or success false)
expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"success":false}}'

# send the curl command for sendStandbyMessage when CEC is disabled
curl_response = Utils.send_curl_command(HdmiCecSourceApis.send_standby_message)
if curl_response:
    Utils.info_log("curl command send for send_standby_message when CEC is disabled")
else:
    Utils.error_log("curl command send failed for send_standby_message when CEC is disabled")
print("")

print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if str(curl_response) == str(expected_output_response) or 'error' in str(curl_response).lower() or 'false' in str(curl_response):
    status = 'Pass'
    message = 'Output response is as expected. sendStandbyMessage returns error/false when CEC is disabled'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# Re-enable CEC as post condition
Utils.send_curl_command(HdmiCecSourceApis.set_enabled_true)
Utils.info_log("Reset - Re-enabled CEC as post condition")

# generate logs in terminal
tc_id = 'TCID044_sendStandbyMessage_CECDisabled'
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


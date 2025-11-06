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

# Testcase ID : TCID006
# Testcase Description : Verify that cec enable status is true in output response

from Utilities import Utils, ReportGenerator
from HdmiCecSource import HdmiCecSourceApis

# store the expected output response
expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"enabled":true,"success":true}}'

print("TC Description - Verify that cec enable status is true in output response")
print("---------------------------------------------------------------------------------------------------------------------------")
Utils.initiliaze_flask_for_HdmiCecSource()
# send the curl command and fetch the output json response
curl_response = Utils.send_curl_command(HdmiCecSourceApis.VOLUME_UP)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.send_keypress_VOLUME_DOWN)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_MUTE)
curl_response = Utils.send_curl_command(HdmiCecSourceApis."HdmiCecSourceApis.send_keypress_UP)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_DOWN)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_LEFT)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_RIGHT)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_SELECT)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_HOME)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_BACK)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_0)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_1)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_2)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_3)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_4)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_5)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_6)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_7)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_8)
curl_response = Utils.send_curl_command(HdmiCecSourceApis.HdmiCecSourceApis.send_keypress_NUMBER_9)


HdmiCecSourceApis.send_keypress_NUMBER_6

HdmiCecSourceApis.send_keypress_VOLUME_DOWN
if curl_response:
    Utils.info_log("curl command send for get_enabled")
else:
    Utils.error_log("curl command send failed for get_enabled")

print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if str(curl_response) == str(expected_output_response):
    status = 'Pass'
    message = 'Output response is matching with expected one. The cec enable status is true'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# generate logs in terminal
tc_id = 'TCID006_getEnabled_CEC_enabled'
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

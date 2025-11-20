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

# Testcase ID : TCID036
# Testcase Description : Hit the curl command for getActiveSourceStatus method and
# verify that status is obtained as true in output response after performing OTP action

import json
import requests
import time
import Config
from Utilities import Utils, ReportGenerator
from HdmiCecSource import HdmiCecSourceApis

print("TC Description - Hit the curl command for getActiveSourceStatus method and verify that status is obtained as true in output response after performing OTP action")
Utils.initiliaze_flask_for_HdmiCecSource()
print("---------------------------------------------------------------------------------------------------------------------------")

# send the curl command to perform OTP action
curl_response_otp = Utils.send_curl_command(HdmiCecSourceApis.perform_otp_action)
if curl_response_otp:
    Utils.info_log("curl command send for perform_otp_action")
else:
    Utils.error_log("curl command send failed for perform_otp_action")
print("")

# send messages required for getting power status of device
message1_response = requests.get("http://{}/Hdmicec.sendMessage/{}".format(
    Config.flask_server_ip, json.dumps(Config.getPowerStatusMessage)))
if "200" in str(message1_response):
    Utils.info_log("sendMessage emulation success for querying the powerstatus of TV")
else:
    Utils.error_log("sendMessage emulation failed for querying the powerstatus of TV")
time.sleep(3)
print("")

# send messages required for reporting power status of device
message2_response = requests.get("http://{}/Hdmicec.sendMessage/{}".format(
    Config.flask_server_ip, json.dumps(Config.reportPowerStatusMessage)))
if "200" in str(message2_response):
    Utils.info_log("sendMessage emulation success for reporting the powerstatus of TV")
else:
    Utils.error_log("sendMessage emulation failed for reporting the powerstatus of TV")
time.sleep(3)
print("")

# store the expected output response
expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"status":true,"success":true}}'

# send the curl command and fetch the output json response
curl_response = Utils.send_curl_command(HdmiCecSourceApis.get_active_source_status)
if curl_response:
    Utils.info_log("curl command send for get_active_source_status")
else:
    Utils.error_log("curl command send failed for get_active_source_status")

print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if str(curl_response) == str(expected_output_response):
    status = 'Pass'
    message = 'Output response is matching with expected one. The active source status is true after OTP action'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# generate logs in terminal
tc_id = 'TCID036_getActiveSourceStatus_True'
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


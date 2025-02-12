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

# Testcase ID : TCID021_setApis_021_invalidDatatype
# Testcase Description : Verify the error response when unknown data type is given for set rpc calls.

from Utilities import Utils, ReportGenerator
from FrontPanel import FrontPanelApis

print("TC Description - Verify the error response when unknown data type is given for set rpc calls..")
print("---------------------------------------------------------------------------------------------------------------------------")
# store the expected output response
expected_output_response = '{"jsonrpc":"2.0","id":42,"error":{"code":1,"message":"ERROR_GENERAL"}}'

# send the curl command and fetch the output json response
curl_response = Utils.send_curl_command(FrontPanelApis.get_brightness_invalid )
if curl_response:
     Utils.info_log("curl command to get brightness is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed")



curl_response = Utils.send_curl_command(FrontPanelApis.set_brightness_invalid)
if curl_response:
     Utils.info_log("curl command to set the brightness is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed")

curl_response = Utils.send_curl_command(FrontPanelApis.power_led_on_invalid)
if curl_response:
     Utils.info_log("curl command to set the powerled is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed")

 
print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if str(curl_response) == str(expected_output_response):
    status = 'Pass'
    message = 'Output response is matching with expected one. The preferences list ' \
              'is obtained in output response'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# generate logs in terminal
tc_id = 'TCID021_DS_FrontPanel_powerLedsInvalid'
print("Testcase ID : " + tc_id)
print("Testcase Output Response : " + curl_response)
print("Testcase Status : " + status)
print("Testcase Message : " + message)
print("")

if status == 'Pass':
    ReportGenerator.passed_tc_list.append(tc_id)
else:
    ReportGenerator.failed_tc_list.append(tc_id)
# push the testcase execution details to report file
ReportGenerator.append_test_results_to_csv(tc_id, curl_response, status, message)

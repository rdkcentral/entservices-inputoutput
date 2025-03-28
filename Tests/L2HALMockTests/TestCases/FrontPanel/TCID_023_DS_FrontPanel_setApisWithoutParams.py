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

# Testcase ID : TCID023_setGetcallsWithoutParamsNegative
# Testcase Description :All json RPC calls which has parameter dependencies in its curl command input has been passed without parameters here, inorder to cover the loggers and exceptions

from Utilities import Utils, ReportGenerator
from FrontPanel import FrontPanelApis

print("TC Description -All json RPC calls which has parameter dependencies in its curl command input has been passed without parameters here, inorder to cover the loggers and exceptions")
print("---------------------------------------------------------------------------------------------------------------------------")
# store the expected output response
expected_output_response = '{"jsonrpc":"2.0","id":42,"error":{"code":1,"message":"ERROR_GENERAL"}}'

# send the curl command and fetch the output json response
curl_response = Utils.send_curl_command(FrontPanelApis.set_brightness_withoutParams)
if curl_response:
     Utils.info_log("curl command to set brightness without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_brightness_withoutParams))

curl_response = Utils.send_curl_command(FrontPanelApis.get_brightness_withoutParams)
if curl_response:
     Utils.info_log("curl command to get brightness without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.get_brightness_withoutParams))


curl_response = Utils.send_curl_command(FrontPanelApis.set_powerLedOff_withoutParams)
if curl_response:
     Utils.info_log("curl command to set powerLed off without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.powerLedOff_withoutParams))
 
curl_response = Utils.send_curl_command(FrontPanelApis.set_powerLedOn_withoutParams)
if curl_response:
     Utils.info_log("curl command to set powerLed on is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_powerLedOn_withoutParams))

curl_response = Utils.send_curl_command(FrontPanelApis.set_24HourClockWithoutParams)
if curl_response:
     Utils.info_log("curl command to set 24 hour clock without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_24HourClockWithoutParams))

curl_response = Utils.send_curl_command(FrontPanelApis.set_blinkWithoutParams)
if curl_response:
     Utils.info_log("curl command to set blink without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_blinkWithoutParams))


curl_response = Utils.send_curl_command(FrontPanelApis.set_clockBrightnesswithoutParams)
if curl_response:
     Utils.info_log("curl command to set clock brightness without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_clockBrightnesswithoutParams))

curl_response = Utils.send_curl_command(FrontPanelApis.set_ledWithoutParams)
if curl_response:
     Utils.info_log("curl command to set clock brightness without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_ledWithoutParams))


curl_response = Utils.send_curl_command(FrontPanelApis.set_preferencesWithoutParams)
if curl_response:
     Utils.info_log("curl command to set preferences without params is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.set_preferencesWithoutParams))


print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if str(curl_response) == str(expected_output_response):
    status = 'Pass'
    message = 'Output response is matching with expected one. Error general with exceptions ' \
              'are obtained in output response'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# generate logs in terminal
tc_id = 'TCID023_setGetcallsWithoutParamsNegative'
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

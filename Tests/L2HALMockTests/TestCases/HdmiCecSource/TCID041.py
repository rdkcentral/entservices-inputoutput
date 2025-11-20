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

# Testcase ID : TCID041
# Testcase Description : Set the OTP enabled with missing parameter and verify that
# error response is obtained

from Utilities import Utils, ReportGenerator
from HdmiCecSource import HdmiCecSourceApis

print("TC Description - Set the OTP enabled with missing parameter and verify that error response is obtained")
Utils.initiliaze_flask_for_HdmiCecSource()
print("---------------------------------------------------------------------------------------------------------------------------")

# send the curl command to set the OTP enabled with missing parameter
set_response = Utils.send_curl_command(HdmiCecSourceApis.set_otp_enabled_invalid)
if set_response:
    Utils.info_log("curl command sent for setting the OTP enabled with missing parameter")
else:
    Utils.error_log("curl command failed for setting the OTP enabled with missing parameter")
print("")

# store the expected output response (should return error)
expected_output_response = '{"jsonrpc":"2.0","id":42,"error":{"code":'

# check if error is present in response
if expected_output_response in str(set_response):
    status = 'Pass'
    message = 'Output response contains error as expected when invalid parameter is provided'
else:
    status = 'Fail'
    message = 'Output response does not contain expected error'

print("---------------------------------------------------------------------------------------------------------------------------")
# generate logs in terminal
tc_id = 'TCID041_setOTPEnabled_MissingParameter'
print("Testcase ID : " + tc_id)
print("Testcase Output Response : " + str(set_response))
print("Testcase Status : " + status)
print("Testcase Message : " + message)
print("")

if status == 'Pass':
    ReportGenerator.passed_tc_list.append(tc_id)
else:
    ReportGenerator.failed_tc_list.append(tc_id)
Utils.initiliaze_flask_for_HdmiCecSource()
# push the testcase execution details to report file
ReportGenerator.append_test_results_to_csv(tc_id, str(set_response), status, message)


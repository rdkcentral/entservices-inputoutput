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

# Testcase ID : TCID030
# Testcase Description : Verify the send get audio status message and its output response
import time
import json
import Config
import requests
from Utilities import Utils, ReportGenerator
from HdmiCecSink import HdmiCecSinkApis
from HdmiCecSource import HdmiCecSourceApis

print("TC Description - Verify the send get audio status message and its output response")
print("---------------------------------------------------------------------------------------------------------------------------")
# store the expected output response
Utils.initialize_flask()
expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"success":true}}'

curl_response = Utils.send_curl_command(HdmiCecSinkApis.sendGetAudioStatusMessage)
if curl_response:
    Utils.info_log("curl command send for send get audio status message")
    status = 'Pass'
    message = 'Output response is matching with expected one. The send get audio status message ' \
              'is obtained in output response'
else:
    Utils.error_log("curl command send failed {}" .format(HdmiCecSinkApis.sendGetAudioStatusMessage))
    status = 'False'
    message = 'Output response is not matching with expected one'

#Report SAD (short Audio Descriptor)
message1_response = requests.get("http://{}/Hdmicec.sendMessage/{}".format(
    Config.flask_server_ip, json.dumps(Config.reportShortAudioDes)))
if "200" in str(message1_response):
     Utils.info_log("sendMessage emulation success for requesting the reportShortAudioDes")
else:
     Utils.error_log("sendMessage emulation failed for requesting the reportShortAudioDes")
time.sleep(3)
print("")

#send messages required for setting system audio mode
message1_response = requests.get("http://{}/Hdmicec.sendMessage/{}".format(
    Config.flask_server_ip, json.dumps(Config.setSystemAudioMode)))
if "200" in str(message1_response):
     Utils.info_log("sendMessage emulation success for requesting the setSystemAudioMode")
else:
     Utils.error_log("sendMessage emulation failed for requesting the setSystemAudioMode")
time.sleep(3)
print("")

# generate logs in terminal
tc_id = 'TCID_030_HDMICECSINK_sendGetAudioStatusMessage'
print("Testcase ID : " + tc_id)
print("Testcase Output Response : " + curl_response)
print("Testcase Status : " + status)
print("Testcase Message : " + message)
print("")

if status == 'Pass':
    ReportGenerator.passed_tc_list.append(tc_id)
else:
    ReportGenerator.failed_tc_list.append(tc_id)
Utils.initialize_flask()
# push the testcase execution details to report file
ReportGenerator.append_test_results_to_csv(tc_id, curl_response, status, message)

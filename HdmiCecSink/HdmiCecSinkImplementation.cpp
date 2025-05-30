/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include "HdmiCecSinkImplementation.h"

#include "ccec/Connection.hpp"
#include "ccec/CECFrame.hpp"
#include "ccec/MessageEncoder.hpp"
#include "host.hpp"
#include "UtilsgetRFCConfig.h"

#include "dsMgr.h"
#include "dsRpc.h"
#include "dsDisplay.h"
#include "videoOutputPort.hpp"
#include "manager.hpp"
#include "websocket/URL.h"

#include "UtilsIarm.h"
#include "UtilsJsonRpc.h"
#include "UtilssyncPersistFile.h"
#include "UtilsSearchRDKProfile.h"

#define TEST_ADD 0
#define HDMICECSINK_REQUEST_MAX_RETRY                 3
#define HDMICECSINK_REQUEST_MAX_WAIT_TIME_MS         2000
#define HDMICECSINK_PING_INTERVAL_MS                 10000
#define HDMICECSINK_WAIT_FOR_HDMI_IN_MS             1000
#define HDMICECSINK_REQUEST_INTERVAL_TIME_MS         500
#define HDMICECSINK_NUMBER_TV_ADDR                     2
#define HDMICECSINK_UPDATE_POWER_STATUS_INTERVA_MS    (60 * 1000)
#define HDMISINK_ARC_START_STOP_MAX_WAIT_MS           4000
#define HDMICECSINK_UPDATE_AUDIO_STATUS_INTERVAL_MS    500


#define SAD_FMT_CODE_AC3 2
#define SAD_FMT_CODE_ENHANCED_AC3 10

#define SYSTEM_AUDIO_MODE_ON 0x01
#define SYSTEM_AUDIO_MODE_OFF 0x00
#define AUDIO_DEVICE_POWERSTATE_OFF 1

#define DEFAULT_VIDEO_LATENCY 100
#define DEFAULT_LATENCY_FLAGS 3
#define DEFAULT_AUDIO_OUTPUT_DELAY 100

//Device Type is TV - Bit 7 is set to 1
#define ALL_DEVICE_TYPES  128

//RC Profile of TV is 3 - Typical TV Remote
#define RC_PROFILE_TV 10

//Device Features supported by TV - ARC Tx
#define DEVICE_FEATURES_TV 4

#define TR181_HDMICECSINK_CEC_VERSION "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.HdmiCecSink.CECVersion"


#define CEC_SETTING_ENABLED_FILE "/opt/persistent/ds/cecData_2.json"
#define CEC_SETTING_OTP_ENABLED "cecOTPEnabled"
#define CEC_SETTING_ENABLED "cecEnabled"
#define CEC_SETTING_OSD_NAME "cecOSDName"
#define CEC_SETTING_VENDOR_ID "cecVendorId"

static std::vector<uint8_t> defaultVendorId = {0x00,0x19,0xFB};
static VendorID appVendorId = {defaultVendorId.at(0),defaultVendorId.at(1),defaultVendorId.at(2)};
static VendorID lgVendorId = {0x00,0xE0,0x91};
static PhysicalAddress physical_addr = {0x0F,0x0F,0x0F,0x0F};
static LogicalAddress logicalAddress = 0xF;
static Language defaultLanguage = "eng";
static OSDName osdName = "TV Box";
static int32_t powerState = DEVICE_POWER_STATE_OFF;
static std::vector<uint8_t> formatid = {0,0};
static std::vector<uint8_t> audioFormatCode = { SAD_FMT_CODE_ENHANCED_AC3,SAD_FMT_CODE_AC3 };
static uint8_t numberofdescriptor = 2;
static int32_t HdmiArcPortID = -1;
static float cecVersion = 1.4;
static AllDeviceTypes allDevicetype = ALL_DEVICE_TYPES;
static std::vector<RcProfile> rcProfile = {RC_PROFILE_TV};
static std::vector<DeviceFeatures> deviceFeatures = {DEVICE_FEATURES_TV};

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 3
#define API_VERSION_NUMBER_PATCH 7

using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

namespace WPEFramework
{

    namespace Plugin
    {
        SERVICE_REGISTRATION(HdmiCecSinkImplementation, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        HdmiCecSinkImplementation* HdmiCecSinkImplementation::_instance = nullptr;
        static int libcecInitStatus = 0;

//=========================================== HdmiCecSinkFrameListener =========================================
        void HdmiCecSinkFrameListener::notify(const CECFrame &in) const {
                const uint8_t *buf = NULL;
                char strBuffer[512] = {0}; 
                size_t len = 0;

                in.getBuffer(&buf, &len);
                for (unsigned int i = 0; i < len; i++) {
                   snprintf(strBuffer + (i*3) , sizeof(strBuffer) - (i*3), "%02X ",(uint8_t) *(buf + i));
                }
                LOGINFO("   >>>>>    Received CEC Frame: :%s \n",strBuffer);

                MessageDecoder(processor).decode(in);
       }

//=========================================== HdmiCecSinkProcessor =========================================
       void HdmiCecSinkProcessor::process (const ActiveSource &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: ActiveSource %s : %s  : %s \n",GetOpName(msg.opCode()),msg.physicalAddress.name().c_str(),msg.physicalAddress.toString().c_str());
         if(!(header.to == LogicalAddress(LogicalAddress::BROADCAST))){
        LOGINFO("Ignore Direct messages, accepts only broadcast messages");
        return;
         }
             HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());    
             HdmiCecSinkImplementation::_instance->updateActiveSource(header.from.toInt(), msg);
      }
       void HdmiCecSinkProcessor::process (const InActiveSource &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: InActiveSource %s : %s : %s  \n",GetOpName(msg.opCode()),msg.physicalAddress.name().c_str(),msg.physicalAddress.toString().c_str());
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }

             HdmiCecSinkImplementation::_instance->updateInActiveSource(header.from.toInt(), msg);
       }

       void HdmiCecSinkProcessor::process (const ImageViewOn &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: ImageViewOn from %s\n", header.from.toString().c_str());
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
             HdmiCecSinkImplementation::_instance->updateImageViewOn(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const TextViewOn &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: TextViewOn\n");
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
             HdmiCecSinkImplementation::_instance->updateTextViewOn(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const RequestActiveSource &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: RequestActiveSource\n");
         if(!(header.to == LogicalAddress(LogicalAddress::BROADCAST))){
        LOGINFO("Ignore Direct messages, accepts only broadcast messages");
        return;
         }

             HdmiCecSinkImplementation::_instance->setActiveSource(true);
       }
       void HdmiCecSinkProcessor::process (const Standby &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: Standby from %s\n", header.from.toString().c_str());
             HdmiCecSinkImplementation::_instance->SendStandbyMsgEvent(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const GetCECVersion &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: GetCECVersion sending CECVersion response \n");
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             try
             { 
                 if(cecVersion == 2.0) {
             conn.sendToAsync(header.from, MessageEncoder().encode(CECVersion(Version::V_2_0)));
         }
         else{
             conn.sendToAsync(header.from, MessageEncoder().encode(CECVersion(Version::V_1_4)));
         }
         }
             catch(...)
             {
                 LOGWARN("Exception while sending CECVersion ");
             }
       }
       void HdmiCecSinkProcessor::process (const CECVersion &msg, const Header &header)
       {
         bool updateStatus;
         printHeader(header);
             LOGINFO("Command: CECVersion Version : %s \n",msg.version.toString().c_str());

         HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
         updateStatus = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isVersionUpdated;
             LOGINFO("updateStatus %d\n",updateStatus);
         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(msg.version);
         if(!updateStatus)
         HdmiCecSinkImplementation::_instance->sendDeviceUpdateInfo(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const SetMenuLanguage &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: SetMenuLanguage Language : %s \n",msg.language.toString().c_str());
       }
       void HdmiCecSinkProcessor::process (const GiveOSDName &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: GiveOSDName sending SetOSDName : %s\n",osdName.toString().c_str());
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             try
             { 
                 conn.sendToAsync(header.from, MessageEncoder().encode(SetOSDName(osdName)));
             } 
             catch(...)
             {
                 LOGWARN("Exception while sending SetOSDName");
             }
       }
       void HdmiCecSinkProcessor::process (const GivePhysicalAddress &msg, const Header &header)
       {
             LOGINFO("Command: GivePhysicalAddress\n");
             if (!(header.to == LogicalAddress(LogicalAddress::BROADCAST)))
             {
                 try
                 { 
                     LOGINFO(" sending ReportPhysicalAddress response physical_addr :%s logicalAddress :%x \n",physical_addr.toString().c_str(), logicalAddress.toInt());
                     conn.sendTo(LogicalAddress(LogicalAddress::BROADCAST), MessageEncoder().encode(ReportPhysicalAddress(physical_addr,logicalAddress.toInt())), 500);
                 } 
                 catch(...)
                 {
                    LOGWARN("Exception while sending ReportPhysicalAddress ");
                 }
             }
       }
       void HdmiCecSinkProcessor::process (const GiveDeviceVendorID &msg, const Header &header)
       {
             printHeader(header);
         if(header.to == LogicalAddress(LogicalAddress::BROADCAST)){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             try
             {
                 LOGINFO("Command: GiveDeviceVendorID sending VendorID response :%s\n",appVendorId.toString().c_str());
                 conn.sendToAsync(LogicalAddress(LogicalAddress::BROADCAST), MessageEncoder().encode(DeviceVendorID(appVendorId)));
             }
             catch(...)
             {
                 LOGWARN("Exception while sending DeviceVendorID");
             }

       }
       void HdmiCecSinkProcessor::process (const SetOSDString &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: SetOSDString OSDString : %s\n",msg.osdString.toString().c_str());
       }
       void HdmiCecSinkProcessor::process (const SetOSDName &msg, const Header &header)
       {
             printHeader(header);
         bool updateStatus ;
             LOGINFO("Command: SetOSDName OSDName : %s\n",msg.osdName.toString().c_str());
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }

         HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
         updateStatus = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isOSDNameUpdated;
         LOGINFO("updateStatus %d\n",updateStatus);
         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(msg.osdName);
         if(HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isRequestRetry > 0 &&
            HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isRequested == CECDeviceParams::REQUEST_OSD_NAME) {
             HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isRequestRetry = 0;
         }
             if(!updateStatus)
         HdmiCecSinkImplementation::_instance->sendDeviceUpdateInfo(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const RoutingChange &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: RoutingChange From : %s To: %s \n",msg.from.toString().c_str(),msg.to.toString().c_str());
       }
       void HdmiCecSinkProcessor::process (const RoutingInformation &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: RoutingInformation Routing Information to Sink : %s\n",msg.toSink.toString().c_str());
       }
       void HdmiCecSinkProcessor::process (const SetStreamPath &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: SetStreamPath Set Stream Path to Sink : %s\n",msg.toSink.toString().c_str());
       }
       void HdmiCecSinkProcessor::process (const GetMenuLanguage &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: GetMenuLanguage\n");
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             HdmiCecSinkImplementation::_instance->sendMenuLanguage();  
       }
       void HdmiCecSinkProcessor::process (const ReportPhysicalAddress &msg, const Header &header)
       {
             printHeader(header);
         bool updateDeviceTypeStatus;
         bool updatePAStatus;
             LOGINFO("Command: ReportPhysicalAddress\n");
         if(!(header.to == LogicalAddress(LogicalAddress::BROADCAST))){
        LOGINFO("Ignore Direct messages, accepts only broadcast messages");
        return;
         }

         if(!HdmiCecSinkImplementation::_instance)
            return;
             HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
         updateDeviceTypeStatus = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isDeviceTypeUpdated;
             updatePAStatus   = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isPAUpdated;
         LOGINFO("updateDeviceTypeStatus %d updatePAStatus %d \n",updateDeviceTypeStatus,updatePAStatus);
         if(HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_physicalAddr.toString() != msg.physicalAddress.toString() && updatePAStatus){
                updatePAStatus= false;
                LOGINFO("There is a change in physical address from current PA %s to newly reported PA %s\n",HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_physicalAddr.toString().c_str(),msg.physicalAddress.toString().c_str());
             }
         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(msg.physicalAddress);
         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(msg.deviceType);
         if(HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isRequestRetry > 0 &&
            HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isRequested == CECDeviceParams::REQUEST_PHISICAL_ADDRESS) {
             HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isRequestRetry = 0;
         }
         HdmiCecSinkImplementation::_instance->updateDeviceChain(header.from, msg.physicalAddress);
         if (!updateDeviceTypeStatus || !updatePAStatus)
             HdmiCecSinkImplementation::_instance->sendDeviceUpdateInfo(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const DeviceVendorID &msg, const Header &header)
       {
         bool updateStatus ;
         printHeader(header);
             LOGINFO("Command: DeviceVendorID VendorID : %s\n",msg.vendorId.toString().c_str());
         if(!(header.to == LogicalAddress(LogicalAddress::BROADCAST))){
        LOGINFO("Ignore Direct messages, accepts only broadcast messages");
        return;
         }

         HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
         updateStatus = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_isVendorIDUpdated;
             LOGINFO("updateStatus %d\n",updateStatus);
         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(msg.vendorId);
         if (!updateStatus)
             HdmiCecSinkImplementation::_instance->sendDeviceUpdateInfo(header.from.toInt());
       }
       void HdmiCecSinkProcessor::process (const GiveDevicePowerStatus &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: GiveDevicePowerStatus sending powerState :%d \n",powerState);
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             try
             { 
                 conn.sendTo(header.from, MessageEncoder().encode(ReportPowerStatus(PowerStatus(powerState))));
             } 
             catch(...)
             {
                 LOGWARN("Exception while sending ReportPowerStatus");
             }
       }
       void HdmiCecSinkProcessor::process (const ReportPowerStatus &msg, const Header &header)
       {
       uint32_t  oldPowerStatus,newPowerStatus;
       printHeader(header);
       LOGINFO("Command: ReportPowerStatus Power Status from:%s status : %s \n",header.from.toString().c_str(),msg.status.toString().c_str());
       if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
           oldPowerStatus = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_powerStatus.toInt();
       HdmiCecSinkImplementation::_instance->addDevice(header.from.toInt());
       HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(msg.status);
       newPowerStatus = HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_powerStatus.toInt();
       LOGINFO(" oldPowerStatus %d newpower status %d \n",oldPowerStatus,newPowerStatus);
           if ((oldPowerStatus != newPowerStatus) )
       {
           HdmiCecSinkImplementation::_instance->sendDeviceUpdateInfo(header.from.toInt());
       }

           if((header.from.toInt() == LogicalAddress::AUDIO_SYSTEM) && (HdmiCecSinkImplementation::_instance->m_audioDevicePowerStatusRequested)) {
               HdmiCecSinkImplementation::_instance->reportAudioDevicePowerStatusInfo(header.from.toInt(), newPowerStatus);
           }

       }
       void HdmiCecSinkProcessor::process (const FeatureAbort &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: FeatureAbort opcode=%s, Reason = %s\n", msg.feature.toString().c_str(), msg.reason.toString().c_str());
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }

             if(header.from.toInt() < LogicalAddress::UNREGISTERED &&    
                     msg.reason.toInt()   == AbortReason::UNRECOGNIZED_OPCODE)
             {
                 switch(msg.feature.opCode())
                 {
                     case GET_CEC_VERSION :
                     {
                         /* If we get a Feature abort for CEC Version then default to 1.4b */
                         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(Version(Version::V_1_4));
                     }
                        break;
                    case GIVE_DEVICE_VENDOR_ID :
                     {
                         /* If we get a Feature abort for CEC Version then default to 1.4b */
                         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(VendorID((uint8_t *)"FA", 2));
                     }
                        break;

                    case GIVE_OSD_NAME :
                     {
                         HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(OSDName(""));
                     }
                        break;

                    case GIVE_DEVICE_POWER_STATUS :
                    {
                        HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].update(PowerStatus(PowerStatus::POWER_STATUS_FEATURE_ABORT));
                    }
                        break;
                 }

                HdmiCecSinkImplementation::_instance->deviceList[header.from.toInt()].m_featureAborts.push_back(msg);
             }

            LogicalAddress logicaladdress = header.from.toInt();
                        OpCode featureOpcode =  msg.feature;
            AbortReason abortReason = msg.reason;

                        HdmiCecSinkImplementation::_instance->reportFeatureAbortEvent(logicaladdress,featureOpcode,abortReason);

                         if(msg.feature.opCode() == REQUEST_SHORT_AUDIO_DESCRIPTOR)
                 {
                            JsonArray audiodescriptor;
                            audiodescriptor.Add(0);
                HdmiCecSinkImplementation::_instance->Send_ShortAudioDescriptor_Event(audiodescriptor);
                        }

       }
       void HdmiCecSinkProcessor::process (const Abort &msg, const Header &header)
       {
              printHeader(header);
             LOGINFO("Command: Abort\n");
          if (!(header.to == LogicalAddress(LogicalAddress::BROADCAST)))
             {
                AbortReason reason = AbortReason::UNRECOGNIZED_OPCODE;
                LogicalAddress logicaladdress =header.from.toInt();
                OpCode feature = msg.opCode();
                HdmiCecSinkImplementation::_instance->sendFeatureAbort(logicaladdress, feature,reason);
         }
         else
         {
        LOGINFO("Command: Abort broadcast msg so ignore\n");
         }
       }
       void HdmiCecSinkProcessor::process (const Polling &msg, const Header &header)                                 {
             printHeader(header);
             LOGINFO("Command: Polling\n");
       }

       void HdmiCecSinkProcessor::process (const InitiateArc &msg, const Header &header)
       {
            printHeader(header);
        if((!(header.from.toInt() == 0x5)) || (header.to.toInt() == LogicalAddress::BROADCAST)){
                LOGINFO("Ignoring the message coming from addresses other than 0X5 or a braodcast message");
                return;
            }
            PhysicalAddress physical_addr_invalid = {0x0F,0x0F,0x0F,0x0F};
            PhysicalAddress physical_addr_arc_port = {0x0F,0x0F,0x0F,0x0F};

            LOGINFO("Command: INITIATE_ARC \n");
            if(!HdmiCecSinkImplementation::_instance || HdmiArcPortID == -1)
        return;

            if (HdmiArcPortID == 0 )
               physical_addr_arc_port = {0x01,0x00,0x00,0x00};
            if (HdmiArcPortID == 1 )
               physical_addr_arc_port = {0x02,0x00,0x00,0x00};
            if (HdmiArcPortID == 2 )
               physical_addr_arc_port = {0x03,0x00,0x00,0x00};

            if( (HdmiCecSinkImplementation::_instance->deviceList[0x5].m_physicalAddr.toString() == physical_addr_arc_port.toString()) || (HdmiCecSinkImplementation::_instance->deviceList[0x5].m_physicalAddr.toString() == physical_addr_invalid.toString()) ) {
                LOGINFO("Command: INITIATE_ARC InitiateArc success %s \n",HdmiCecSinkImplementation::_instance->deviceList[0x5].m_physicalAddr.toString().c_str());
                HdmiCecSinkImplementation::_instance->Process_InitiateArc();
            } else {
                LOGINFO("Command: INITIATE_ARC InitiateArc ignore %s \n",HdmiCecSinkImplementation::_instance->deviceList[0x5].m_physicalAddr.toString().c_str());
            }
       }
       void HdmiCecSinkProcessor::process (const TerminateArc &msg, const Header &header)
       {
           printHeader(header);
       if((!(header.from.toInt() == 0x5)) || (header.to.toInt() == LogicalAddress::BROADCAST)){
            LOGINFO("Ignoring the message coming from addresses other than 0X5 or a braodcast message");
                return;
           }
           if(!HdmiCecSinkImplementation::_instance)
         return;
           HdmiCecSinkImplementation::_instance->Process_TerminateArc();
       }
       void HdmiCecSinkProcessor::process (const ReportShortAudioDescriptor  &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: ReportShortAudioDescriptor %s : %d \n",GetOpName(msg.opCode()),numberofdescriptor);
            HdmiCecSinkImplementation::_instance->Process_ShortAudioDescriptor_msg(msg);
       }

       void HdmiCecSinkProcessor::process (const SetSystemAudioMode &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: SetSystemAudioMode  %s audio status %d audio status is  %s \n",GetOpName(msg.opCode()),msg.status.toInt(),msg.status.toString().c_str());
          HdmiCecSinkImplementation::_instance->Process_SetSystemAudioMode_msg(msg);
       }
      void HdmiCecSinkProcessor::process (const ReportAudioStatus &msg, const Header &header)
       {
             printHeader(header);
             LOGINFO("Command: ReportAudioStatus  %s audio Mute status %d  means %s  and current Volume level is %d \n",GetOpName(msg.opCode()),msg.status.getAudioMuteStatus(),msg.status.toString().c_str(),msg.status.getAudioVolume());
         if(header.to.toInt() == LogicalAddress::BROADCAST){
        LOGINFO("Ignore Broadcast messages, accepts only direct messages");
        return;
         }
             HdmiCecSinkImplementation::_instance->Process_ReportAudioStatus_msg(msg);
       }
      void HdmiCecSinkProcessor::process (const GiveFeatures &msg, const Header &header)
       {
            printHeader(header);
            LOGINFO("Command: GiveFeatures \n");
            try
            {
            if(cecVersion == 2.0) {
            conn.sendToAsync(LogicalAddress(LogicalAddress::BROADCAST),MessageEncoder().encode(ReportFeatures(Version::V_2_0,allDevicetype,rcProfile,deviceFeatures)));
        }
            }
            catch(...)
            {
                LOGWARN("Exception while sending ReportFeatures");
            }
       }
      void HdmiCecSinkProcessor::process (const RequestCurrentLatency &msg, const Header &header)
       {
         printHeader(header);
             LOGINFO("Command: Request Current Latency :%s, physical address: %s",GetOpName(msg.opCode()),msg.physicaladdress.toString().c_str());

         if(msg.physicaladdress.toString() == physical_addr.toString()) {
             HdmiCecSinkImplementation::_instance->setLatencyInfo();
         }
         else {
             LOGINFO("Physical Address does not match with TV's physical address");
             return;
         }
       }
//=========================================== HdmiCecSinkImplementation =========================================

       HdmiCecSinkImplementation::HdmiCecSinkImplementation()
       : PluginHost::JSONRPC()
        , _pwrMgrNotification(*this)
        , _registeredEventHandlers(false)
       {
           LOGWARN("Initlaizing HdmiCecSinkImplementation");
       }

       HdmiCecSinkImplementation::~HdmiCecSinkImplementation()
       {
            if(_powerManagerPlugin)
            {
               _powerManagerPlugin->Unregister(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
               _powerManagerPlugin.Reset();
            }
            _registeredEventHandlers = false;
        
         profileType = searchRdkProfile();
        
         if (profileType == STB || profileType == NOT_FOUND)
         {
             LOGINFO("Invalid profile type for TV \n");
             return ;
         }
     
         CECDisable();
         m_currentArcRoutingState = ARC_STATE_ARC_EXIT;
     
             m_semSignaltoArcRoutingThread.release();
     
             try
         {
         if (m_arcRoutingThread.joinable())
             m_arcRoutingThread.join();
         }
         catch(const std::system_error& e)
         {
         LOGERR("system_error exception in thread join %s", e.what());
         }
         catch(const std::exception& e)
         {
         LOGERR("exception in thread join %s", e.what());
         }
     
         {
             m_sendKeyEventThreadExit = true;
                 std::unique_lock<std::mutex> lk(m_sendKeyEventMutex);
                 m_sendKeyEventThreadRun = true;
                 m_sendKeyCV.notify_one();
             }
         
         try
         {
             if (m_sendKeyEventThread.joinable())
                 m_sendKeyEventThread.join();
         }
         catch(const std::system_error& e)
         {
             LOGERR("system_error exception in thread join %s", e.what());
         }
         catch(const std::exception& e)
         {
             LOGERR("exception in thread join %s", e.what());
         }
     
             HdmiCecSinkImplementation::_instance = nullptr;
             DeinitializeIARM();
       }
       const std::string HdmiCecSinkImplementation::Configure(PluginHost::IShell *service)
       {
            InitializePowerManager(service);
           profileType = searchRdkProfile();

           if (profileType == STB || profileType == NOT_FOUND)
           {
               LOGINFO("Invalid profile type for TV \n");
               return (std::string("Not supported"));
           }

           HdmiCecSinkImplementation::_instance = this;
           smConnection=NULL;
           cecEnableStatus = false;
           HdmiCecSinkImplementation::_instance->m_numberOfDevices = 0;
           m_logicalAddressAllocated = LogicalAddress::UNREGISTERED;
           m_currentActiveSource = -1;
           m_isHdmiInConnected = false;
           hdmiCecAudioDeviceConnected = false;
           m_isAudioStatusInfoUpdated = false;
		   m_audioStatusReceived = false;
		   m_audioStatusTimerStarted = false;
           m_audioDevicePowerStatusRequested = false;
           m_pollNextState = POLL_THREAD_STATE_NONE;
           m_pollThreadState = POLL_THREAD_STATE_NONE;
           m_video_latency = DEFAULT_VIDEO_LATENCY;
           m_latency_flags = DEFAULT_LATENCY_FLAGS ;
           m_audio_output_delay = DEFAULT_AUDIO_OUTPUT_DELAY;

           logicalAddressDeviceType = "None";
           logicalAddress = 0xFF;
           // load persistence setting
           loadSettings();

           int err;
           dsHdmiInGetNumberOfInputsParam_t hdmiInput;
           InitializeIARM();
           m_sendKeyEventThreadExit = false;
           m_sendKeyEventThread = std::thread(threadSendKeyEvent);

           m_currentArcRoutingState = ARC_STATE_ARC_TERMINATED;
           m_semSignaltoArcRoutingThread.acquire();
           m_arcRoutingThread = std::thread(threadArcRouting);

           m_audioStatusDetectionTimer.connect( std::bind( &HdmiCecSinkImplementation::audioStatusTimerFunction, this ) );
	       m_audioStatusDetectionTimer.setSingleShot(true);
           m_arcStartStopTimer.connect( std::bind( &HdmiCecSinkImplementation::arcStartStopTimerFunction, this ) );
           m_arcStartStopTimer.setSingleShot(true);

            // get power state:
            uint32_t res = Core::ERROR_GENERAL;
            PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
            PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;

            ASSERT (_powerManagerPlugin);
            if (_powerManagerPlugin) {
                res = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
                if (Core::ERROR_NONE == res) {
                    powerState = (pwrStateCur == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON) ? DEVICE_POWER_STATE_ON : DEVICE_POWER_STATE_OFF;
                    LOGINFO("Current state is PowerManagerPlugin: (%d) powerState :%d \n", pwrStateCur, powerState);
                }
            }

            err = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            IARM_BUS_DSMGR_API_dsHdmiInGetNumberOfInputs,
                            (void *)&hdmiInput,
                            sizeof(hdmiInput));

           if (err == IARM_RESULT_SUCCESS && hdmiInput.result == dsERR_NONE)
           {
                LOGINFO("Number of Inputs [%d] \n", hdmiInput.numHdmiInputs );
                m_numofHdmiInput = hdmiInput.numHdmiInputs;
           }else{
                LOGINFO("Not able to get Numebr of inputs so defaulting to 3 \n");
                m_numofHdmiInput = 3;
            }

            LOGINFO("initalize inputs \n");

           for (int i = 0; i < m_numofHdmiInput; i++){
                HdmiPortMap hdmiPort((uint8_t)i);
                LOGINFO(" Add to vector [%d] \n", i);
                hdmiInputs.push_back(hdmiPort);
            }

            LOGINFO("Check the HDMI State \n");

            CheckHdmiInState();
            if (cecSettingEnabled)
            {
               try
               {
                   CECEnable();
               }
               catch(...)
               {
                   LOGWARN("Exception while enabling CEC settings .\r\n");
               }
            }
            getCecVersion();
            LOGINFO(" HdmiCecSinkImplementation plugin Initialize completed \n");
            return (std::string());

       }

       uint32_t HdmiCecSinkImplementation::Register(Exchange::IHdmiCecSink::INotification* notification)
        {
            LOGINFO("Register");
            if(notification != nullptr){
                _adminLock.Lock();
                if(std::find(_hdmiCecSinkNotifications.begin(), _hdmiCecSinkNotifications.end(), notification) == _hdmiCecSinkNotifications.end())
                {
                    _hdmiCecSinkNotifications.push_back(notification);
                    notification->AddRef();
                }
                else
                {
                    LOGERR("Same notification is registered already");
                }
                _adminLock.Unlock();
            }

            return Core::ERROR_NONE;
        }

        uint32_t HdmiCecSourceImplementation::Unregister(Exchange::IHdmiCecSink::INotification* notification)
        {
            LOGINFO("Unregister");
            if(notification != nullptr){
                _adminLock.Lock();
                std::list<Exchange::IHdmiCecSink::INotification*>::iterator index = std::find(_hdmiCecSinkNotifications.begin(), _hdmiCecSinkNotifications.end(), notification);
                if(index != _hdmiCecSinkNotifications.end())
                {
                    (*index)->Release();
                    _hdmiCecSinkNotifications.erase(index);
                }
                else
                {
                    LOGERR("Notification is not registered");
                }
                _adminLock.Unlock();
            }

            return Core::ERROR_NONE;
        }


       const void HdmiCecSinkImplementation::InitializeIARM()
       {
            if (Utils::IARM::init())
            {
                IARM_Result_t res;
                IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG, dsHdmiEventHandler) );
           }
       }

       void HdmiCecSinkImplementation::DeinitializeIARM()
       {
            if (Utils::IARM::isConnected())
            {
                IARM_Result_t res;
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG, dsHdmiEventHandler) );
            }
       }

        void HdmiCecSinkImplementation::InitializePowerManager(PluginHost::IShell *service)
        {
            _powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
                                    .withIShell(service)
                                    .withRetryIntervalMS(200)
                                    .withRetryCount(25)
                                    .createInterface();
            registerEventHandlers();
        }
       void HdmiCecSinkImplementation::registerEventHandlers()
       {
           ASSERT (_powerManagerPlugin);

           if(!_registeredEventHandlers && _powerManagerPlugin) {
               _registeredEventHandlers = true;
               _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
           }
       }

       void HdmiCecSinkImplementation::dsHdmiEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
       {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if (IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG == eventId)
            {
                IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                bool isHdmiConnected = eventData->data.hdmi_in_connect.isPortConnected;
                dsHdmiInPort_t portId = eventData->data.hdmi_in_connect.port;
                LOGINFO("Received IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG event port: %d data:%d \r\n",portId,  isHdmiConnected);
                HdmiCecSinkImplementation::_instance->onHdmiHotPlug(portId,isHdmiConnected);
            }
       }

       void HdmiCecSinkImplementation::onPowerModeChanged(const PowerState &currentState, const PowerState &newState)
       {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            LOGINFO("Event IARM_BUS_PWRMGR_EVENT_MODECHANGED: State Changed %d -- > %d\r",
                    currentState, newState);
            LOGWARN(" m_logicalAddressAllocated 0x%x CEC enable status %d \n",_instance->m_logicalAddressAllocated,_instance->cecEnableStatus);
            if(newState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON)
            {
                powerState = DEVICE_POWER_STATE_ON; 
            }
            else
            {
                    powerState = DEVICE_POWER_STATE_OFF;
                    if((_instance->m_currentArcRoutingState == ARC_STATE_REQUEST_ARC_INITIATION) || (_instance->m_currentArcRoutingState == ARC_STATE_ARC_INITIATED))
                    {
                        LOGINFO("%s: Stop ARC \n",__FUNCTION__);
                        _instance->stopArc();
            }

                }
                if (_instance->cecEnableStatus)
            {
            if ( _instance->m_logicalAddressAllocated != LogicalAddress::UNREGISTERED )
            {
                _instance->deviceList[_instance->m_logicalAddressAllocated].m_powerStatus = PowerStatus(powerState);

                if ( powerState != DEVICE_POWER_STATE_ON )
                {
                   /*  reset the current active source when TV on going to standby */
                                   HdmiCecSinkImplementation::_instance->m_currentActiveSource = -1;
                }
                                        /* Initiate a ping straight away */
                                        HdmiCecSinkImplementation::_instance->m_pollNextState = POLL_THREAD_STATE_PING;
                                        HdmiCecSinkImplementation::_instance->m_ThreadExitCV.notify_one();
            }
            }
            else
            {
            LOGWARN("CEC not Enabled\n");
            }
       }


      void HdmiCecSinkImplementation::sendStandbyMessage()
      {
              if(!HdmiCecSinkImplementation::_instance)
                return;
          if(!(HdmiCecSinkImplementation::_instance->smConnection))
              return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED){
                LOGERR("Logical Address NOT Allocated Or its not valid");
                return;
            }

            _instance->smConnection->sendTo(LogicalAddress(LogicalAddress::BROADCAST), MessageEncoder().encode(Standby()), 1000);
       }

       void HdmiCecSinkImplementation::onHdmiHotPlug(int portId , int connectStatus)
       {
            LOGINFO("onHdmiHotPlug Status : %d ", connectStatus);
                        if(!connectStatus)
                        {
                            LOGINFO(" removeDevice port: %d Logical address :%d  \r\n",portId,hdmiInputs[portId].m_logicalAddr.toInt() );
                            _instance->removeDevice(hdmiInputs[portId].m_logicalAddr.toInt());
                        }
            CheckHdmiInState();

          if(cecEnableStatus) {
              LOGINFO("cecEnableStatus : %d Trigger CEC Ping !!! \n", cecEnableStatus);
              m_pollNextState = POLL_THREAD_STATE_PING;
              m_ThreadExitCV.notify_one();
          }
          if( HdmiArcPortID >= 0 ) {
              updateArcState();  
          }
          return;
       }
       void HdmiCecSinkImplementation::updateArcState()
       {
           if ( m_currentArcRoutingState != ARC_STATE_ARC_TERMINATED )
           {
            if (!(hdmiInputs[HdmiArcPortID].m_isConnected))
        {
                   std::lock_guard<std::mutex> lock(_instance->m_arcRoutingStateMutex);
           m_currentArcRoutingState = ARC_STATE_ARC_TERMINATED;
        }
                else
                {
                   LOGINFO("updateArcState :not updating ARC state current arc state %d ",m_currentArcRoutingState);
                }
           } 
      }
      void HdmiCecSinkImplementation::arcStartStopTimerFunction()
      {
        JsonObject params;

        if (m_arcstarting)
        {
            LOGINFO("arcStartStopTimerFunction ARC start timer expired");
            LOGINFO("notify_device setting that Initiate ARC failed to get the ARC_STATE_ARC_INITIATED state\n");
            params["status"] = string("failure");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ArcInitiationEvent("failure");
                index++;
            }
        }
        else
        {
            LOGINFO("arcStartStopTimerFunction ARC stop timer expired");
            LOGINFO("notify_device setting that Terminate  ARC failed to get the ARC_STATE_ARC_TERMINATED state\n");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ArcTerminationEvent("failure");
                index++;
            }
        }
        /* bring the state machine to the clean state for a new start */ 
        std::lock_guard<std::mutex> lock(_instance->m_arcRoutingStateMutex);
        m_currentArcRoutingState = ARC_STATE_ARC_TERMINATED;
      }
       void  HdmiCecSinkImplementation::Send_ShortAudioDescriptor_Event(JsonArray audiodescriptor)
       {

            LOGINFO("Notify the DS ");
            string shortAudioDescriptors;

            if (!audiodescriptor.ToString(shortAudioDescriptors)) {
                LOGERR("Failed to  stringify JsonArray");
            }
            
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ShortAudioDescriptorEvent(shortAudioDescriptors);
                index++;
            }
       }

       void HdmiCecSinkImplementation::Process_ShortAudioDescriptor_msg(const ReportShortAudioDescriptor  &msg)
       {
        uint8_t numberofdescriptor = msg.numberofdescriptor;
        uint32_t descriptor =0;
        JsonArray audiodescriptor;

        if (numberofdescriptor)
            {
         for( uint8_t  i=0; i < numberofdescriptor; i++)
            {
               descriptor = msg.shortAudioDescriptor[i].getAudiodescriptor();

           LOGINFO("descriptor%d 0x%x\n",i,descriptor);
           audiodescriptor.Add(descriptor);

        }
        }
        else
        {
            audiodescriptor.Add(descriptor);
        }
       HdmiCecSinkImplementation::_instance->Send_ShortAudioDescriptor_Event(audiodescriptor);
        }

       void HdmiCecSinkImplementation::updateCurrentLatency(int videoLatency, bool lowLatencyMode,int audioOutputCompensated, int audioOutputDelay = 0)
        {
        uint8_t latencyFlags = 0;
        latencyFlags = ((lowLatencyMode & 0x1) << 2) | (audioOutputCompensated & 0x3);
        LOGINFO("Video Latency : %d , Low Latency Mode : %d ,Audio Output Compensated value : %d , Audio Output Delay : %d , Latency Flags: %d ", videoLatency, lowLatencyMode, audioOutputCompensated, audioOutputDelay, latencyFlags);
        m_video_latency = (videoLatency/2) + 1;
        m_latency_flags = latencyFlags;
        m_audio_output_delay = (audioOutputDelay/2) + 1;
        setLatencyInfo();
        }

        void HdmiCecSinkImplementation::setLatencyInfo()
        {
        if(!HdmiCecSinkImplementation::_instance)
            return;

        if(!(_instance->smConnection))
        return;

        LOGINFO("Send Report Current Latency message \n");
        _instance->smConnection->sendTo(LogicalAddress::BROADCAST,MessageEncoder().encode(ReportCurrentLatency(physical_addr,m_video_latency,m_latency_flags,m_audio_output_delay)));

        }

       void HdmiCecSinkImplementation::Process_SetSystemAudioMode_msg(const SetSystemAudioMode &msg)
        {
            if(!HdmiCecSinkImplementation::_instance)
               return;

            //DD: Check cecSettingEnabled to prevent race conditions which gives immediate UI setting status
            //SetSystemAudioMode message may come from AVR/Soundbar while CEC disable is in-progress
            if ( cecSettingEnabled != true  )
            {
                 LOGINFO("Process SetSystemAudioMode from Audio device: Cec is disabled-> EnableCEC first");
                 return;
            }

        if ( (msg.status.toInt() == SYSTEM_AUDIO_MODE_OFF) && (m_currentArcRoutingState == ARC_STATE_ARC_INITIATED))
            {
                /* ie system audio mode off -> amplifier goign to standby but still ARC is in initiated state,stop ARC and 
                 bring the ARC state machine to terminated state*/
                LOGINFO("system audio mode off message but arc is not in terminated state so stopping ARC");
                stopArc();

            }

        if (msg.status.toInt() == SYSTEM_AUDIO_MODE_ON) {
            LOGINFO("panel power state is %s", powerState ? "Off" : "On");
            if (powerState == DEVICE_POWER_STATE_ON ) {
                LOGINFO("Notifying system audio mode ON event");
                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->SetSystemAudioModeEvent(msg.status.toString());
                    index++;
                }
        } else {
            LOGINFO("Not notifying system audio mode ON event");
        }
        } else {
            LOGINFO("Notifying system audio Mode OFF event");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->SetSystemAudioModeEvent(msg.status.toString());
                index++;
            }
        }
         }
         void HdmiCecSinkImplementation::Process_ReportAudioStatus_msg(const ReportAudioStatus msg)
         {
            if(!HdmiCecSinkImplementation::_instance)
               return;
               if (m_audioStatusTimerStarted)
               {
                   m_audioStatusReceived = true;
                   m_isAudioStatusInfoUpdated = true;
                   m_audioStatusTimerStarted = false;
                   if (m_audioStatusDetectionTimer.isActive())
                   {
                       LOGINFO("AudioStatus received from the Audio Device and the timer is still active. So stopping the timer!\n");
                       m_audioStatusDetectionTimer.stop();
                   }
                   LOGINFO("AudioStatus received from the Audio Device. Updating the AudioStatus info! m_isAudioStatusInfoUpdated :%d, m_audioStatusReceived :%d, m_audioStatusTimerStarted:%d ", m_isAudioStatusInfoUpdated,m_audioStatusReceived,m_audioStatusTimerStarted);
               }
            LOGINFO("Command: ReportAudioStatus  %s audio Mute status %d  means %s  and current Volume level is %d \n",GetOpName(msg.opCode()),msg.status.getAudioMuteStatus(),msg.status.toString().c_str(),msg.status.getAudioVolume());
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ReportAudioStatusEvent(msg.status.getAudioMuteStatus(), msg.status.getAudioVolume());
                index++;
            }

         }
         void HdmiCecSinkImplementation::sendKeyPressEvent(const int logicalAddress, int keyCode)
         {
                    if(!(_instance->smConnection))
                        return;
            LOGINFO(" sendKeyPressEvent logicalAddress 0x%x keycode 0x%x\n",logicalAddress,keyCode);
                    switch(keyCode)
                   {
                       case VOLUME_UP:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_VOLUME_UP)),100);
               break;
               case VOLUME_DOWN:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_VOLUME_DOWN)), 100);
                          break;
               case MUTE:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_MUTE)), 100);
               break;
               case UP:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_UP)), 100);
               break;
               case DOWN:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_DOWN)), 100);
               break;
               case LEFT:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_LEFT)), 100);
               break;
               case RIGHT:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_RIGHT)), 100);
               break;
               case SELECT:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_SELECT)), 100);
               break;
               case HOME:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_HOME)), 100);
               break;
               case BACK:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_BACK)), 100);
               break;
               case NUMBER_0:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_0)), 100);
               break;
               case NUMBER_1:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_1)), 100);
               break;
               case NUMBER_2:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_2)), 100);
               break;
               case NUMBER_3:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_3)), 100);
               break;
               case NUMBER_4:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_4)), 100);
               break;
               case NUMBER_5:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_5)), 100);
               break;
               case NUMBER_6:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_6)), 100);
               break;
               case NUMBER_7:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_7)), 100);
               break;
               case NUMBER_8:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_8)), 100);
               break;
               case NUMBER_9:
               _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_9)), 100);
               break;

                   }
         }

         void HdmiCecSinkImplementation::sendUserControlPressed(const int logicalAddress, int keyCode)
                 {
                    if(!(_instance->smConnection))
                        return;
                   LOGINFO(" sendUserControlPressed logicalAddress 0x%x keycode 0x%x\n",logicalAddress,keyCode);
                    switch(keyCode)
                   {
                       case VOLUME_UP:
                          _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_VOLUME_UP)),100);
                          break;
                      case VOLUME_DOWN:
                          _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_VOLUME_DOWN)), 100);
                          break;
                      case MUTE:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_MUTE)), 100);
                           break;
                       case UP:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_UP)), 100);
                           break;
                       case DOWN:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_DOWN)), 100);
                           break;
                       case LEFT:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_LEFT)), 100);
                           break;
                       case RIGHT:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_RIGHT)), 100);
                           break;
                       case SELECT:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_SELECT)), 100);
                           break;
                       case HOME:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_HOME)), 100);
                           break;
                       case BACK:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_BACK)), 100);
                           break;
                       case NUMBER_0:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_0)), 100);
                           break;
                       case NUMBER_1:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_1)), 100);
                           break;
                       case NUMBER_2:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_2)), 100);
                           break;
                       case NUMBER_3:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_3)), 100);
                           break;
                       case NUMBER_4:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_4)), 100);
                           break;
                       case NUMBER_5:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_5)), 100);
                           break;
                       case NUMBER_6:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_6)), 100);
                           break;
                       case NUMBER_7:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_7)), 100);
                           break;
                       case NUMBER_8:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_8)), 100);
                           break;
                       case NUMBER_9:
                           _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlPressed(UICommand::UI_COMMAND_NUM_9)), 100);
                           break;

                  }
                }

         void HdmiCecSinkImplementation::sendKeyReleaseEvent(const int logicalAddress)
         {
                    if(!(_instance->smConnection))
                        return;
         _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlReleased()), 100);

         }

         void HdmiCecSinkImplementation::sendUserControlReleased(const int logicalAddress)
                 {
                    if(!(_instance->smConnection))
                        return;
                   LOGINFO(" User Control Released \n");
                 _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(UserControlReleased()), 100);
                 }

         void  HdmiCecSinkImplementation::sendDeviceUpdateInfo(const int logicalAddress)
         {
            LOGINFO("Send Device Update Info \n");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->OnDeviceInfoUpdated(logicalAddress);
                index++;
            }
        }
    void HdmiCecSinkImplementation::systemAudioModeRequest()
        {
         if ( cecEnableStatus != true  )
         {
               LOGINFO("systemAudioModeRequest: Cec is disabled-> EnableCEC first");
              return;
             } 

            if(!HdmiCecSinkImplementation::_instance)
             return;
            if(!(_instance->smConnection))
                return;
             LOGINFO(" Send systemAudioModeRequest ");
           _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(SystemAudioModeRequest(physical_addr)), 1000);

        }
         void HdmiCecSinkImplementation::sendGiveAudioStatusMsg()
        {
            if(!HdmiCecSinkImplementation::_instance)
             return;
            if(!(_instance->smConnection))
                return;
             LOGINFO(" Send GiveAudioStatus ");
          _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(GiveAudioStatus()), 100);

        }
        void  HdmiCecSinkImplementation::reportAudioDevicePowerStatusInfo(const int logicalAddress, const int powerStatus)
        {
            JsonObject params;
            params["powerStatus"] = JsonValue(powerStatus);
            LOGINFO("Panle power state is %s", powerState ? "Off" : "On");
            if (powerStatus != AUDIO_DEVICE_POWERSTATE_OFF) {
                if (powerState == DEVICE_POWER_STATE_ON ) {
                        LOGINFO("Notify DS!!! logicalAddress = %d , Audio device power status = %d \n", logicalAddress, powerStatus);
                        while (index != _hdmiCecSourceNotifications.end()) {
                            (*index)->ReportAudioDevicePowerStatus(powerStatus);
                            index++;
                        }
            } else {
                LOGINFO("Not notifying audio device power state to DS");
            }
            } else {
                LOGINFO("Notify DS!!! logicalAddress = %d , Audio device power status = %d \n", logicalAddress, powerStatus);
                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->ReportAudioDevicePowerStatus(powerStatus);
                    index++;
                }
            }
        }

        void HdmiCecSinkImplementation::SendStandbyMsgEvent(const int logicalAddress)
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->StandbyMessageReceived(logicalAddress);
                index++;
            }
            
       }
       uint32_t HdmiCecSinkImplementation::SetEnabled(const bool enabled, HdmiCecSinkSuccess &success)
       {
            LOGINFOMETHOD();
            getBoolParameter("enabled", enabled);

            setEnabled(enabled);
            success.success = true;
       }

       uint32_t HdmiCecSinkImplementation::GetEnabled(bool &enabled, bool &success)
       {
            enabled = getEnabled();
            success = true;
       }

       uint32_t HdmiCecSinkImplementation::GetAudioDeviceConnectedStatus(bool &connected, bool &success)
       {
            connected = getAudioDeviceConnectedStatus();
            success = true;
       }

       uint32_t HdmiCecSinkImplementation::requestAudioDevicePowerStatusWrapper(const JsonObject& parameters, JsonObject& response)
       {
            requestAudioDevicePowerStatus();
            returnResponse(true);
       }

      uint32_t HdmiCecSinkImplementation::GetActiveSource(bool &available, uint8_t &logicalAddress, string &physicalAddress, string &deviceType, string &cecVersion, string &osdName, string &vendorID, string &powerStatus, string &port, bool &success)
       {
               char routeString[1024] = {'\0'};
            int length = 0;
            std::stringstream temp;

               if ( HdmiCecSinkImplementation::_instance->m_currentActiveSource != -1 )
            {
                int n = HdmiCecSinkImplementation::_instance->m_currentActiveSource;
                available = true;
                logicalAddress = HdmiCecSinkImplementation::_instance->deviceList[n].m_logicalAddress.toInt();
                physicalAddress = HdmiCecSinkImplementation::_instance->deviceList[n].m_physicalAddr.toString().c_str();
                deviceType = HdmiCecSinkImplementation::_instance->deviceList[n].m_deviceType.toString().c_str();
                cecVersion = HdmiCecSinkImplementation::_instance->deviceList[n].m_cecVersion.toString().c_str();
                osdName = HdmiCecSinkImplementation::_instance->deviceList[n].m_osdName.toString().c_str();
                vendorID = HdmiCecSinkImplementation::_instance->deviceList[n].m_vendorID.toString().c_str();
                powerStatus = HdmiCecSinkImplementation::_instance->deviceList[n].m_powerStatus.toString().c_str();


                if ( HdmiCecSinkImplementation::_instance->deviceList[n].m_physicalAddr.getByteValue(0) != 0 )
                {
                    snprintf(&routeString[length], sizeof(routeString) - length, "%s%d", "HDMI",(HdmiCecSinkImplementation::_instance->deviceList[n].m_physicalAddr.getByteValue(0) - 1));
                }
                else if ( HdmiCecSinkImplementation::_instance->deviceList[n].m_physicalAddr.getByteValue(0) == 0 )
                {
                    snprintf(&routeString[length], sizeof(routeString) - length, "%s", "TV");
                }

                temp << (char *)routeString;
                port = temp.str();

            }
            else
            {
                available = false;
            }

            success = true;
            
       }

       uint32_t HdmiCecSinkImplementation::GetDeviceList(uint32_t &numberofdevices, IHdmiCecSinkDeviceListIterator*& deviceList, bool &success)
       {
            LOGINFOMETHOD();

            numberofdevices = HdmiCecSinkImplementation::_instance->m_numberOfDevices;
            LOGINFO("getDeviceListWrapper  m_numberOfDevices :%d \n", HdmiCecSinkImplementation::_instance->m_numberOfDevices);
            std::vector<Exchange::IHdmiCecSource::HdmiCecSinkDevices> localDevices;
            Exchange::IHdmiCecSource::HdmiCecSinkDevices actual_hdmicecdevices = {0};

            for (int n = 0; n <= LogicalAddress::UNREGISTERED; n++)
            {

                if ( n != HdmiCecSinkImplementation::_instance->m_logicalAddressAllocated && 
                        HdmiCecSinkImplementation::_instance->deviceList[n].m_isDevicePresent )
                {

                    actual_hdmicecdevices.logicalAddress = HdmiCecSinkImplementation::_instance->deviceList[n].m_logicalAddress.toInt();
                    actual_hdmicecdevices.physicalAddress = HdmiCecSinkImplementation::_instance->deviceList[n].m_physicalAddr.toString().c_str();
                    actual_hdmicecdevices.deviceType = HdmiCecSinkImplementation::_instance->deviceList[n].m_deviceType.toString().c_str();
                    actual_hdmicecdevices.cecVersion = HdmiCecSinkImplementation::_instance->deviceList[n].m_cecVersion.toString().c_str();
                    actual_hdmicecdevices.osdName = HdmiCecSinkImplementation::_instance->deviceList[n].m_osdName.toString().c_str();
                    actual_hdmicecdevices.vendorID = HdmiCecSinkImplementation::_instance->deviceList[n].m_vendorID.toString().c_str();
                    actual_hdmicecdevices.powerStatus = HdmiCecSinkImplementation::_instance->deviceList[n].m_powerStatus.toString().c_str();
                    int hdmiPortNumber = -1;
                    LOGINFO("getDeviceListWrapper  m_numofHdmiInput:%d looking for Logical Address :%d \n", m_numofHdmiInput, HdmiCecSinkImplementation::_instance->deviceList[n].m_logicalAddress.toInt());
                    for (int i=0; i < m_numofHdmiInput; i++)
                    {
                         LOGINFO("getDeviceListWrapper  connected : %d, portid:%d LA: %d  \n", hdmiInputs[i].m_isConnected, hdmiInputs[i].m_portID, hdmiInputs[i].m_logicalAddr.toInt());
                         if(hdmiInputs[i].m_isConnected  && hdmiInputs[i].m_logicalAddr.toInt() == HdmiCecSinkImplementation::_instance->deviceList[n].m_logicalAddress.toInt())
                         {
                             hdmiPortNumber = hdmiInputs[i].m_portID;
                             LOGINFO("got portid :%d break \n", hdmiPortNumber);
                             break;
                         }
                    }
                    actual_hdmicecdevices.powerStatus = hdmiPortNumber;
                    localDevices.push_back(actual_hdmicecdevices);
                }
            }

            deviceList = new HdmiCecSinkDeviceListIterator(localDevices);
            success = true;
       }


       uint32_t HdmiCecSinkImplementation::SetOSDName(const string &name, HdmiCecSinkSuccess &success)
       {
            LOGINFOMETHOD();

            LOGINFO("SetOSDName osdName: %s",name.c_str());
            osdName = name.c_str();
            Utils::persistJsonSettings (CEC_SETTING_ENABLED_FILE, CEC_SETTING_OSD_NAME, JsonValue(name.c_str()));

            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::GetOSDName(string &name, bool &success)
        {
            name = osdName.toString(); 
            LOGINFO("GetOSDName osdName : %s \n",osdName.toString().c_str());
            success = true;
        }

        uint32_t HdmiCecSinkImplementation::PrintDeviceList(bool &printed, bool &success)
        {
            printDeviceList();
            printed = true;
            success = true;
        }

        uint32_t HdmiCecSinkImplementation::SetActiveSource(HdmiCecSinkSuccess &success)
        {
            setActiveSource(false);
            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::SetActivePath(const string &activePath, HdmiCecSinkSuccess &success)
        {

            std::string id = activePath.String();
            PhysicalAddress phy_addr = PhysicalAddress(id);
            LOGINFO("Addr = %s, length = %zu", id.c_str(), id.length());
            setStreamPath(phy_addr);
            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::GetActiveRoute(bool &available, uint8_t &length, IHdmiCecSinkActivePathIterator*& pathList, string &ActiveRoute, bool &success)
        {
              std::vector<uint8_t> route;    
            char routeString[1024] = {'\0'};
            int length = 0;
            std::vector<Exchange::IHdmiCecSink::HdmiCecSinkActivePath> paths;
            std::stringstream temp;

             if (HdmiCecSinkImplementation::_instance->m_currentActiveSource != -1 &&
                    HdmiCecSinkImplementation::_instance->m_currentActiveSource != HdmiCecSinkImplementation::_instance->m_logicalAddressAllocated )
            {
                   HdmiCecSinkImplementation::_instance->getActiveRoute(LogicalAddress(HdmiCecSinkImplementation::_instance->m_currentActiveSource), route);

                if (route.size())
                {
                    available = true;
                    length = route.size();

                    for (unsigned int i=0; i < route.size(); i++)
                    {
                        if ( route[i] != LogicalAddress::UNREGISTERED )
                        {
                            Exchange::IHdmiCecSink::HdmiCecSinkActivePath device;

                            device.logicalAddress = HdmiCecSinkImplementation::_instance->deviceList[route[i]].m_logicalAddress.toInt();
                            device.physicalAddress = HdmiCecSinkImplementation::_instance->deviceList[route[i]].m_physicalAddr.toString().c_str();
                            device.deviceType = HdmiCecSinkImplementation::_instance->deviceList[route[i]].m_deviceType.toString().c_str();
                            device.vendorID = HdmiCecSinkImplementation::_instance->deviceList[route[i]].m_osdName.toString().c_str();
                            device.osdName = HdmiCecSinkImplementation::_instance->deviceList[route[i]].m_vendorID.toString().c_str();

                            paths.push_back(device);

                            snprintf(&routeString[length], sizeof(routeString) - length, "%s", _instance->deviceList[route[i]].m_logicalAddress.toString().c_str());
                            length += _instance->deviceList[route[i]].m_logicalAddress.toString().length();
                            snprintf(&routeString[length], sizeof(routeString) - length, "(%s", _instance->deviceList[route[i]].m_osdName.toString().c_str());
                            length += _instance->deviceList[route[i]].m_osdName.toString().length();
                            snprintf(&routeString[length], sizeof(routeString) - length, "%s", ")-->");
                            length += strlen(")-->");
                            if( i + 1 ==  route.size() )
                            {
                                snprintf(&routeString[length], sizeof(routeString) - length, "%s%d", "HDMI",(HdmiCecSinkImplementation::_instance->deviceList[route[i]].m_physicalAddr.getByteValue(0) - 1));
                            }
                        }
                    }

                    pathList = new HdmiCecSinkActivePathIterator(paths);
                    temp << (char *)routeString;
                    ActiveRoute = temp.str(); 
                    LOGINFO("ActiveRoute = [%s]", routeString);
                }
                else{
                    available = false;
                }

            }
            else if ( HdmiCecSinkImplementation::_instance->m_currentActiveSource == HdmiCecSinkImplementation::_instance->m_logicalAddressAllocated )
            {
                available = true;
                ActiveRoute = "TV";
            }
            else
            {
                available = false;
            }

            success = true;
        }

        uint32_t HdmiCecSinkImplementation::RequestActiveSource(HdmiCecSinkSuccess &success)
        {
            requestActiveSource();
            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::SetRoutingChange(const string &oldPort, const string &newPort, HdmiCecSinkSuccess &success)
        {
            if ((oldPort.find("HDMI",0) != std::string::npos ||
                    oldPort.find("TV",0) != std::string::npos ) &&
                        (newPort.find("HDMI", 0) != std::string::npos ||
                        newPort.find("TV", 0) != std::string::npos ))
            {
                setRoutingChange(oldPort, newPort);
                success.success = true;
            }
            else
            {
                success.success = false;
            }
       }


       uint32_t HdmiCecSinkImplementation::setMenuLanguageWrapper(const JsonObject& parameters, JsonObject& response)
       {
            std::string lang;

            returnIfParamNotFound(parameters, "language");

            lang = parameters["language"].String();

            setCurrentLanguage(Language(lang.data()));
            sendMenuLanguage();
            returnResponse(true);
       }


        uint32_t HdmiCecSinkImplementation::SetVendorId(const string &vendorId, HdmiCecSinkSuccess &success)
        {
            LOGINFOMETHOD();

            unsigned int vendorID = 0x00;
            try
            {
                vendorID = stoi(vendorId,NULL,16);
            }
            catch (...)
            {
                LOGWARN("Exception in setVendorIdWrapper set default value\n");
                vendorID = 0x0019FB;
            }
            appVendorId = {(uint8_t)(vendorID >> 16 & 0xff),(uint8_t)(vendorID>> 8 & 0xff),(uint8_t) (vendorID & 0xff)};
            LOGINFO("appVendorId : %s  vendorID :%x \n",appVendorId.toString().c_str(), vendorID );
            Utils::persistJsonSettings (CEC_SETTING_ENABLED_FILE, CEC_SETTING_VENDOR_ID, JsonValue(vendorID));

            success.success = true;
        }
        uint32_t HdmiCecSinkImplementation::SetupARCRouting(const bool enabled, HdmiCecSinkSuccess &success)
       {
            if(enabled)
            {
                startArc();
            }
            else
            {
                stopArc();
            }

            success.success = true;
       }
        uint32_t HdmiCecSinkImplementation::GetVendorId(string &vendorid, bool &success)
        {
            LOGINFO("GetVendorId  appVendorId : %s  \n",appVendorId.toString().c_str());
            vendorid = appVendorId.toString() ;
            success = true;
        }

        uint32_t HdmiCecSinkImplementation::RequestShortAudioDescriptor(HdmiCecSinkSuccess &success) 
        {
            requestShortaudioDescriptor();
            success.success = true;
        }
        uint32_t HdmiCecSinkImplementation::SendStandbyMessage(HdmiCecSinkSuccess &success)
        {
            sendStandbyMessage();
            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::SendAudioDevicePowerOnMessage(HdmiCecSinkSuccess &success)
        {
            LOGINFO("%s invoked. \n",__FUNCTION__);
            systemAudioModeRequest();
            success.success = true;
        }
        uint32_t HdmiCecSinkImplementation::SendKeyPressEvent(const uint32_t &logicalAddress, const uint32_t &keyCode, HdmiCecSinkSuccess &success)
        {
            string logicalAddress = logicalAddress.String();
            string keyCode = keyCode.String();
            SendKeyInfo keyInfo;
            keyInfo.logicalAddr = stoi(logicalAddress);
            keyInfo.keyCode     = stoi(keyCode);
            keyInfo.UserControl = "sendKeyPressEvent";
            std::unique_lock<std::mutex> lk(m_sendKeyEventMutex);
            m_SendKeyQueue.push(keyInfo);
                        m_sendKeyEventThreadRun = true;
            m_sendKeyCV.notify_one();
            LOGINFO("Post send key press event to queue size:%zu \n",m_SendKeyQueue.size());
            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::SendUserControlPressed(const uint32_t &logicalAddress, const uint32_t &keyCode, HdmiCecSinkSuccess &success)
        {
            string logicalAddress = logicalAddress.String();
            string keyCode = keyCode.String();
            SendKeyInfo keyInfo;
            keyInfo.logicalAddr = stoi(logicalAddress);
            keyInfo.keyCode     = stoi(keyCode);
            keyInfo.UserControl = "sendUserControlPressed";
            std::unique_lock<std::mutex> lk(m_sendKeyEventMutex);
            m_SendKeyQueue.push(keyInfo);
            m_sendKeyEventThreadRun = true;
            m_sendKeyCV.notify_one();
            LOGINFO("User control pressed, queue size:%zu \n",m_SendKeyQueue.size());
            success.success = true;
        }

        uint32_t HdmiCecSinkImplementation::SendUserControlReleased(const uint32_t &logicalAddress, HdmiCecSinkSuccess &success)
        {
            string logicalAddress = logicalAddress.String();
            SendKeyInfo keyInfo;
            keyInfo.logicalAddr = stoi(logicalAddress);
            keyInfo.keyCode     = 0;
            keyInfo.UserControl = "sendUserControlReleased";
            std::unique_lock<std::mutex> lk(m_sendKeyEventMutex);
            m_SendKeyQueue.push(keyInfo);
            m_sendKeyEventThreadRun = true;
            m_sendKeyCV.notify_one();
            LOGINFO("User Control Released, queue size:%zu \n",m_SendKeyQueue.size());
            success.success = true;
        }

       uint32_t HdmiCecSinkImplementation::SendGetAudioStatusMessage(HdmiCecSinkSuccess &success)
        {
            sendGiveAudioStatusMsg();
            success.success = true;
        }
       uint32_t HdmiCecSinkImplementation::SetLatencyInfo(const string &videoLatency, const string &lowLatencyMode, const string &audioOutputCompensated, const string &audioOutputDelay, HdmiCecSinkSuccess &success) 
        {
           int video_latency,audio_output_compensated,audio_output_delay;
           bool low_latency_mode;

           LOGINFO("SetLatencyInfo videoLatency : %s lowLatencyMode : %s audioOutputCompensated : %s audioOutputDelay : %s \n",videoLatency.c_str(),lowLatencyMode.c_str(),audioOutputCompensated.c_str(),audioOutputDelay.c_str());
           video_latency = stoi(videoLatency);
           low_latency_mode = stoi(lowLatencyMode);
           audio_output_compensated = stoi(audioOutputCompensated);
           audio_output_delay = stoi(audioOutputDelay);

           updateCurrentLatency(video_latency, low_latency_mode,audio_output_compensated, audio_output_delay);
           success.success = true;
        }
        bool HdmiCecSinkImplementation::loadSettings()
        {
            Core::File file;
            file = CEC_SETTING_ENABLED_FILE;

            if( file.Open())
            {
                JsonObject parameters;
                parameters.IElement::FromFile(file);
                bool isConfigAdded = false;

                if( parameters.HasLabel(CEC_SETTING_ENABLED))
                {
                     getBoolParameter(CEC_SETTING_ENABLED, cecSettingEnabled);
                     LOGINFO("CEC_SETTING_ENABLED present value:%d",cecSettingEnabled);
                }
                else
                {
                    parameters[CEC_SETTING_ENABLED] = true;
                    cecSettingEnabled = true;
                    isConfigAdded = true;
                    LOGINFO("CEC_SETTING_ENABLED not present set dafult true:\n ");
                }

                if( parameters.HasLabel(CEC_SETTING_OTP_ENABLED))
                {
                    getBoolParameter(CEC_SETTING_OTP_ENABLED, cecOTPSettingEnabled);
                    LOGINFO("CEC_SETTING_OTP_ENABLED present value :%d",cecOTPSettingEnabled);
                }
                else
                {
                    parameters[CEC_SETTING_OTP_ENABLED] = true;
                    cecOTPSettingEnabled = true;
                    isConfigAdded = true;
                    LOGINFO("CEC_SETTING_OTP_ENABLED not present set dafult true:\n ");
                }
                if( parameters.HasLabel(CEC_SETTING_OSD_NAME))
                {
                    std::string osd_name;
                    getStringParameter(CEC_SETTING_OSD_NAME, osd_name);
                    osdName = osd_name.c_str();
                    LOGINFO("CEC_SETTING_OSD_NAME present osd_name :%s",osdName.toString().c_str());
                }
                else
                {
                    parameters[CEC_SETTING_OSD_NAME] = osdName.toString();
                    LOGINFO("CEC_SETTING_OSD_NMAE not  present set dafult value :%s\n ",osdName.toString().c_str());
                    isConfigAdded = true;
                }
                unsigned int  vendorId = (defaultVendorId.at(0) <<16) | ( defaultVendorId.at(1) << 8 ) | defaultVendorId.at(2);
                if( parameters.HasLabel(CEC_SETTING_VENDOR_ID))
                {
                    getNumberParameter(CEC_SETTING_VENDOR_ID, vendorId);
                    LOGINFO("CEC_SETTING_VENDOR_ID present :%x ",vendorId);
                }
                else
                {
                    LOGINFO("CEC_SETTING_VENDOR_ID not  present set dafult value :%x \n ",vendorId);
                    parameters[CEC_SETTING_VENDOR_ID] = vendorId;
                    isConfigAdded = true;
                }

                appVendorId = {(uint8_t)(vendorId >> 16 & 0xff),(uint8_t)(vendorId >> 8 & 0xff),(uint8_t) (vendorId & 0xff)}; 
                LOGINFO("appVendorId : %s  vendorId :%x \n",appVendorId.toString().c_str(), vendorId );

                if(isConfigAdded)
                {
                    LOGINFO("isConfigAdded true so update file:\n ");
                    file.Destroy();
                    file.Create();
                    parameters.IElement::ToFile(file);

                }

                file.Close();
            }
            else
            {
                LOGINFO("CEC_SETTING_ENABLED_FILE file not present create with default settings ");
                file.Open(false);
                if (!file.IsOpen())
                    file.Create();

                JsonObject parameters;
                unsigned int  vendorId = (defaultVendorId.at(0) <<16) | ( defaultVendorId.at(1) << 8 ) | defaultVendorId.at(2);
                parameters[CEC_SETTING_ENABLED] = true;
                parameters[CEC_SETTING_OSD_NAME] = osdName.toString();
                parameters[CEC_SETTING_VENDOR_ID] = vendorId;

                cecSettingEnabled = true;
                cecOTPSettingEnabled = true;
                parameters.IElement::ToFile(file);

                file.Close();

            }

            return cecSettingEnabled;
        }

        void HdmiCecSinkImplementation::setEnabled(bool enabled)
        {
           LOGINFO("Entered setEnabled: %d  cecSettingEnabled :%d ",enabled, cecSettingEnabled);

           if (cecSettingEnabled != enabled)
           {
               Utils::persistJsonSettings (CEC_SETTING_ENABLED_FILE, CEC_SETTING_ENABLED, JsonValue(enabled));
               cecSettingEnabled = enabled;
           }
           if(true == enabled)
           {
               CECEnable();
           }
           else
           {
               CECDisable();
           }
           return;
        }

        void HdmiCecSinkImplementation::updateImageViewOn(const int logicalAddress)
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ||
                    logicalAddress == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if (_instance->deviceList[logicalAddress].m_isDevicePresent &&
                    _instance->deviceList[_instance->m_logicalAddressAllocated].m_powerStatus.toInt() == PowerStatus::STANDBY)
            {
                /* Bringing TV out of standby is handled by application.notify UI to bring the TV out of standby */
                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->OnWakeupFromStandby(logicalAddress);
                    index++;
                }
            }

            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->OnInActiveSource(logicalAddress);
                index++;
            }
        }

        void HdmiCecSinkImplementation::updateTextViewOn(const int logicalAddress)
        {
            JsonObject params;
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ||
                    logicalAddress == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if (_instance->deviceList[logicalAddress].m_isDevicePresent &&
                    _instance->deviceList[_instance->m_logicalAddressAllocated].m_powerStatus.toInt() == PowerStatus::STANDBY)
            {
                    /* Bringing TV out of standby is handled by application.notify UI to bring the TV out of standby */
                    while (index != _hdmiCecSourceNotifications.end()) {
                        (*index)->OnWakeupFromStandby(logicalAddress);
                        index++;
                    }
            }

            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->OnTextViewOnMsg(logicalAddress);
                index++;
            }
        }


        void HdmiCecSinkImplementation::updateDeviceChain(const LogicalAddress &logicalAddress, const PhysicalAddress &phy_addr)
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if (_instance->deviceList[logicalAddress.toInt()].m_isDevicePresent &&
                    logicalAddress.toInt() != _instance->m_logicalAddressAllocated)
            {
                for (int i=0; i < m_numofHdmiInput; i++) 
                {
                    LOGINFO(" addr = %d, portID = %d", phy_addr.getByteValue(0), hdmiInputs[i].m_portID);
                    if (phy_addr.getByteValue(0) == (hdmiInputs[i].m_portID + 1)) {
                        hdmiInputs[i].addChild(logicalAddress, phy_addr);
                    }
                }
            }
        }

        void HdmiCecSinkImplementation::getActiveRoute(const LogicalAddress &logicalAddress, std::vector<uint8_t> &route)
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ||
                    logicalAddress.toInt() == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if (_instance->deviceList[logicalAddress.toInt()].m_isDevicePresent &&
                    logicalAddress.toInt() != _instance->m_logicalAddressAllocated &&
                        _instance->deviceList[logicalAddress.toInt()].m_isActiveSource )
            {
                route.clear();
                for (int i=0; i < m_numofHdmiInput; i++) 
                {
                    LOGINFO("physicalAddress = [%d], portID = %d", _instance->deviceList[logicalAddress.toInt()].m_physicalAddr.getByteValue(0), hdmiInputs[i].m_portID);
                    if (_instance->deviceList[logicalAddress.toInt()].m_physicalAddr.getByteValue(0) == (hdmiInputs[i].m_portID + 1)) {
                        hdmiInputs[i].getRoute(_instance->deviceList[logicalAddress.toInt()].m_physicalAddr, route);
                    }
                }
            }
            else {
                LOGERR("Not in correct state to Find Route");
            }
        }


        void HdmiCecSinkImplementation::CheckHdmiInState()
        {
            int err;
            bool isAnyPortConnected = false;

            dsHdmiInGetStatusParam_t params;
            err = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            IARM_BUS_DSMGR_API_dsHdmiInGetStatus,
                            (void *)&params,
                            sizeof(params));

            if(err == IARM_RESULT_SUCCESS && params.result == dsERR_NONE )
            {
                   for( int i = 0; i < m_numofHdmiInput; i++ )
                   {
                       LOGINFO("Is HDMI In Port [%d] connected [%d] \n",i, params.status.isPortConnected[i]);
                    if ( params.status.isPortConnected[i] )
                    {
                        isAnyPortConnected = true;
                    }

                    LOGINFO("update Port Status [%d] \n", i);
                    hdmiInputs[i].update(params.status.isPortConnected[i]);
                   }
            }

            if ( isAnyPortConnected ) {
                m_isHdmiInConnected = true;
            } else {
                m_isHdmiInConnected = false;
            }
        }

        void HdmiCecSinkImplementation::requestActiveSource()
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

                        if(!(_instance->smConnection))
                            return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            _instance->smConnection->sendTo(LogicalAddress::BROADCAST, 
                                        MessageEncoder().encode(RequestActiveSource()), 500);
        }

        void HdmiCecSinkImplementation::setActiveSource(bool isResponse)
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

                        if(!(_instance->smConnection))
                            return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if (isResponse && (_instance->m_currentActiveSource != _instance->m_logicalAddressAllocated) )
            {
                LOGWARN("TV is not current Active Source");
                return;
            }

            _instance->smConnection->sendTo(LogicalAddress::BROADCAST, 
                                        MessageEncoder().encode(ActiveSource(_instance->deviceList[_instance->m_logicalAddressAllocated].m_physicalAddr)), 500);
            _instance->m_currentActiveSource = _instance->m_logicalAddressAllocated;
        }

        void HdmiCecSinkImplementation::setCurrentLanguage(const Language &lang)
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            _instance->deviceList[_instance->m_logicalAddressAllocated].m_currentLanguage = lang;
        }

        void HdmiCecSinkImplementation::sendMenuLanguage()
        {
            Language lang = "";
            if(!HdmiCecSinkImplementation::_instance)
                return;

                        if(!(_instance->smConnection))
                            return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            lang = _instance->deviceList[_instance->m_logicalAddressAllocated].m_currentLanguage;

            _instance->smConnection->sendTo(LogicalAddress::BROADCAST, MessageEncoder().encode(SetMenuLanguage(lang)), 100);
        }

        void HdmiCecSinkImplementation::updateInActiveSource(const int logical_address, const InActiveSource &source )
        {
            JsonObject params;
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if( logical_address != _instance->m_logicalAddressAllocated )
            {
                _instance->deviceList[logical_address].m_isActiveSource = false;

                if ( _instance->m_currentActiveSource == logical_address )
                {
                    _instance->m_currentActiveSource = -1;
                }

                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->OnInActiveSource(logicalAddress, source.physicalAddress.toString());
                    index++;
                }
            }
        }

    void HdmiCecSinkImplementation::updateActiveSource(const int logical_address, const ActiveSource &source )
           {
               JsonObject params;
            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if( logical_address != _instance->m_logicalAddressAllocated )
            {
                if ( _instance->m_currentActiveSource != -1 )
                {
                    _instance->deviceList[_instance->m_currentActiveSource].m_isActiveSource = false;
                }

                _instance->deviceList[logical_address].m_isActiveSource = true;
                _instance->deviceList[logical_address].update(source.physicalAddress);
                _instance->m_currentActiveSource = logical_address;

                if (_instance->deviceList[logical_address].m_isDevicePresent &&
                                    _instance->deviceList[_instance->m_logicalAddressAllocated].m_powerStatus.toInt() == PowerStatus::STANDBY)
                {
                     /* Bringing TV out of standby is handled by application.notify UI to bring the TV out of standby */
                     while (index != _hdmiCecSourceNotifications.end()) {
                        (*index)->OnWakeupFromStandby(logicalAddress);
                        index++;
                    }
                }

                string physicalAddress = _instance->deviceList[logical_address].m_physicalAddr.toString().c_str(); 
                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->OnActiveSourceChange(logicalAddress, physicalAddress);
                    index++;
                }
            }
           }

                void HdmiCecSinkImplementation::requestShortaudioDescriptor()
            {
                if ( cecEnableStatus != true  )
                        {
                             LOGINFO("requestShortaudioDescriptor: cec is disabled-> EnableCEC first");
                             return;
                        }

                if(!HdmiCecSinkImplementation::_instance)
                return;

                        if(!(_instance->smConnection))
                            return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

                        LOGINFO(" Send requestShortAudioDescriptor Message ");
                    _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(RequestShortAudioDescriptor(formatid,audioFormatCode,numberofdescriptor)), 1000);

        }

                void HdmiCecSinkImplementation::requestAudioDevicePowerStatus()
                {
                        if ( cecEnableStatus != true  )
                        {
                             LOGWARN("cec is disabled-> EnableCEC first");
                             return;
                        }

                        if(!HdmiCecSinkImplementation::_instance)
                                return;

                        if(!(_instance->smConnection))
                            return;
                        if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                                LOGERR("Logical Address NOT Allocated");
                                return;
                        }

                        LOGINFO(" Send GiveDevicePowerStatus Message to Audio system in the network \n");
                        _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM, MessageEncoder().encode(GiveDevicePowerStatus()), 500);

            m_audioDevicePowerStatusRequested = true;
                }

        void HdmiCecSinkImplementation::sendFeatureAbort(const LogicalAddress logicalAddress, const OpCode feature, const AbortReason reason)
            {

                       if(!HdmiCecSinkImplementation::_instance)
                               return;
                       if(!(_instance->smConnection))
                           return;
               LOGINFO(" Sending FeatureAbort to %s for opcode %s with reason %s ",logicalAddress.toString().c_str(),feature.toString().c_str(),reason.toString().c_str());
                       _instance->smConnection->sendTo(logicalAddress, MessageEncoder().encode(FeatureAbort(feature,reason)), 500);
                 }

               void HdmiCecSinkImplementation::reportFeatureAbortEvent(const LogicalAddress logicalAddress, const OpCode featureOpcode, const AbortReason abortReason)
               {
                        LOGINFO(" Notifying the UI FeatureAbort from the %s for the opcode %s with the reason %s ",logicalAddress.toString().c_str(),featureOpcode.toString().c_str(),abortReason.toString().c_str());
                        while (index != _hdmiCecSourceNotifications.end()) {
                            (*index)->ReportFeatureAbortEvent(logicalAddress.toInt(), featureOpcode.opCode(), abortReason.toInt());
                            index++;
                        }
               }

    void HdmiCecSinkImplementation::pingDevices(std::vector<int> &connected , std::vector<int> &disconnected)
        {
            int i;

        if(!HdmiCecSinkImplementation::_instance)
                return;
                if(!(_instance->smConnection))
                    return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED ){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            for(i=0; i< LogicalAddress::UNREGISTERED; i++ ) {
                if ( i != _instance->m_logicalAddressAllocated )
                {
                    //LOGWARN("PING for  0x%x \r\n",i);
                    try {
                        _instance->smConnection->ping(LogicalAddress(_instance->m_logicalAddressAllocated), LogicalAddress(i), Throw_e());
                    }
                    catch(CECNoAckException &e)
                    {
                        if ( _instance->deviceList[i].m_isDevicePresent ) {
                            disconnected.push_back(i);
                        }
                                                //LOGWARN("Ping device: 0x%x caught %s \r\n", i, e.what());
                        usleep(50000);
                        continue;
                    }
                      catch(Exception &e)
                      {
                        LOGWARN("Ping device: 0x%x caught %s \r\n", i, e.what());
                                                usleep(50000);
                                                continue;
                      }

                      /* If we get ACK, then the device is present in the network*/
                      if ( !_instance->deviceList[i].m_isDevicePresent )
                      {
                          connected.push_back(i);
                                                //LOGWARN("Ping success, added device: 0x%x \r\n", i);
                      }
                      usleep(50000);      
                }
               }
        }

        int HdmiCecSinkImplementation::requestType( const int logicalAddress ) {
            int requestType = CECDeviceParams::REQUEST_NONE;

            if ( !_instance->deviceList[logicalAddress].m_isPAUpdated || !_instance->deviceList[logicalAddress].m_isDeviceTypeUpdated ) {
                requestType = CECDeviceParams::REQUEST_PHISICAL_ADDRESS;     
            }else if ( !_instance->deviceList[logicalAddress].m_isOSDNameUpdated ) {
                requestType = CECDeviceParams::REQUEST_OSD_NAME;
            }else if ( !_instance->deviceList[logicalAddress].m_isVersionUpdated ) {
                requestType = CECDeviceParams::REQUEST_CEC_VERSION;
            }else if ( !_instance->deviceList[logicalAddress].m_isVendorIDUpdated ) {
                requestType = CECDeviceParams::REQUEST_DEVICE_VENDOR_ID;
            }else if ( !_instance->deviceList[logicalAddress].m_isPowerStatusUpdated ) {
                requestType = CECDeviceParams::REQUEST_POWER_STATUS;
            }

            return requestType;
        }

        void HdmiCecSinkImplementation::printDeviceList() {
            int i;

            if(!HdmiCecSinkImplementation::_instance)
                return;

            for(i=0; i< 16; i++)
            {
                if (HdmiCecSinkImplementation::_instance->deviceList[i].m_isDevicePresent) {
                LOGWARN("------ Device ID = %d--------", i);
                HdmiCecSinkImplementation::_instance->deviceList[i].printVariable();
                LOGWARN("-----------------------------");
                }
            }
        }

        void HdmiCecSinkImplementation::setStreamPath( const PhysicalAddress &physical_addr) {

            if(!HdmiCecSinkImplementation::_instance)
                return;

                        if(!(_instance->smConnection))
                            return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED){
                LOGERR("Logical Address NOT Allocated Or its not valid");
                return;
            }

            _instance->smConnection->sendTo(LogicalAddress(LogicalAddress::BROADCAST), MessageEncoder().encode(SetStreamPath(physical_addr)), 500);
        }

        void HdmiCecSinkImplementation::setRoutingChange(const std::string &from, const std::string &to) {
            PhysicalAddress oldPhyAddr = {0xF,0xF,0xF,0xF};
            PhysicalAddress newPhyAddr = {0xF,0xF,0xF,0xF};
            int oldPortID = -1;
            int newPortID = -1;

            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED){
                LOGERR("Logical Address NOT Allocated Or its not valid");
                return;
            }

            if( from.find("TV",0) != std::string::npos )
            {
                oldPhyAddr = _instance->deviceList[_instance->m_logicalAddressAllocated].m_physicalAddr;                
                _instance->m_currentActiveSource = -1;
            }
            else
            {
                oldPortID = stoi(from.substr(4,1),NULL,16);
                if ( oldPortID < _instance->m_numofHdmiInput )
                {
                    oldPhyAddr = _instance->hdmiInputs[oldPortID].m_physicalAddr;
                }
                else
                {
                    LOGERR("Invalid HDMI Old Port ID");
                    return;
                }
            }

            if( to.find("TV",0) != std::string::npos )
            {
                newPhyAddr = _instance->deviceList[_instance->m_logicalAddressAllocated].m_physicalAddr;
                /*set active source as TV */
                _instance->m_currentActiveSource = _instance->m_logicalAddressAllocated;
            }
            else
            {
                newPortID = stoi(to.substr(4,1),NULL,16);

                if ( newPortID < _instance->m_numofHdmiInput )
                {
                    newPhyAddr = _instance->hdmiInputs[newPortID].m_physicalAddr;
                }
                else
                {
                    LOGERR("Invalid HDMI New Port ID");
                    return;
                }
            }

                        if(!(_instance->smConnection))
                            return;
            _instance->smConnection->sendTo(LogicalAddress(LogicalAddress::BROADCAST), MessageEncoder().encode(RoutingChange(oldPhyAddr, newPhyAddr)), 500);
        }

        void HdmiCecSinkImplementation::addDevice(const int logicalAddress) {
            JsonObject params;

            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if ( !HdmiCecSinkImplementation::_instance->deviceList[logicalAddress].m_isDevicePresent )
             {
                 HdmiCecSinkImplementation::_instance->deviceList[logicalAddress].m_isDevicePresent = true;
                HdmiCecSinkImplementation::_instance->deviceList[logicalAddress].m_logicalAddress = LogicalAddress(logicalAddress);
                HdmiCecSinkImplementation::_instance->m_numberOfDevices++;
                HdmiCecSinkImplementation::_instance->m_pollNextState = POLL_THREAD_STATE_INFO;

                if(logicalAddress == 0x5)
                {
                    LOGINFO(" logicalAddress =%d , Audio device detected, Notify Device Settings", logicalAddress );
                    hdmiCecAudioDeviceConnected = true;
                    while (index != _hdmiCecSourceNotifications.end()) {
                        (*index)->ReportAudioDeviceConnectedStatus("success", "true");
                        index++;
                    }
                }

                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->OnDeviceAdded(logicalAddress);
                    index++;
                }
             }
        }

        void HdmiCecSinkImplementation::removeDevice(const int logicalAddress) {
            JsonObject params;

            if(!HdmiCecSinkImplementation::_instance)
                return;

            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED){
                LOGERR("Logical Address NOT Allocated");
                return;
            }

            if (_instance->deviceList[logicalAddress].m_isDevicePresent)
            {
                _instance->m_numberOfDevices--;

                for (int i=0; i < m_numofHdmiInput; i++) 
                {
                    if (_instance->deviceList[logicalAddress].m_physicalAddr.getByteValue(0) == (hdmiInputs[i].m_portID + 1)) {
                        hdmiInputs[i].removeChild(_instance->deviceList[logicalAddress].m_physicalAddr);
                                                hdmiInputs[i].update(LogicalAddress(LogicalAddress::UNREGISTERED));
                    }
                }

                if(logicalAddress == 0x5)
                {
                    LOGINFO(" logicalAddress =%d , Audio device removed, Notify Device Settings", logicalAddress );

                    hdmiCecAudioDeviceConnected = false;
                    if (m_audioStatusDetectionTimer.isActive()){
						m_audioStatusDetectionTimer.stop();
					}
					m_isAudioStatusInfoUpdated = false;
					m_audioStatusReceived = false;
					m_audioStatusTimerStarted = false;
					LOGINFO("Audio device removed, reset the audio status info.  m_isAudioStatusInfoUpdated :%d, m_audioStatusReceived :%d, m_audioStatusTimerStarted:%d ", m_isAudioStatusInfoUpdated,m_audioStatusReceived,m_audioStatusTimerStarted);
                    while (index != _hdmiCecSourceNotifications.end()) {
                        (*index)->ReportAudioDeviceConnectedStatus("success", "false");
                        index++;
                    }
                }

                _instance->deviceList[logicalAddress].m_isRequestRetry = 0;
                _instance->deviceList[logicalAddress].clear();
                while (index != _hdmiCecSourceNotifications.end()) {
                    (*index)->OnDeviceRemoved(logicalAddress);
                    index++;
                }
            }
        }

        void HdmiCecSinkImplementation::request(const int logicalAddress) {
            int requestType;

            if(!HdmiCecSinkImplementation::_instance)
                return;
                        if(!(_instance->smConnection))
                            return;
            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED || logicalAddress >= LogicalAddress::UNREGISTERED + TEST_ADD ){
                LOGERR("Logical Address NOT Allocated Or its not valid");
                return;
            }

            requestType = _instance->requestType(logicalAddress);
            _instance->deviceList[logicalAddress].m_isRequested = requestType;

            switch (requestType)
            {
                case CECDeviceParams::REQUEST_PHISICAL_ADDRESS :
                {
                    _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(GivePhysicalAddress()), 200);
                }
                    break;

                case CECDeviceParams::REQUEST_CEC_VERSION :
                {
                    _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(GetCECVersion()), 100);
                }
                    break;

                case CECDeviceParams::REQUEST_DEVICE_VENDOR_ID :
                {
                    _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(GiveDeviceVendorID()), 100);
                }
                    break;

                case CECDeviceParams::REQUEST_OSD_NAME :    
                {
                    _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(GiveOSDName()), 500);
                }
                    break;

                case CECDeviceParams::REQUEST_POWER_STATUS :    
                {
                    _instance->smConnection->sendTo(LogicalAddress(logicalAddress), MessageEncoder().encode(GiveDevicePowerStatus()), 100);
                }
                    break;
                default:
                {
                    _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;
                }
                    break;
            }

            _instance->deviceList[logicalAddress].m_requestTime = std::chrono::system_clock::now();
            LOGINFO("request type %d", _instance->deviceList[logicalAddress].m_isRequested);
        }

        int HdmiCecSinkImplementation::requestStatus(const int logicalAddress) {
            std::chrono::duration<double,std::milli> elapsed;
            bool isElapsed = false;

            if(!HdmiCecSinkImplementation::_instance)
                return -1;


            if ( _instance->m_logicalAddressAllocated == LogicalAddress::UNREGISTERED || logicalAddress >= LogicalAddress::UNREGISTERED + TEST_ADD ) {
                LOGERR("Logical Address NOT Allocated Or its not valid");
                return -1;
            }

            switch ( _instance->deviceList[logicalAddress].m_isRequested ) {
                case CECDeviceParams::REQUEST_PHISICAL_ADDRESS :
                {
                    if( _instance->deviceList[logicalAddress].m_isPAUpdated &&
                            _instance->deviceList[logicalAddress].m_isDeviceTypeUpdated )
                    {
                        _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;
                    }
                }
                    break;

                case CECDeviceParams::REQUEST_CEC_VERSION :
                {
                    if( _instance->deviceList[logicalAddress].m_isVersionUpdated )
                    {
                        _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;
                    }
                }
                    break;

                case CECDeviceParams::REQUEST_DEVICE_VENDOR_ID :
                {
                    if( _instance->deviceList[logicalAddress].m_isVendorIDUpdated )
                    {
                        _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;
                    }
                }
                    break;

                case CECDeviceParams::REQUEST_OSD_NAME :    
                {
                    if( _instance->deviceList[logicalAddress].m_isOSDNameUpdated )
                    {
                        _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;
                    }
                }
                    break;

                case CECDeviceParams::REQUEST_POWER_STATUS :    
                {
                    if( _instance->deviceList[logicalAddress].m_isPowerStatusUpdated )
                    {
                        _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;
                    }
                }
                    break;
                default:
                    break;    
            }

            if ( _instance->deviceList[logicalAddress].m_isRequested != CECDeviceParams::REQUEST_NONE )
            {
                elapsed = std::chrono::system_clock::now() - _instance->deviceList[logicalAddress].m_requestTime;

                if ( elapsed.count() > HDMICECSINK_REQUEST_MAX_WAIT_TIME_MS )
                {
                    LOGINFO("request elapsed ");
                    isElapsed = true;    
                }
            }

            if (isElapsed)
            {
                /* For some request it should be retry, like report physical address etc for other we can have default values */
                switch( _instance->deviceList[logicalAddress].m_isRequested )
                {
                    case CECDeviceParams::REQUEST_PHISICAL_ADDRESS :
                    {
                        LOGINFO("Retry for REQUEST_PHISICAL_ADDRESS = %d", _instance->deviceList[logicalAddress].m_isRequestRetry);
                        /* Update with Invalid Physical Address */
                        if ( _instance->deviceList[logicalAddress].m_isRequestRetry++ >= HDMICECSINK_REQUEST_MAX_RETRY )
                        {
                            LOGINFO("Max retry for REQUEST_PHISICAL_ADDRESS = %d", _instance->deviceList[logicalAddress].m_isRequestRetry);
                            _instance->deviceList[logicalAddress].update(PhysicalAddress(0xF,0xF,0xF,0xF));
                            _instance->deviceList[logicalAddress].update(DeviceType(DeviceType::RESERVED));
                            _instance->deviceList[logicalAddress].m_isRequestRetry = 0;
                        }
                    }
                        break;

                    case CECDeviceParams::REQUEST_CEC_VERSION :
                    {
                        /*Defaulting to 1.4*/
                        _instance->deviceList[logicalAddress].update(Version(Version::V_1_4));
                    }
                        break;

                    case CECDeviceParams::REQUEST_DEVICE_VENDOR_ID :
                    {
                        _instance->deviceList[logicalAddress].update(VendorID(0,0,0));
                    }
                        break;

                    case CECDeviceParams::REQUEST_OSD_NAME :    
                    {
                        if ( _instance->deviceList[logicalAddress].m_isRequestRetry++ >= HDMICECSINK_REQUEST_MAX_RETRY )
                        {
                            LOGINFO("Max retry for REQUEST_OSD_NAME = %d", _instance->deviceList[logicalAddress].m_isRequestRetry);
                            _instance->deviceList[logicalAddress].update(OSDName(""));
                            _instance->deviceList[logicalAddress].m_isRequestRetry = 0;
                        }
                    }
                        break;

                    case CECDeviceParams::REQUEST_POWER_STATUS :    
                    {
                        _instance->deviceList[logicalAddress].update(PowerStatus(PowerStatus::POWER_STATUS_NOT_KNOWN));
                    }
                        break;
                    default:
                        break;    
                }


                _instance->deviceList[logicalAddress].m_isRequested = CECDeviceParams::REQUEST_NONE;    
            }

            if( _instance->deviceList[logicalAddress].m_isRequested == CECDeviceParams::REQUEST_NONE)
            {
                LOGINFO("Request Done");
                return CECDeviceParams::REQUEST_DONE;
            }

            //LOGINFO("Request NOT Done");
            return CECDeviceParams::REQUEST_NOT_DONE;
        }

        void HdmiCecSinkImplementation::threadRun()
        {
            std::vector <int> connected;
            std::vector <int> disconnected;
            int logicalAddressRequested = LogicalAddress::UNREGISTERED + TEST_ADD;
            bool isExit = false;

            if(!HdmiCecSinkImplementation::_instance)
                    return;

                if(!(_instance->smConnection))
                    return;
               LOGINFO("Entering ThreadRun: _instance->m_pollThreadExit %d isExit %d _instance->m_pollThreadState %d  _instance->m_pollNextState %d",_instance->m_pollThreadExit,isExit,_instance->m_pollThreadState,_instance->m_pollNextState );
            _instance->m_sleepTime = HDMICECSINK_PING_INTERVAL_MS;

            while(1)
            {

                   if (_instance->m_pollThreadExit || isExit ){
                    LOGWARN("Thread Exits _instance->m_pollThreadExit %d isExit %d _instance->m_pollThreadState %d  _instance->m_pollNextState %d",_instance->m_pollThreadExit,isExit,_instance->m_pollThreadState,_instance->m_pollNextState );
                    break;
                }

                if ( _instance->m_pollNextState != POLL_THREAD_STATE_NONE )
                {
                    _instance->m_pollThreadState = _instance->m_pollNextState;
                    _instance->m_pollNextState = POLL_THREAD_STATE_NONE;
                }

                switch (_instance->m_pollThreadState)  {

                case POLL_THREAD_STATE_POLL :
                {
                    //LOGINFO("POLL_THREAD_STATE_POLL");
                    _instance->allocateLogicalAddress(DeviceType::TV);
                    if ( _instance->m_logicalAddressAllocated != LogicalAddress::UNREGISTERED)
                    {
                        try{
                            
						    logicalAddress = LogicalAddress(_instance->m_logicalAddressAllocated);
						    LibCCEC::getInstance().addLogicalAddress(logicalAddress);
						    _instance->smConnection->setSource(logicalAddress);
						    _instance->m_numberOfDevices = 0;
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_deviceType = DeviceType::TV;
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_isDevicePresent = true;
                            			_instance->deviceList[_instance->m_logicalAddressAllocated].update(physical_addr);
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_cecVersion = Version::V_1_4;
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_vendorID = appVendorId;
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_powerStatus = PowerStatus(powerState);
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_currentLanguage = defaultLanguage;
						    _instance->deviceList[_instance->m_logicalAddressAllocated].m_osdName = osdName.toString().c_str();
						    if(cecVersion == 2.0) {
						        _instance->deviceList[_instance->m_logicalAddressAllocated].m_cecVersion = Version::V_2_0;
						        _instance->smConnection->sendTo(LogicalAddress(LogicalAddress::BROADCAST),
                                                                    MessageEncoder().encode(ReportFeatures(Version::V_2_0,allDevicetype,rcProfile,deviceFeatures)), 500);
						    }
						    _instance->smConnection->addFrameListener(_instance->msgFrameListener);
						    _instance->smConnection->sendTo(LogicalAddress(LogicalAddress::BROADCAST), 
						    		MessageEncoder().encode(ReportPhysicalAddress(physical_addr, _instance->deviceList[_instance->m_logicalAddressAllocated].m_deviceType)), 100);

						    _instance->m_sleepTime = 0;
						    _instance->m_pollThreadState = POLL_THREAD_STATE_PING;
                        }
                        catch(InvalidStateException &e){
                            LOGWARN("InvalidStateException caught while allocated logical address. %s", e.what());
						    _instance->m_pollThreadState = POLL_THREAD_STATE_EXIT;
                        }
                        catch(IOException &e){
                            LOGWARN("IOException caught while allocated logical address. %s", e.what());
						    _instance->m_pollThreadState = POLL_THREAD_STATE_EXIT;
                        }
                        catch(...){
                            LOGWARN("Exception caught while allocated logical address.");
						    _instance->m_pollThreadState = POLL_THREAD_STATE_EXIT;
                        }
                    }
                    else
                    {
                        LOGINFO("Not able allocate Logical Address for TV");    
                        _instance->m_pollThreadState = POLL_THREAD_STATE_EXIT;
                    }
                }
                break;

                case POLL_THREAD_STATE_PING :
                {
                    //LOGINFO("POLL_THREAD_STATE_PING");
                    _instance->m_pollThreadState = POLL_THREAD_STATE_INFO;
                    connected.clear();
                    disconnected.clear();
                    _instance->pingDevices(connected, disconnected);

                    if ( disconnected.size() ){
                        for( unsigned int i=0; i< disconnected.size(); i++ )
                        {
                            LOGWARN("Disconnected Devices [%zu]", disconnected.size());
                            _instance->removeDevice(disconnected[i]);
                        }
                    }

                    if (connected.size()) {
                        LOGWARN("Connected Devices [%zu]", connected.size());
                        for( unsigned int i=0; i< connected.size(); i++ )
                        {
                            _instance->addDevice(connected[i]);
                            /* If new device is connected, then try to aquire the information */
                            _instance->m_pollThreadState = POLL_THREAD_STATE_INFO;
                            _instance->m_sleepTime = 0;
                        }
                    }
                    else
                    {
                        for(int i=0;i<LogicalAddress::UNREGISTERED + TEST_ADD;i++)
                        {
                            if(i != _instance->m_logicalAddressAllocated &&
                                _instance->deviceList[i].m_isDevicePresent &&
                                !_instance->deviceList[i].isAllUpdated() )
                            {
                                _instance->m_pollNextState = POLL_THREAD_STATE_INFO;
                                _instance->m_sleepTime = 0;
                            }
                        }
                        /* Check for any update required */
                        _instance->m_pollThreadState = POLL_THREAD_STATE_UPDATE;
                        _instance->m_sleepTime = 0;
                    }
                }
                break;

                case POLL_THREAD_STATE_INFO :
                {
                    //LOGINFO("POLL_THREAD_STATE_INFO");

                    if ( logicalAddressRequested == LogicalAddress::UNREGISTERED + TEST_ADD )
                    {
                        int i = 0;
                        for(;i<LogicalAddress::UNREGISTERED + TEST_ADD;i++)
                        {
                            if( i != _instance->m_logicalAddressAllocated &&
                                _instance->deviceList[i].m_isDevicePresent &&
                                !_instance->deviceList[i].isAllUpdated() )
                            {
                                //LOGINFO("POLL_THREAD_STATE_INFO -> request for %d", i);
                                logicalAddressRequested = i;
                                _instance->request(logicalAddressRequested);
                                _instance->m_sleepTime = HDMICECSINK_REQUEST_INTERVAL_TIME_MS;
                                break;
                            }
                        }

                        if ( i ==  LogicalAddress::UNREGISTERED)
                        {
                            /*So there is no update required, try to ping after some seconds*/
                            _instance->m_pollThreadState = POLL_THREAD_STATE_IDLE;        
                            _instance->m_sleepTime = 0;
                            //LOGINFO("POLL_THREAD_STATE_INFO -> state change to Ping", i);
                        }
                    }
                    else
                    {
                        /*So there is request sent for logical address, so wait and check the status */
                        if ( _instance->requestStatus(logicalAddressRequested) == CECDeviceParams::REQUEST_DONE )
                        {
                            logicalAddressRequested = LogicalAddress::UNREGISTERED;
                        }
                        else
                        {
                            _instance->m_sleepTime = HDMICECSINK_REQUEST_INTERVAL_TIME_MS;                            
                        }
                    }
                }
                break;

                /* updating the power status and if required we can add other information later*/
                case POLL_THREAD_STATE_UPDATE :
                {
                    //LOGINFO("POLL_THREAD_STATE_UPDATE");

                    for(int i=0;i<LogicalAddress::UNREGISTERED + TEST_ADD;i++)
                    {
                        if( i != _instance->m_logicalAddressAllocated &&
                            _instance->deviceList[i].m_isDevicePresent &&
                            _instance->deviceList[i].m_isPowerStatusUpdated )
                        {
                            std::chrono::duration<double,std::milli> elapsed = std::chrono::system_clock::now() - _instance->deviceList[i].m_lastPowerUpdateTime;

                            if ( elapsed.count() > HDMICECSINK_UPDATE_POWER_STATUS_INTERVA_MS )
                            {
                                _instance->deviceList[i].m_isPowerStatusUpdated = false;
                                _instance->m_pollNextState = POLL_THREAD_STATE_INFO;        
                                _instance->m_sleepTime = 0;
                            }
                        }
                    }

                    _instance->m_pollThreadState = POLL_THREAD_STATE_IDLE;        
                    _instance->m_sleepTime = 0;
                }
                break;

                case POLL_THREAD_STATE_IDLE :
                {
                    //LOGINFO("POLL_THREAD_STATE_IDLE");
                    _instance->m_sleepTime = HDMICECSINK_PING_INTERVAL_MS;
                    _instance->m_pollThreadState = POLL_THREAD_STATE_PING;
                }
                break;

                case POLL_THREAD_STATE_WAIT :
                {
                    /* Wait for Hdmi is connected, in case it disconnected */
                    //LOGINFO("19Aug2020-[01] -> POLL_THREAD_STATE_WAIT");
                    _instance->m_sleepTime = HDMICECSINK_WAIT_FOR_HDMI_IN_MS;

                    if ( _instance->m_isHdmiInConnected == true )
                    {
                        _instance->m_pollThreadState = POLL_THREAD_STATE_POLL;
                    }
                }
                break;

                case POLL_THREAD_STATE_EXIT :
                {
                    isExit = true;
                    _instance->m_sleepTime = 0;
                }
                break;
                }

                std::unique_lock<std::mutex> lk(_instance->m_pollExitMutex);
                if ( _instance->m_ThreadExitCV.wait_for(lk, std::chrono::milliseconds(_instance->m_sleepTime)) == std::cv_status::timeout )
                    continue;
                else
                    LOGINFO("Thread is going to Exit m_pollThreadExit %d\n", _instance->m_pollThreadExit );

            }
        }

        void HdmiCecSinkImplementation::allocateLAforTV()
        {
            bool gotLogicalAddress = false;
            int addr = LogicalAddress::TV;
            int i, j;
            if (!(_instance->smConnection))
                return;

            for (i = 0; i< HDMICECSINK_NUMBER_TV_ADDR; i++)
            {
                /* poll for TV logical address - retry 5 times*/
                for (j = 0; j < 5; j++)
                {
                    try {
                        smConnection->poll(LogicalAddress(addr), Throw_e());
                    }
                    catch(CECNoAckException &e )
                    {
                        LOGWARN("Poll caught %s \r\n",e.what());
                        gotLogicalAddress = true;
                        break;
                    }
                    catch(Exception &e)
                    {
                        LOGWARN("Poll caught %s \r\n",e.what());
                        usleep(250000);
                    }
                }
                if (gotLogicalAddress)
                {
                    break;
                }
                addr = LogicalAddress::SPECIFIC_USE;
            }

            if ( gotLogicalAddress )
            {
                m_logicalAddressAllocated = addr;
            }
            else
            {
                m_logicalAddressAllocated = LogicalAddress::UNREGISTERED;
            }

            LOGWARN("Logical Address for TV 0x%x \r\n",m_logicalAddressAllocated);
        }

        void HdmiCecSinkImplementation::allocateLogicalAddress(int deviceType)
        {
            if( deviceType == DeviceType::TV )
            {
                allocateLAforTV();                
            }
        }

        void HdmiCecSinkImplementation::CECEnable(void)
        {
            std::lock_guard<std::mutex> lock(m_enableMutex);
        JsonObject params;
            LOGINFO("Entered CECEnable");
            if (cecEnableStatus)
            {
                LOGWARN("CEC Already Enabled");
                return;
            }

            if(0 == libcecInitStatus)
            {
                try
                {
                    LibCCEC::getInstance().init("HdmiCecSinkImplementation");
                }
                catch(InvalidStateException &e){
                    LOGWARN("InvalidStateException caught in LibCCEC::init %s", e.what());
                }
                catch(IOException &e){
                    LOGWARN("IOException caught in LibCCEC::init %s", e.what());
                }
                catch(...){
                    LOGWARN("Exception caught in LibCCEC::init");
                }
            }
            libcecInitStatus++;

            //Acquire CEC Addresses
            getPhysicalAddress();

            smConnection = new Connection(LogicalAddress::UNREGISTERED,false,"ServiceManager::Connection::");
            smConnection->open();
            allocateLogicalAddress(DeviceType::TV);
            LOGINFO("logical address allocalted: %x  \n",m_logicalAddressAllocated);
            if ( m_logicalAddressAllocated != LogicalAddress::UNREGISTERED && smConnection)
            {
                logicalAddress = LogicalAddress(m_logicalAddressAllocated);
                LOGINFO(" add logical address  %x  \n",m_logicalAddressAllocated);
                LibCCEC::getInstance().addLogicalAddress(logicalAddress);
                smConnection->setSource(logicalAddress);
            }
            msgProcessor = new HdmiCecSinkProcessor(*smConnection);
            msgFrameListener = new HdmiCecSinkFrameListener(*msgProcessor);
            if(smConnection)
            {
                   LOGWARN("Start Thread %p", smConnection );
                m_pollThreadState = POLL_THREAD_STATE_POLL;
                            m_pollNextState = POLL_THREAD_STATE_NONE;
                            m_pollThreadExit = false;
                m_pollThread = std::thread(threadRun);
            }
            cecEnableStatus = true;

            params["cecEnable"] = string("true");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ReportCecEnabledEvent("true");
                index++;
            }

            return;
        }

        void HdmiCecSinkImplementation::CECDisable(void)
        {
            std::lock_guard<std::mutex> lock(m_enableMutex);
        JsonObject params;
            LOGINFO("Entered CECDisable ");
            if(!cecEnableStatus)
            {
                LOGWARN("CEC Already Disabled ");
                return;
            }

            if(m_currentArcRoutingState != ARC_STATE_ARC_TERMINATED)
            {
                stopArc();
          while(m_currentArcRoutingState != ARC_STATE_ARC_TERMINATED)    
              {
                     usleep(500000);
               }
            }

             LOGINFO(" CECDisable ARC stopped ");
           cecEnableStatus = false;
            if (smConnection != NULL)
            {
        LOGWARN("Stop Thread %p", smConnection );
        m_pollThreadExit = true;
        m_ThreadExitCV.notify_one();

        try
        {
            if (m_pollThread.joinable())
            {
                LOGWARN("Join Thread %p", smConnection );
                m_pollThread.join();
            }
        }
        catch(const std::system_error& e)
        {
            LOGERR("system_error exception in thread join %s", e.what());
        }
        catch(const std::exception& e)
        {
            LOGERR("exception in thread join %s", e.what());
        }

                m_pollThreadState = POLL_THREAD_STATE_NONE;
                m_pollNextState = POLL_THREAD_STATE_NONE;

        LOGWARN("Deleted Thread %p", smConnection );

                smConnection->close();
                delete smConnection;
                smConnection = NULL;
            }

        m_logicalAddressAllocated = LogicalAddress::UNREGISTERED;
        m_currentArcRoutingState = ARC_STATE_ARC_TERMINATED;
        if (m_audioStatusDetectionTimer.isActive()){
			    m_audioStatusDetectionTimer.stop();
	    }
	    m_isAudioStatusInfoUpdated = false;
	    m_audioStatusReceived = false;
	    m_audioStatusTimerStarted = false;
	    LOGINFO("CEC Disabled, reset the audio status info. m_isAudioStatusInfoUpdated :%d, m_audioStatusReceived :%d, m_audioStatusTimerStarted:%d ", m_isAudioStatusInfoUpdated,m_audioStatusReceived,m_audioStatusTimerStarted);

        for(int i=0; i< 16; i++)
            {
         if (_instance->deviceList[i].m_isDevicePresent)
             {
             _instance->deviceList[i].clear();
             }
            }

            if(1 == libcecInitStatus)
            {
                try
                {
                   LibCCEC::getInstance().term();
                }
                catch(InvalidStateException &e){
                    LOGWARN("InvalidStateException caught in LibCCEC::term %s", e.what());
                }
                catch(IOException &e){
                    LOGWARN("IOException caught in LibCCEC::term %s", e.what());
                }
                catch(...){
                    LOGWARN("Exception caught in LibCCEC::term");
            }

            libcecInitStatus--;
            LOGWARN("CEC Disabled %d",libcecInitStatus); 

       params["cecEnable"] = string("false");
           while (index != _hdmiCecSourceNotifications.end()) {
            (*index)->ReportCecEnabledEvent("false");
            index++;
        }

            return;
        }


        void HdmiCecSinkImplementation::getPhysicalAddress()
        {
            LOGINFO("Entered getPhysicalAddress ");

            uint32_t physAddress = 0x0F0F0F0F;

            try {
                    LibCCEC::getInstance().getPhysicalAddress(&physAddress);
                    physical_addr = {(uint8_t)((physAddress >> 24) & 0xFF),(uint8_t)((physAddress >> 16) & 0xFF),(uint8_t) ((physAddress >> 8)  & 0xFF),(uint8_t)((physAddress) & 0xFF)};
                    LOGINFO("getPhysicalAddress: physicalAddress: %s ", physical_addr.toString().c_str());
            }
            catch (const std::exception& e)
            {
                LOGWARN("exception caught from getPhysicalAddress");
            }
            return;
        }

        bool HdmiCecSinkImplementation::getEnabled()
        {


            LOGINFO("getEnabled :%d ",cecEnableStatus);
            if(true == cecEnableStatus)
                return true;
            else
                return false;
        }

        bool HdmiCecSinkImplementation::getAudioDeviceConnectedStatus()
        {
            LOGINFO("getAudioDeviceConnectedStatus :%d ", hdmiCecAudioDeviceConnected);
            if(true == hdmiCecAudioDeviceConnected)
                return true;
            else
                return false;
        }
        //Arc Routing related  functions
        void HdmiCecSinkImplementation::startArc()
        {
           if ( cecEnableStatus != true  )
           {
              LOGINFO("Initiate_Arc Cec is disabled-> EnableCEC first");
          return;
           }
           if(!HdmiCecSinkImplementation::_instance)
            return;

             LOGINFO("Current ARC State : %d\n", m_currentArcRoutingState);

        _instance->requestArcInitiation();

          // start initiate ARC timer 3 sec
            if (m_arcStartStopTimer.isActive())
            {
                m_arcStartStopTimer.stop();
            }
            m_arcstarting = true;
            m_arcStartStopTimer.start((HDMISINK_ARC_START_STOP_MAX_WAIT_MS)); 

        }
        void  HdmiCecSinkImplementation::requestArcInitiation()
        {
      {
           std::lock_guard<std::mutex> lock(m_arcRoutingStateMutex);
           m_currentArcRoutingState = ARC_STATE_REQUEST_ARC_INITIATION;
      }
          LOGINFO("requestArcInitiation release sem");
          _instance->m_semSignaltoArcRoutingThread.release();

        }
        void HdmiCecSinkImplementation::stopArc()
        {
            if ( cecEnableStatus != true  )
            {
              LOGINFO("Initiate_Arc Cec is disabled-> EnableCEC first");
          return;
            }
            if(!HdmiCecSinkImplementation::_instance)
                return;
        if(m_currentArcRoutingState == ARC_STATE_REQUEST_ARC_TERMINATION || m_currentArcRoutingState == ARC_STATE_ARC_TERMINATED)
            {
               LOGINFO("ARC is either Termination  in progress or already Terminated");
               return;
            }

           _instance->requestArcTermination();
           /* start a timer for 3 sec to get the desired ARC_STATE_ARC_TERMINATED */
           if (m_arcStartStopTimer.isActive())
            {
                m_arcStartStopTimer.stop();
            }
            /* m_arcstarting = true means starting the ARC start timer ,false means ARC stopping timer*/
            m_arcstarting = false; 
            m_arcStartStopTimer.start((HDMISINK_ARC_START_STOP_MAX_WAIT_MS));


        }
        void HdmiCecSinkImplementation::requestArcTermination()
        {  
      {
           std::lock_guard<std::mutex> lock(m_arcRoutingStateMutex);
           m_currentArcRoutingState = ARC_STATE_REQUEST_ARC_TERMINATION;
      }
           LOGINFO("requestArcTermination release sem");
           _instance->m_semSignaltoArcRoutingThread.release();

       }      

    void  HdmiCecSinkImplementation::Process_InitiateArc()
        {
            JsonObject params;

            LOGINFO("Command: INITIATE_ARC \n");

            if(!HdmiCecSinkImplementation::_instance)
        return;

        //DD: Check cecSettingEnabled to prevent race conditions which gives immediate UI setting status
        //Initiate ARC message may come from AVR/Soundbar while CEC disable is in-progress
            if ( cecSettingEnabled != true  )
            {
              LOGINFO("Process InitiateArc from Audio device: Cec is disabled-> EnableCEC first");
              return;
            }

            LOGINFO("Got : INITIATE_ARC  and current Arcstate is %d\n",_instance->m_currentArcRoutingState);

            if (m_arcStartStopTimer.isActive())
            {
               m_arcStartStopTimer.stop();
            }
        if (powerState == DEVICE_POWER_STATE_ON ) {
            LOGINFO("Notifying Arc Initiation event as power state is %s", powerState ? "Off" : "On");
            std::lock_guard<std::mutex> lock(_instance->m_arcRoutingStateMutex);
            _instance->m_currentArcRoutingState = ARC_STATE_ARC_INITIATED;

            _instance->m_semSignaltoArcRoutingThread.release();
            LOGINFO("Got : ARC_INITIATED  and notify Device setting");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ArcInitiationEvent("success");
                index++;
        }
        } else {
        LOGINFO("Not notifying Arc Initiation event as power state is %s", powerState ? "Off" : "On");
        }

       }
       void HdmiCecSinkImplementation::Process_TerminateArc()
       {
            JsonObject params;

            LOGINFO("Command: TERMINATE_ARC current arc state %d \n",HdmiCecSinkImplementation::_instance->m_currentArcRoutingState);
            if (m_arcStartStopTimer.isActive())
            {
                  m_arcStartStopTimer.stop();
            }
            std::lock_guard<std::mutex> lock(m_arcRoutingStateMutex);
            HdmiCecSinkImplementation::_instance->m_currentArcRoutingState = ARC_STATE_ARC_TERMINATED;
            _instance->m_semSignaltoArcRoutingThread.release();

            // trigger callback to Device setting informing to TERMINATE_ARC
            LOGINFO("Got : ARC_TERMINATED  and notify Device setting");
            while (index != _hdmiCecSourceNotifications.end()) {
                (*index)->ArcTerminationEvent("success");
                index++;
            }
        }

        void HdmiCecSinkImplementation::threadSendKeyEvent()
        {
            if(!HdmiCecSinkImplementation::_instance)
                return;

        SendKeyInfo keyInfo = {-1,-1};

            while(!_instance->m_sendKeyEventThreadExit)
            {
                keyInfo.logicalAddr = -1;
                keyInfo.keyCode = -1;
                {
                    // Wait for a message to be added to the queue
                    std::unique_lock<std::mutex> lk(_instance->m_sendKeyEventMutex);
                    _instance->m_sendKeyCV.wait(lk, []{return (_instance->m_sendKeyEventThreadRun == true);});
                }

                if (_instance->m_sendKeyEventThreadExit == true)
                {
                    LOGINFO(" threadSendKeyEvent Exiting");
                    _instance->m_sendKeyEventThreadRun = false;
                    break;
                }

                if (_instance->m_SendKeyQueue.empty()) {
                    _instance->m_sendKeyEventThreadRun = false;
                    continue;
                }

                    keyInfo = _instance->m_SendKeyQueue.front();
                    _instance->m_SendKeyQueue.pop();

            if(keyInfo.UserControl == "sendUserControlPressed" )
            {
                LOGINFO("sendUserControlPressed : logical addr:0x%x keyCode: 0x%x  queue size :%zu \n",keyInfo.logicalAddr,keyInfo.keyCode,_instance->m_SendKeyQueue.size());
                _instance->sendUserControlPressed(keyInfo.logicalAddr,keyInfo.keyCode);
            }
            else if(keyInfo.UserControl == "sendUserControlReleased")
            {
                LOGINFO("sendUserControlReleased : logical addr:0x%x  queue size :%zu \n",keyInfo.logicalAddr,_instance->m_SendKeyQueue.size());
                _instance->sendUserControlReleased(keyInfo.logicalAddr);
            }
            else
            {
                LOGINFO("sendKeyPressEvent : logical addr:0x%x keyCode: 0x%x  queue size :%zu \n",keyInfo.logicalAddr,keyInfo.keyCode,_instance->m_SendKeyQueue.size());
                            _instance->sendKeyPressEvent(keyInfo.logicalAddr,keyInfo.keyCode);
                            _instance->sendKeyReleaseEvent(keyInfo.logicalAddr);
                    }

            if((_instance->m_SendKeyQueue.size()<=1 || (_instance->m_SendKeyQueue.size() % 2 == 0)) && ((keyInfo.keyCode == VOLUME_UP) || (keyInfo.keyCode == VOLUME_DOWN) || (keyInfo.keyCode == MUTE)) )
            {
                if(keyInfo.keyCode == MUTE)
			{
				_instance->sendGiveAudioStatusMsg();
			}
			else
			{
				LOGINFO("m_isAudioStatusInfoUpdated :%d, m_audioStatusReceived :%d, m_audioStatusTimerStarted:%d ",_instance->m_isAudioStatusInfoUpdated,_instance->m_audioStatusReceived,_instance->m_audioStatusTimerStarted);
				if (!_instance->m_isAudioStatusInfoUpdated)
				{
					if ( !(_instance->m_audioStatusDetectionTimer.isActive()))
					{
						LOGINFO("Audio status info not updated. Starting the Timer!");
						_instance->m_audioStatusTimerStarted = true;
						_instance->m_audioStatusDetectionTimer.start((HDMICECSINK_UPDATE_AUDIO_STATUS_INTERVAL_MS));
					}
					LOGINFO("m_isAudioStatusInfoUpdated :%d, m_audioStatusReceived :%d, m_audioStatusTimerStarted:%d ", _instance->m_isAudioStatusInfoUpdated,_instance->m_audioStatusReceived,_instance->m_audioStatusTimerStarted);
				}
				else
				{
					if (!_instance->m_audioStatusReceived){
						_instance->sendGiveAudioStatusMsg();
					}
				}
			}
		}
            }

            }//while(!_instance->m_sendKeyEventThreadExit)
        }//threadSendKeyEvent

        void HdmiCecSinkImplementation::audioStatusTimerFunction()
	    {
	    	m_audioStatusTimerStarted = false;
	    	m_isAudioStatusInfoUpdated = true;
	    	LOGINFO("Timer Expired. Requesting the AudioStatus since not received.\n");
	    	sendGiveAudioStatusMsg();
	    	LOGINFO("m_isAudioStatusInfoUpdated :%d, m_audioStatusReceived :%d, m_audioStatusTimerStarted:%d ", m_isAudioStatusInfoUpdated,m_audioStatusReceived,m_audioStatusTimerStarted);
	    }


        void HdmiCecSinkImplementation::threadArcRouting()
        {
        bool isExit = false;
        uint32_t currentArcRoutingState;

        if(!HdmiCecSinkImplementation::_instance)
                return;

        LOGINFO("Running threadArcRouting");
        _instance->getHdmiArcPortID();

            while(1)
            {

            _instance->m_semSignaltoArcRoutingThread.acquire();



        { 
                   LOGINFO(" threadArcRouting Got semaphore"); 
            std::lock_guard<std::mutex> lock(_instance->m_arcRoutingStateMutex);

           currentArcRoutingState = _instance->m_currentArcRoutingState;

           LOGINFO(" threadArcRouting  Got Sem arc state %d",currentArcRoutingState);
        }       

          switch (currentArcRoutingState) 
          {   

                 case ARC_STATE_REQUEST_ARC_INITIATION :
                             { 

                                 _instance->systemAudioModeRequest();
                 _instance->Send_Request_Arc_Initiation_Message();

                 }
                      break;
                case ARC_STATE_ARC_INITIATED :
                {
                   _instance->Send_Report_Arc_Initiated_Message();
                     }
                 break;
                case ARC_STATE_REQUEST_ARC_TERMINATION :
                {

                   _instance->Send_Request_Arc_Termination_Message();

                }
                   break;
                case ARC_STATE_ARC_TERMINATED :
                {
                  _instance->Send_Report_Arc_Terminated_Message();
                }
                   break;
                case ARC_STATE_ARC_EXIT :
                {
                isExit = true;
                }
                break;
             }

             if (isExit == true)
             {  
             LOGINFO(" threadArcRouting EXITing"); 
                 break;
              }
            }//while(1)
        }//threadArcRouting

        void HdmiCecSinkImplementation::Send_Request_Arc_Initiation_Message()
    {
           if(!HdmiCecSinkImplementation::_instance)
         return;
           if(!(_instance->smConnection))
               return;
          LOGINFO(" Send_Request_Arc_Initiation_Message ");
           _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(RequestArcInitiation()), 1000);

        }
        void HdmiCecSinkImplementation::Send_Report_Arc_Initiated_Message()
        {   
            if(!HdmiCecSinkImplementation::_instance)
        return;
            if(!(_instance->smConnection))
               return;
            _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(ReportArcInitiation()), 1000);

        }
        void HdmiCecSinkImplementation::Send_Request_Arc_Termination_Message()
        {

            if(!HdmiCecSinkImplementation::_instance)
         return;
            if(!(_instance->smConnection))
               return;
            _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(RequestArcTermination()), 1000);
        }

       void HdmiCecSinkImplementation::Send_Report_Arc_Terminated_Message()
       {
            if(!HdmiCecSinkImplementation::_instance)
        return;
            if(!(_instance->smConnection))
               return;
           _instance->smConnection->sendTo(LogicalAddress::AUDIO_SYSTEM,MessageEncoder().encode(ReportArcTermination()), 1000);

       }

      void HdmiCecSinkImplementation::getHdmiArcPortID()
      {
         int err;
         dsGetHDMIARCPortIdParam_t param;
         err = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            (char *)IARM_BUS_DSMGR_API_dsGetHDMIARCPortId,
                            (void *)&param,
                            sizeof(param));
          if (IARM_RESULT_SUCCESS == err)
          {
             LOGINFO("HDMI ARC port ID HdmiArcPortID=[%d] \n", param.portId);
             HdmiArcPortID = param.portId;
          }
      }

      void HdmiCecSinkImplementation::getCecVersion()
      {
      RFC_ParamData_t param = {0};
          WDMP_STATUS status = getRFCParameter((char*)"thunderapi", TR181_HDMICECSINK_CEC_VERSION, &param);
      if(WDMP_SUCCESS == status && param.type == WDMP_STRING) {
             LOGINFO("CEC Version from RFC = [%s] \n", param.value);
             cecVersion = atof(param.value);
          }
      else {
         LOGINFO("Error while fetching CEC Version from RFC ");
      }
      }

    } // namespace Plugin
} // namespace WPEFrameworklk

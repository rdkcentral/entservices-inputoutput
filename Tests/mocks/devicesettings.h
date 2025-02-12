/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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

#pragma once

#include <exception>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <list>




typedef enum _dsAudioPortType_t {
    dsAUDIOPORT_TYPE_ID_LR, /**< RCA audio output.                  */
    dsAUDIOPORT_TYPE_HDMI, /**< HDMI audio output.                 */
    dsAUDIOPORT_TYPE_SPDIF, /**< S/PDIF audio output.               */
    dsAUDIOPORT_TYPE_SPEAKER, /**< SPEAKER audio output.             */
    dsAUDIOPORT_TYPE_HDMI_ARC, /** < HDMI ARC/EARC audio output.     */
    dsAUDIOPORT_TYPE_HEADPHONE, /**< 3.5mm headphone jack.           */
    dsAUDIOPORT_TYPE_MAX /**< Maximum index for audio port type. */
} dsAudioPortType_t;

typedef enum _dsAudioFormat_t {
    dsAUDIO_FORMAT_NONE,
    dsAUDIO_FORMAT_PCM,
    dsAUDIO_FORMAT_DOLBY_AC3,
    dsAUDIO_FORMAT_DOLBY_EAC3,
    dsAUDIO_FORMAT_DOLBY_AC4,
    dsAUDIO_FORMAT_DOLBY_MAT,
    dsAUDIO_FORMAT_DOLBY_TRUEHD,
    dsAUDIO_FORMAT_DOLBY_EAC3_ATMOS,
    dsAUDIO_FORMAT_DOLBY_TRUEHD_ATMOS,
    dsAUDIO_FORMAT_DOLBY_MAT_ATMOS,
    dsAUDIO_FORMAT_DOLBY_AC4_ATMOS,
    dsAUDIO_FORMAT_AAC,
    dsAUDIO_FORMAT_VORBIS,
    dsAUDIO_FORMAT_WMA,
    dsAUDIO_FORMAT_UNKNOWN,
    dsAUDIO_FORMAT_MAX /**< Maximum . */
} dsAudioFormat_t;

typedef enum _dsAudioCapabilities_t {
    dsAUDIOSUPPORT_NONE = 0x0,
    dsAUDIOSUPPORT_ATMOS = 0x01,
    dsAUDIOSUPPORT_DD = 0x02,
    dsAUDIOSUPPORT_DDPLUS = 0x04,
    dsAUDIOSUPPORT_DAD = 0x08,
    dsAUDIOSUPPORT_DAPv2 = 0x10,
    dsAUDIOSUPPORT_MS12 = 0x20,
    dsAUDIOSUPPORT_MS12V2 = 0x40,
    dsAUDIOSUPPORT_Invalid = 0x80,
} dsAudioCapabilities_t;

typedef enum _dsAudioARCTypes_t {
    dsAUDIOARCSUPPORT_NONE = 0x0,
    dsAUDIOARCSUPPORT_ARC = 0x01,
    dsAUDIOARCSUPPORT_eARC = 0x02,
} dsAudioARCTypes_t;

typedef enum _dsMS12Capabilities_t {
    dsMS12SUPPORT_NONE = 0x0,
    dsMS12SUPPORT_DolbyVolume = 0x01,
    dsMS12SUPPORT_InteligentEqualizer = 0x02,
    dsMS12SUPPORT_DialogueEnhancer = 0x04,
    dsMS12SUPPORT_Invalid = 0x80,
} dsMS12Capabilities_t;

typedef struct _dsVolumeLeveller_t {
    int mode; // 0 = off, 1= on, 2= auto
    int level;
} dsVolumeLeveller_t;

typedef struct _dsSurroundVirtualizer_t {
    int mode; // 0 = off, 1= on, 2= auto
    int boost;
} dsSurroundVirtualizer_t;

typedef enum StereoMode {
    dsAUDIO_STEREO_UNKNOWN, /**< Unknown mode.                         */
    dsAUDIO_STEREO_MONO = 1, /**< Mono mode.                            */
    dsAUDIO_STEREO_STEREO, /**< Normal stereo mode (L+R).             */
    dsAUDIO_STEREO_SURROUND, /**< Surround mode.                        */
    dsAUDIO_STEREO_PASSTHRU, /**< Passthrough mode.                     */
    dsAUDIO_STEREO_DD, /**< Dolby Digital.                        */
    dsAUDIO_STEREO_DDPLUS, /**< Dolby Digital Plus.                   */
    dsAUDIO_STEREO_MAX /**< Maximum index for audio stereo types. */
} dsAudioStereoMode_t;

typedef enum _dsATMOSCapability_t {
    dsAUDIO_ATMOS_NOTSUPPORTED = 0, /**< ATMOS audio not supported */
    dsAUDIO_ATMOS_DDPLUSSTREAM, /**< can handle dd plus stream which is only way to pass ATMOS metadata */
    dsAUDIO_ATMOS_ATMOSMETADATA, /**< capable of parsing ATMOS metadata */
} dsATMOSCapability_t;

typedef enum _dsAudioInput_t
{
    dsAUDIO_INPUT_PRIMARY = 0,   /**< Primary Audio Input is main audio input provided to audio mixer */
    dsAUDIO_INPUT_SYSTEM,        /**< System Audio Input to audio mixer , e.g Beep Sounds */
    dsAUDIO_INPUT_MAX            /**< Out of range */
} dsAudioInput_t;

typedef enum _dsHdcpProtocolVersion_t {
    dsHDCP_VERSION_1X = 0, /**< HDCP Protocol version 1.x */
    dsHDCP_VERSION_2X, /**< HDCP Protocol version 2.x */
    dsHDCP_VERSION_MAX /**< Maximum index for HDCP protocol. */
} dsHdcpProtocolVersion_t;

typedef enum _dsHdcpStatus_t {
    dsHDCP_STATUS_UNPOWERED = 0, /**< Connected Sink Device does not support HDCP */
    dsHDCP_STATUS_UNAUTHENTICATED, /**< HDCP Authentication Process is not initiated */
    dsHDCP_STATUS_AUTHENTICATED, /**< HDCP Authentication Process is initiated and Passed */
    dsHDCP_STATUS_AUTHENTICATIONFAILURE, /**< HDCP Authentication Failure or Link Integroty Failure */
    dsHDCP_STATUS_INPROGRESS, /**< HDCP Authentication in Progress */
    dsHDCP_STATUS_PORTDISABLED, /**< HDMI output port disabled */
    dsHDCP_STATUS_MAX /**< Maximum index for HDCP status. */
} dsHdcpStatus_t;

typedef enum _dsVideoResolution_t {
    dsVIDEO_PIXELRES_720x480, /**< 720x480 Resolution.                         */
    dsVIDEO_PIXELRES_720x576, /**< 720x576 Resolution.                         */
    dsVIDEO_PIXELRES_1280x720, /**< 1280x720 Resolution.                        */
    dsVIDEO_PIXELRES_1920x1080, /**< 1920x1080 Resolution.                       */
    dsVIDEO_PIXELRES_3840x2160, /**< 3840x2160 Resolution.                       */
    dsVIDEO_PIXELRES_4096x2160, /**< 3840x2160 Resolution.                       */
    dsVIDEO_PIXELRES_MAX /**< Maximum index for Video ouptut resolutions. */
} dsVideoResolution_t;

// typedef enum _dsTVResolution_t {
//     dsTV_RESOLUTION_480i = 0x0001, /**< 480i Resolution.                         */
//     dsTV_RESOLUTION_480p = 0x0002, /**< 480p Resolution.                         */
//     dsTV_RESOLUTION_576i = 0x0004, /**< 576p Resolution.                         */
//     dsTV_RESOLUTION_576p = 0x0008, /**< 576p Resolution.                         */
//     dsTV_RESOLUTION_720p = 0x0010, /**< 720p Resolution.                         */
//     dsTV_RESOLUTION_1080i = 0x0020, /**< 1080i Resolution.                         */
//     dsTV_RESOLUTION_1080p = 0x0040, /**< 1080p Resolution.                         */
//     dsTV_RESOLUTION_2160p30 = 0x0080, /**< 2160p30 Resolution.                         */
//     dsTV_RESOLUTION_2160p60 = 0x0100, /**< 2160p60 Resolution.                         */
// } dsTVResolution_t;

typedef enum _dsTVResolution_t {
    dsTV_RESOLUTION_480i = 0x0001,
    dsTV_RESOLUTION_480p = 0x0002,
    dsTV_RESOLUTION_576i = 0x0004,
    dsTV_RESOLUTION_576p = 0x0008,
    dsTV_RESOLUTION_576p50 = 0x0010, 
    dsTV_RESOLUTION_720p = 0x0020,
    dsTV_RESOLUTION_720p50 = 0x0040, 
    dsTV_RESOLUTION_1080i = 0x0080,
    dsTV_RESOLUTION_1080p = 0x0100,
    dsTV_RESOLUTION_1080p24 = 0x0200, 
    dsTV_RESOLUTION_1080p25 = 0x0400, 
    dsTV_RESOLUTION_1080i25 = 0x0800, 
    dsTV_RESOLUTION_1080p30 = 0x1000, 
    dsTV_RESOLUTION_1080i50 = 0x2000, 
    dsTV_RESOLUTION_1080p50 = 0x4000, 
    dsTV_RESOLUTION_1080p60 = 0x8000, 
    dsTV_RESOLUTION_2160p24 = 0x10000, 
    dsTV_RESOLUTION_2160p25 = 0x20000, 
    dsTV_RESOLUTION_2160p30 = 0x40000,
    dsTV_RESOLUTION_2160p50 = 0x80000, 
    dsTV_RESOLUTION_2160p60 = 0x100000,
} dsTVResolution_t;

typedef enum _dsVideoFrameRate_t {
    dsVIDEO_FRAMERATE_UNKNOWN, /**< Unknown frame rate.                       */
    dsVIDEO_FRAMERATE_24, /**< Played at 24 frames per second.           */
    dsVIDEO_FRAMERATE_25, /**< Played at 25 frames per second.           */
    dsVIDEO_FRAMERATE_30, /**< Played at 30 frames per second.           */
    dsVIDEO_FRAMERATE_60, /**< Played at 60 frames per second.           */
    dsVIDEO_FRAMERATE_23dot98, /**< Played at 23.98 frames per second.        */
    dsVIDEO_FRAMERATE_29dot97, /**< Played at 29.97 frames per second.        */
    dsVIDEO_FRAMERATE_50, /**< Played at 50 frames per second.           */
    dsVIDEO_FRAMERATE_59dot94, /**< Played at 59.94 frames per second.        */
    dsVIDEO_FRAMERATE_MAX /**< Maximum index for video frame rates.      */
} dsVideoFrameRate_t;

typedef enum _dsVideoZoom_t {
    dsVIDEO_ZOOM_UNKNOWN = -1, /**< Unknown mode.                                                                                   */
    dsVIDEO_ZOOM_NONE = 0, /**< Decoder format conversion is inactive.                                                          */
    dsVIDEO_ZOOM_FULL, /**< Full screen (16:9 video is streched to fit 4:3 frame).                                          */
    dsVIDEO_ZOOM_LB_16_9, /**< 16:9 Letterbox (16:9 video is placed in a 4:3 frame with a full letterbox effect).              */
    dsVIDEO_ZOOM_LB_14_9, /**< 14:9 Letterbox (16:9 video is transferred into a 4:3 frame with 14:9 letterbox effect).         */
    dsVIDEO_ZOOM_CCO, /**< Center Cut-out (16:9 video is cut to fill 4:3 frame with its center part).                      */
    dsVIDEO_ZOOM_PAN_SCAN, /**< Pan & Scan (16:9 is cut and panned to fille 4:3 frame, following MPEG pan & scan vectors).      */
    dsVIDEO_ZOOM_LB_2_21_1_ON_4_3, /**< 2.21:1 Letterbox on 4:3 (video is transferred into 4:3 frame with a 2.21:1 letterbox effect).   */
    dsVIDEO_ZOOM_LB_2_21_1_ON_16_9, /**< 2.21:1 Letterbox on 16:9 (video is transferred into 16:9 frame with a 2.21:1 letterbox effect). */
    dsVIDEO_ZOOM_PLATFORM, /**< Control over the decoder format conversions is managed by the platform.                         */
    dsVIDEO_ZOOM_16_9_ZOOM, /**< 16:9 Zoom (4:3 video is zoomed to fill 16:9 frame).                                             */
    dsVIDEO_ZOOM_PILLARBOX_4_3, /**< Pillarbox 4:3 (4:3 video is placed in a 16:9 frame with a pillarbox effect)                     */
    dsVIDEO_ZOOM_WIDE_4_3, /**< Wide 4:3 (4:3 video is stretched to fill 16:9 frame).                                           */
    dsVIDEO_ZOOM_MAX /**< Maximum index for screen zoom modes.                                                            */
} dsVideoZoom_t;

typedef enum _dsVideoAspectRatio_t {
    dsVIDEO_ASPECT_RATIO_4x3, /**< 4:3 aspect ratio.                      */
    dsVIDEO_ASPECT_RATIO_16x9, /**< 16:9 aspect ratio.                     */
    dsVIDEO_ASPECT_RATIO_MAX /**< Maximum index for video aspect ratios. */
} dsVideoAspectRatio_t;

typedef enum _dsVideoStereoScopicMode_t {
    dsVIDEO_SSMODE_UNKNOWN = 0, /**< Unknown mode.                                */
    dsVIDEO_SSMODE_2D, /**< 2D mode.                                     */
    dsVIDEO_SSMODE_3D_SIDE_BY_SIDE, /**< 3D side by side (L/R) stereo mode.           */
    dsVIDEO_SSMODE_3D_TOP_AND_BOTTOM, /**< 3D top & bottom stereo mode.                 */
    dsVIDEO_SSMODE_MAX /**< Maximum index for video stereoscopic modes.  */
} dsVideoStereoScopicMode_t;

typedef struct _dsVideoPortResolution_t {
    char name[32]; /**< Name the resolution (e.g. 480i, 480p, 1080p24).   */
    dsVideoResolution_t pixelResolution; /**< The resolution associated with the name.                 */
    dsVideoAspectRatio_t aspectRatio; /**< The associated aspect ratio.                             */
    dsVideoStereoScopicMode_t stereoScopicMode; /**< The associated stereoscopic mode.                        */
    dsVideoFrameRate_t frameRate; /**< The associated frame rate.                               */
    bool interlaced; /**< The associated scan mode(@a true if interlaced, @a false if progressive). */
} dsVideoPortResolution_t;

typedef enum _dsHDRStandard_t {
    dsHDRSTANDARD_NONE = 0x0,
    dsHDRSTANDARD_HDR10 = 0x01,
    dsHDRSTANDARD_HLG = 0x02,
    dsHDRSTANDARD_DolbyVision = 0x04,
    dsHDRSTANDARD_TechnicolorPrime = 0x08,
    dsHDRSTANDARD_HDR10PLUS = 0x10,
    dsHDRSTANDARD_SDR = 0x20,
    dsHDRSTANDARD_Invalid = 0x80,
} dsHDRStandard_t;

typedef enum _dsSURROUNDMode_t {
    dsSURROUNDMODE_NONE = 0x0,
    dsSURROUNDMODE_DD = 0x1,
    dsSURROUNDMODE_DDPLUS = 0x2,
} dsSURROUNDMode_t;

typedef enum _dsDisplayColorDepth_t {
    dsDISPLAY_COLORDEPTH_UNKNOWN = 0x0, /* Unknown color depth */
    dsDISPLAY_COLORDEPTH_8BIT = 0x01, /* 8 bit color depth */
    dsDISPLAY_COLORDEPTH_10BIT = 0x02, /* 10 bit color depth */
    dsDISPLAY_COLORDEPTH_12BIT = 0x04, /* 12 bit color depth */
    dsDISPLAY_COLORDEPTH_AUTO = 0x08 /* Automatic color depth */
} dsDisplayColorDepth_t;

typedef enum _dsHdmiInPort_t {
    dsHDMI_IN_PORT_NONE = -1,
    dsHDMI_IN_PORT_0,
    dsHDMI_IN_PORT_1,
    dsHDMI_IN_PORT_2,
    dsHDMI_IN_PORT_MAX
} dsHdmiInPort_t;

typedef enum _dsHdmiInSignalStatus_t {
    dsHDMI_IN_SIGNAL_STATUS_NONE = -1,
    dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL,
    dsHDMI_IN_SIGNAL_STATUS_UNSTABLE,
    dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED,
    dsHDMI_IN_SIGNAL_STATUS_STABLE,
    dsHDMI_IN_SIGNAL_STATUS_MAX
} dsHdmiInSignalStatus_t;

typedef enum dsAviContentType {
  dsAVICONTENT_TYPE_GRAPHICS,
  dsAVICONTENT_TYPE_PHOTO,
  dsAVICONTENT_TYPE_CINEMA,
  dsAVICONTENT_TYPE_GAME,
  dsAVICONTENT_TYPE_INVALID,
}dsAviContentType_t;

struct dsSpd_infoframe_st {
    uint8_t pkttype;
    uint8_t version;
    uint8_t length; /*length=25*/
    uint8_t rsd;
    uint8_t checksum;
    /*Vendor Name Character*/
    uint8_t vendor_name[8];
    /*Product Description Character*/
    uint8_t product_des[16];
    /*byte 25*/
    uint8_t source_info;
};

typedef enum tv_hdmi_edid_version_e {
    HDMI_EDID_VER_14 = 0,
    HDMI_EDID_VER_20,
    HDMI_EDID_VER_MAX,
} tv_hdmi_edid_version_t;

typedef enum _dsAudioPortState {
    dsAUDIOPORT_STATE_UNINITIALIZED,
    dsAUDIOPORT_STATE_INITIALIZED,
    dsAUDIOPORT_STATE_MAX
} dsAudioPortState_t;

typedef enum {
    dsERR_NONE = 0, /**< No error (success).               */
    dsERR_GENERAL = 0x1000, /**< General failure to take action.   */
    dsERR_INVALID_PARAM, /**< Invalid input parameter.          */
    dsERR_INVALID_STATE, /**< Failure due to invalid state.     */
    dsERR_OPERATION_NOT_SUPPORTED, /**< Operation not supported.          */
    dsERR_UNKNOWN /**< Unknown error.                    */
} dsError_t;

#define MAX_LANGUAGE_LEN 10
#define PWRMGR_MAX_VIDEO_PORT_NAME_LENGTH 128

typedef struct {
        bool isEnabled;
        bool result;
        char port[PWRMGR_MAX_VIDEO_PORT_NAME_LENGTH];
    } dsMgrStandbyVideoStateParam_t;

typedef struct {
        bool result;
        bool isEnabled;
        char port[PWRMGR_MAX_VIDEO_PORT_NAME_LENGTH];
    } IARM_Bus_PWRMgr_StandbyVideoState_Param_t;

typedef enum _dsSleepMode_t {
    dsHOST_SLEEP_MODE_LIGHT, /**< Light sleep mode.                                */
    dsHOST_SLEEP_MODE_DEEP, /**< Deep sleep mode.                                 */
    dsHOST_SLEEP_MODE_MAX, /**< Maximum index for sleep modes                    */
} dsSleepMode_t;

typedef enum __dsFPDTimeFormat_t {
    dsFPD_TIME_12_HOUR, /**< 12 hour time format. */
    dsFPD_TIME_24_HOUR, /**< 24 hour time format. */
    dsFPD_TIME_STRING /**< Text string.         */
} dsFPDTimeFormat_t;

typedef enum _dsCompInSignalStatus_t {
    dsCOMP_IN_SIGNAL_STATUS_NONE = -1,
    dsCOMP_IN_SIGNAL_STATUS_NOSIGNAL,
    dsCOMP_IN_SIGNAL_STATUS_UNSTABLE,
    dsCOMP_IN_SIGNAL_STATUS_NOTSUPPORTED,
    dsCOMP_IN_SIGNAL_STATUS_STABLE,
    dsCOMP_IN_SIGNAL_STATUS_MAX
} dsCompInSignalStatus_t;

typedef enum _dsCompositeInPort_t {
    dsCOMPOSITE_IN_PORT_NONE = -1,
    dsCOMPOSITE_IN_PORT_0,
    dsCOMPOSITE_IN_PORT_1,
    dsCOMPOSITE_IN_PORT_MAX
} dsCompositeInPort_t;

/*! DS Manager  Event Data */
typedef struct _DSMgr_EventData_t {
    union {
        struct _RESOLUTION_DATA {
            /* Declare Event Data structure for Video resolution Event */
            int width; /*!< Resolution Width  */
            int height; /*!< Key code */
        } resn; /*Reolution data*/
        struct _DFC_DATA {
            /* Declare Event Data structure for Zoom settings Event */
            int zoomsettings;
        } dfc; /*zoom data*/

        struct _AUDIOMODE_DATA {
            /* Declare Event Data structure for Video resolution Event */
            int type; /*!< device type  */
            int mode; /*!< device mode */
        } Audioport; /*Audio mode  data*/

        struct _ATMOS_CAPS_CHANGE {
            dsATMOSCapability_t caps;  
            bool status;
        } AtmosCapsChange;

        struct _HDMI_HPD_DATA {
            /* Declare HDMI HPD Data */
            int event;
        } hdmi_hpd; /*HDMI Hot Plug detect*/

        struct _HDMI_HDCP_DATA {
            /* Declare HDMI DCP Data */
            int hdcpStatus;
        } hdmi_hdcp; /*HDMI HDCP Hot Plug detect*/

        struct _HDMI_RXSENSE_DATA {
            /* Declare HDMI Rx Sense status */
            int status;
        } hdmi_rxsense; /*HDMI Rx Sense Data*/

        struct _HDMI_IN_CONNECT_DATA {
            dsHdmiInPort_t port;
            bool isPortConnected;
        } hdmi_in_connect;

        struct _HDMI_IN_STATUS_DATA {
            /* Declare HDMI Input status*/
            dsHdmiInPort_t port;
            bool isPresented;
        } hdmi_in_status; /*HDMI in status change detect*/

        struct _HDMI_IN_SIG_STATUS_DATA {
            /* Declare HDMI In signal status*/
            dsHdmiInPort_t port;
            dsHdmiInSignalStatus_t status;
        } hdmi_in_sig_status; /*HDMI in signal change detect*/

        struct _HDMI_IN_VIDEO_MODE_DATA {
            /* Declare HDMI In signal status*/
            dsHdmiInPort_t port;
            dsVideoPortResolution_t resolution;
        } hdmi_in_video_mode; /*HDMI in video mode update*/

        struct _COMPOSITE_IN_CONNECT_DATA {
            dsCompositeInPort_t port;
            bool isPortConnected;
        } composite_in_connect;

        struct _COMPOSITE_IN_STATUS_DATA {
            /* Declare Composite Input status*/
            dsCompositeInPort_t port;
            bool isPresented;
        } composite_in_status; /*Composite in status change detect*/

        struct _COMPOSITE_IN_SIG_STATUS_DATA {
            /* Declare Composite In signal status*/
            dsCompositeInPort_t port;
            dsCompInSignalStatus_t status;
        } composite_in_sig_status; /*Composite in signal change detect*/

        struct _FPD_TIME_FORMAT {
            dsFPDTimeFormat_t eTimeFormat;
        } FPDTimeFormat;

        struct _HDCP_PROTOCOL_DATA {
            dsHdcpProtocolVersion_t protocolVersion;
        } HDCPProtocolVersion;
        struct _SLEEP_MODE_DATA {
            dsSleepMode_t sleepMode;
        } sleepModeInfo;

        struct _AUDIO_LEVEL_DATA {
            int level;
        } AudioLevelInfo;

        struct _AUDIO_OUT_CONNECT_DATA {
            dsAudioPortType_t portType;
            unsigned int uiPortNo;
            bool isPortConnected;
        } audio_out_connect;

        struct _AUDIO_FORMAT_DATA {
            dsAudioFormat_t audioFormat;
        } AudioFormatInfo;

        struct _LANGUAGE_DATA {
            char audioLanguage[MAX_LANGUAGE_LEN];
        } AudioLanguageInfo;

        struct _FADER_CONTROL_DATA {
            int mixerbalance;
        } FaderControlInfo;

        struct _ASSOCIATED_AUDIO_MIXING_DATA {
            bool mixing;
        } AssociatedAudioMixingInfo;

        struct _VIDEO_FORMAT_DATA {
            dsHDRStandard_t videoFormat;
        } VideoFormatInfo;

        struct _AUDIO_PORTSTATE_DATA {
            dsAudioPortState_t audioPortState;
        } AudioPortStateInfo;

        struct _HDMI_IN_ALLM_MODE_DATA {
            /* Declare HDMI In ALLM Mode*/
            dsHdmiInPort_t port;
            bool allm_mode;
        } hdmi_in_allm_mode; /*HDMI in ALLM Mode change*/
        struct _HDMI_IN_CONTENT_TYPE_DATA{
            dsHdmiInPort_t port;
            dsAviContentType_t aviContentType;
        }hdmi_in_content_type;
        struct _HDMI_IN_AV_LATENCY{
               int audio_output_delay;
               int video_latency;
        }hdmi_in_av_latency; /*HDMI in AVLatency change*/
        struct _DISPLAY_FRAMERATE_CHANGE {
            char framerate[20];
        }DisplayFrameRateChange;
    } data;
} IARM_Bus_DSMgr_EventData_t;

typedef enum _dsDisplayEvent_t {
    dsDISPLAY_EVENT_CONNECTED = 0, //!< Display connected event.
    dsDISPLAY_EVENT_DISCONNECTED, //!< Display disconnected event.
    dsDISPLAY_RXSENSE_ON, //!< Rx Sense ON event
    dsDISPLAY_RXSENSE_OFF, //!< Rx Sense OFF event
    dsDISPLAY_HDCPPROTOCOL_CHANGE, //!< HDCP Protocol Version Change event
    dsDISPLAY_EVENT_MAX
} dsDisplayEvent_t;

typedef uint32_t dsFPDColor_t;
#define dsFPDColor_Make(R8,G8,B8) (((R8)<< 8)|((G8)<<8)|((B8) ))
#define dsFPD_COLOR_WHITE dsFPDColor_Make(0xFF, 0xFF, 0xFF)

typedef struct _dsFPDColorConfig_t{
        int id;
        dsFPDColor_t color;
}dsFPDColorConfig_t;

namespace device {

template <class T>
using List = std::vector<T>;

}

namespace device {

class Exception : public std::exception {
    int _err;
    std::string _msg;

public:
    Exception(const char* msg = "No Message for this exception") throw()
        : _msg(msg)
    {
    }

    Exception(int err, const char* msg = "No Message for this Exception") throw()
        : _err(err)
        , _msg(msg){};

    virtual const std::string& getMessage() const
    {
        return _msg;
    }

    virtual int getCode() const
    {
        return _err;
    }

    virtual const char* what() const throw()
    {
        return _msg.c_str();
    }

    virtual ~Exception() throw(){};
};

}

namespace device {
class AudioStereoModeImpl {
public:
    virtual ~AudioStereoModeImpl() = default;
    virtual const std::string& getName() const = 0;

    virtual std::string toString() = 0;
    
};

class AudioStereoMode {
protected:
    static AudioStereoModeImpl* impl;

public:
    AudioStereoMode();
    static void setImpl(AudioStereoModeImpl* newImpl);
    const std::string& getName() const;
    
	static const int kMono;       //!< Indicates audio mode of type mono.
	static const int kStereo;     //!< Indicates audio mode of type stereo.
	static const int kSurround;   //!< Indicates audio mode of type surround.
	static const int kPassThru;   //!< Indicates audio mode of type pass through.
	static const int kDD;         //!< Indicates audio mode of type dolby digital.
	static const int kDDPlus;     //!< Indicates audio mode of type dolby digital plus.
	static const int kMax;        //!< Indicates maximum number of audio modes supported.

    bool operator==(const AudioStereoMode& other) const {

        return this == &other; 
    }

    std::string toString();
    AudioStereoMode(int id);    
};

}

namespace device {

class AudioOutputPortTypeImpl {
public:
    virtual ~AudioOutputPortTypeImpl() = default;
    
    virtual int getId() const = 0;
};

class AudioOutputPortType {
protected:
    static AudioOutputPortTypeImpl* impl;

public:
    AudioOutputPortType();
    static void setImpl(AudioOutputPortTypeImpl* newImpl);

    static const int kHDMI;
    static const int kARC;
    static const int kSPDIF;
    int getId() const;


};

}
namespace device {

class AudioOutputPortImpl {
public:
    virtual ~AudioOutputPortImpl() = default;

    virtual const AudioOutputPortType& getType() const = 0;

    virtual const std::string& getName() const = 0;
    virtual std::vector<std::string> getMS12AudioProfileList() const = 0;
    virtual void getAudioCapabilities(int* capabilities) = 0;
    virtual void getMS12Capabilities(int* capabilities) = 0;
    virtual bool isAudioMSDecode() const = 0;
    
    virtual bool getEnablePersist() = 0;
    virtual void getHdmiArcPortId(int *portId) = 0;
    virtual bool isConnected() = 0;
    virtual void setDRCMode(int DRCMode) = 0;
    virtual void setCompression (int compresionLevel) = 0;
    virtual int getCompression() = 0;
    virtual void setDolbyVolumeMode(bool dolbyVolumeMode) = 0;
    virtual void setDialogEnhancement(int enhancerlevel) = 0;
    virtual int getDialogEnhancement() = 0;
    virtual void resetDialogEnhancement() = 0;
    virtual void setIntelligentEqualizerMode (int intelligentEqualizerMode) = 0;
    virtual int getIntelligentEqualizerMode() = 0;
    virtual void setGraphicEqualizerMode (int graphicEqualizerMode) = 0;
    virtual int getGraphicEqualizerMode() = 0;
    virtual void setMS12AudioProfile(std::string audioProfileName) = 0;
    virtual void resetVolumeLeveller() = 0;
    virtual void resetSurroundVirtualizer() = 0;
    virtual void resetBassEnhancer() = 0;
    virtual void getSupportedARCTypes(int *types) = 0;
    virtual void setEnablePersist(bool pEnable) = 0;
    virtual dsError_t setEnablePort(bool pEnable) = 0;
    virtual bool isMuted() = 0;
    virtual void setAudioAtmosOutputMode (int enable) = 0;
    virtual bool getSinkDeviceAtmosCapability(dsATMOSCapability_t& atmosCapability) = 0;
    virtual void setAudioDelayOffset(int audioDelayOffsetMs) = 0;
    virtual void setAudioDelay(int audioDelayMs) = 0;
    virtual void getAudioDelay(uint32_t audioDelayMs) = 0;
    virtual void getSecondaryLanguage(std::string secondaryLanguage) = 0;
    virtual void setSecondaryLanguage(std::string secondaryLanguage) = 0;
    virtual void setPrimaryLanguage(std::string primaryLanguage) = 0;
    virtual void getPrimaryLanguage(std::string primaryLanguage) = 0;
    virtual void getFaderControl(int *mixerBalance) = 0;
    virtual void setFaderControl(int mixerBalance) = 0;
    virtual void setAssociatedAudioMixing(bool mixing) = 0; 
    virtual void setGain(float newGain) = 0;
    virtual void setMISteering(bool MISteering) = 0;
    virtual void setSurroundVirtualizer(dsSurroundVirtualizer_t surroundVirtualizer) = 0;
    virtual void setBassEnhancer(int bassBoost) = 0;
    virtual void enableSurroundDecoder(bool enableSurroundDecoder) = 0;
    virtual void setVolumeLeveller(dsVolumeLeveller_t volumeLeveller) = 0;
    virtual bool getMISteering() = 0;
    virtual dsSurroundVirtualizer_t getSurroundVirtualizer() = 0;
    virtual int getDRCMode() = 0;
    virtual float getLevel() = 0;
    virtual float getGain() = 0;
    virtual bool isSurroundDecoderEnabled() = 0;
    virtual int getBassEnhancer() = 0;
    virtual dsVolumeLeveller_t getVolumeLeveller() = 0;
    virtual bool getStereoAuto() = 0;
    virtual void setStereoAuto(bool stereoAuto, bool persist) = 0;
    virtual std::string getMS12AudioProfile() = 0;
    virtual void setMS12AudioProfileSetttingsOverride(std::string audioProfileState,std::string audioProfileName,std::string audioProfileSettingsName, std::string audioProfileSettingValue) = 0;
    virtual void setLevel(float level) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual void getAssociatedAudioMixing(bool *mixing) = 0;
    virtual bool isEnabled() = 0;
    virtual void setSAD(std::vector<int> sad_list) = 0;
    virtual void reInitializeAudioOutputPort() = 0;
    virtual void getAudioDelayOffset (uint32_t audioDelayOffsetMs) = 0;
    // virtual const List<AudioStereoMode> getSupportedStereoModes() const = 0;
    virtual std::vector<device::AudioStereoMode> getSupportedStereoModes() const = 0;

    
    
    virtual AudioStereoMode getStereoMode() const= 0;
    

    virtual void enableARC(dsAudioARCTypes_t type, bool enable) = 0;

    virtual uint32_t getDolbyVolumeMode() const = 0;
    
    virtual void setStereoMode(const std::string &mode, bool persist) = 0;


};

class AudioOutputPort {
protected:
    static AudioOutputPortImpl* impl;

public:
    AudioOutputPort();
    static void setImpl(AudioOutputPortImpl* newImpl);
    static AudioOutputPort& getInstance();

    const AudioOutputPortType& getType() const;
    
    // dsATMOSCapability_t atmosCapability=dsATMOSCapability_t::dsAUDIO_ATMOS_ATMOSMETADATA;
    const std::string& getName() const;
    std::vector<std::string> getMS12AudioProfileList() const;
    void getAudioCapabilities(int* capabilities);
    void getMS12Capabilities(int* capabilities);
    bool isAudioMSDecode();
    void reInitializeAudioOutputPort();
    bool getEnablePersist();
    void getHdmiArcPortId(int *portId);
    bool isConnected();
    
    int getDRCMode();
    void setDRCMode(int DRCMode);
    void setCompression (int compresionLevel);
    int getCompression();
    void setDolbyVolumeMode(bool dolbyVolumeMode);
    
    void setDialogEnhancement(int enhancerlevel);
    int getDialogEnhancement();
    void setIntelligentEqualizerMode (int intelligentEqualizerMode);
    int getIntelligentEqualizerMode();
    void setGraphicEqualizerMode (int graphicEqualizerMode);
    int getGraphicEqualizerMode();
    void setMS12AudioProfile(std::string audioProfileName);
    void resetVolumeLeveller();
    void resetSurroundVirtualizer();
    void resetBassEnhancer();
    void resetDialogEnhancement();
    void getSupportedARCTypes(int *types);
    void setEnablePersist(bool pEnable);
    dsError_t setEnablePort(bool pEnable);
    bool isMuted();
    void setAudioAtmosOutputMode (int enable);
    bool getSinkDeviceAtmosCapability(dsATMOSCapability_t& atmosCapability);
    void setAudioDelayOffset(int audioDelayOffsetMs);
    void setAudioDelay(int audioDelayMs);
    void getAudioDelay(uint32_t audioDelayMs);
    void getSecondaryLanguage(std::string secondaryLanguage);
    void setSecondaryLanguage(std::string secondaryLanguage);
    void setPrimaryLanguage(std::string primaryLanguage);
    void getPrimaryLanguage(std::string primaryLanguage);
    void getFaderControl(int *mixerBalance);
    void setFaderControl(int mixerBalance);
    void setAssociatedAudioMixing(bool mixing);
    void setGain(float newGain);
    void setMISteering(bool MISteering);
    void setSurroundVirtualizer(dsSurroundVirtualizer_t surroundVirtualizer);
    void setBassEnhancer(int bassBoost);
    void enableSurroundDecoder(bool enableSurroundDecoder);
    void setVolumeLeveller(dsVolumeLeveller_t volumeLeveller);
    bool getMISteering();
    dsSurroundVirtualizer_t getSurroundVirtualizer();
    float getLevel();
    float getGain();
    bool isSurroundDecoderEnabled();
    int getBassEnhancer();
    dsVolumeLeveller_t getVolumeLeveller();
    bool getStereoAuto();
    void setStereoAuto(bool stereoAuto, bool persist);
    std::string getMS12AudioProfile();
    void setMS12AudioProfileSetttingsOverride(std::string audioProfileState,std::string audioProfileName,std::string audioProfileSettingsName, std::string audioProfileSettingValue);
    void setLevel(float level);
    void setMuted(bool muted);
    void getAssociatedAudioMixing(bool *mixing);
    bool isEnabled();
    void setSAD(std::vector<int> sad_list);
    void getAudioDelayOffset (uint32_t audioDelayOffsetMs);
    
    // const List<AudioStereoMode> getSupportedStereoModes() const;
    const std::vector<device::AudioStereoMode> getSupportedStereoModes() const;

    AudioStereoMode getStereoMode() const;
    
    void enableARC(dsAudioARCTypes_t type, bool enable);
    uint32_t getDolbyVolumeMode() const;
    void setStereoMode(const std::string &mode, bool persist);



    
};

}

namespace device {
class CompositeInputImpl {
public:
    virtual ~CompositeInputImpl() = default;
    virtual uint8_t getNumberOfInputs() const = 0;
    virtual bool isPortConnected(int8_t Port) const = 0;
    virtual void selectPort(int8_t Port) const = 0;
    virtual void scaleVideo(int32_t x, int32_t y, int32_t width, int32_t height) const = 0;
};

class CompositeInput {
protected:
    static CompositeInputImpl* impl;

public:
    CompositeInput();
    static void setImpl(CompositeInputImpl* newImpl);
    static CompositeInput& getInstance();

    uint8_t getNumberOfInputs() const;
    bool isPortConnected(int8_t port) const;
    void selectPort(int8_t port) const;
    void scaleVideo(int32_t x, int32_t y, int32_t width, int32_t height) const;
};

}

namespace device {

class HdmiInputImpl {
public:
    virtual ~HdmiInputImpl() = default;

    virtual uint8_t getNumberOfInputs() const = 0;
    virtual bool isPortConnected(int8_t Port) const = 0;
    virtual std::string getCurrentVideoMode() const = 0;
    virtual void selectPort(int8_t Port,bool audioMix = false, int videoPlane = 0,bool topMost = false) const = 0;
    virtual void scaleVideo(int32_t x, int32_t y, int32_t width, int32_t height) const = 0;
    // virtual void setAudioMixerLevels(dsAudioInput_t gain, int volume) const = 0;
    virtual void getEDIDBytesInfo(int iHdmiPort, std::vector<uint8_t>& edid) const = 0;
    virtual void getHDMISPDInfo(int iHdmiPort, std::vector<uint8_t>& data) const = 0;
    virtual void setEdidVersion(int iHdmiPort, int iEdidVersion) const = 0;
    virtual void getEdidVersion(int iHdmiPort, int* iEdidVersion) const = 0;
    virtual void getHdmiALLMStatus(int iHdmiPort, bool* allmStatus) const = 0;
    virtual void getSupportedGameFeatures(std::vector<std::string>& featureList) const = 0;
    virtual void getAVLatency(int *audio_output_delay, int *video_latency) const = 0;
    virtual void setEdid2AllmSupport(int iHdmiPort, bool allmsupport) const = 0;
    virtual void getEdid2AllmSupport(int iHdmiPort, bool *allmsupport) const = 0;
    virtual void getCurrentVideoModeObj (dsVideoPortResolution_t &resolution) const = 0;
};

class HdmiInput {
protected:
    static HdmiInputImpl* impl;

public:
    HdmiInput();
    static void setImpl(HdmiInputImpl* newImpl);
    static HdmiInput& getInstance(); // Changed return type

    uint8_t getNumberOfInputs() const;
    bool isPortConnected(int8_t port) const;
    std::string getCurrentVideoMode() const;
    void selectPort(int8_t Port,bool audioMix = false, int videoPlane = 0,bool topMost = false) const;
    void scaleVideo(int32_t x, int32_t y, int32_t width, int32_t height) const;
    void getEDIDBytesInfo(int iHdmiPort, std::vector<uint8_t>& edid) const;
    void getHDMISPDInfo(int iHdmiPort, std::vector<uint8_t>& data) const;
    void setEdidVersion(int iHdmiPort, int iEdidVersion) const;
    void getEdidVersion(int iHdmiPort, int* iEdidVersion) const;

    void getHdmiALLMStatus(int iHdmiPort, bool* allmStatus) const;
    void getSupportedGameFeatures(std::vector<std::string>& featureList) const;
    void getAVLatency(int *audio_output_delay, int *video_latency)const;
    void setEdid2AllmSupport(int iport, bool allmSupport) const;
    void getEdid2AllmSupport(int iport, bool *allmSupport) const;
    void getCurrentVideoModeObj (dsVideoPortResolution_t &resolution) const;
};

}

namespace device {
class SleepMode;
class SleepModeImpl {
public:
    virtual ~SleepModeImpl() = default;
    virtual SleepMode & getInstanceById(int id) = 0;
    virtual SleepMode & getInstanceByName(const std::string &name) = 0;
    virtual List<SleepMode> getSleepModes() = 0;
    virtual const std::string& toString() const = 0;
};

class SleepMode {
protected:
    static SleepModeImpl* impl;

public:
    SleepMode();
    static void setImpl(SleepModeImpl* newImpl);

    static SleepMode& getInstance();
    static SleepMode& getInstance(int id);
    static SleepMode& getInstance(const std::string &name);

    List<SleepMode> getSleepModes();
    const std::string& toString() const;
};
}


namespace device {

class VideoDFCImpl {
public:
    virtual ~VideoDFCImpl() = default;
    virtual const std::string& getName() const = 0;
    
};

class VideoDFC {
protected:
    static VideoDFCImpl* impl;

public:
    VideoDFC();
    static void setImpl(VideoDFCImpl* newImpl);
    const std::string& getName() const;
    
};

}

namespace device {

class VideoDeviceImpl {
public:
    virtual ~VideoDeviceImpl() = default;

    virtual int getFRFMode(int* frfmode) const = 0;
    virtual int setFRFMode(int frfmode) const = 0;
    virtual int getCurrentDisframerate(char* framerate) const = 0;
    virtual int setDisplayframerate(const char* framerate) const = 0;
    virtual void setDFC(std::string zoomSetting) = 0;
    virtual const VideoDFC& getDFC() = 0;
    virtual void getHDRCapabilities(int* capabilities) = 0;
    virtual void getSettopSupportedResolutions(std::list<std::string>& resolutions) = 0;
};

class VideoDevice {
protected:
    static VideoDeviceImpl* impl;

public:
    VideoDevice();

    static void setImpl(VideoDeviceImpl* newImpl);
    static VideoDevice& getInstance();

    int getFRFMode(int* frfmode) const;
    int setFRFMode(int frfmode) const;
    int getCurrentDisframerate(char* framerate) const;
    int setDisplayframerate(const char* framerate) const;
    
    const VideoDFC& getDFC();
    void setDFC(std::string zoomSetting);
    void getHDRCapabilities(int* capabilities);
    void getSettopSupportedResolutions(std::list<std::string>& resolutions);
    
};

}

namespace device {

class VideoResolutionImpl {
public:
    virtual ~VideoResolutionImpl() = default;

    virtual const std::string& getName() const = 0;
};

class VideoResolution {
protected:
    static VideoResolutionImpl* impl;

public:
   static void setImpl(VideoResolutionImpl* newImpl);
   const std::string& getName() const;
};

}

namespace device {

class VideoOutputPortTypeImpl {
public:
    virtual ~VideoOutputPortTypeImpl() = default;

    virtual int getId() const = 0;
    virtual const List<VideoResolution> getSupportedResolutions() const = 0;
};

class VideoOutputPortType {
protected:
    static VideoOutputPortTypeImpl* impl;

public:
    VideoOutputPortType();

    static void setImpl(VideoOutputPortTypeImpl* newImpl);

    int getId() const;
    const List<VideoResolution> getSupportedResolutions() const;
};

}

namespace device{
        class DisplayImpl{
                public:
                virtual ~DisplayImpl() = default;

                virtual void getEDIDBytes(std::vector<uint8_t> &edid) const = 0;
                virtual bool isConnectedDeviceRepeater() = 0;
                virtual int getSurroundMode() = 0;
        };
        class Display {
    protected:
       static DisplayImpl* impl;

    public:
       static void setImpl(DisplayImpl* newImpl);
       static Display& getInstance();

       void getEDIDBytes(std::vector<uint8_t>& edid);
       int getSurroundMode();
       bool isConnectedDeviceRepeater();

};

}

namespace device {

class VideoOutputPortImpl {
public:
    virtual ~VideoOutputPortImpl() = default;

    virtual const VideoOutputPortType& getType() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const VideoResolution& getDefaultResolution() const = 0;
    virtual int getHDCPProtocol() = 0;
    virtual int getHDCPReceiverProtocol() = 0;
    virtual int getHDCPCurrentProtocol() = 0;
    virtual int getHDCPStatus() = 0;
    virtual AudioOutputPort& getAudioOutputPort() const = 0;
    virtual bool isDisplayConnected() = 0;
    virtual bool isContentProtected() = 0;
    virtual Display& getDisplay() = 0;
    virtual bool setScartParameter(std::string sScartParameter, std::string sScartParameterData) = 0;
    
    virtual void getTVHDRCapabilities(int* capabilities) = 0;
    virtual  bool setForceHDRMode (dsHDRStandard_t mode) = 0;
    virtual int getPreferredColorDepth(bool persist) = 0;
    virtual void getColorDepthCapabilities(unsigned int *capabilities) = 0;
    virtual void setPreferredColorDepth(dsDisplayColorDepth_t colorDepth, bool persist) = 0;
     virtual bool getCurrentOutputSettings(int& videoEOTF, int& matrixCoefficients, int& colorSpace, int& colorDepth, int& quantizationRange) = 0;
     virtual bool isConnected() const = 0;
    virtual void setResolution(std::string resolution, bool persist, bool isIgnoreEdid) = 0;
    
    virtual const VideoResolution& getResolution() const =0;

    virtual void getSupportedTvResolutions(int *tvResolutions) = 0;
    virtual int getVideoEOTF() = 0;
    virtual bool isActive() = 0;
    

};

class VideoOutputPort {
protected:
    static VideoOutputPortImpl* impl;

public:
    VideoOutputPort();

    static void setImpl(VideoOutputPortImpl* newImpl);
    static VideoOutputPort& getInstance();

    const VideoOutputPortType& getType() const;
    const std::string& getName() const;
    const VideoResolution& getDefaultResolution() const;
    int getHDCPProtocol();
    int getHDCPReceiverProtocol();
    int getHDCPCurrentProtocol();
    int getHDCPStatus();
    AudioOutputPort& getAudioOutputPort();
    bool isDisplayConnected();
    bool isContentProtected();
    Display& getDisplay();
    void getSupportedTvResolutions(int *tvResolutions);
    const VideoResolution& getResolution() const;

    void setResolution(std::string resolution, bool persist, bool isIgnoreEdid);
    bool getCurrentOutputSettings(int& videoEOTF, int& matrixCoefficients, int& colorSpace, int& colorDepth, int& quantizationRange);
    
    void setPreferredColorDepth(dsDisplayColorDepth_t colorDepth, bool persist);
    void  getColorDepthCapabilities(unsigned int *capabilities);
    int getPreferredColorDepth(bool persist);
    bool setForceHDRMode (dsHDRStandard_t mode);
    void getTVHDRCapabilities(int* capabilities);
    bool setScartParameter(std::string sScartParameter, std::string sScartParameterData);
    int getVideoEOTF();
    bool isActive();
    

};

}

namespace device {

class VideoOutputPortConfigImpl {
public:
    virtual ~VideoOutputPortConfigImpl() = default;

    virtual VideoOutputPortType& getPortType(int id) = 0;
    virtual VideoOutputPort& getPort(const std::string& name) = 0;
};

class VideoOutputPortConfig {
protected:
    static VideoOutputPortConfigImpl* impl;

public:
    VideoOutputPortConfig();

    static void setImpl(VideoOutputPortConfigImpl* newImpl);
    static VideoOutputPortConfig& getInstance();

    VideoOutputPortType& getPortType(int id);
    VideoOutputPort& getPort(const std::string& name);
};

}

class ManagerImpl {
public:
    virtual ~ManagerImpl() = default;

    virtual void Initialize() = 0;
    virtual void DeInitialize() = 0;
};

namespace device {
class Manager {
protected:
    static ManagerImpl* impl;

public:
    Manager();

    static void setImpl(ManagerImpl* newImpl);
    static Manager& getInstance();

    static void Initialize();
    static void DeInitialize();
};
}

namespace device {

class HostImpl {
public:
    virtual ~HostImpl() = default;
    virtual SleepMode getPreferredSleepMode() = 0;
    virtual int setPreferredSleepMode(const SleepMode mode) = 0;
    virtual List<SleepMode> getAvailableSleepModes() = 0;
    virtual List<VideoOutputPort> getVideoOutputPorts() = 0;
    virtual List<AudioOutputPort> getAudioOutputPorts() = 0;
    virtual List<VideoDevice> getVideoDevices() = 0;
    virtual VideoOutputPort& getVideoOutputPort(const std::string& name) = 0;
    virtual AudioOutputPort& getAudioOutputPort(const std::string& name) = 0;
    virtual void getHostEDID(std::vector<uint8_t>& edid) const = 0;
    virtual std::string getDefaultVideoPortName() = 0;
    virtual std::string getDefaultAudioPortName() = 0;

    virtual void getMS12ConfigDetails(std::string type)=0;
    virtual bool isHDMIOutPortPresent() = 0;
    virtual void getSecondaryLanguage(std::string secondaryLanguage) = 0;
    virtual void setSecondaryLanguage(std::string secondaryLanguage) = 0;
    virtual void getSinkDeviceAtmosCapability(dsATMOSCapability_t atmosCapability) = 0;
    virtual void getFaderControl(int *mixerBalance) = 0;
    virtual void setFaderControl(int mixerBalance) = 0;
    virtual void setPrimaryLanguage(std::string primaryLanguage) = 0;
    virtual void getPrimaryLanguage(std::string primaryLanguage) = 0;
    virtual void setAudioAtmosOutputMode (int enable) = 0;
    virtual void setAssociatedAudioMixing(bool mixing) = 0;
    virtual void getCurrentAudioFormat(dsAudioFormat_t audioFormat) = 0;
    virtual void getAssociatedAudioMixing(bool *mixing) = 0;
    virtual void setAudioMixerLevels(dsAudioInput_t inputType, int volumeLevel) = 0; 
};

class Host {
protected:
    static HostImpl* impl;

public:
    static void setImpl(HostImpl* newImpl);
    static Host& getInstance();

    SleepMode getPreferredSleepMode();
    int setPreferredSleepMode(const SleepMode mode);
    List<SleepMode> getAvailableSleepModes();
    List<VideoOutputPort> getVideoOutputPorts();
    List<AudioOutputPort> getAudioOutputPorts();
    List<VideoDevice> getVideoDevices();
    VideoOutputPort& getVideoOutputPort(const std::string& name);
    AudioOutputPort& getAudioOutputPort(const std::string& name);
    void getHostEDID(std::vector<uint8_t>& edid) const;
    std::string getDefaultVideoPortName();
    std::string getDefaultAudioPortName();

    void getMS12ConfigDetails(std::string type);
    bool isHDMIOutPortPresent();
    void getSecondaryLanguage(std::string secondaryLanguage);
    void setSecondaryLanguage(std::string secondaryLanguage);
    void getSinkDeviceAtmosCapability(dsATMOSCapability_t atmosCapability);
    void getFaderControl(int *mixerBalance);
    void setFaderControl(int mixerBalance);
    void setPrimaryLanguage(std::string primaryLanguage);
    void getPrimaryLanguage(std::string primaryLanguage);
    void setAudioAtmosOutputMode (int enable);
    void setAssociatedAudioMixing(bool mixing);
    void getCurrentAudioFormat(dsAudioFormat_t audioFormat);
    void getAssociatedAudioMixing(bool *mixing);
    void setAudioMixerLevels(dsAudioInput_t gain, int volume) ;
};

}

namespace device {
class FrontPanelIndicator {
public:
    class Color;
    class ColorImpl {
    public:
        virtual ~ColorImpl() = default;
        virtual const Color& getInstanceById(int id) = 0;
        virtual const Color& getInstanceByName(const std::string& name) = 0;
        virtual std::string getName() const = 0;
    };

    class Color {
protected:
    static ColorImpl* impl;

public:
    Color();

    static void setImpl(ColorImpl* newImpl);
    static Color& getInstance();
    static const Color& getInstance(int id);
    static const Color& getInstance(const std::string& name);

    static const int kWhite;

    std::string getName() const;
    virtual ~Color();
    };

    static FrontPanelIndicator& getInstance()
    {
        static FrontPanelIndicator instance;
        return instance;
    }

    class FrontPanelIndicatorImpl {
    public:
        virtual ~FrontPanelIndicatorImpl() = default;
        virtual FrontPanelIndicator& getInstanceInt(int id) = 0;
        virtual FrontPanelIndicator& getInstanceString(const std::string& name) = 0;
        virtual void setState(const bool bState) const = 0;
        virtual std::string getName() const = 0;
        virtual void setBrightness(const int brightness, const bool toPersist) const = 0;
        virtual int getBrightness() const = 0;
        virtual void setColor(const Color &newColor, const bool toPersist) const = 0;
        virtual void setColorInt(const uint32_t color, const bool toPersist) const = 0;
        virtual void getBrightnessLevels(int &levels,int &min,int &max) const = 0;
        virtual int getColorMode() const = 0;
        virtual std::string getColorName() const = 0;
        virtual List<Color> getSupportedColors() const =0;
    };

    FrontPanelIndicatorImpl* impl;

    static FrontPanelIndicator& getInstance(int id)
    {
        return getInstance().impl->getInstanceInt(id);
    }
    static FrontPanelIndicator& getInstance(const std::string& name)
    {
        return getInstance().impl->getInstanceString(name);
    }

    void setState(const bool bState) const
    {
        return impl->setState(bState);
    }

    std::string getName() const
    {
        return impl->getName();
    }
    void setBrightness(const int brightness, const bool toPersist = true)
    {
        return impl->setBrightness(brightness, toPersist);
    }
    int getBrightness()
    {
        return impl->getBrightness();
    }
    void setColor(const Color &newColor, bool toPersist = true)
    {
        return impl->setColor(newColor, toPersist);
    }
    void setColor(const uint32_t color, const bool toPersist = true)
    {
        return impl->setColorInt(color, toPersist);
    }
    void getBrightnessLevels(int &levels,int &min,int &max) const
    {
        return impl->getBrightnessLevels( levels, min, max);
    }
    List<Color> getSupportedColors()
    {
        return impl->getSupportedColors();
    }
    int getColorMode() const
    {
        return impl->getColorMode();
    }
};

class FrontPanelTextDisplay;
class FrontPanelTextDisplayImpl {
public:
    virtual ~FrontPanelTextDisplayImpl() = default;
    virtual int getCurrentTimeFormat() const = 0;
    virtual void setTimeFormat(const int iTimeFormat) const = 0;
    virtual void setText(const std::string text) const = 0;
    virtual void setMode(int mode) const = 0;
    virtual int getTextBrightness() const = 0;
    virtual void setTextBrightness(const int brightness) const = 0;
    virtual FrontPanelTextDisplay& getInstanceById(int id) = 0;
    virtual FrontPanelTextDisplay& getInstanceByName(const std::string& name) = 0;
};

class FrontPanelTextDisplay : public FrontPanelIndicator{
protected:
    static FrontPanelTextDisplayImpl* impl;

public:
    FrontPanelTextDisplay();

    static void setImpl(FrontPanelTextDisplayImpl* newImpl);

    int getCurrentTimeFormat() const;
    void setTimeFormat(const int iTimeFormat);
    static FrontPanelTextDisplay& getInstance(const std::string& name);
    static FrontPanelTextDisplay& getInstance(int id);
    static FrontPanelTextDisplay& getInstance();
    static const int kModeClock12Hr;
    static const int kModeClock24Hr;

    void setText(const std::string text);
    void setMode(int mode);
    int getTextBrightness() const;
    void setTextBrightness(const int brightness) const;
};

class FrontPanelConfig;
class FrontPanelConfigImpl {
public:
    virtual ~FrontPanelConfigImpl() = default;
    virtual List<FrontPanelIndicator> getIndicators() const = 0;
    virtual FrontPanelTextDisplay& getTextDisplay() const = 0;
    virtual FrontPanelTextDisplay& getTextDisplay(const std::string &name) const = 0;
    virtual List<FrontPanelTextDisplay> getTextDisplays() const = 0;
    virtual FrontPanelTextDisplay& getTextDisplay(int id) const = 0;
};

class FrontPanelConfig {
protected:
    static FrontPanelConfigImpl* impl;

public:
    FrontPanelConfig();

    static void setImpl(FrontPanelConfigImpl* newImpl);
    static FrontPanelConfig& getInstance();

    List<FrontPanelIndicator> getIndicators();
    FrontPanelTextDisplay& getTextDisplay(const std::string& name);
    List<FrontPanelTextDisplay> getTextDisplays();
    FrontPanelTextDisplay& getTextDisplay(int id);
    FrontPanelTextDisplay& getTextDisplay() const;
};

}


#define RT_OK 0
#define RT_ERROR 1
#define RT_ERROR_QUEUE_EMPTY 1006


typedef uint32_t rtError;
typedef void(*remoteDisconnectCallback)(void *data);

class rtValue;

class rtIObject 
{
  public:
    typedef unsigned long refcount_t;
    
    virtual ~rtIObject(){
    }
};

class rtString{
    public:
        std::string mData;
        rtString(const char* s){
            mData = s;
        }
        rtString(const rtString& s){
            mData = s.mData;
        }
        rtString(){
            mData = "";
        }
        const char* cString() const{
            return mData.c_str();
        }
        rtString& operator=(const char* s){
            mData = s;
            return *this;
        }

        rtString& operator=(const rtString& s){
            mData = s.mData;
            return *this;
        }

};
typedef rtError (*rtFunctionCB)(int numArgs, const rtValue* args, rtValue* result, void* context);

class rtFunctionCallback{
    public:
        rtFunctionCallback(rtFunctionCB cb, void* context = NULL){

        }
        ~rtFunctionCallback() = default;
        
};

/*
Based on pxCore, Copyright 2015-2018 John Robinson
Licensed under the Apache License, Version 2.0
*/
template <class T>
class rtRef
{
public:
  rtRef():                  mRef(NULL) {}
  rtRef(const T* p):        mRef(NULL) {asn(p);         }
  rtRef(const rtRef<T>& r): mRef(NULL) {asn(r.getPtr());}
  rtRef(rtRef<T>&& r) noexcept: mRef(r.mRef) {r.mRef = nullptr;}
  virtual ~rtRef()                     {
  }

  T* operator->()   const {return mRef;}
  operator T* ()    const {return mRef;}
  T* getPtr()       const {return mRef;}
  T* ptr()          const {return mRef;}
  T& operator*()    const {return *mRef;}
 
  bool operator! () const {return mRef == NULL; }  
  inline rtRef<T>& operator=(const T* p)                                  {asn(p);      return *this;}
  inline rtRef<T>& operator=(const rtRef<T>& r)                           {asn(r.mRef); return *this;}

  inline friend bool operator==(const T* lhs,const rtRef<T>& rhs)         {return lhs==rhs.mRef;}
  inline friend bool operator==(const rtRef<T>& lhs,const T* rhs)         {return lhs.mRef==rhs;}
  inline friend bool operator==(const rtRef<T>& lhs,const rtRef<T>& rhs)  {return lhs.mRef==rhs.mRef;}
  
  void asn(const T* p) 
  {
    if (mRef != p) 
    {
      if (mRef) 
      {
        delete mRef;
        mRef = NULL;
      }
      mRef = const_cast<T*>(p);
      
    }
  }

  T* mRef;
};

class rtObjectBaseImpl{
    public:
    virtual ~rtObjectBaseImpl() = default;
    virtual rtError set(const char* name, const char* value) const = 0;
    virtual rtError set(const char* name, bool value) const = 0;
    virtual rtError set(const char* name, const rtValue& value) const = 0;
    virtual rtString get(const char* name) const = 0;
    virtual rtError sendReturns(const char* messageName, rtString& result) const  = 0;
};

class rtObjectBase{
protected:
        static rtObjectBaseImpl* impl;
public:

    static rtObjectBase& getInstance();
    static void setImpl(rtObjectBaseImpl* newImpl);

    rtError set(const char* name, const char* value);
    rtError set(const char* name, bool value);
    rtError set(const char* name, const rtValue& value);

        //To avoid linker issues with templated code, the complete definition of this
        //templated function is included in this header file instead of separating it to .cpp
    template <typename T>
    rtError sendReturns(const char* messageName, T& result) {
    return impl->sendReturns(messageName, result);
    }

    template <typename T>
    T get(const char* name) {
        return impl->get(name);
    }
    virtual ~rtObjectBase() = default;
};

class rtMapObject: public rtObjectBase, public rtIObject{
    public:
     virtual ~rtMapObject() = default;

};

class rtObjectRef;
class rtObjectRefImpl{
    public:
        virtual rtError send(const char* messageName, const rtValue& arg1)  = 0;
        virtual rtError send(const char* messageName)  = 0;
        virtual rtError send(const char* messageName, const char* method, rtFunctionCallback* callback)  = 0;
        virtual rtError send(const char* messageName, rtObjectRef& base)  = 0;

};

class rtObjectRef : public rtRef<rtIObject>, public rtObjectBase{
protected:
     static rtObjectRefImpl* impl;
public:


    static rtObjectRef& getInstance();

    static void setImpl(rtObjectRefImpl* newImpl);

    rtObjectRef();

    rtObjectRef(const rtObjectRef&);

    rtObjectRef(const rtMapObject* o);

    rtObjectRef& operator=(rtMapObject* o);

    rtObjectRef& operator=(const rtObjectRef&);

    rtObjectRef& operator=(rtIObject* o);

    rtObjectRef& operator=(rtObjectRef&&);

    rtError send(const char* messageName);

    rtError send(const char* messageName, const char* method, rtFunctionCallback* callback);

    rtError send(const char* messageName, const rtValue& arg1);

    rtError send(const char* messageName, rtObjectRef& base);
    virtual ~rtObjectRef();

};

class rtArrayObject;

struct rtValue_{
    std::string stringValue;
    bool boolValue;
};

class rtValueImpl{
    public:
        virtual void rtValueConstructor(bool v) const = 0;
        virtual void rtValueConstructor(const char* v) const = 0;
        virtual void rtValueConstructor(rtArrayObject* v) const = 0;
        virtual void rtValueConstructor(const rtString& v) const = 0;
};

class rtValue
{
 protected:
  static   rtValueImpl* impl;
 public:
  rtValue_ mValue;

  static rtValue& getInstance();
  static void setImpl(rtValueImpl* newImpl);

  rtValue();
  rtValue(bool v);
  rtValue(const char* v);
  rtValue(const rtString& v);
  rtValue(rtIObject* v);

  rtValue(const rtObjectRef& v);
  rtValue(const rtValue& v);
  ~rtValue();
  rtValue& operator=(bool v);
  rtValue& operator=(const char* v);
  rtValue& operator=(const rtString& v);
  rtValue& operator=(const rtIObject* v);
  rtValue& operator=(const rtObjectRef& v);
  rtValue& operator=(const rtValue& v);

  rtObjectRef toObject() const;
  void setString (const char* v);
  void setString (const rtString& v);

};

class rtArrayObjectImpl{
    public:
        virtual void pushBack(const char* v) const = 0;
        virtual void pushBack(rtValue v) const = 0;
};


class rtArrayObject : public rtObjectBase, public rtIObject{
protected:
    static rtArrayObjectImpl* impl;
public:
    static rtArrayObject& getInstance();
    static void setImpl(rtArrayObjectImpl* newImpl);

    void pushBack(const char* v);
    void pushBack(rtValue v);
    virtual ~rtArrayObject() = default;

};

class rtRemoteEnvironment{


};

class floatingRtFunctionsImpl{
    public:
        virtual ~floatingRtFunctionsImpl() = default;
        virtual rtError rtRemoteLocateObject(rtRemoteEnvironment *env, const char* str, rtObjectRef& obj, int x, remoteDisconnectCallback back, void *cbdata=NULL) = 0;
        virtual rtRemoteEnvironment* rtEnvironmentGetGlobal() = 0;
        virtual rtError rtRemoteShutdown(rtRemoteEnvironment *env) = 0;
        virtual rtError rtRemoteInit(rtRemoteEnvironment *env) = 0;
        virtual rtError rtRemoteProcessSingleItem() = 0;
        virtual char* rtStrError(rtError err) = 0;

};

class floatingRtFunctions{
public:
    static  floatingRtFunctionsImpl* impl;
    static floatingRtFunctions& getInstance();
    static  void setImpl(floatingRtFunctionsImpl* newImpl);

};

    rtError rtRemoteProcessSingleItem();
    rtError rtRemoteLocateObject(rtRemoteEnvironment* env, const char* str, rtObjectRef& obj, int x, remoteDisconnectCallback back, void* cbdata = nullptr);
    rtRemoteEnvironment* rtEnvironmentGetGlobal();
    rtError rtRemoteInit(rtRemoteEnvironment* env);
    rtError rtRemoteShutdown(rtRemoteEnvironment* env);
    char* rtStrError(rtError err);


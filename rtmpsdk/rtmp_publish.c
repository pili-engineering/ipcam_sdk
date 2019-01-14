#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "aac_encoder.h"
#include "rtmp_publish.h"
#include "rtmp_sys.h"


#define xDEBUG

#ifndef DEBUG
#define Debug(fmt, arg...)

#else
#define Debug(fmt, arg...) do {\
                                        printf( "file:%s, line:%d->" fmt "\n",  __FILE__, __LINE__, ##arg);\
                                } while(0)
#endif



#define FLV_AUDIO_TAG_HEADER(format, rate, size, type) (((format & 0xf) << 4) | \
                                                        ((rate & 0x3) << 2) | \
                                                        ((size & 0x1) << 1) | \
                                                        (type & 0x1))

// format of SoundData
#define SOUND_FORMAT_LINEAR_PCM_PE   0  // Linear PCM, platform endian
#define SOUND_FORMAT_ADPCM           1  // ADPCM
#define SOUND_FORMAT_MP3             2  // MP3
#define SOUND_FORMAT_LINEAR_PCM_LE   3  // Linear PCM, little endian
#define SOUND_FORMAT_NELLYMOSER_16K  4  // Nellymoser 16 kHz mono
#define SOUND_FORMAT_NELLYMOSER_8K   5  // Nellymose 8 kHz mono
#define SOUND_FORMAT_NELLYMOSER      6  // Nellymoser
#define SOUND_FORMAT_G711A           7  // G.711 A-law logarithmic PCM
#define SOUND_FORMAT_G711U           8  // G.711 mu-law logarithmic PCM
#define SOUND_FORMAT_RESERVED        9  // reserved
#define SOUND_FORMAT_AAC             10 // AAC
#define SOUND_FORMT_SPEEX            11 // Speex
#define SOUND_FORMAT_MP3_8K          14 // MP3 8 kHz
#define SOUND_FORMAT_DEV_SPEC        15 // Device-specific sound

// AAC specific field
#define SOUND_AAC_TYPE_SEQ_HEADER 0 // AAC sequence header
#define SOUND_AAC_TYPE_RAW        1 // AAC raw

#define ASC_OBJTYPE_NULL		0
#define ASC_OBJTYPE_AAC_MAIN		1
#define ASC_OBJTYPE_AAC_LC		2
#define ASC_OBJTYPE_AAC_SSR             3
#define ASC_OBJTYPE_AAC_LTP             4
#define ASC_OBJTYPE_SBR                 5
#define ASC_OBJTYPE_AAC_SCALABLE        6
#define ASC_OBJTYPE_TWINVQ              7
#define ASC_OBJTYPE_CELP                8
#define ASC_OBJTYPE_HXVC                9
#define ASC_OBJTYPE_RESERVED1           10
#define ASC_OBJTYPE_RESERVED2           11
#define ASC_OBJTYPE_TTSI                12
#define ASC_OBJTYPE_MAIN_SYNTHESIS      13
#define ASC_OBJTYPE_WAVETABLE_SYNTHESIS 14
#define ASC_OBJTYPE_GENERAL_MIDI        15
#define ASC_OBJTYPE_ALGORITHMIC_SYNTHESIS_AND_AUDIO_EFFECTS 16
#define ASC_OBJTYPE_ER_AAC_LC           17
#define ASC_OBJTYPE_RESERVED3           18
#define ASC_OBJTYPE_ER_AAC_LTP          19
#define ASC_OBJTYPE_ER_AAC_SCALABLE     20
#define ASC_OBJTYPE_ER_TWINVQ           21
#define ASC_OBJTYPE_ER_BSAC             22
#define ASC_OBJTYPE_ER_AAC_LD           23
#define ASC_OBJTYPE_ER_CELP             24
#define ASC_OBJTYPE_ER_HVXC             25
#define ASC_OBJTYPE_ER_HILN             26
#define ASC_OBJTYPE_ER_PARAMETRIC       27
#define ASC_OBJTYPE_SSC                 28
#define ASC_OBJTYPE_PS                  29
#define ASC_OBJTYPE_MPEG_SURROUND       30
#define ASC_OBJTYPE_ESCAPE_VALUE        31
#define ASC_OBJTYPE_LAYER_1             32
#define ASC_OBJTYPE_LAYER_2             33
#define ASC_OBJTYPE_LAYER_3             34
#define ASC_OBJTYPE_DST                 35
#define ASC_OBJTYPE_ALS                 36
#define ASC_OBJTYPE_SLS                 37
#define ASC_OBJTYPE_SLS_NON_CORE        38
#define ASC_OBJTYPE_ER_AAC_ELD          39
#define ASC_OBJTYPE_SMR_SIMPLE          40
#define ASC_OBJTYPE_SMR_MAIN            41
#define ASC_OBJTYPE_USAC_NO_SBR         42
#define ASC_OBJTYPE_SAOC                43
#define ASC_OBJTYPE_LD_MPEG_SURROUND    44
#define ASC_OBJTYPE_USAC                45

//
// sampling frequencies
//

#define ASC_SF_96000      0
#define ASC_SF_88200      1
#define ASC_SF_64000      2
#define ASC_SF_48000      3
#define ASC_SF_44100      4
#define ASC_SF_32000      5
#define ASC_SF_24000      6
#define ASC_SF_22050      7
#define ASC_SF_16000      8
#define ASC_SF_12000      9
#define ASC_SF_11025      10
#define ASC_SF_8000       11
#define ASC_SF_7350       12
#define ASC_SF_RESERVED_1 13
#define ASC_SF_RESERVED_2 14
#define ASC_SF_CUSTOM     15

//
// channel configurations
//

#define ASC_CHAN_AOT_SPEC         0
#define ASC_CHAN_FC               1
#define ASC_CHAN_FLR              2
#define ASC_CHAN_FCLR             3
#define ASC_CHAN_FCLR_BC          4
#define ASC_CHAN_FCLR_BLR         5
#define ASC_CHAN_FCLR_BLR_LFE     6
#define ASC_CHAN_FCLR_SLR_BLR_LFE 7
#define ASC_CHAN_RESERVED         8


// sampling rate. The following values are defined
#define SOUND_RATE_5_5K  0 // 5.5 kHz
#define SOUND_RATE_11K   1 // 11 kHz
#define SOUND_RATE_22K   2 // 22 kHz
#define SOUND_RATE_44K   3 // 44 kHz

// size of each audio sample
#define SOUND_SIZE_8BIT   0 // 8-bit samples
#define SOUND_SIZE_16BIT  1 // 16-bit samples

// mono or stereo sound
#define SOUND_TYPE_MONO   0 // Mono sound
#define SOUND_TYPE_STEREO 1 // Stereo sound

// AAC specific field
#define SOUND_AAC_TYPE_SEQ_HEADER 0 // AAC sequence header
#define SOUND_AAC_TYPE_RAW        1 // AAC raw





static int SendAacConfigRecord(RtmpPubContext * _pRtmp, const char * _pAacCfgRecord, unsigned int _nSize, unsigned int _nTimeStamp);

static void GetVideoInfoAbs(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, unsigned int * _nResTimeStamp, unsigned char * _nResPacketType, unsigned char * _nChannel);


static void GetAudioInfoAbs(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, unsigned int * _nResTimeStamp, unsigned char * _nResPacketType, unsigned char * _nChannel);

static void GetInfoRelative(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, unsigned int * _nResTimeStamp, unsigned char * _nResPacketType, unsigned char * _nChannel);

static int RtmpPubSendAacConfig(RtmpPubContext * _pRtmp, const char * _pDataWithAdts, unsigned int _nSize, unsigned int _nTimeStamp);
static int RtmpPubSendG711aFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp);
static int RtmpPubSendG711uFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp);
static int RtmpPubSendPcmFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp);

typedef struct _AdtsHeader
{
        unsigned int nSyncWord;
        unsigned int nId;
        unsigned int nLayer;
        unsigned int nProtectionAbsent;
        unsigned int nProfile;
        unsigned int nSfIndex;
        unsigned int nPrivateBit;
        unsigned int nChannelConfiguration;
        unsigned int nOriginal;
        unsigned int nHome;

        unsigned int nCopyrightIdentificationBit;
        unsigned int nCopyrigthIdentificationStart;
        unsigned int nAacFrameLength;
        unsigned int nAdtsBufferFullness;

        unsigned int nNoRawDataBlocksInFrame;
} AdtsHeader;

static void RtmpPubFillNalunit(RtmpPubNalUnit * _pUnit, const char * _pData, unsigned int _nSize)
{
        if (_pUnit->m_pData) {
                free(_pUnit->m_pData);
        }
        _pUnit->m_pData = (char *)malloc(_nSize);
        memcpy(_pUnit->m_pData, _pData, _nSize);
        _pUnit->m_nSize = _nSize;
}

static void RtmpPubDestroyNalunit(RtmpPubNalUnit * _pUnit)
{
        if (_pUnit->m_pData) {
                free(_pUnit->m_pData);
                _pUnit->m_pData = NULL;
                _pUnit->m_nSize = 0;
        }
}

int RtmpPubInit(RtmpPubContext * _pRtmp)
{
        _pRtmp->m_videoType = RTMP_PUB_VIDEOTYPE_AVC;
        if (_pRtmp->m_nAudioInputType <= RTMP_PUB_AUDIO_AAC) {
                return 0;
        }
        if (_pRtmp->m_nAudioOutputType != RTMP_PUB_AUDIO_AAC) {
                return 0;
        }
        int nRet = AacEncoderInit(_pRtmp->m_pAudioEncoderContext);
        if (nRet < 0) {
                Debug("AacEncoderInit");
                return nRet;
        }
        return nRet;
}

void RtmpPubSetVideoType(RtmpPubContext * _pRtmp, RtmpPubVideoType _type)
{
        _pRtmp->m_videoType = _type;
        return;
}

RtmpPubContext * RtmpPubNew(const char * _url, unsigned int _nTimeout, RtmpPubAudioType _nInputAudioType, RtmpPubAudioType _nOutputAudioType, 
                                                                RtmpPubTimeStampPolicy _nTimePolicy)

{
        if (_url == NULL) {
                return NULL;
        }
        
        //
        if (_nInputAudioType != _nOutputAudioType && _nOutputAudioType != RTMP_PUB_AUDIO_AAC) {
                return NULL;
        }

        if (_nOutputAudioType == RTMP_PUB_AUDIO_PCM) {
                return NULL;
        }

        RtmpPubContext * pRtmp = (RtmpPubContext *)malloc(sizeof(*pRtmp));
        memset(pRtmp, 0, sizeof(*pRtmp));
                

        if (_nInputAudioType > RTMP_PUB_AUDIO_AAC && _nOutputAudioType == RTMP_PUB_AUDIO_AAC) {
                pRtmp->m_pAudioEncoderContext = AacEncoderNew();                
                if (NULL == pRtmp->m_pAudioEncoderContext) {
                        free(pRtmp);
                        return NULL;
                }
        }
        
        pRtmp->m_nAudioInputType = _nInputAudioType;
        pRtmp->m_nAudioOutputType = _nOutputAudioType;
        pRtmp->m_nTimePolicy = _nTimePolicy;
        pRtmp->m_pRtmp = RTMP_Alloc();
        RTMP_Init(pRtmp->m_pRtmp);
        
        int len = strlen(_url);
        pRtmp->m_pPubUrl = (char *)malloc(len + 1);
        memcpy(pRtmp->m_pPubUrl, _url, len);
        pRtmp->m_pPubUrl[len] = 0; 

        pRtmp->m_nTimeout = _nTimeout;
        
        pRtmp->m_nLastAudioTimeStamp = -1;
        pRtmp->m_nLastVideoTimeStamp = -1;
        pRtmp->m_nLastMediaTimeStamp = -1;
        pRtmp->m_nMediaTimebase = -1;

        return pRtmp;
}

void RtmpPubDel(RtmpPubContext * _pRtmp)
{
        if (_pRtmp == NULL) {
                return;
        }
        
        if (_pRtmp->m_pAudioEncoderContext != NULL) {
                AacEncoderDel(_pRtmp->m_pAudioEncoderContext);
                _pRtmp->m_pAudioEncoderContext = NULL;
        }

        if (_pRtmp->m_pPubUrl) {
                free(_pRtmp->m_pPubUrl);
        }

        RtmpPubDestroyNalunit(&_pRtmp->m_pSps);
        RtmpPubDestroyNalunit(&_pRtmp->m_pPps);
        RtmpPubDestroyNalunit(&_pRtmp->m_pSei);
        RtmpPubDestroyNalunit(&_pRtmp->m_aac);
       
        if (_pRtmp->m_pRtmp) {
                RTMP_Close(_pRtmp->m_pRtmp);
                RTMP_Free(_pRtmp->m_pRtmp);
                _pRtmp->m_pRtmp = NULL;
        }

        free(_pRtmp);
}

int RtmpPubConnect(RtmpPubContext * _pRtmp)
{
        if (_pRtmp == NULL) {
                return -1;
        }
        struct RTMP * m_pRtmp = _pRtmp->m_pRtmp;

        m_pRtmp->Link.timeout = _pRtmp->m_nTimeout;

        if (RTMP_SetupURL(m_pRtmp, (char *)_pRtmp->m_pPubUrl) == 0) {
                return -1;
        }
        RTMP_EnableWrite(m_pRtmp);
        if (RTMP_Connect(m_pRtmp, NULL) == 0) {
                return -1;
        }
        if (RTMP_ConnectStream(m_pRtmp, 0) == 0) {
                return -1;
        }
        if (RTMP_IsConnected(m_pRtmp) == 0) {
                return -1;
        }
        //The librtmp only set the recv timeout. In some case, the client may block in send forever
        SET_RCVTIMEO(tv, m_pRtmp->Link.timeout);
        if (setsockopt(RTMP_Socket(m_pRtmp), SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv))) {
             return -1;
        }       
        return 0;
}


static int SendPacket(RtmpPubContext * _pRtmp, int _nPacketType, const char * _pData, unsigned int _nSize, unsigned int _nTimestamp, unsigned char _nHeaderType, int channel)
{
        struct RTMP * m_pRtmp = _pRtmp->m_pRtmp;
        RTMPPacket packet;
        RTMPPacket_Reset(&packet);
        RTMPPacket_Alloc(&packet, _nSize);

        packet.m_packetType = _nPacketType;
        packet.m_nChannel = channel;
        packet.m_headerType = _nHeaderType;
        packet.m_nTimeStamp = _nTimestamp;
        packet.m_nInfoField2 = m_pRtmp->m_stream_id;
        packet.m_nBodySize = _nSize;
        memcpy(packet.m_body, _pData, _nSize);
        
        int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);
        RTMPPacket_Free(&packet);
        if (nRet == 0) {
                return -1;
        }
        
        return 0;
}

static int SendAudios(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        unsigned int nPacketStamp = 0;
        unsigned char nHeaderType = 0;
        unsigned char nChannel = 0;
        
        if (_pRtmp->m_nTimePolicy == RTMP_PUB_TIMESTAMP_ABSOLUTE) {
                GetAudioInfoAbs(_pRtmp, _nTimeStamp, &nPacketStamp, &nHeaderType, &nChannel);
        } else {
                GetInfoRelative(_pRtmp, _nTimeStamp, &nPacketStamp, &nHeaderType, &nChannel);
        }
        Debug("Audio Frame send- timestamp:%u, headertype:%d channel:%d", nPacketStamp, nHeaderType, nChannel);
        return SendPacket(_pRtmp, RTMP_PACKET_TYPE_AUDIO, _pData, _nSize, nPacketStamp, nHeaderType, nChannel);
}

static int SendVideos(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        unsigned int nPacketStamp = 0;
        unsigned char nHeaderType = 0;
        unsigned char nChannel = 0;
        if (_pRtmp->m_nTimePolicy == RTMP_PUB_TIMESTAMP_ABSOLUTE) {
                GetVideoInfoAbs(_pRtmp, _nTimeStamp, &nPacketStamp, &nHeaderType, &nChannel);
        } else {
                GetInfoRelative(_pRtmp, _nTimeStamp, &nPacketStamp, &nHeaderType, &nChannel);
        }
        Debug("Video Frame send- timestamp:%u, headertype:%d channel:%d", nPacketStamp, nHeaderType, nChannel);
        return SendPacket(_pRtmp, RTMP_PACKET_TYPE_VIDEO, _pData, _nSize, nPacketStamp, nHeaderType, nChannel);
}

/*
 aligned(8) class AVCDecoderConfigurationRecord {
 unsigned int(8) configurationVersion = 1;
 unsigned int(8) AVCProfileIndication;
 unsigned int(8) profile_compatibility;
 unsigned int(8) AVCLevelIndication;
 bit(6) reserved = ‘111111’b;
 unsigned int(2) lengthSizeMinusOne;
 bit(3) reserved = ‘111’b;
 unsigned int(5) numOfSequenceParameterSets;
 for (i=0; i< numOfSequenceParameterSets; i++) {
   unsigned int(16) sequenceParameterSetLength ;
   bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
 }
 unsigned int(8) numOfPictureParameterSets;
 for (i=0; i< numOfPictureParameterSets; i++) {
   unsigned int(16) pictureParameterSetLength;
   bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
 }
 }
 */
static int RtmpPubSendH264Config(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp)
{
        int i = 0;
        char body[1024];

        // sanity check
        if (_pRtmp->m_pPps.m_nSize == 0 || _pRtmp->m_pSps.m_nSize <  4) {
                Debug("No pps or sps");
                return -1;
        }

        // header
        body[i++] = 0x17; // 1:keyframe  7:AVC
        body[i++] = 0x00; // AVC sequence header

        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00; // fill in 0;

        // AVCDecoderConfigurationRecord.
        body[i++] = 0x01; // configurationVersion
        body[i++] = _pRtmp->m_pSps.m_pData[1]; // AVCProfileIndication
        body[i++] = _pRtmp->m_pSps.m_pData[2]; // profile_compatibility
        body[i++] = _pRtmp->m_pSps.m_pData[3]; // AVCLevelIndication
        body[i++] = 0xff; // lengthSizeMinusOne

        // sps nums
        body[i++] = 0xE1; //&0x1f
        // sps data length
        body[i++] = _pRtmp->m_pSps.m_nSize >> 8;
        body[i++] = _pRtmp->m_pSps.m_nSize & 0xff;
        // sps data
        memcpy(&body[i], _pRtmp->m_pSps.m_pData, _pRtmp->m_pSps.m_nSize);
        i = i + _pRtmp->m_pSps.m_nSize;

        // pps nums
        body[i++] = 0x01; //&0x1f
        // pps data length
        body[i++] = _pRtmp->m_pPps.m_nSize>> 8;
        body[i++] = _pRtmp->m_pPps.m_nSize& 0xff;
        // sps data
        memcpy(&body[i], _pRtmp->m_pPps.m_pData, _pRtmp->m_pPps.m_nSize);
        i = i + _pRtmp->m_pPps.m_nSize;


        return SendVideos(_pRtmp, body, i, _nTimeStamp);
}


/*
 unsigned int(8)  configurationVersion;
 unsigned int(2)  general_profile_space;
 unsigned int(1)  general_tier_flag;
 unsigned int(5)  general_profile_idc;
 unsigned int(32) general_profile_compatibility_flags;
 unsigned int(48) general_constraint_indicator_flags;
 unsigned int(8)  general_level_idc;
 bit(4) reserved = ‘1111’b;
 unsigned int(12) min_spatial_segmentation_idc;
 bit(6) reserved = ‘111111’b;
 unsigned int(2)  parallelismType;
 bit(6) reserved = ‘111111’b;
 unsigned int(2)  chromaFormat;
 bit(5) reserved = ‘11111’b;
 unsigned int(3)  bitDepthLumaMinus8;
 bit(5) reserved = ‘11111’b;
 unsigned int(3)  bitDepthChromaMinus8;
 bit(16) avgFrameRate;
 bit(2)  constantFrameRate;
 bit(3)  numTemporalLayers;
 bit(1)  temporalIdNested;
 unsigned int(2) lengthSizeMinusOne;
 unsigned int(8) numOfArrays;
 for (j=0; j < numOfArrays; j++) {
   bit(1) array_completeness;
   unsigned int(1)  reserved = 0;
   unsigned int(6)  NAL_unit_type;
   unsigned int(16) numNalus;
   for (i=0; i< numNalus; i++) {
     unsigned int(16) nalUnitLength;
     bit(8*nalUnitLength) nalUnit;
   }
 }
*/

static int RtmpPubSendH265Config(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp)
{
        int i = 0;
        char body[1024];
        
        // sanity check
        if (_pRtmp->m_pPps.m_nSize == 0 || _pRtmp->m_pSps.m_nSize <  4) {
                Debug("No pps or sps");
                return -1;
        }
        
        // header
        body[i++] = 0x1C; // 1:keyframe  12:HEVC
        body[i++] = 0x00; // AVC sequence header
        
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00; // fill in 0;
        
        // HEVCDecoderConfigurationRecord.
        body[i++] = 0x01; // configurationVersion
        
        //bitcopy(&body[i], 0, _pRtmp->m_pVps.m_pData, 45, 96);
        memcpy(&body[i], _pRtmp->m_pVps.m_pData + 6, 96/8);
        //6 2byte naluheader, 2byte{vps_video_parameter_set_id(4bit) vps_reserved_three_2bits(2bit) vps_max_layers_minus1(6bit)
        //vps_max_sub_layers_minus1(3bit) vps_temporal_id_nesting_flag(1bit)},2byte vps_reserved_0xffff_16bits
        i+=(96/8);
        
        // FIXME: parse min_spatial_segmentation_idc.
        body[i++] = 0xf0;
        body[i++] = 0;
        
        // FIXME: derive parallelismType properly.
        body[i++] = 0xfc;
        //TODO parse SPS to get follow value
        body[i++] = 0xfc | 0x01;//| chromaFormatIdc; 1表示yuv420
        body[i++] = 0xf8 ;//| bitDepthLumaMinus8;
        body[i++] = 0xf8 ;//| bitDepthChromaMinus8;
        
        // FIXME: derive avgFrameRate
        body[i++] = 0;
        body[i++] = 0;
        
        body[i++] = 3; //constantFrameRate, numTemporalLayers, temporalIdNested are set to 0 and lengthSizeMinusOne set to 3
        
        body[i++] = 3; //vps sps pps

        body[i++] = 32;
        body[i++] = 0;
        body[i++] = 1; //只有一个nalu
        body[i++] = _pRtmp->m_pVps.m_nSize >> 8;
        body[i++] = _pRtmp->m_pVps.m_nSize & 0xff;
        memcpy(&body[i], _pRtmp->m_pVps.m_pData, _pRtmp->m_pVps.m_nSize);
        i = i + _pRtmp->m_pVps.m_nSize;
        
        body[i++] = 33;
        body[i++] = 0;
        body[i++] = 1;
        body[i++] = _pRtmp->m_pSps.m_nSize >> 8;
        body[i++] = _pRtmp->m_pSps.m_nSize & 0xff;
        memcpy(&body[i], _pRtmp->m_pSps.m_pData, _pRtmp->m_pSps.m_nSize);
        i = i + _pRtmp->m_pSps.m_nSize;
        
        // pps nums
        body[i++] = 34; //&0x1f
        body[i++] = 0;
        body[i++] = 1;
        body[i++] = _pRtmp->m_pPps.m_nSize>> 8;
        body[i++] = _pRtmp->m_pPps.m_nSize& 0xff;
        // sps data
        memcpy(&body[i], _pRtmp->m_pPps.m_pData, _pRtmp->m_pPps.m_nSize);
        i = i + _pRtmp->m_pPps.m_nSize;
        
        printf("decoder:----%d %d %d %d\n",_pRtmp->m_pVps.m_nSize, _pRtmp->m_pSps.m_nSize, _pRtmp->m_pPps.m_nSize, i);
        return SendVideos(_pRtmp, body, i, _nTimeStamp);
}

static unsigned int WriteNalDataToBuffer(char *_pBuffer, const char *_pData, unsigned int _nLength)
{
        int i = 0;

        // NALU size
        _pBuffer[i++] = _nLength >> 24;
        _pBuffer[i++] = _nLength >> 16;
        _pBuffer[i++] = _nLength >> 8;
        _pBuffer[i++] = _nLength & 0xff;

        // NALU data
        memcpy(&_pBuffer[i], _pData, _nLength);
        return _nLength + 4;
}

static unsigned int WriteNalUnitToBuffer(char *_pBuffer, RtmpPubNalUnit *_pNalu)
{
        return WriteNalDataToBuffer(_pBuffer, _pNalu->m_pData, _pNalu->m_nSize);
}

static int SendH264Packet(RtmpPubContext * _pRtmp, const char *_data, unsigned int _size, int _bIsKeyFrame, unsigned int _nTimeStamp, unsigned int _nCompositionTime)
{
        if (_data == NULL && _size < 11) {
                return -1;
        }

        char *body = (char *)malloc(_size + 9);

        int i = 0;
        
        int x = 0x07;
        if (_pRtmp->m_videoType == RTMP_PUB_VIDEOTYPE_HEVC) {
                x = 0x0c;
        }
        if (_bIsKeyFrame) {
                body[i++] = 0x10 | x; // 1:Iframe 7:AVC c:hevc
        } else {
                body[i++] = 0x20 | x; // 2:Pframe 7:AVC c:hevc
        }
        body[i++] = 0x01; // AVC NALU

        // composition time adjustment
        body[i++] = _nCompositionTime >> 16;
        body[i++] = _nCompositionTime >> 8;
        body[i++] = _nCompositionTime & 0xff;

        // NALU size
        body[i++] = _size >> 24;
        body[i++] = _size >> 16;
        body[i++] = _size >> 8;
        body[i++] = _size & 0xff;

        // NALU data
        memcpy(&body[i], _data, _size);
        
        int bRet = SendVideos(_pRtmp, body, i + _size, _nTimeStamp);
        free(body);
        return bRet;
}

static int RtmpPubSendH264Keyframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime, unsigned int _nCompositionTime)
{
        if (_pRtmp == NULL || _pData == NULL) {
                Debug("Fatal");
                return -1;
        }
                // at least we have ever received SPS
        if (_pRtmp->m_pSps.m_nSize  == 0) {
                Debug("No pps");
                return SendH264Packet(_pRtmp, _pData, _nSize, 1, _presentationTime, _nCompositionTime);
        }

        if (_nSize < 11) {
                return -1;
        }
        // this is a bit different, the body consists of [header][nalu_size][nalu][nalu_size][nalu][...] 
        int nNalUnitNum = (_pRtmp->m_pVps.m_nSize > 0 ? 1 : 0) + 2 + (_pRtmp->m_pSei.m_nSize > 0 ? 1 : 0) + 1; // VPS+SPS+PPS + [SEI] + IDR
        int nDataSize = _pRtmp->m_pVps.m_nSize + _pRtmp->m_pSps.m_nSize + _pRtmp->m_pPps.m_nSize + _pRtmp->m_pSei.m_nSize +  _nSize + 5 + nNalUnitNum * 4;
        char *body = (char *)malloc(nDataSize);
        int i = 0;
        int bRet;

        //
        // header (5bytes)， included in nDataSize
        //
        
        body[i] = 0x17; // 1:Iframe 7:AVC
        if (_pRtmp->m_videoType == RTMP_PUB_VIDEOTYPE_HEVC) {
                 body[i] = 0x1c; // 1:Iframe c:hevc
        }
        i++;
        body[i++] = 0x01; // AVC NALU

        // composition time adjustment
        body[i++] = _nCompositionTime >> 16;
        body[i++] = _nCompositionTime >> 8;
        body[i++] = _nCompositionTime & 0xff;

        //
        // NAL Units (at most 4)
        //

        unsigned int nBytes;
        if (_pRtmp->m_pVps.m_nSize != 0) {
                nBytes = WriteNalUnitToBuffer(&body[i], &_pRtmp->m_pVps);
                i += nBytes;
        }

        // SPS & PPS
        nBytes = WriteNalUnitToBuffer(&body[i], &_pRtmp->m_pSps);
        i += nBytes;

        if (_pRtmp->m_pPps.m_nSize != 0) {
                nBytes = WriteNalUnitToBuffer(&body[i], &_pRtmp->m_pPps);
                i += nBytes;
        }

        // SEI
        if (_pRtmp->m_pSei.m_nSize) {
                nBytes = WriteNalUnitToBuffer(&body[i], &_pRtmp->m_pSei);
                i += nBytes;
        }
        
        // IDR
        WriteNalDataToBuffer(&body[i], _pData, _nSize);

        bRet = SendVideos(_pRtmp, body, nDataSize, _presentationTime);

        free(body);
        return bRet;
}

int RtmpPubSendVideoInterframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime) 
{
        return SendH264Packet(_pRtmp, _pData, _nSize, 0, _presentationTime, 0);
}

void RtmpPubSetPps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize)
{
        if (_pRtmp == NULL || _pData == NULL || _nSize == 0) {
                return;
        }
        RtmpPubFillNalunit(&_pRtmp->m_pPps, _pData, _nSize);
}

void RtmpPubSetSps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize)
{
        if (_pRtmp == NULL || _pData == NULL || _nSize == 0) {
                return;
        };
        RtmpPubFillNalunit(&_pRtmp->m_pSps, _pData, _nSize);
}

void RtmpPubSetVps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize)
{
        if (_pRtmp == NULL || _pData == NULL || _nSize == 0) {
                return;
        };
        RtmpPubFillNalunit(&_pRtmp->m_pVps, _pData, _nSize);
}

void RtmpPubSetSei(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize)
{
        if (_pRtmp == NULL || _pData == NULL || _nSize == 0) {
                return;
        }
        RtmpPubFillNalunit(&_pRtmp->m_pSei, _pData, _nSize);
}

int GetAacHeader(const char *_pBuffer, AdtsHeader *_header)
{
        // headers begin with FFFxxxxx...
        if ((unsigned char)_pBuffer[0] == 0xff && (((unsigned char)_pBuffer[1] & 0xf0) == 0xf0)) {
                _header->nSyncWord = (_pBuffer[0] << 4) | (_pBuffer[1] >> 4);
                _header->nId = ((unsigned int)_pBuffer[1] & 0x08) >> 3;
                _header->nLayer = ((unsigned int)_pBuffer[1] & 0x06) >> 1;
                _header->nProtectionAbsent = (unsigned int)_pBuffer[1] & 0x01;
                _header->nProfile = ((unsigned int)_pBuffer[2] & 0xc0) >> 6;
                _header->nSfIndex = ((unsigned int)_pBuffer[2] & 0x3c) >> 2;
                _header->nPrivateBit = ((unsigned int)_pBuffer[2] & 0x02) >> 1;
                _header->nChannelConfiguration = ((((unsigned int)_pBuffer[2] & 0x01) << 2) | (((unsigned int)_pBuffer[3] & 0xc0) >> 6));
                _header->nOriginal = ((unsigned int)_pBuffer[3] & 0x20) >> 5;
                _header->nHome = ((unsigned int)_pBuffer[3] & 0x10) >> 4;
                _header->nCopyrightIdentificationBit = ((unsigned int)_pBuffer[3] & 0x08) >> 3;
                _header->nCopyrigthIdentificationStart = (unsigned int)_pBuffer[3] & 0x04 >> 2;
                _header->nAacFrameLength = (((((unsigned int)_pBuffer[3]) & 0x03) << 11) |
                                            (((unsigned int)_pBuffer[4] & 0xFF) << 3) |
                                            ((unsigned int)_pBuffer[5] & 0xE0) >> 5);
                _header->nAdtsBufferFullness = (((unsigned int)_pBuffer[5] & 0x1f) << 6 | ((unsigned int)_pBuffer[6] & 0xfc) >> 2);
                _header->nNoRawDataBlocksInFrame = ((unsigned int)_pBuffer[6] & 0x03);
                return 0;
        } else {
                return -1;
        }
}

static int SendAacPacket(RtmpPubContext * _pRtmp, const char *_pData, unsigned int _nSize, unsigned int _nTimeStamp,
                               char _chSoundRate, char _chSoundSize, char _chSoundType)
{
        // additional 2-byte audio spec config
        char *pBody = (char *)malloc(_nSize + 2);

        // decoder spec info bytes
        memset(pBody, 0, _nSize + 2);
        pBody[0] = FLV_AUDIO_TAG_HEADER(SOUND_FORMAT_AAC, _chSoundRate, _chSoundSize, _chSoundType);
        // this is not a sequence header
        pBody[1] = SOUND_AAC_TYPE_RAW;
        // send aac frame
        memcpy(&pBody[2], _pData, _nSize);
        int status = SendAudios(_pRtmp, pBody, _nSize + 2, _nTimeStamp);
        free(pBody);
        return status;
}

static int SendAacFrame(RtmpPubContext * _pRtmp, unsigned int nTimeStamp, const char * _pData, unsigned int _nLength)
{
        if (_nLength < _pRtmp->m_nAdtsSize) {
                return -1;
        }
        return SendAacPacket(_pRtmp, _pData + _pRtmp->m_nAdtsSize, _nLength - _pRtmp->m_nAdtsSize, nTimeStamp,
                        SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO);
}

static int SendAacRawFrame(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, const char * _pData, unsigned int _nLength)
{
        return SendAacPacket(_pRtmp, _pData, _nLength, _nTimeStamp, SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO);
}

static int RtmpPubSendAacFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        int nRet = 0;
        if (_pRtmp->m_nIsAudioConfigSent == 0) {
                nRet = RtmpPubSendAacConfig(_pRtmp, _pData, _nSize, _nTimeStamp);
                if (nRet < 0) {
                        return nRet;
                }
                _pRtmp->m_nIsAudioConfigSent = 1;
                Debug("Audio config sent _nSize = %d", _nSize);
        }
        return SendAacFrame(_pRtmp, _nTimeStamp, _pData, _nSize);
}

static int RtmpPubSendRawAac(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        int nRet = 0;
        if (_pRtmp->m_nIsAudioConfigSent == 0) {
                if (_pRtmp->m_aac.m_nSize == 0) {
                        return -1;
                }
                nRet = SendAacConfigRecord(_pRtmp, _pRtmp->m_aac.m_pData, _pRtmp->m_aac.m_nSize, _nTimeStamp);
                if (nRet < 0) {
                        return nRet;
                }
                _pRtmp->m_nIsAudioConfigSent = 1;
                Debug("Raw aac config sent");
        }
        return SendAacRawFrame(_pRtmp, _nTimeStamp, _pData, _nSize);
}

int RtmpPubSendAudioFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, int _nTimeStamp)
{
        int nRet = 0;
        switch (_pRtmp->m_nAudioInputType) {
        case RTMP_PUB_AUDIO_AAC:
                nRet = RtmpPubSendRawAac(_pRtmp, _pData, _nSize, _nTimeStamp);
                break;
        case RTMP_PUB_AUDIO_G711A:
                nRet = RtmpPubSendG711aFrame(_pRtmp, _pData, _nSize, _nTimeStamp);
                break;
        case RTMP_PUB_AUDIO_G711U:
                nRet = RtmpPubSendG711uFrame(_pRtmp, _pData, _nSize, _nTimeStamp);
                break;
        case RTMP_PUB_AUDIO_PCM:
                nRet = RtmpPubSendPcmFrame(_pRtmp, _pData, _nSize, _nTimeStamp);
                break;
        default:
                return -1;
        }
        return nRet;
}

static int SendAacConfig(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, char _chSoundRate, char _chSoundSize, char _chSoundType, const AdtsHeader *_pAdts)
{
        char body[4] = {0};

        // decoder spec info bytes
        body[0] = FLV_AUDIO_TAG_HEADER(SOUND_FORMAT_AAC, _chSoundRate, _chSoundSize, _chSoundType);
        // this is a sequence header
        body[1] = SOUND_AAC_TYPE_SEQ_HEADER;

        // spec config bytes
        unsigned int nProfile = 0;
        unsigned int nSfIndex = 0;
        unsigned int nChannel = 0;
        if (_pAdts != NULL) {
                nProfile = _pAdts->nProfile + 1;
                nSfIndex = _pAdts->nSfIndex;
                nChannel = _pAdts->nChannelConfiguration;
        } else {
                nProfile = ASC_OBJTYPE_AAC_LC;
                nSfIndex = ASC_SF_44100;
                nChannel = ASC_CHAN_FLR;
        }

        unsigned int nAudioSpecConfig = 0;
        nAudioSpecConfig |= ((nProfile << 11) & 0xf800);
        nAudioSpecConfig |= ((nSfIndex << 7) & 0x0780);
        nAudioSpecConfig |= ((nChannel << 3) & 0x78);
        nAudioSpecConfig |= 0 & 0x7;
        body[2] = (nAudioSpecConfig & 0xff00) >> 8;
        body[3] = nAudioSpecConfig & 0xff;

        return SendAudios(_pRtmp, body, sizeof(body), _nTimeStamp);
}

static int SendAacConfigRecord(RtmpPubContext * _pRtmp, const char * _pAacCfgRecord, unsigned int _nSize, unsigned int _nTimeStamp)
{
        if (_nSize < 2) {
               return -1;
        }
        char * body = (char *)malloc(2 + _nSize);
        // decoder spec info bytes
        body[0] = FLV_AUDIO_TAG_HEADER(SOUND_FORMAT_AAC, SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO);
        // this is a sequence header
        body[1] = SOUND_AAC_TYPE_SEQ_HEADER;

        memcpy(body + 2, _pAacCfgRecord, _nSize);

        int nRet = SendAudios(_pRtmp, body, _nSize + 2, _nTimeStamp);
        free(body);
        return nRet;
}

void RtmpPubSetAac(RtmpPubContext * _pRtmp, const char * _pAacCfgRecord, unsigned int _nSize)
{
        if (_pRtmp == NULL || _nSize == 0) {
                return;
        }
        RtmpPubFillNalunit(&_pRtmp->m_aac, _pAacCfgRecord, _nSize);
        return; 
}

static int RtmpPubSendAacConfig(RtmpPubContext * _pRtmp, const char * _pDataWithAdts, unsigned int _nSize, unsigned int _nTimeStamp)
{
        AdtsHeader adts;
        AdtsHeader *pAdts = NULL;
        // no ADTS when AAC data frame is raw
        _pRtmp->m_nAdtsSize = 0;

        if (_nSize <= 0) {
                return -1;
        }
        
        if (_pDataWithAdts != NULL) {
                if (GetAacHeader(_pDataWithAdts, &adts) == 0) {
                        pAdts = &adts;
                        // with 7 or 9 bytes of ADTS headers
                        _pRtmp->m_nAdtsSize = (pAdts->nProtectionAbsent == 1 ? 7 : 9);
                }
        }
        return SendAacConfig(_pRtmp, _nTimeStamp, SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO, pAdts);
}


static int SendAudioPacket(RtmpPubContext * _pRtmp, const char _chHeader, const char *_pPayload, unsigned int
_nPayloadSize, unsigned int _nTimestamp)
{
        unsigned int nSize = _nPayloadSize + 1;
        char *pData = (char *)malloc(nSize);
        pData[0] = _chHeader;
        memcpy(&pData[1], _pPayload, _nPayloadSize);
        int bStatus = SendAudios(_pRtmp, pData, nSize, _nTimestamp);
        free(pData);
        return bStatus;
}

static int SendAdpcmPacket(RtmpPubContext * _pRtmp, const char *_pData, unsigned int _nSize, unsigned int _nTimestamp)
{
        char chHeader = FLV_AUDIO_TAG_HEADER(SOUND_FORMAT_ADPCM, SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO);
        return SendAudioPacket(_pRtmp, chHeader, _pData, _nSize, _nTimestamp);
}

static int SendG711aPacket(RtmpPubContext * _pRtmp, const char *_pData, unsigned int _nSize, unsigned int _nTimestamp)
{
        char chHeader = FLV_AUDIO_TAG_HEADER(SOUND_FORMAT_G711A, SOUND_RATE_5_5K, SOUND_SIZE_16BIT, SOUND_TYPE_MONO);
        return SendAudioPacket(_pRtmp, chHeader, _pData, _nSize, _nTimestamp);
}

static int SendG711uPacket(RtmpPubContext * _pRtmp, const char *_pData, unsigned int _nSize, unsigned int _nTimestamp)
{
        char chHeader = FLV_AUDIO_TAG_HEADER(SOUND_FORMAT_G711U, SOUND_RATE_5_5K, SOUND_SIZE_16BIT, SOUND_TYPE_MONO);
        return SendAudioPacket(_pRtmp, chHeader, _pData, _nSize, _nTimestamp);
}

typedef int (G711DecoderProc_t)(AacEncoderContext * _pAacEncoder, void * _pOutBuf, size_t * _pOutSize, const void *_pInBuf, const size_t _nInSize);

static int RtmpPubSendDecodeFrame(RtmpPubContext * _pRtmp, RtmpPubAudioType _nInputAudioType, const char * _pData, size_t _nSize, unsigned _nTimeStamp, G711DecoderProc_t * _pProc)
{
        if (_pRtmp->m_nAudioInputType != _nInputAudioType) {
                Debug("fail");
                return -1;
        }
        if (_pRtmp->m_pAudioEncoderContext == NULL) {
                Debug("fail");
                return -1;
        }
        char * pAacBuffer = (char *)malloc(_nSize * 2 + 10240);
        size_t nAacSize = 0;
        
        int nRet = _pProc(_pRtmp->m_pAudioEncoderContext, pAacBuffer, &nAacSize, _pData, _nSize);
        if (nRet < 0) {
                Debug("fail");
                free(pAacBuffer);
                return nRet;
        }
        
        //Data not enough
        if (nAacSize == 0) {
                Debug("Data not enough");
                free(pAacBuffer);
                return 0;
        }
        nRet = RtmpPubSendAacFrame(_pRtmp, pAacBuffer, nAacSize, _nTimeStamp);
        free(pAacBuffer);
        return nRet; 

}

static int RtmpPubSendG711aFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        if (_pRtmp->m_nAudioInputType == _pRtmp->m_nAudioOutputType) {
                return SendG711aPacket(_pRtmp, _pData, _nSize, _nTimeStamp);
        }
        return RtmpPubSendDecodeFrame(_pRtmp, _pRtmp->m_nAudioInputType, _pData, _nSize, _nTimeStamp, AacEncoderEncodePcma);
}


static int RtmpPubSendG711uFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        if (_pRtmp->m_nAudioInputType == _pRtmp->m_nAudioOutputType) {
                return SendG711uPacket(_pRtmp, _pData, _nSize, _nTimeStamp);
        }
        return RtmpPubSendDecodeFrame(_pRtmp, _pRtmp->m_nAudioInputType, _pData, _nSize, _nTimeStamp, AacEncoderEncodePcmu);
}

static int RtmpPubSendPcmFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        if (_pRtmp->m_nAudioInputType == _pRtmp->m_nAudioOutputType) {
                return SendAdpcmPacket(_pRtmp, _pData, _nSize, _nTimeStamp);
        }
        return RtmpPubSendDecodeFrame(_pRtmp, _pRtmp->m_nAudioInputType, _pData, _nSize, _nTimeStamp, AacEncoderEncodePcm);
}

//TODO:Merge this timestamp functions
static void GetInfoRelative(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, unsigned int * _nResTimeStamp, unsigned char * _nResPacketType, unsigned char * _nChannel)
{
        if (_nTimeStamp < _pRtmp->m_nLastMediaTimeStamp) {
                Debug("TimeStamp reverse");
                //Time reverse resend an absolute packet
                _nTimeStamp = _pRtmp->m_nLastMediaTimeStamp;
                _pRtmp->m_bIsMediaPktSmall = 0;
        }
        long long int nDiff = _nTimeStamp - (long long int)_pRtmp->m_nMediaTimebase;
        if (nDiff < 0) {
                _pRtmp->m_nMediaTimebase = _nTimeStamp;
                nDiff = 0;
                _pRtmp->m_bIsMediaPktSmall = 0;
        }
        _pRtmp->m_nLastMediaTimeStamp = _nTimeStamp;
        *_nResTimeStamp = nDiff;
        *_nResPacketType = RTMP_PACKET_SIZE_MEDIUM;
        *_nChannel = 4;
        if (_pRtmp->m_bIsMediaPktSmall == 0) {
                *_nResPacketType = RTMP_PACKET_SIZE_LARGE;
                _pRtmp->m_bIsMediaPktSmall = 1;
        }
}

static void GetAudioInfoAbs(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, unsigned int * _nResTimeStamp, unsigned char * _nResPacketType, unsigned char * _nChannel)
{
        if (_nTimeStamp < _pRtmp->m_nLastAudioTimeStamp) {
                Debug("TimeStamp reverse");
                _nTimeStamp = _pRtmp->m_nLastAudioTimeStamp;
        }
        long long int nDiff = _nTimeStamp - (long long int)_pRtmp->m_nAudioTimebase;
        if (nDiff < 0) {
                _pRtmp->m_nAudioTimebase = _nTimeStamp;
                nDiff = 0;
        }
        
        *_nResTimeStamp = nDiff;
        *_nResPacketType = RTMP_PACKET_SIZE_LARGE;
        *_nChannel = 4;
}

static void GetVideoInfoAbs(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp, unsigned int * _nResTimeStamp, unsigned char * _nResPacketType, unsigned char * _nChannel)
{
        if (_nTimeStamp < _pRtmp->m_nLastVideoTimeStamp) {
                Debug("TimeStamp reverse");
                _nTimeStamp = _pRtmp->m_nLastVideoTimeStamp;
        }
        long long int nDiff = _nTimeStamp - (long long int)_pRtmp->m_nVideoTimebase;
        if (nDiff < 0) {
                _pRtmp->m_nVideoTimebase = _nTimeStamp;
                nDiff = 0;
        }
        
        *_nResTimeStamp = nDiff;
        *_nResPacketType = RTMP_PACKET_SIZE_LARGE;
        *_nChannel = 4;
}

void RtmpPubSetAudioTimebase(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp)
{
        _pRtmp->m_nAudioTimebase = _nTimeStamp;
        _pRtmp->m_nLastAudioTimeStamp = _nTimeStamp;
        _pRtmp->m_nIsAudioConfigSent = 0;
        _pRtmp->m_bIsMediaPktSmall = 0;
        _pRtmp->m_nLastMediaTimeStamp = 0;

        if (_pRtmp->m_nMediaTimebase < 0) {
                _pRtmp->m_nMediaTimebase = _nTimeStamp;
        }

}

void RtmpPubSetVideoTimebase(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp)
{
        _pRtmp->m_nVideoTimebase = _nTimeStamp;
        _pRtmp->m_nLastVideoTimeStamp = _nTimeStamp;
        _pRtmp->m_nIsVideoConfigSent = 0;
        _pRtmp->m_bIsMediaPktSmall = 0;
        _pRtmp->m_nLastMediaTimeStamp = 0;

        if (_pRtmp->m_nMediaTimebase < 0) {
                _pRtmp->m_nMediaTimebase = _nTimeStamp;
        }
}

static int RtmpPubTransferPacket(RtmpPubContext * _pRtmp, unsigned int _nPacketType, const char * _pRtmpData, unsigned int _nSize, int _nTimeStamp)
{
        switch (_nPacketType) {
        case RTMP_PACKET_TYPE_AUDIO:
                return SendAudios(_pRtmp, _pRtmpData, _nSize, _nTimeStamp);
        case RTMP_PACKET_TYPE_VIDEO:
                return SendVideos(_pRtmp, _pRtmpData, _nSize, _nTimeStamp); 
        default:
                Debug("Error");
                return -1;
        }
        return 0;
}


int RtmpPubSendVideoKeyframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime)
{
        int ret = 0;
        if (_pRtmp->m_nIsVideoConfigSent == 0) {
                if (_pRtmp->m_videoType == RTMP_PUB_VIDEOTYPE_AVC) {
                        ret = RtmpPubSendH264Config(_pRtmp, _presentationTime);
                } else {
                        ret = RtmpPubSendH265Config(_pRtmp, _presentationTime);
                }
                if (ret < 0) {
                        printf("RtmpPubSend video Config fail\n");
                        return ret;
                }
                _pRtmp->m_nIsVideoConfigSent = 1;
        }
        
        ret = RtmpPubSendH264Keyframe(_pRtmp, _pData, _nSize, _presentationTime, 0); 
        if (ret < 0) {
                printf("RtmpPubSendH264Keyframe fail\n");
                return ret;
        }
        return 0;
        
}

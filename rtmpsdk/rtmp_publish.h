#ifndef __RTMP_PUBLISH__
#define __RTMP_PUBLISH__

#ifdef __cplusplus    
extern "C" {         
#endif
#include "rtmp.h"


typedef enum {
        RTMP_PUB_AUDIO_NONE = 0,
        RTMP_PUB_AUDIO_AAC = 1,
        RTMP_PUB_AUDIO_G711A,
        RTMP_PUB_AUDIO_G711U,
        RTMP_PUB_AUDIO_PCM,
} RtmpPubAudioType;

typedef enum {
        RTMP_PUB_TIMESTAMP_ABSOLUTE = 1,
        RTMP_PUB_TIMESTAMP_RELATIVE
} RtmpPubTimeStampPolicy;
        
typedef enum {
        RTMP_PUB_VIDEOTYPE_AVC = 1,
        RTMP_PUB_VIDEOTYPE_HEVC
} RtmpPubVideoType;

typedef struct {
        int m_nType;
        char * m_pData;
        unsigned int m_nSize;
} RtmpPubNalUnit;

typedef struct {
        struct RTMP * m_pRtmp;        
        char * m_pPubUrl;
        unsigned int m_nTimeout;
        RtmpPubNalUnit m_pPps;
        RtmpPubNalUnit m_pSps;
        RtmpPubNalUnit m_pVps;
        RtmpPubNalUnit m_pSei;
        unsigned int m_nAudioTimebase;
        unsigned int m_nVideoTimebase;

        long long int m_nLastVideoTimeStamp;
        long long int m_nLastAudioTimeStamp;
        long long int m_nLastMediaTimeStamp;

        int m_bIsMediaPktSmall;         
        long long int m_nMediaTimebase;
        unsigned int m_nAdtsSize;
        void * m_pAudioEncoderContext;
        int m_nIsAudioConfigSent;
        int m_nIsVideoConfigSent;
        RtmpPubAudioType m_nAudioInputType;
        RtmpPubAudioType m_nAudioOutputType;
        RtmpPubTimeStampPolicy m_nTimePolicy;

        RtmpPubNalUnit m_aac;
        RtmpPubVideoType m_videoType;
} RtmpPubContext;

RtmpPubContext * RtmpPubNew(const char * _url, unsigned int _nTimeout, RtmpPubAudioType _nInputAudioType, RtmpPubAudioType m_nOutputAudioType, 
                                                                RtmpPubTimeStampPolicy _nTimePolicy);
int RtmpPubInit(RtmpPubContext * _pRtmp);
void RtmpPubSetVideoType(RtmpPubContext * _pRtmp, RtmpPubVideoType _type);
void RtmpPubDel(RtmpPubContext * _pRtmp);

int RtmpPubConnect(RtmpPubContext * _pRtmp);

void RtmpPubSetAudioTimebase(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp);
void RtmpPubSetVideoTimebase(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp);


int RtmpPubSendVideoKeyframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime);

int RtmpPubSendVideoInterframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime);

void RtmpPubSetPps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);
void RtmpPubSetSps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);
void RtmpPubSetSei(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);
void RtmpPubSetVps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);


int RtmpPubSendAudioFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, int _nTimeStamp);


//If the input and output format are botch aac, call this function before the first audio packet
void RtmpPubSetAac(RtmpPubContext * _pRtmp, const char * _pAacCfgRecord, unsigned int _nSize);





#ifdef __cplusplus
}
#endif
#endif

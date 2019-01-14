#include "qnRtmp.h"
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

static Nalu* ParseNalu(IN const char *pStart, IN int nlen, IN Nalu *pNalu, int *_pSepLen);
AppContext gAppContext;

static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
        const uint8_t *a = p + 4 - ((intptr_t)p & 3);
        
        for (end -= 3; p < a && p < end; p++) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1)
                        return p;
        }
        
        for (end -= 3; p < end; p += 4) {
                uint32_t x = *(const uint32_t*)p;
                //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
                //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
                if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
                        if (p[1] == 0) {
                                if (p[0] == 0 && p[2] == 1)
                                        return p;
                                if (p[2] == 0 && p[3] == 1)
                                        return p+1;
                        }
                        if (p[3] == 0) {
                                if (p[2] == 0 && p[4] == 1)
                                        return p+2;
                                if (p[4] == 0 && p[5] == 1)
                                        return p+3;
                        }
                }
        }
        
        for (end += 3; p < end; p++) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1)
                        return p;
        }
        
        return end + 3;
}

static const uint8_t *ff_avc_find_startcode(const uint8_t *p, const uint8_t *end){
        const uint8_t *out= ff_avc_find_startcode_internal(p, end);
        if(p<out && out<end && !out[-1]) out--;
        return out;
}

static Nalu* ParseNalu(IN const    char *_pStart, IN int _nLen, IN Nalu *pNalu, OUT int *_pSepLen)
{
        const uint8_t *endptr = (const uint8_t *)(_pStart + _nLen);
        const uint8_t * pStart = (uint8_t *)ff_avc_find_startcode((const uint8_t *)_pStart, endptr);
        if (pStart == NULL)
                return NULL;
        uint8_t * pEnd = (uint8_t *)ff_avc_find_startcode(pStart+4, endptr);
        if (pEnd == NULL)
                return NULL;

        char sep[3] = {0x00, 0x00, 0x01};
        if ( memcmp(pStart, sep, 3) == 0) {
                *_pSepLen = 3;
                pNalu->data = pStart + 3;
                pNalu->size = (int)(pEnd - pStart) - 3;
        } else {
                *_pSepLen = 4;
                pNalu->data = pStart + 4;
                pNalu->size = (int)(pEnd - pStart) - 4;
        }

        return pNalu;
}

int VideoCallBack(IN int _nStreamNO, IN char *_pFrame, IN int _nLen, 
        IN int _nIskey, IN double _dTimestamp, IN unsigned long _ulFrameIndex,        
        IN unsigned long _ulKeyFrameIndex, void *_pContext)
{
        static unsigned int nLastTimeStamp = 0;
        int *pStreamNo = (int*)_pContext;
        AppContext *appCtx = (AppContext *)_pContext;
        int streamno = *pStreamNo;

        nLastTimeStamp = (unsigned int)_dTimestamp;
        if (gAppContext.RtmpH264Send == NULL) {
                if (_nIskey)
                    QnDemoPrint(DEMO_WARNING, "%s[%d] g_AContext.RtmpH264Send is NULL.\n", __func__, __LINE__);
                return QN_FAIL;
        }
        gAppContext.RtmpH264Send(_pFrame, _nLen, _dTimestamp, _nIskey);
        return QN_SUCCESS;
}

int AudioCallBack(IN char *_pFrame, IN int _nLen, IN double _dTimestamp, IN unsigned long _ulFrameIndex, void *_pContext)
{
        if (!gAppContext.RtmpAudioSend) {
                return QN_SUCCESS;
        }
        
        gAppContext.RtmpAudioSend(_pFrame, _nLen, _dTimestamp, RTMP_PUB_AUDIO_AAC);

        return 0;
}

static void setVideoParameters(RtmpPubContext *pRtmpc, Nalu *pNalu) {
        int type = 0;
        if (gAppContext.videoType == RTMP_PUB_VIDEOTYPE_HEVC) {

                type = pNalu->data[0] & 0x7E;
                type = (type >> 1);
                switch (type) {
                        case 32: //vps
                                printf("set h265 vps\n");
                                RtmpPubSetVps(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        case 33: //sps
                                printf("set h265 sps\n");
                                RtmpPubSetSps(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        case 34: //pps
                                printf("set h265 pps\n");
                                RtmpPubSetPps(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        case 39: //SEI_PREFIX
                                printf("set h265 sei prefix\n");
                                RtmpPubSetSei(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        default:
                                break;
                }
        } else {
                type = pNalu->data[0] & 0x1F;
                switch (type) {
                        case 6: //sei
                                printf("set h264 sei\n");
                                RtmpPubSetSei(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        case 7: //sps
                                printf("set h264 sps\n");
                                RtmpPubSetSps(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        case 8: //pps
                                printf("set h264 pps\n");
                                RtmpPubSetPps(pRtmpc, pNalu->data, pNalu->size);
                                break;
                        default:
                                break;
                }
        }
}
enum HEVCNALUnitType {
        HEVC_NAL_TRAIL_N    = 0,
        HEVC_NAL_TRAIL_R    = 1,
        HEVC_NAL_TSA_N      = 2,
        HEVC_NAL_TSA_R      = 3,
        HEVC_NAL_STSA_N     = 4,
        HEVC_NAL_STSA_R     = 5,
        HEVC_NAL_RADL_N     = 6,
        HEVC_NAL_RADL_R     = 7,
        HEVC_NAL_RASL_N     = 8,
        HEVC_NAL_RASL_R     = 9,
        HEVC_NAL_VCL_N10    = 10,
        HEVC_NAL_VCL_R11    = 11,
        HEVC_NAL_VCL_N12    = 12,
        HEVC_NAL_VCL_R13    = 13,
        HEVC_NAL_VCL_N14    = 14,
        HEVC_NAL_VCL_R15    = 15,
        HEVC_NAL_BLA_W_LP   = 16,
        HEVC_NAL_BLA_W_RADL = 17,
        HEVC_NAL_BLA_N_LP   = 18,
        HEVC_NAL_IDR_W_RADL = 19,
        HEVC_NAL_IDR_N_LP   = 20,
        HEVC_NAL_CRA_NUT    = 21,
        HEVC_NAL_IRAP_VCL22 = 22,
        HEVC_NAL_IRAP_VCL23 = 23,
        HEVC_NAL_RSV_VCL24  = 24,
        HEVC_NAL_RSV_VCL25  = 25,
        HEVC_NAL_RSV_VCL26  = 26,
        HEVC_NAL_RSV_VCL27  = 27,
        HEVC_NAL_RSV_VCL28  = 28,
        HEVC_NAL_RSV_VCL29  = 29,
        HEVC_NAL_RSV_VCL30  = 30,
        HEVC_NAL_RSV_VCL31  = 31,
        HEVC_NAL_VPS        = 32,
        HEVC_NAL_SPS        = 33,
        HEVC_NAL_PPS        = 34,
        HEVC_NAL_AUD        = 35,
        HEVC_NAL_EOS_NUT    = 36,
        HEVC_NAL_EOB_NUT    = 37,
        HEVC_NAL_FD_NUT     = 38,
        HEVC_NAL_SEI_PREFIX = 39,
        HEVC_NAL_SEI_SUFFIX = 40,
};

static int isVideoFrameData(Nalu *pNalu) {
        int type = 0;
        if (gAppContext.videoType == RTMP_PUB_VIDEOTYPE_HEVC) {
                
                type = pNalu->data[0] & 0x7E;
                type = (type >> 1);
                switch (type) {
                        case HEVC_NAL_IDR_W_RADL:
                        case HEVC_NAL_CRA_NUT:
                        case HEVC_NAL_TRAIL_N:
                        case HEVC_NAL_TRAIL_R:
                        case HEVC_NAL_RASL_N:
                        case HEVC_NAL_RASL_R:
                                return 1;
                                
                }
        } else {
                type = pNalu->data[0] & 0x1F;
                switch (type) {
                        case 1:
                        case 5:
                                return 1;
                }
        }
        return 0;
}

static char h264Aud3[3]={0, 0, 1};
static char h264Aud4[4]={0, 0, 0, 1};
static int RtmpH264Send(IN char *_pData, IN int _nLen, IN double _dTimeStamp, IN int _nIskey)
{
        RtmpPubContext *pRtmpc = gAppContext.pRtmpc;
        int s32Ret = QN_FAIL;

        if (_pData == NULL || pRtmpc == NULL) {
                if (pRtmpc == NULL) {
                        QnDemoPrint(DEMO_NOTICE, "%s[%d] Rtmp context is null.\n", __func__, __LINE__);
                } else {
                        QnDemoPrint(DEMO_NOTICE, "%s[%d] _pData is NULL.\n", __func__, __LINE__);
                        s32Ret = QN_SUCCESS;
                }
                goto RTMPH264SEND_EXIT;
        }
        
        long long int presentationTime = (long long int)_dTimeStamp;
        Nalu *pNalu = NULL;
        Nalu nalu;
        int nIdx = 0;
        
        pthread_mutex_lock(&gAppContext.pushLock);
        if (gAppContext.status == RTMP_START) {
                
                if (_nIskey ) {
                        if (gAppContext.videoState == QN_FALSE) {
                                fprintf(stderr, "first set video config:%d\n", _nLen);
                        }
                        
                        int nSepLen;
                        while(1) {
                                pNalu = ParseNalu(_pData + nIdx, _nLen - nIdx, &nalu, &nSepLen );
                                if (!pNalu) {
                                        goto RTMPH264SEND_UNLOCK;
                                }
                                nIdx += pNalu->size + nSepLen;
                                
                                if (isVideoFrameData(pNalu)) {
                                        fprintf(stderr, "key:%d %d\n", pNalu->size, nSepLen);
                                        _pData = pNalu->data;
                                        _nLen = pNalu->size;
                                        break;
                                } else {
                                        if (gAppContext.videoState == QN_FALSE) {
                                                fprintf(stderr, "meta:%d %02x \n", pNalu->size + nSepLen, (uint8_t)pNalu->data[nSepLen]);
                                                setVideoParameters(pRtmpc, pNalu);
                                        }
                                }
                                
                        }
                        if (gAppContext.videoState == QN_FALSE) {
                                gAppContext.videoState = QN_TRUE;
                                gAppContext.isOk = QN_TRUE;
                        }
                }

                if (false == RtmpIsConnected(gAppContext.pRtmpc)) {
                        QnDemoPrint(DEMO_ERR, "connect error ,reInit Rtmp.\n");
                        goto REINIT_RTMP;
                }

                if (_nIskey) {
                        if (RtmpPubSendVideoKeyframe(pRtmpc, pNalu->data, pNalu->size, presentationTime) != QN_SUCCESS) {
                                QnDemoPrint(DEMO_ERR, "Send video key frame fail, reInit Rtmp.\n");
                                goto REINIT_RTMP;
                        }
                }
                else {
                        if (memcmp(h264Aud3, _pData, 3) == 0) {
                                _pData += 3;
                                _nLen -= 3;
                                
                        } else {
                                _pData += 4;
                                _nLen -= 4;
                        }
                        if (RtmpPubSendVideoInterframe(pRtmpc, _pData, _nLen, presentationTime) != QN_SUCCESS) {
                                QnDemoPrint(DEMO_ERR, "Send video inter frame fail, reInit Rtmp.\n");
                                goto REINIT_RTMP;
                        }
                }

                s32Ret = QN_SUCCESS;
                goto RTMPH264SEND_UNLOCK;
                
REINIT_RTMP:
                RtmpReconnect();

        }  else {
                s32Ret = QN_SUCCESS;
        }
                
RTMPH264SEND_UNLOCK:
        pthread_mutex_unlock(&gAppContext.pushLock);

RTMPH264SEND_EXIT:
        return s32Ret;
}

static int RtmpAudioSend(IN char *_pData, IN int _nLen, IN double _nTimeStamp, IN unsigned int _uAudioType)
{
        long long int presentationTime = (long long int)_nTimeStamp;
        RtmpPubContext *pRtmpc = gAppContext.pRtmpc;
        static unsigned char audioSpecCfg[] = {0x14, 0x08};// 0x10}; 
        int nRet = QN_FAIL;

        pthread_mutex_lock(&gAppContext.pushLock);
        if (gAppContext.status == RTMP_START) {
                if (gAppContext.isOk == QN_FALSE) {
                        QnDemoPrint(DEMO_INFO, "%s[%d] wait for video metadata send.\n", __func__, __LINE__);
                        usleep(5000);
                        goto RTMPAUDIO_UNLOCK;
                }

                if (gAppContext.audioState == QN_FALSE) {
                        RtmpPubSetAudioTimebase(pRtmpc, presentationTime);
                        if (_uAudioType == AUDIO_TYPE_AAC) {
                                RtmpPubSetAac(pRtmpc, audioSpecCfg, sizeof(audioSpecCfg));
                        }
                        gAppContext.audioState = QN_TRUE;
                }
                
                nRet = RtmpPubSendAudioFrame(pRtmpc, _pData, _nLen, presentationTime);
                if ( nRet == 0 ) {
                        nRet = QN_SUCCESS;
                }
        }
        
RTMPAUDIO_UNLOCK:
        pthread_mutex_unlock(&gAppContext.pushLock);
        
        return nRet;
}

int RtmpInit(const char * pRtmpUrl, char * pType)
{
        if (gAppContext.pRtmpc) {
                QnDemoPrint(DEMO_INFO, "%s[%d] RtmpPubDel release Rtmp context.\n", __func__, __LINE__);
                RtmpPubDel(gAppContext.pRtmpc);
        }
        char RtmpUrl[RTMPURL_LEN] = {0};
        
        if (strcmp(pType, "h265") == 0) {
                gAppContext.videoType = RTMP_PUB_VIDEOTYPE_HEVC;
        } else {
               gAppContext.videoType = RTMP_PUB_VIDEOTYPE_AVC;
        }

        strcpy(RtmpUrl, pRtmpUrl);
        strcpy(gAppContext.rtmpUrl, pRtmpUrl);
        QnDemoPrint(DEMO_ERR, "%s[%d] Rtmp url :%s\n", __func__, __LINE__, RtmpUrl);

        pthread_mutex_init(&gAppContext.pushLock, NULL);        

        return RtmpReconnect();
}

void RtmpRelease()
{
        RtmpPubDel(gAppContext.pRtmpc);
        gAppContext.pRtmpc = NULL;
        gAppContext.RtmpH264Send = NULL;
        gAppContext.RtmpAudioSend = NULL;
        pthread_mutex_destroy(&gAppContext.pushLock);        
}

int RtmpIsConnected(RtmpPubContext* rpc)
{
        return rpc->m_pRtmp->m_sb.sb_socket != -1;
}

int RtmpReconnect()
{
        char RtmpUrl[RTMPURL_LEN] = {0};
        strcpy(RtmpUrl, gAppContext.rtmpUrl);;
        QnDemoPrint(DEMO_ERR, "%s[%d] Rtmp url :%s\n", __func__, __LINE__, RtmpUrl);

        if (gAppContext.pRtmpc)
                RtmpPubDel(gAppContext.pRtmpc);

        do {
                 gAppContext.pRtmpc = RtmpPubNew(RtmpUrl, 10, RTMP_PUB_AUDIO_AAC, RTMP_PUB_AUDIO_AAC, RTMP_PUB_TIMESTAMP_ABSOLUTE);
                 if (!gAppContext.pRtmpc) {
                         QnDemoPrint(DEMO_ERR, "%s[%d] Get Rtmp context failed.\n", __func__, __LINE__);
                         return QN_FAIL;
                 }
                 if (RtmpPubInit(gAppContext.pRtmpc) != QN_SUCCESS) {
                         RtmpPubDel(gAppContext.pRtmpc);
                         gAppContext.pRtmpc = NULL;
                         continue;
                 }
                 RtmpPubSetVideoType(gAppContext.pRtmpc, gAppContext.videoType);
                 if (RtmpPubConnect(gAppContext.pRtmpc) != QN_SUCCESS) {
                         RtmpPubDel(gAppContext.pRtmpc);
                         gAppContext.pRtmpc = NULL;
                 }
                 else {
                         break;
                 }
        } while(1);
        gAppContext.RtmpH264Send = RtmpH264Send;
        gAppContext.RtmpAudioSend = RtmpAudioSend;
        gAppContext.videoState = QN_FALSE;
        gAppContext.audioState = QN_FALSE;
        gAppContext.isOk = QN_FALSE;
        gAppContext.status = RTMP_START;
        QnDemoPrint(DEMO_WARNING, "%s[%d] Reconnect succeed.\n", __func__, __LINE__);
        return QN_SUCCESS;

}

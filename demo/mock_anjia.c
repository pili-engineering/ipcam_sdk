#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "adts.h"
#include "devsdk.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define VERSION "v1.0.0"

typedef int (*DataCallback)(void *opaque, void *pData, int nDataLen, int nFlag, int64_t timestamp, int nIsKeyFrame);
#define THIS_IS_AUDIO 1
#define THIS_IS_VIDEO 2

// start aac
static int aacfreq[13] = {96000, 88200,64000,48000,44100,32000,24000, 22050 , 16000 ,12000,11025,8000,7350};
typedef struct ADTS{
        ADTSFixheader fix;
        ADTSVariableHeader var;
}ADTS;
//end aac

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
enum HevcType {
        HEVC_META = 0,
        HEVC_I = 1,
        HEVC_B =2
};

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

static inline int64_t getCurrentMilliSecond(){
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec*1000 + tv.tv_usec/1000);
}

static int getFileAndLength(char *_pFname, FILE **_pFile, int *_pLen)
{
        FILE * f = fopen(_pFname, "r");
        if ( f == NULL ) {
                return -1;
        }
        *_pFile = f;
        fseek(f, 0, SEEK_END);
        long nLen = ftell(f);
        fseek(f, 0, SEEK_SET);
        *_pLen = (int)nLen;
        return 0;
}

static int readFileToBuf(char * _pFilename, char ** _pBuf, int *_pLen)
{
        int ret;
        FILE * pFile;
        int nLen = 0;
        ret = getFileAndLength(_pFilename, &pFile, &nLen);
        if (ret != 0) {
                fprintf(stderr, "open file %s fail\n", _pFilename);
                return -1;
        }
        char *pData = malloc(nLen);
        assert(pData != NULL);
        ret = fread(pData, 1, nLen, pFile);
        if (ret <= 0) {
                fprintf(stderr, "open file %s fail\n", _pFilename);
                fclose(pFile);
                free(pData);
                return -2;
        }
        fclose(pFile);
        *_pBuf = pData;
        *_pLen = nLen;
        return 0;
}

static int is_h265_picture(int t)
{
        switch (t) {
                case HEVC_NAL_VPS:
                case HEVC_NAL_SPS:
                case HEVC_NAL_PPS:
                case HEVC_NAL_SEI_PREFIX:
                        return HEVC_META;
                case HEVC_NAL_IDR_W_RADL:
                case HEVC_NAL_CRA_NUT:
                        return HEVC_I;
                case HEVC_NAL_TRAIL_N:
                case HEVC_NAL_TRAIL_R:
                case HEVC_NAL_RASL_N:
                case HEVC_NAL_RASL_R:
                        return HEVC_B;
                default:
                        return -1;
        }
}

typedef struct {
        VIDEO_CALLBACK videoCb;
        AUDIO_CALLBACK audioCb;
        pthread_t tid;
        int nStreamNo;
        char file[256];
        unsigned char isAac;
        unsigned char isStop;
        unsigned char isH265;
        unsigned char IsTestAACWithoutAdts;
        unsigned char isAudio;
        int64_t nRolloverTestBase;
        void *pContext
}Stream;

typedef struct  {
        int nId;
        Stream audioStreams[2];
        Stream videoStreams[2];
}Camera;

Camera cameras[10];

static int dataCallback(void *opaque, void *pData, int nDataLen, int nFlag, int64_t timestamp, int nIsKeyFrame)
{
        Stream *pStream = (Stream*)opaque;
        int ret = 0;
        if (nFlag == THIS_IS_AUDIO){
                //fprintf(stderr, "push audio ts:%lld\n", timestamp);
                //(char *frame, int len, double timestatmp, unsigned long frame_index, void *pcontext);
                ret = pStream->audioCb( pData, nDataLen, (double)timestamp, 0, pStream->pContext);
        } else {
                //printf("------->push video key:%d ts:%lld size:%d\n",nIsKeyFrame, timestamp, nDataLen);
                //(int streamno, char *frame, int len, int iskey, double timestatmp, unsigned long frame_index, unsigned long keyframe_index, void *pcontext);
                ret = pStream->videoCb(pStream->nStreamNo, pData, nDataLen, nIsKeyFrame, (double)timestamp, 0, 0, pStream->pContext);
        }
        return ret;
}

char h264Aud3[3]={0, 0, 1};
char h264Aud4[3]={0, 0, 0, 1};
int start_video_file_test(void *opaque)
{
        sleep(3);
        printf("----------_>start_video_file_test\n");
        Stream *pStream = (Stream*)opaque;
        int ret;
        
        char * pVideoData = NULL;
        int nVideoDataLen = 0;
        ret = readFileToBuf(pStream->file, &pVideoData, &nVideoDataLen);
        if (ret != 0) {
                free(pVideoData);
                printf( "map data to buffer fail:%s", pStream->file);
                return -2;
        }

        int64_t nSysTimeBase = getCurrentMilliSecond();
        int64_t nNextVideoTime = nSysTimeBase;
        int64_t nNow = nSysTimeBase;
        
        int bVideoOk = 1;
        int videoOffset = 0;
        int cbRet = 0;
        int nIDR = 0;
        int nNonIDR = 0;
        
         while (!pStream->isStop && bVideoOk) {
                 int nLen;
                 int shouldReset = 1;
                 int type = -1;
                 
                 if (videoOffset+4 < nVideoDataLen) {
                         memcpy(&nLen, pVideoData+videoOffset, 4);
                         if (videoOffset + 4 + nLen < nVideoDataLen) {
                                 shouldReset = 0;
                                 if (!pStream->isH265) {
                                         if (memcmp(h264Aud3, pVideoData + videoOffset + 4, 3) == 0) {
                                                 type = pVideoData[videoOffset + 7] & 0x1F;
                                         } else {
                                                 type = pVideoData[videoOffset + 8] & 0x1F;
                                         }
                                         if (type == 1) {
                                                 nNonIDR++;
                                         } else {
                                                 nIDR++;
                                         }
                                         
                                         cbRet = dataCallback(opaque, pVideoData + videoOffset + 4, nLen, THIS_IS_VIDEO, pStream->nRolloverTestBase+nNextVideoTime-nSysTimeBase, !(type == 1));
                                         if (cbRet != 0) {
                                                 bVideoOk = 0;
                                         }
                                         videoOffset = videoOffset + 4 + nLen;
                                 }else {
                                         if (memcmp(h264Aud3, pVideoData + videoOffset + 4, 3) == 0) {
                                                 type = pVideoData[videoOffset + 7] & 0x7F;
                                         } else {
                                                 type = pVideoData[videoOffset + 8] & 0x7F;
                                         }
                                         type = (type >> 1);
                                         
                                         if (type == 32){
                                                 nIDR++;
                                         } else {
                                                 nNonIDR++;
                                         }
                                         
                                         cbRet = dataCallback(opaque, pVideoData + videoOffset + 4, nLen, THIS_IS_VIDEO,pStream->nRolloverTestBase+nNextVideoTime-nSysTimeBase, type == 32);//hevctype == HEVC_I);
                                         if (cbRet != 0) {
                                                 bVideoOk = 0;
                                         }
                                         videoOffset = videoOffset + 4 + nLen;
                                 }
                         }
                 }
                 nNextVideoTime += 40;
                 if (shouldReset){
                         videoOffset = 0;
                 }
                 
                 int64_t nSleepTime = 0;
                 nSleepTime = (nNextVideoTime - nNow - 1) * 1000;
                 if (nSleepTime > 0) {
                         //printf("sleeptime:%lld\n", nSleepTime);
                         if (nSleepTime > 40 * 1000) {
                                 printf("abnormal time diff:%lld", nSleepTime);
                         }
                         usleep(nSleepTime);
                 }
                 nNow = getCurrentMilliSecond();
        }
        
        if (pVideoData) {
                free(pVideoData);
                printf("IDR:%d nonIDR:%d\n", nIDR, nNonIDR);
        }
        return 0;
}


int start_audio_file_test(void *opaque)
{
        sleep(3);
        printf("----------_>start_audio_file_test\n");
        Stream *pStream = (Stream*)opaque;
        int ret;
        
        char * pAudioData = NULL;
        int nAudioDataLen = 0;
        ret = readFileToBuf(pStream->file, &pAudioData, &nAudioDataLen);
        if (ret != 0) {
                printf("map data to buffer fail:%s", pStream->file);
                return -1;
        }
        
        int bAudioOk = 1;
        int64_t nSysTimeBase = getCurrentMilliSecond();
        int64_t nNextAudioTime = nSysTimeBase;
        int64_t nNow = nSysTimeBase;
        
        int audioOffset = 0;
        int isAAC = 1;
        int64_t aacFrameCount = 0;
        if (!pStream->isAac)
                isAAC = 0;
        int cbRet = 0;
        
        int duration = 0;
        
        while (!pStream->isStop && bAudioOk) {
                if (isAAC) {
                        ADTS adts;
                        if(audioOffset+7 <= nAudioDataLen) {
                                ParseAdtsfixedHeader((unsigned char *)(pAudioData + audioOffset), &adts.fix);
                                int hlen = adts.fix.protection_absent == 1 ? 7 : 9;
                                ParseAdtsVariableHeader((unsigned char *)(pAudioData + audioOffset), &adts.var);
                                if (audioOffset+hlen+adts.var.aac_frame_length <= nAudioDataLen) {
                                        
                                        if (pStream->IsTestAACWithoutAdts)
                                                cbRet = dataCallback(opaque, pAudioData + audioOffset + hlen, adts.var.aac_frame_length - hlen,
                                                                     THIS_IS_AUDIO, nNextAudioTime-nSysTimeBase+pStream->nRolloverTestBase, 0);
                                        else
                                                cbRet = dataCallback(opaque, pAudioData + audioOffset, adts.var.aac_frame_length,
                                                                     THIS_IS_AUDIO, nNextAudioTime-nSysTimeBase+pStream->nRolloverTestBase, 0);
                                        if (cbRet != 0) {
                                                bAudioOk = 0;
                                                continue;
                                        }
                                        audioOffset += adts.var.aac_frame_length;
                                        aacFrameCount++;
                                        int64_t d = ((1024*1000.0)/aacfreq[adts.fix.sampling_frequency_index]) * aacFrameCount;
                                        nNextAudioTime = nSysTimeBase + d;
                                } else {
                                        aacFrameCount++;
                                        int64_t d = ((1024*1000.0)/aacfreq[adts.fix.sampling_frequency_index]) * aacFrameCount;
                                        nNextAudioTime = nSysTimeBase + d;
                                        audioOffset = 0;
                                }
                                if (duration == 0) {
                                        duration = ((1024*1000.0)/aacfreq[adts.fix.sampling_frequency_index]);
                                }
                        } else {
                                aacFrameCount++;
                                int64_t d = ((1024*1000.0)/aacfreq[adts.fix.sampling_frequency_index]) * aacFrameCount;
                                nNextAudioTime = nSysTimeBase + d;
                                audioOffset = 0;
                        }
                } else {
                        duration = 20;
                        if(audioOffset+160 <= nAudioDataLen) {
                                cbRet = dataCallback(opaque, pAudioData + audioOffset, 160, THIS_IS_AUDIO, nNextAudioTime-nSysTimeBase+pStream->nRolloverTestBase, 0);
                                if (cbRet != 0) {
                                        bAudioOk = 0;
                                        continue;
                                }
                                audioOffset += 160;
                                nNextAudioTime += 20;
                        } else {
                                nNextAudioTime += 20;
                                audioOffset = 0;
                        }
                }
               
                int64_t nSleepTime = (nNextAudioTime - nNow - 1) * 1000;
                if (nSleepTime > 0) {
                        //printf("sleeptime:%lld\n", nSleepTime);
                        if (nSleepTime > duration * 1000) {
                                printf("abnormal time diff:%lld", nSleepTime);
                        }
                        usleep(nSleepTime);
                }
                nNow = getCurrentMilliSecond();
        }
        
        if (pAudioData) {
                free(pAudioData);
                printf("quie audio test");
        }
        return 0;
}


void dev_sdk_set_audio_filepath(int camera, int stream, char *pFile, int nFileLen)
{
        int nLen = nFileLen;
        if (nLen > 255) {
                nLen = 255;
        }
        cameras[camera].audioStreams[stream].file[nLen] = 0;
        memcpy(cameras[camera].audioStreams[stream].file, pFile, nLen);
}

void dev_sdk_set_video_filepath(int camera, int stream, char *pFile, int nFileLen)
{
        int nLen = nFileLen;
        if (nLen > 255) {
                nLen = 255;
        }
        cameras[camera].videoStreams[stream].file[nLen] = 0;
        memcpy(cameras[camera].videoStreams[stream].file, pFile, nLen);
}

void dev_sdk_set_audio_format(int camera, int stream, int isAac)
{
        cameras[camera].audioStreams[stream].isAac = isAac;
}

void dev_sdk_set_video_format(int camera, int stream, int isH265)
{
        cameras[camera].audioStreams[stream].isH265 = isH265;
}

int dev_sdk_init(DevSdkServerType type)
{
        memset(&cameras, 0, sizeof(cameras));
        int i = 0;
        for (i = 0; i < sizeof(cameras) / sizeof(Camera); i++) {
                cameras[i].nId = i;
                cameras[i].audioStreams[0].nStreamNo = 0;
                cameras[i].audioStreams[1].nStreamNo = 1;
                cameras[i].videoStreams[0].nStreamNo = 0;
                cameras[i].videoStreams[1].nStreamNo = 1;
                cameras[i].audioStreams[0].isAac = 1;
                cameras[i].audioStreams[1].isAac = 1;
                cameras[i].audioStreams[0].IsTestAACWithoutAdts = 1;
                cameras[i].audioStreams[1].IsTestAACWithoutAdts = 1;
                //TODO default file
                strcpy(cameras[i].audioStreams[0].file, "1_16000_a.aac");
                strcpy(cameras[i].audioStreams[1].file, "1_16000_a.aac");
		if (type == 0) {
                        printf("set video file to:len.264\n");
                        strcpy(cameras[i].videoStreams[0].file, "len.h264");
                        strcpy(cameras[i].videoStreams[1].file, "len.h264");
		} else {
                        printf("set video file to:len.265\n");
                        strcpy(cameras[i].videoStreams[0].file, "len.h265");
                        strcpy(cameras[i].videoStreams[1].file, "len.h265");
                        cameras[i].videoStreams[1].isH265 = 1;
                        cameras[i].videoStreams[0].isH265 = 1;
		}
        }
        
        return 0;
}

int dev_sdk_stop_video(int camera, int stream)
{
        cameras[camera].videoStreams[stream].isStop = 1;
        pthread_t tid;
        memset(&tid, 0, sizeof(tid));
        if(memcmp(&tid, &cameras[camera].audioStreams[stream].tid, sizeof(tid)) != 0) {
                pthread_join(cameras[camera].videoStreams[stream].tid, NULL);
        }
        cameras[camera].videoStreams[stream].videoCb = NULL;
        cameras[camera].videoStreams[stream].pContext = NULL;
        memset(&cameras[camera].videoStreams[stream].tid, 0, sizeof(pthread_t));
        return 0;
}

int dev_sdk_stop_audio(int camera, int stream)
{
        cameras[camera].audioStreams[stream].isStop = 1;
        pthread_t tid;
        memset(&tid, 0, sizeof(tid));
        if(memcmp(&tid, &cameras[camera].audioStreams[stream].tid, sizeof(tid)) != 0) {
                pthread_join(cameras[camera].audioStreams[stream].tid, NULL);
        }
        cameras[camera].audioStreams[stream].audioCb = NULL;
        cameras[camera].videoStreams[stream].pContext = NULL;
        memset(&cameras[camera].audioStreams[stream].tid, 0, sizeof(pthread_t));
        return 0;
}

int dev_sdk_stop_audio_play(void)
{
        return 0;
}

void *VideoCaptureTask( void *param )
{
        start_video_file_test(param);
        return NULL;
}


int dev_sdk_start_video(int camera, int stream, VIDEO_CALLBACK vcb, void *pcontext)
{
        (void)pcontext;
        Stream *pStream = &cameras[camera].videoStreams[stream];
        if (pStream->videoCb != NULL) {
                return 0;
        }
        pStream->videoCb = vcb;
        
        cameras[camera].videoStreams[stream].isStop = 0;
        cameras[camera].videoStreams[stream].pContext = pcontext;
        pthread_create( &pStream->tid, NULL, VideoCaptureTask, (void *)pStream );
        return 0;
}

void *AudioCaptureTask( void *param )
{
        start_audio_file_test(param);
        return NULL;
}

int dev_sdk_start_audio(int camera, int stream, AUDIO_CALLBACK acb, void *pcontext)
{
        Stream *pStream = &cameras[camera].audioStreams[stream];
        if (pStream->audioCb != NULL) {
                return 0;
        }
        pStream->audioCb = acb;
        
        cameras[camera].audioStreams[stream].isStop = 0;
        cameras[camera].videoStreams[stream].pContext = pcontext;
        pthread_create( &pStream->tid, NULL, AudioCaptureTask, (void *)pStream );
        
        return 0;
}

int dev_sdk_release(void)
{
        return 0;
}


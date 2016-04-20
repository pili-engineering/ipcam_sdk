#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "aac_encoder.h"
#include "g711.h"

#define DEBUG

#ifndef DEBUG
#define Debug(fmt, arg...)

#else
#include <stdio.h>
#define Debug(fmt, arg...) do {\
                                        printf( "file:%s, line:%d->" fmt "\n",  __FILE__, __LINE__, ##arg);\
                                } while(0)
#endif



#ifndef xyzz
//#ifdef  __USE_FDKAAC__

#include <aacenc_lib.h>

static void AacEncoderDestroy(AacEncoderContext * _pAacCtx);

typedef struct _FdkaacConfig
{ 
        /* IN */
        AUDIO_OBJECT_TYPE objectType;
        unsigned long nSampleRate;
        CHANNEL_MODE channelMode;
        unsigned int nBitrate;
        TRANSPORT_TYPE transportType;

        /* INTERNAL USE */
        unsigned int nSampleSize;
        uint8_t *pConvertBuf;
        int nInputSize;
        HANDLE_AACENCODER pEncoder;
        AACENC_InfoStruct info;
        unsigned int nChannels;
} FdkaacConfig;

void AacEncoderSetChannels(AacEncoderContext * pAacContext, unsigned int _nChannels)
{
        FdkaacConfig * pAac = (FdkaacConfig * )pAacContext;
        CHANNEL_MODE mode;

        switch (_nChannels) {
        case 1:
                mode = MODE_1;
                break;
        case 2:
                mode = MODE_2;
                break;
        case 3:
                mode = MODE_1_2;
                break;
        case 4:
                mode = MODE_1_2_1;
                break;
        case 5:
                mode = MODE_1_2_2;
                break;
        case 6:
                mode = MODE_1_2_2_1;
                break;
        default:
                mode = MODE_1;
        }
        pAac->channelMode = mode;
        pAac->nChannels = _nChannels;

}

AacEncoderContext * AacEncoderNew()
{ 
        FdkaacConfig * pEncoder = (FdkaacConfig *)malloc(sizeof(*pEncoder));
        if (pEncoder == NULL) {
                return NULL;
        }
        memset(pEncoder, 0, sizeof(*pEncoder));

        pEncoder->objectType = AOT_AAC_LC;
        pEncoder->nSampleRate = 8000;
        AacEncoderSetChannels((void *)pEncoder, 1);

        pEncoder->nBitrate = 6400;
        pEncoder->transportType = TT_MP4_ADTS;

        pEncoder->nSampleSize = 2;
        pEncoder->pConvertBuf = NULL;
        pEncoder->nInputSize = 0;


        AacEncoderSetMpegType(pEncoder, AAC_MPEG_MPEG2);        

        return pEncoder; 
}

void AacEncoderDel(AacEncoderContext *_pAacCtx)
{
        if (_pAacCtx != NULL) {
                if (((FdkaacConfig *)_pAacCtx)->pEncoder) {
                        AacEncoderDestroy(_pAacCtx);
                }
                free(_pAacCtx);
        }
}


void AacEncoderSetMpegType(AacEncoderContext * _pAacCtx, AacMpegType _nMpegType)
{
        AUDIO_OBJECT_TYPE nObjType =  ((FdkaacConfig *)_pAacCtx)->objectType;
        switch (_nMpegType) {
        case AAC_MPEG_MPEG2:
                ((FdkaacConfig *)_pAacCtx)->objectType = (AUDIO_OBJECT_TYPE)(nObjType + 127);
                break;
        case AAC_MPEG_MPEG4:
                break;
        }
}


void AacEncoderSetObjectType(AacEncoderContext * _pAacCtx, AacObjectType _nObjectType)
{
        switch (_nObjectType) {
        case AAC_ENC_LC:
                ((FdkaacConfig *)_pAacCtx)->objectType = AOT_AAC_LC;
                break;
        }
}

void AacEncoderSetOutputType(AacEncoderContext * _pAacCtx, AacOutputType _nOutputType)
{
        switch (_nOutputType) {
        case AAC_OUTPUT_ADTS:
                ((FdkaacConfig *)_pAacCtx)->transportType = TT_MP4_ADTS;
                break;
        case AAC_OUTPUT_RAW:
                ((FdkaacConfig *)_pAacCtx)->transportType = TT_MP4_RAW;
                break;
        }
}


void AacEncoderSetSampleRate(AacEncoderContext * _pAacCtx, unsigned int _nSampleRate)
{
        ((FdkaacConfig *)_pAacCtx)->nSampleRate = _nSampleRate;
}

int AacEncoderInit(AacEncoderContext *_pAacCtx)
{
        FdkaacConfig *pFdkaac;
        CHANNEL_MODE mode;
  
        pFdkaac = (FdkaacConfig *)_pAacCtx;
        if (aacEncOpen(&pFdkaac->pEncoder, 1, pFdkaac->channelMode) != AACENC_OK) {
                pFdkaac->pEncoder = NULL;
                Debug("err");
                return -1;
        }
        if((aacEncoder_SetParam(pFdkaac->pEncoder, AACENC_AOT, pFdkaac->objectType)) !=  AACENC_OK){
                Debug("error");
                return -1;
        }
        int nRet =  aacEncoder_SetParam(pFdkaac->pEncoder, AACENC_SAMPLERATE, pFdkaac->nSampleRate);
        if( nRet != AACENC_OK){
                Debug("error, nRet = %d", nRet);
                return -1;
        }
        if(aacEncoder_SetParam(pFdkaac->pEncoder, AACENC_CHANNELMODE, pFdkaac->channelMode) != AACENC_OK) {
                Debug("error");
                return -1;
        }
        if(aacEncoder_SetParam(pFdkaac->pEncoder, AACENC_TRANSMUX, pFdkaac->transportType) != AACENC_OK) {
                Debug("error");
                return -1;
        }
        // TODO add bitrate control
/*
        if (aacEncoder_SetParam(pFdkaac->pEncoder, AACENC_BITRATE, pFdkaac->nBitrate) != AACENC_OK) {
                return false;
        }
*/
        if (aacEncEncode(pFdkaac->pEncoder, NULL, NULL, NULL, NULL) != AACENC_OK) {
                Debug("error");
                return -1;
        }
        if (aacEncInfo(pFdkaac->pEncoder, &pFdkaac->info) != AACENC_OK) {
                Debug("error");
                return -1;
        }
        // calculate input size
        pFdkaac->nInputSize = pFdkaac->nChannels * pFdkaac->info.frameLength * pFdkaac->nSampleSize;
        pFdkaac->pConvertBuf = (uint8_t*)malloc(pFdkaac->nInputSize);

        return 0;
}

int AacEncoderEncodePcm(AacEncoderContext * _pAacEncoder, void *_pOutBuf, size_t *_pOutSize, const void *_pInBuf, const size_t _nInSize)

{
        FdkaacConfig *pFdkaac = (FdkaacConfig *)_pAacEncoder;
        int nInputSize = pFdkaac->nInputSize;
        uint8_t *pConvertBuf = pFdkaac->pConvertBuf;
        uint8_t *pCurrentIn = (uint8_t *)_pInBuf;
        uint8_t *pCurrentOut = (uint8_t *)_pOutBuf;
        *_pOutSize = 0;

#ifdef __PRINT_AAC_DEBUG_INFO__
        clock_t nStart, nEnd, nDuration = 0;
        unsigned int nTimesHandled = 0;
        int nBytesHandled = 0, nBytesConsumed = 0;

        cout << "AAC: ----------------" << endl
             << "AAC: nInputSize=" << nInputSize << endl
             << "AAC: ----------------" << endl;
#endif

        while (pCurrentIn < (uint8_t *)_pInBuf + _nInSize) {
                AACENC_BufDesc inBuf = { 0 }, outBuf = { 0 };
                AACENC_InArgs inArgs = { 0 };
                AACENC_OutArgs outArgs = { 0 };
                int nInIdentifier = IN_AUDIO_DATA;
                int nInSize, nInElemSize;
                int nOutIdentifier = OUT_BITSTREAM_DATA;
                int nOutSize, nOutElemSize;
                AACENC_ERROR err;

                // move inputs to convert buffer
                int nRestBytes = (uint8_t *)_pInBuf + _nInSize - pCurrentIn;
                int nBytesToEnc;
                if (nRestBytes >= nInputSize) {
                        memcpy(pConvertBuf, pCurrentIn, nInputSize);
                        nBytesToEnc = nInputSize;
                } else {
                        memcpy(pConvertBuf, pCurrentIn, nRestBytes);
                        nBytesToEnc = nRestBytes;
                }
                // exchange endian
                int i = 0;
                for (i = 0; i < nBytesToEnc / 2; i++) {
                        const uint8_t *pIn = &pCurrentIn[2 * i];
                        pConvertBuf[2 * i] = pIn[0];
                        pConvertBuf[2 * i + 1] = pIn[1];
                }

                // IN-param info
                inArgs.numInSamples = nBytesToEnc / pFdkaac->nSampleSize;
                inBuf.numBufs = 1;
                inBuf.bufs = (void **)(&pConvertBuf);
                inBuf.bufferIdentifiers = &nInIdentifier;
                inBuf.bufSizes = &nBytesToEnc;
                inBuf.bufElSizes = (INT *)(&pFdkaac->nSampleSize);

                // OUT-param info
                nOutElemSize = 1;
                nOutSize = pFdkaac->nInputSize;
                outBuf.numBufs = 1;
                outBuf.bufs = (void **)(&pCurrentOut);
                outBuf.bufferIdentifiers = &nOutIdentifier;
                outBuf.bufSizes = &nOutSize;
                outBuf.bufElSizes = &nOutElemSize;

#ifdef __PRINT_AAC_DEBUG_INFO__
                cout << "AAC: +++++" << endl
                     << "AAC: nBytesToEnc=" << nBytesToEnc << endl;
                nStart = clock();
#endif
                // begin to encode
                err = aacEncEncode(pFdkaac->pEncoder, &inBuf, &outBuf, &inArgs, &outArgs);
#ifdef __PRINT_AAC_DEBUG_INFO__
                nEnd = clock();
#endif
                if (err != AACENC_OK) {
                        if (err == AACENC_ENCODE_EOF) {
                                break;
                        }
                        return -1;
                }

#ifdef __PRINT_AAC_DEBUG_INFO__
                nDuration += nEnd - nStart;
                nTimesHandled++;
                cout << "AAC: nTimesHandled=" << nTimesHandled << endl
                     << "AAC: nDuration=" << nDuration << endl;

                // move input/output current pointer
                nBytesConsumed = outArgs.numInSamples * pFdkaac->nSampleSize;
                nBytesHandled += nBytesConsumed;
                cout << "AAC: out_bytes=" << outArgs.numOutBytes << endl
                     << "AAC: in_bytes_consumed=" << nBytesConsumed << endl
                     << "AAC: nBytesHandled=" << nBytesHandled << "/" << _nInSize << endl;
#endif

                if (outArgs.numOutBytes > 0) {
                        pCurrentOut += outArgs.numOutBytes;
                        *_pOutSize += outArgs.numOutBytes;
                }
                pCurrentIn += outArgs.numInSamples * pFdkaac->nSampleSize;
        }
        return 0;
}

static void AacEncoderDestroy(AacEncoderContext * _pAacCtx)
{
        if (_pAacCtx == NULL) {
                return;
        }
        
        if (((FdkaacConfig *)_pAacCtx)->pConvertBuf != NULL) {
                free(((FdkaacConfig *)_pAacCtx)->pConvertBuf);
        }
        if (aacEncClose(&((FdkaacConfig *)_pAacCtx)->pEncoder) != AACENC_OK) {
                return;
        }
        return;
}

typedef int (*DecoderProc_t)(void *, size_t *, const void *, const size_t);

static int DecodeToPcm(AacEncoderContext * pAacCondext, DecoderProc_t _pProc, void * _pOutBuf, size_t * pOutSize, const void * _pInBuf, const size_t _nInSize)
{
        size_t nPcmBufferSize = _nInSize * 2;

        char *pPcmBuffer = (char *)malloc(nPcmBufferSize);

        // zero out buffer
        memset(pPcmBuffer, 0, nPcmBufferSize);
        
        if (_pProc != NULL) {
                if (!_pProc(pPcmBuffer, &nPcmBufferSize, _pInBuf, _nInSize)) {
                        free(pPcmBuffer);
                        return -1;
                }
        }
        int nRet = AacEncoderEncodePcm(pAacCondext, _pOutBuf, pOutSize, pPcmBuffer, nPcmBufferSize);
        if (nRet < 0) {
                free(pPcmBuffer);
                return  -1;
        }
        free(pPcmBuffer);
        return 0;

}

int AacEncoderEncodePcma(AacEncoderContext * _pAacEncoder, void * _pOutBuf, size_t * _pOutSize, const void *_pInBuf, const size_t _nInSize)
{
        if (_nInSize == 0) {
                return -1;
        }
        return DecodeToPcm(_pAacEncoder, PcmAlawDecode, _pOutBuf, _pOutSize, _pInBuf, _nInSize);
}

int AacEncoderEncodePcmu(AacEncoderContext * _pAacEncoder, void * _pOutBuf, size_t * _pOutSize, const void *_pInBuf, const size_t _nInSize)
{
        if (_nInSize == 0) {
                return -1;
        }
        return DecodeToPcm(_pAacEncoder, PcmMulawDecode, _pOutBuf, _pOutSize, _pInBuf, _nInSize);
}


#endif

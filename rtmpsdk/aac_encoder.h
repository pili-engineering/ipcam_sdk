#ifndef __AAC_ENCODER_PUBLISH__
#define __AAC_ENCODER_PUBLISH__

#ifdef __cplusplus    
extern "C" {         
#endif

typedef void  AacEncoderContext;

typedef enum
{
        AAC_ENC_LC
} AacObjectType;

typedef enum
{
        AAC_MPEG_MPEG2,
        AAC_MPEG_MPEG4
} AacMpegType;

typedef enum
{
        AAC_INPUT_16BIT
} AacInputType;

typedef enum
{
        AAC_OUTPUT_ADTS,
        AAC_OUTPUT_RAW
} AacOutputType;

AacEncoderContext * AacEncoderNew();
int AacEncoderInit(AacEncoderContext *_pAacCtx);
void AacEncoderDel(AacEncoderContext *_pAacCtx);
void AacEncoderSetMpegType(AacEncoderContext * _pAacCtx, AacMpegType _nMpegType);
void AacEncoderSetObjectType(AacEncoderContext * _pAacCtx, AacObjectType _nObjectType);
void AacEncoderSetOutputType(AacEncoderContext * _pAacCtx, AacOutputType _nOutputType);
void AacEncoderSetSampleRate(AacEncoderContext * _pAacCtx, unsigned int _nSampleRate);
int AacEncoderEncodePcm(AacEncoderContext * _pAacEncoder, void *_pOutBuf, size_t *_pOutSize, const void *_pInBuf, const size_t _nInSize);


void AacEncoderSetChannels(AacEncoderContext * pAacContext, unsigned int _nChannels);

int AacEncoderEncodePcma(AacEncoderContext * _pAacEncoder, void * _pOutBuf, size_t * _pOutSize, const void *_pInBuf, const size_t _nInSize);

int AacEncoderEncodePcmu(AacEncoderContext * _pAacEncoder, void * _pOutBuf, size_t * _pOutSize, const void *_pInBuf, const size_t _nInSize);


#ifdef __cplusplus    
}       
#endif

#endif

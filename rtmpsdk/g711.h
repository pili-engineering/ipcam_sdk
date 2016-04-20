#ifndef __G711_PUBLISH__
#define __G711_PUBLISH__

#ifdef __cplusplus
extern "C" {
#endif


int PcmAlawDecode(void * pOutBuf, size_t * pOutSamples, const void * pInBuf, const size_t nInSamples);
int PcmMulawDecode(void * pOutBuf, size_t * pOutSamples, const void * pInBuf, const size_t nInSamples);




#ifdef __cplusplus
}
#endif

#endif






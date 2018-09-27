#include "adts.h"
#include <math.h>

void InitAdtsFixedHeader(ADTSFixheader *_pHeader) {
    _pHeader->syncword                 = 0xFFF;
    _pHeader->id                       = 0;
    _pHeader->layer                    = 0;
    _pHeader->protection_absent        = 1;
    _pHeader->profile                  = 1;
    _pHeader->sampling_frequency_index = 4;
    _pHeader->private_bit              = 0;
    _pHeader->channel_configuration    = 2;
    _pHeader->original_copy            = 0;
    _pHeader->home                     = 0;
}

void InitAdtsVariableHeader(ADTSVariableHeader *_pHeader, const int _nAacLenWithoutHeader) {
    _pHeader->copyright_identification_bit       = 0;
    _pHeader->copyright_identification_start     = 0;
    _pHeader->aac_frame_length                   = _nAacLenWithoutHeader + 7;
    _pHeader->adts_buffer_fullness               = 0x7ff;
    _pHeader->number_of_raw_data_blocks_in_frame = 0;
}

void ParseAdtsfixedHeader(const unsigned char *pData, ADTSFixheader *_pHeader) {
    unsigned long long adts = 0;
    const unsigned char *p = pData;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++;
    
    
    _pHeader->syncword                 = (adts >> 44);
    _pHeader->id                       = (adts >> 43) & 0x01;
    _pHeader->layer                    = (adts >> 41) & 0x03;
    _pHeader->protection_absent        = (adts >> 40) & 0x01;
    _pHeader->profile                  = (adts >> 38) & 0x03;
    _pHeader->sampling_frequency_index = (adts >> 34) & 0x0f;
    _pHeader->private_bit              = (adts >> 33) & 0x01;
    _pHeader->channel_configuration    = (adts >> 30) & 0x07;
    _pHeader->original_copy            = (adts >> 29) & 0x01;
    _pHeader->home                     = (adts >> 28) & 0x01;
}

void ParseAdtsVariableHeader(const unsigned char *pData, ADTSVariableHeader *_pHeader) {
    unsigned long long adts = 0;
    adts  = pData[0]; adts <<= 8;
    adts |= pData[1]; adts <<= 8;
    adts |= pData[2]; adts <<= 8;
    adts |= pData[3]; adts <<= 8;
    adts |= pData[4]; adts <<= 8;
    adts |= pData[5]; adts <<= 8;
    adts |= pData[6];
    
    _pHeader->copyright_identification_bit = (adts >> 27) & 0x01;
    _pHeader->copyright_identification_start = (adts >> 26) & 0x01;
    _pHeader->aac_frame_length = (adts >> 13) & ((int)pow(2, 14) - 1);
    _pHeader->adts_buffer_fullness = (adts >> 2) & ((int)pow(2, 11) - 1);
    _pHeader->number_of_raw_data_blocks_in_frame = adts & 0x03;
}

void ConvertAdtsHeader2Int64(const ADTSFixheader *_pFixedHeader, const ADTSVariableHeader *_pVarHeader, uint64_t *_pHeader) {
    uint64_t ret_value = 0;
    ret_value = _pFixedHeader->syncword;
    ret_value <<= 1;
    ret_value |= _pFixedHeader->id;
    ret_value <<= 2;
    ret_value |= _pFixedHeader->layer;
    ret_value <<= 1;
    ret_value |= (_pFixedHeader->protection_absent) & 0x01;
    ret_value <<= 2;
    ret_value |= _pFixedHeader->profile;
    ret_value <<= 4;
    ret_value |= (_pFixedHeader->sampling_frequency_index) & 0x0f;
    ret_value <<= 1;
    ret_value |= (_pFixedHeader->private_bit) & 0x01;
    ret_value <<= 3;
    ret_value |= (_pFixedHeader->channel_configuration) & 0x07;
    ret_value <<= 1;
    ret_value |= (_pFixedHeader->original_copy) & 0x01;
    ret_value <<= 1;
    ret_value |= (_pFixedHeader->home) & 0x01;
    
    ret_value <<= 1;
    ret_value |= (_pVarHeader->copyright_identification_bit) & 0x01;
    ret_value <<= 1;
    ret_value |= (_pVarHeader->copyright_identification_start) & 0x01;
    ret_value <<= 13;
    ret_value |= (_pVarHeader->aac_frame_length) & ((int)pow(2, 13) - 1);
    ret_value <<= 11;
    ret_value |= (_pVarHeader->adts_buffer_fullness) & ((int)pow(2, 11) - 1);
    ret_value <<= 2;
    ret_value |= (_pVarHeader->number_of_raw_data_blocks_in_frame) & ((int)pow(2, 2) - 1);
    
    *_pHeader = ret_value;
}

void ConvertAdtsHeader2Char(const ADTSFixheader *_pFixedHeader, const ADTSVariableHeader *_pVarHeader, unsigned char *pHeader) {
    uint64_t value = 0;
    ConvertAdtsHeader2Int64(_pFixedHeader, _pVarHeader, &value);
    
    pHeader[0] = (value >> 48) & 0xff;
    pHeader[1] = (value >> 40) & 0xff;
    pHeader[2] = (value >> 32) & 0xff;
    pHeader[3] = (value >> 24) & 0xff;
    pHeader[4] = (value >> 16) & 0xff;
    pHeader[5] = (value >> 8) & 0xff;
    pHeader[6] = (value) & 0xff;
}

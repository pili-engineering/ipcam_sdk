#ifndef __ADTS_H__
#define __ADTS_H__


#include <stdio.h>
#include <stdint.h>

typedef struct _ADTSFixheader {
    unsigned short syncword:		    12;
    unsigned char id:                       1;
    unsigned char layer:		    2;
    unsigned char protection_absent:        1;
    unsigned char profile:                  2;
    unsigned char sampling_frequency_index: 4;
    unsigned char private_bit:              1;
    unsigned char channel_configuration:    3;
    unsigned char original_copy:	    1;
    unsigned char home:                     1;
} ADTSFixheader;

typedef struct _ADTSVariableHeader {
    unsigned char copyright_identification_bit:		1;
    unsigned char copyright_identification_start:	1;
    unsigned short aac_frame_length:			13;
    unsigned short adts_buffer_fullness:		11;
    unsigned char number_of_raw_data_blocks_in_frame:   2;
} ADTSVariableHeader;

void InitAdtsFixedHeader(ADTSFixheader *pHeader);

void InitAdtsVariableHeader(ADTSVariableHeader *pHeader, const int nAacLenWithoutHeader);

extern void ParseAdtsfixedHeader(const unsigned char *pData, ADTSFixheader *pHeader);

extern void ParseAdtsVariableHeader(const unsigned char *pData, ADTSVariableHeader *pHeader);

// 7 byte adts convert to a int64 number
extern void ConvertAdtsHeader2Int64(const ADTSFixheader *pFixedHeader, const ADTSVariableHeader *pVarHeader, uint64_t *pHeader);

// 7 byte adts convert to char[7]
extern void ConvertAdtsHeader2Char(const ADTSFixheader *pFixedHeader, const ADTSVariableHeader *pVarHeader, unsigned char *pData);

#endif

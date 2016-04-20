#include <stdlib.h>
#include <stdint.h>
#include "g711.h"

#define SIGN_BIT   (0x80)   /* Sign bit for a A-law byte. */
#define QUANT_MASK (0xf)    /* Quantization field mask. */
#define NSEGS      (8)      /* Number of A-law segments. */
#define SEG_SHIFT  (4)      /* Left shift for segment number. */
#define SEG_MASK   (0x70)   /* Segment field mask. */
#define BIAS       (0x84)   /* Bias for linear code. */


int PcmAlawDecode(void * pOutBuf, size_t * pOutSamples, const void * pInBuf, const size_t nInSamples);
int PcmMulawDecode(void * pOutBuf, size_t * pOutSamples, const void * pInBuf, const size_t nInSamples);

static const uint8_t m_a2mu[128] =
{
        1,    3,    5,    7,    9,    11,    13,    15,
        16,    17,    18,    19,    20,    21,    22,    23,
        24,    25,    26,    27,    28,    29,    30,    31,
        32,    32,    33,    33,    34,    34,    35,    35,
        36,    37,    38,    39,    40,    41,    42,    43,
        44,    45,    46,    47,    48,    48,    49,    49,
        50,    51,    52,    53,    54,    55,    56,    57,
        58,    59,    60,    61,    62,    63,    64,    64,
        65,    66,    67,    68,    69,    70,    71,    72,
        73,    74,    75,    76,    77,    78,    79,    79,
        80,    81,    82,    83,    84,    85,    86,    87,
        88,    89,    90,    91,    92,    93,    94,    95,
        96,    97,    98,    99,    100,    101,    102,    103,
        104,    105,    106,    107,    108,    109,    110,    111,
        112,    113,    114,    115,    116,    117,    118,    119,
        120,    121,    122,    123,    124,    125,    126,    127
};

static const uint8_t m_mu2a[128] =
{
        1,    1,    2,    2,    3,    3,    4,    4,
        5,    5,    6,    6,    7,    7,    8,    8,
        9,     10,    11,    12,    13,    14,    15,    16,
        17,    18,    19,    20,    21,    22,    23,    24,
        25,    27,    29,    31,    33,    34,    35,    36,
        37,    38,    39,    40,    41,    42,    43,    44,
        46,    48,    49,    50,    51,    52,    53,    54,
        55,    56,    57,    58,    59,    60,    61,    62,
        64,    65,    66,    67,    68,    69,    70,    71,
        72,    73,    74,    75,    76,    77,    78,    79,
        81,    82,    83,    84,    85,    86,    87,    88,
        89,    90,    91,    92,    93,    94,    95,    96,
        97,    98,    99,    100,    101,    102,    103,    104,
        105,    106,    107,    108,    109,    110,    111,    112,
        113,    114,    115,    116,    117,    118,    119,    120,
        121,    122,    123,    124,    125,    126,    127,    128
};

static int16_t ALaw2Linear(uint8_t _nSample)
{
        int nVal;
        int nSeg;

        _nSample ^= 0x55;
        nVal = (_nSample & QUANT_MASK) << 4;
        nSeg = (((unsigned int)(_nSample)) & SEG_MASK) >> SEG_SHIFT;
        switch (nSeg) {
        case 0:
                nVal += 8;
                break;
        case 1:
                nVal += 0x108;
                break;
        default:
                nVal += 0x108;
                nVal <<= nSeg - 1;
        }
        return (int16_t)((_nSample & SIGN_BIT) ? nVal : -nVal);
}

static int16_t MuLaw2Linear(uint8_t _nSample)
{
        int nVal;

        _nSample = ~_nSample;
        nVal = ((_nSample & QUANT_MASK) << 3) + BIAS;
        nVal <<= (((unsigned int)(_nSample)) & SEG_MASK) >> SEG_SHIFT;
        return (int16_t)((_nSample & SIGN_BIT) ? (BIAS - nVal) : (nVal - BIAS));
}

static int Decode(void *_pOutBuf, size_t *_pOutSamples, const void *_pInBuf, const size_t _nInSamples, 
                      int16_t (*_pDecFunc)(uint8_t))
{
        if (_pOutBuf == NULL || _pOutSamples == NULL || _pInBuf == NULL || _nInSamples == 0) {
                return -1;
        }
        int16_t *pDst = (int16_t *)_pOutBuf;
        const uint8_t *pSrc = (uint8_t *)_pInBuf;
        size_t i = 0;
        for (i = 0; i < _nInSamples; i++) {
                *(pDst++) = _pDecFunc(*(pSrc++));
        }
        *_pOutSamples = _nInSamples * 2;
        return -1;
}

int PcmAlawDecode(void * pOutBuf, size_t * pOutSamples, const void * pInBuf, const size_t nInSamples)
{
        return Decode(pOutBuf, pOutSamples, pInBuf, nInSamples, ALaw2Linear);
}

int PcmMulawDecode(void * pOutBuf, size_t * pOutSamples, const void * pInBuf, const size_t nInSamples)
{
        return Decode(pOutBuf, pOutSamples, pInBuf, nInSamples, MuLaw2Linear);
}


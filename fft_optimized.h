/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Colin Duffy, https://github.com/duff2013
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "coeff.h"
#include "arm_math.h"

#define FFT_SIZE 256
#define TWO_FFT_SIZE ( FFT_SIZE * 2 ) - 1
#define HALF_FFT_SIZE FFT_SIZE / 2

int32_t OutputBuffer_i32[FFT_SIZE * 2];

//----------------------------------------------------------------------------------------------
// see http://www.ti.com/lit/an/spra291/spra291.pdf
static void extract_2_for_1_fft_outputs( uint32_t *fft_buf, int32_t *buf1, int32_t *buf2 ) {
    
    uint32_t *BUF  = ( uint32_t * )fft_buf;
    uint32_t *BUF1 = ( uint32_t * )buf1;
    uint32_t *BUF2 = ( uint32_t * )buf2;
    
    // buf1 and buf2's top 16 bits(imag) are assumed as 0
    BUF1[0] = BUF[0] & 0x0000FFFF;  // buf1 real = buf real
    BUF2[0] = BUF[0] >> 16;         // buf2 real = buf imag
    // compute inner fft values
    for ( int i = 1; i < HALF_FFT_SIZE; i++ ) {
        // rotate to correct position
        uint32_t ror1 = __ROR( BUF[FFT_SIZE - i], 16 );
        uint32_t ror2 = __ROR( BUF[i], 16 );
        // extract the 2 for 1 fft's
        int32_t b1 = __SHSAX( BUF[i], ror1 );
        int32_t b2 = __SHSAX( ror2, BUF[FFT_SIZE - i] );
        // save to partial buffers
        BUF1[i]            = b1;
        BUF1[FFT_SIZE - i] = ( -b1 & 0xFFFF0000 ) | ( b1 & 0xFFFF );
        BUF2[i]            = ( -b2 & 0xFFFF0000 ) | ( b2 & 0xFFFF );
        BUF2[FFT_SIZE - i] = b2;
    }
    // nyquist point
    BUF1[FFT_SIZE >> 1] = BUF[FFT_SIZE >> 1] & 0x0000FFFF;
    BUF2[FFT_SIZE >> 1] = BUF[FFT_SIZE >> 1] >> 16;
}

static void extract_2_for_1_fft_outputs_new( uint32_t *fft_buf ) {
    
    uint32_t *BUF  = ( uint32_t * )fft_buf;
    
    BUF[0]        = BUF[0] & 0x0000FFFF;
    BUF[FFT_SIZE] = BUF[0] >> 16;
    // compute inner fft values
    for ( int i = 1; i < HALF_FFT_SIZE; i++ ) {
        // rotate to correct position
        uint32_t ror1 = __ROR( BUF[FFT_SIZE - i], 16 );
        uint32_t ror2 = __ROR( BUF[i], 16 );
        // extract the 2 for 1 fft's
        int32_t b1 = __SHSAX( BUF[i], ror1 );
        int32_t b2 = __SHSAX( ror2, BUF[FFT_SIZE - i] );
        // save to partial buffers
        BUF[i]            = b1;
        BUF[FFT_SIZE - i] = ( -b1 & 0xFFFF0000 ) | ( b1 & 0xFFFF );
        
        BUF[FFT_SIZE + i]         = ( -b2 & 0xFFFF0000 ) | ( b2 & 0xFFFF );
        BUF[( FFT_SIZE * 2 ) - i] = b2;
    }
    // nyquist point
    BUF[( FFT_SIZE * 2 ) >> 1] = BUF[FFT_SIZE >> 1] >> 16;
    BUF[FFT_SIZE >> 1]         = BUF[FFT_SIZE >> 1] & 0x0000FFFF;
}
//----------------------------------------------------------------------------------------------
// see https://www.dsprelated.com/showarticle/63.php
static uint32_t *combine_ffts( uint32_t *in1, uint32_t *in2, uint32_t *in3, uint32_t *in4 ) {
    
    const uint32_t *input_1 = in1;
    const uint32_t *input_2 = in2;
    const uint32_t *input_3 = in3;
    const uint32_t *input_4 = in4;
    
    // load precomputed coeff's
    const uint32_t *coef_1 = coeff1_256_Q15;
    const uint32_t *coef_2 = coeff2_256_Q15;
    const uint32_t *coef_3 = coeff3_256_Q15;
    
    int idx = 0;
    for ( int i = TWO_FFT_SIZE; i != -1; i-- ) {
        
        idx = i % FFT_SIZE;
        int32_t realData = 0;
        int32_t imagData = 0;
        
        // Two complex numbers x=a+ib and y=c+id are multiplied as follows:
        // x*y = (a+ib)(c+id) = ac+ibc+iad-bd = (ac-bd)+i(ad+bc).
        realData = __SMLSD ( input_2[idx], coef_1[idx], realData );
        imagData = __SMLADX( input_2[idx], coef_1[idx], imagData );
        
        realData = __SMLSD ( input_3[idx], coef_2[idx], realData );
        imagData = __SMLADX( input_3[idx], coef_2[idx], imagData );
        
        realData = __SMLSD ( input_4[idx], coef_3[idx], realData );
        imagData = __SMLADX( input_4[idx], coef_3[idx], imagData );
        
        // This is the final fft output -> real bottom 16 | imag top 16
        OutputBuffer_i32[i] = ( signed_saturate_rshift( realData, 32, 14 ) + ( input_1[idx] & 0xFFFF ) ) | ( ( signed_saturate_rshift( imagData, 32, 14 ) + ( input_1[idx] >> 16 ) ) << 16 );
    }
    
    return ( uint32_t * )OutputBuffer_i32;
}
// TODO: probably slower but we will see... not working yet
/*static uint32_t *combine_ffts_new( uint16_t *in1, uint16_t *in2, uint16_t *in3, uint16_t *in4 ) {
    const uint16_t *input_1 = in1;
    const uint16_t *input_2 = in2;
    const uint16_t *input_3 = in3;
    const uint16_t *input_4 = in4;
    
    // load precomputed coeff's
    const uint32_t *coef_1 = coeff1_256_Q15;
    const uint32_t *coef_2 = coeff2_256_Q15;
    const uint32_t *coef_3 = coeff3_256_Q15;
    
    arm_cmplx_mult_cmplx_q15((q15_t *)input_2, (q15_t *)coeff1_256_Q15_t, (q15_t *)OutputBuffer_i32+000, 512);
    arm_cmplx_mult_cmplx_q15((q15_t *)input_3, (q15_t *)coeff2_256_Q15_t, (q15_t *)OutputBuffer_i32+512, 512);
    arm_cmplx_mult_cmplx_q15((q15_t *)input_4, (q15_t *)coeff3_256_Q15_t, (q15_t *)OutputBuffer_i32+512, 512);
    
    return ( uint32_t * )OutputBuffer_i32;
}*/
//----------------------------------------------------------------------------------------------
static void interleave_block_to_buffers( void *destination1, void *destination2, const void *source, const void *window ) {
    
    int16_t *dst1 = ( int16_t * )destination1;
    int16_t *dst2 = ( int16_t * )destination2;
    
    const int16_t *src = ( int16_t * )source;
    const int16_t *src_end = src + AUDIO_BLOCK_SAMPLES;
    
    const int16_t *win = ( int16_t * )window;
    
    // interleave from source buffer to destination(x) buffer for fft calculation (2 buffers).
    // destination buffer looks like this:
    // destination1 real index -> 0,3,5,...
    // destination1 imag index -> 1,4,6,...
    // destination2 real index -> 2,5,7,...
    // destination2 real index -> 3,6,8,...
    do {
        *dst1++ = ( *src++ * *win++ ) >> 15; // put data into real dst1 index
        *dst1++ = ( *src++ * *win++ ) >> 15; // put data into imag dst1 index
        *dst2++ = ( *src++ * *win++ ) >> 15; // put data into real dst2 index
        *dst2++ = ( *src++ * *win++ ) >> 15; // put data into imag dst2 index
    } while ( src != src_end );
}

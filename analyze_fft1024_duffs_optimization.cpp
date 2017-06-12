/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
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

#include "analyze_fft1024_duffs_optimization.h"
#include "utility/sqrt_integer.h"
#include "utility/dspinst.h"
#include "fft_optimized.h"

//static inline void extract_2_for_1_fft_outputs(uint32_t *fft_buf, int32_t *buf1, int32_t *buf2) __attribute__((always_inline, unused));
//static inline uint32_t *combine_ffts(uint32_t *in1, uint32_t *in2, uint32_t *in3, uint32_t *in4) __attribute__((always_inline, unused));
//static inline void interleave_block_to_buffers(void *destination1, void *destination2, const void *source, const void *window) __attribute__((always_inline, unused));

void AudioAnalyzeFFT1024_Duffs_Optimization::update(void) {
    audio_block_t *block;
    block = receiveReadOnly();
    if (!block) return;
    
#if defined(KINETISK)
    switch (state) {
        case 0:
            blocklist[0] = block;
            state = 1;
            break;
        case 1:
            blocklist[1] = block;
            state = 2;
            break;
        case 2:
            blocklist[2] = block;
            state = 3;
            break;
        case 3:
            blocklist[3] = block;
            state = 4;
            break;
        case 4:
            blocklist[4] = block;
            interleave_block_to_buffers( buffer1+0x00, buffer2+0x00, blocklist[0]->data, window+0x00 );
            interleave_block_to_buffers( buffer1+0x40, buffer2+0x40, blocklist[1]->data, window+0x80 );
            state = 5;
            break;
        case 5:
            blocklist[5] = block;
            interleave_block_to_buffers( buffer1+0x80, buffer2+0x80, blocklist[2]->data, window+0x100 );
            interleave_block_to_buffers( buffer1+0xC0, buffer2+0xC0, blocklist[3]->data, window+0x180 );
            state = 6;
            break;
        case 6:
            blocklist[6] = block;
            interleave_block_to_buffers( buffer1+0x100, buffer2+0x100, blocklist[4]->data, window+0x200 );
            interleave_block_to_buffers( buffer1+0x140, buffer2+0x140, blocklist[5]->data, window+0x280 );
            state = 7;
            break;
        case 7:
            blocklist[7] = block;
            interleave_block_to_buffers( buffer1+0x180, buffer2+0x180, blocklist[6]->data, window+0x300 );
            interleave_block_to_buffers( buffer1+0x1C0, buffer2+0x1C0, blocklist[7]->data, window+0x380 );
            // get two ffts which are actually 4 ffts because of how we packed the buffers
            arm_cfft_radix4_q15( &fft_inst, (q15_t*)buffer1 );
            arm_cfft_radix4_q15( &fft_inst, (q15_t*)buffer2 );
            // TODO: need to reuse buffer1 and buffer2
            extract_2_for_1_fft_outputs( ( uint32_t * )buffer1, partialBuffer1, partialBuffer2 );
            extract_2_for_1_fft_outputs( ( uint32_t * )buffer2, partialBuffer3, partialBuffer4 );
            // TODO: use buffer1 and buffer2 instead
            uint32_t *fft_result = combine_ffts( ( uint32_t * )partialBuffer1, ( uint32_t * )partialBuffer2, ( uint32_t * )partialBuffer3, ( uint32_t * )partialBuffer4 );
            // needs to be optimized?
            for ( int k = 0; k < 512; k++ ) {
                int16_t r = ( int16_t )( fft_result[k] & 0xFFFF );
                int16_t i = ( int16_t )( fft_result[k] >> 16 );
                r >>= 2;
                i >>= 2;
                uint32_t magsq = r * r + i * i;
                output[k] = sqrt_uint32_approx( magsq );
            }

            outputflag = true;
            release(blocklist[0]);
            release(blocklist[1]);
            release(blocklist[2]);
            release(blocklist[3]);
            blocklist[0] = blocklist[4];
            blocklist[1] = blocklist[5];
            blocklist[2] = blocklist[6];
            blocklist[3] = blocklist[7];
            state = 4;
            break;
    }
#else
    release(block);
#endif
}



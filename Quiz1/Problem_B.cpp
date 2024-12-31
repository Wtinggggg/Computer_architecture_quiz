# include <stdint.h>
# include <stdio.h>
# include <iostream>
# include <math.h>
# include <bit>
# include <bitset>
// # include <cuda_bf16.h>

// # define bf16_t __nv_bfloat162 

typedef struct {
    uint16_t bits;
} bf16_t;

using namespace std;

static inline uint32_t fp32_to_bits(float f)
{
    union {
        float as_value;
        uint32_t as_bits;
    } fp32 = {.as_value = f};
    return fp32.as_bits;
}

static inline float bf16_to_fp32(bf16_t h)
{
    union {
        float f;
        uint32_t i;
    } u = {.i = (uint32_t)h.bits << 16};
    return u.f;
}

static inline bf16_t fp32_to_bf16(float s)
{
    bf16_t h;
    union {
        float f;
        uint32_t i;
    } u = {.f = s};
    if ((u.i & 0x7fffffff) > 0x7f800000/*B01*/) { /* NaN */
        h.bits = (u.i >> 16) | 64;         /* force to quiet */
        return h;                                         
    }
    h.bits = (u.i + (0x7fff/*B02*/ + ((u.i >> 0x10/*B03*/) & 1))) >> 0x10/*B04*/;
    return h;
}


int main() {
	float f32;
	while(true) {
        cout << "Input a floating-point number(exit : 0): ";
		cin >> f32;
        uint32_t bitsf32 = fp32_to_bits(f32);
		bf16_t bf16 = fp32_to_bf16(f32);
		cout << "FP32 representation: " << bitset<32>(bitsf32) << " Converted to BF16 representation: " 
             << bitset<16>(bf16.bits) << endl;
        if(!f32) break;
	}
	return 0;

}



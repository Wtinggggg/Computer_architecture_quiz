# include <stdint.h>
# include <stdio.h>
# include <iostream>
# include <math.h>
# include <bit>
# include <bitset>
# include <cuda_fp16.h>

# define fp16_t __half

using namespace std;

static inline float bits_to_fp32(uint32_t w)
{
    union {
        uint32_t as_bits;
        float as_value;
    } fp32 = {.as_bits = w};
    return fp32.as_value;
}

static inline uint32_t fp32_to_bits(float f)
{
    union {
        float as_value;
        uint32_t as_bits;
    } fp32 = {.as_value = f};
    return fp32.as_bits;
}

static inline float fp16_to_fp32(fp16_t h)
{
    const uint32_t w = (uint32_t) h << 16;
    const uint32_t sign = w & UINT32_C(0x80000000);
    const uint32_t two_w = w + w;

    const uint32_t exp_offset = UINT32_C(0xE0) << 23;
    const float exp_scale = 0x1.0p-112f;
    const float normalized_value =
        bits_to_fp32((two_w >> 4) + exp_offset) * exp_scale;

    const uint32_t mask = UINT32_C(126) << 23;
    const float magic_bias = 0.5f;
    const float denormalized_value =
        bits_to_fp32((two_w >> 17) | mask) - magic_bias;

    const uint32_t denormalized_cutoff = UINT32_C(1) << 27;
    const uint32_t result =
        sign | (two_w < denormalized_cutoff ? fp32_to_bits(denormalized_value)
                                            : fp32_to_bits(normalized_value));
    return bits_to_fp32(result);
}

static inline fp16_t fp32_to_fp16(float f)
{
    const float scale_to_inf = 0x1.0p+112f;
    const float scale_to_zero = 0x1.0p-110f;
    float base = (fabsf(f) * scale_to_inf) * scale_to_zero;

    const uint32_t w = fp32_to_bits(f);
    const uint32_t shl1_w = w + w;
    const uint32_t sign = w & UINT32_C(0x80000000);
    uint32_t bias = shl1_w & UINT32_C(0xFF000000);
    if (bias < UINT32_C(0x71000000))
        bias = UINT32_C(0x71000000);

    base = bits_to_fp32((bias >> 1) + UINT32_C(0x07800000/*A01*/)) + base;
    const uint32_t bits = fp32_to_bits(base);
    const uint32_t exp_bits = (bits >> 13) & UINT32_C(0x00007C00);
    const uint32_t mantissa_bits = bits & UINT32_C(0x00000FFF/*A02*/);
    const uint32_t nonsign = exp_bits + mantissa_bits;
    return (sign >> 16) |
           (shl1_w > UINT32_C(0xFF000000) ? UINT16_C(0x7E00/*A03*/) : nonsign);
}

int main() {
	float f32;
	while(true) {
        cout << "Input a floating-point number(exit : 0): ";
		cin >> f32;
        uint32_t bitsf32 = fp32_to_bits(f32);
		uint16_t f16 = fp32_to_fp16(f32);
		cout << "FP32 representation: " << bitset<32>(bitsf32) << " Converted to FP16 representation: " 
             << bitset<16>(f16) << endl;
        if(!f32) break;
	}
	return 0;

}

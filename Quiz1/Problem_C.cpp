# include <stdint.h>
# include <stdio.h>
# include <iostream>
# include <math.h>
# include <bit>
# include <bitset>
// # include <cuda_fp16.h>

// # define fp16_t __half

using namespace std;

static inline int my_clz(uint32_t x) {
    int count = 0;
    for (int i = 31; i >= 0; --i) {
        if (x & (1U << i))
            break;
        count++;
    }
    return count;
}

static inline uint32_t fp16_to_fp32(uint16_t h) {
    /*
     * Extends the 16-bit half-precision floating-point number to 32 bits
     * by shifting it to the upper half of a 32-bit word:
     *      +---+-----+------------+-------------------+
     *      | S |EEEEE|MM MMMM MMMM|0000 0000 0000 0000|
     *      +---+-----+------------+-------------------+
     * Bits  31  26-30    16-25            0-15
     *
     * S - sign bit, E - exponent bits, M - mantissa bits, 0 - zero bits.
     */
    const uint32_t w = (uint32_t) h << 16;
    
    /*
     * Isolates the sign bit from the input number, placing it in the most
     * significant bit of a 32-bit word:
     *
     *      +---+----------------------------------+
     *      | S |0000000 00000000 00000000 00000000|
     *      +---+----------------------------------+
     * Bits  31                 0-31
     */
    const uint32_t sign = w & UINT32_C(0x80000000);
    
    /*
     * Extracts the mantissa and exponent from the input number, placing
     * them in bits 0-30 of the 32-bit word:
     *
     *      +---+-----+------------+-------------------+
     *      | 0 |EEEEE|MM MMMM MMMM|0000 0000 0000 0000|
     *      +---+-----+------------+-------------------+
     * Bits  30  27-31     17-26            0-16
     */
    const uint32_t nonsign = w & UINT32_C(0x7FFFFFFF);
    
    /*
     * The renorm_shift variable indicates how many bits the mantissa
     * needs to be shifted to normalize the half-precision number. 
     * For normalized numbers, renorm_shift will be 0. For denormalized
     * numbers, renorm_shift will be greater than 0. Shifting a 
     * denormalized number will move the mantissa into the exponent,
     * normalizing it.
     */
    uint32_t renorm_shift = my_clz(nonsign);
    renorm_shift = renorm_shift > 5 ? renorm_shift - 5 : 0;
    
    /*
     * If the half-precision number has an exponent of 15, adding a 
     * specific value will cause overflow into bit 31, which converts 
     * the upper 9 bits into ones. Thus:
     *   inf_nan_mask ==
     *                   0x7F800000 if the half-precision number is 
     *                   NaN or infinity (exponent of 15)
     *                   0x00000000 otherwise
     */
    const int32_t inf_nan_mask = ((int32_t)(nonsign + 0x04000000) >> 8) &
                                 INT32_C(0x7F800000);
    
    /*
     * If nonsign equals 0, subtracting 1 will cause overflow, setting
     * bit 31 to 1. Otherwise, bit 31 will be 0. Shifting this result
     * propagates bit 31 across all bits in zero_mask. Thus:
     *   zero_mask ==
     *                0xFFFFFFFF if the half-precision number is 
     *                zero (+0.0h or -0.0h)
     *                0x00000000 otherwise
     */
    const int32_t zero_mask = (int32_t)(nonsign - 1) >> 31;
    
    /*
     * 1. Shifts nonsign left by renorm_shift to normalize it (for denormal
     *    inputs).
     * 2. Shifts nonsign right by 3, adjusting the exponent to fit in the
     *    8-bit exponent field and moving the mantissa into the correct
     *    position within the 23-bit mantissa field of the single-precision
     *    format.
     * 3. Adds 0x70 to the exponent to account for the difference in bias
     *    between half-precision and single-precision.
     * 4. Subtracts renorm_shift from the exponent to account for any
     *    renormalization that occurred.
     * 5. ORs with inf_nan_mask to set the exponent to 0xFF if the input
     *    was NaN or infinity.
     * 6. ANDs with the inverted zero_mask to set the mantissa and exponent
     *    to zero if the input was zero.
     * 7. Combines everything with the sign bit of the input number.
     */
    return sign | ((((nonsign << renorm_shift >> 3) +
            ((0x70/*C02*/ - renorm_shift) << 23/*C03*/)) | inf_nan_mask) & ~zero_mask);
}

int main() {
	uint16_t f16;
	while(true) {
        cout << "Inputput a 16-bit hex number (FP16)(exit : 0): ";
		cin >> hex >> f16;
        // uint16_t bitsf16 = fp32_to_bits(f16);
		uint32_t f32 = fp16_to_fp32(f16);
		cout << "FP16 representation: " << bitset<16>(f16) << " Converted to FP32 representation: " 
             << bitset<32>(f32) << endl;
        if(!f16) break;
	}
	return 0;

}
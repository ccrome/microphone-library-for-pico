
#include <stdint.h>
#include <string.h>

#define UNITY_BITS 12
#define APPLY_EQ 1
static inline int32_t UNITY(float x) {
    x *= (1<<UNITY_BITS);
    return (int32_t)(x + 0.5);
}
static inline int32_t saturate(int32_t in, int32_t low, int32_t high)
{
    if (in < low)
        return low;
    if (in > high)
        return high;
    else return in;
}

static inline void run_equalizer(const int16_t *sample_buffer_in, int16_t *sample_buffer, int n_samples, int bypass) {
    if (bypass)
        memcpy(sample_buffer, sample_buffer_in, n_samples*sizeof(*sample_buffer));
    else {
        // This filter is approximately 21 cycles/sample.
        static int32_t z[2] = {0, 0};
        const int32_t coefficients[3] = {UNITY(0.19), UNITY(0.61), UNITY(0.19)}; // <-- run eq.py in this directory to see where these coefficients came from.
        int32_t acc;
        acc =
            coefficients[0] * z[0] +
            coefficients[1] * z[1] +
            coefficients[2] * sample_buffer_in[0];
        sample_buffer[0] = saturate(acc>>UNITY_BITS, -32768, 32767);
        acc =
            coefficients[0] * z[1] +
            coefficients[1] * sample_buffer_in[0] + 
            coefficients[2] * sample_buffer_in[1];
        sample_buffer[1] = saturate(acc>>UNITY_BITS, -32768, 32767);
        for (int i = 2;  i < n_samples; i++) {
            acc =
                coefficients[0] * sample_buffer_in[i-2] + 
                coefficients[1] * sample_buffer_in[i-1] + 
                coefficients[2] * sample_buffer_in[i-0];
            sample_buffer[i] = saturate(acc>>UNITY_BITS, -32768, 32767);
        }
        z[0] = sample_buffer_in[n_samples-2];
        z[1] = sample_buffer_in[n_samples-1];
    }
}

// quick test of the equalizer.  just compile with gcc -g -o test_equalizer -lm test_equalizer.c
#include "equalizer.h"
#include "math.h"
#include <stdio.h>

void zeros(int16_t *buff, int n) {
    memset(buff, 0, sizeof(*buff)*n);
}
void sinewave(int16_t *buff, int n) {
    int16_t amplitude = 32767;
    float fs = 16000;
    float f0 = 100;
    float w = f0 * M_PI * 2 / fs; // cycles/sec * (rad/cycle) / samp/sec = rad/samp

    for (int i = 0; i < n; i++) {
        buff[i] = (int16_t)(sin(w * i) * amplitude + 0.5);
    }
}
void test1()
{
    int n_samples = 10;
    int16_t buffer_in[n_samples];
    zeros(buffer_in, n_samples);
    buffer_in[3] = 1000;
    int16_t buffer_out[n_samples];
    zeros(buffer_out, n_samples);

    run_equalizer(buffer_in, buffer_out, n_samples);
}

void test2()
{
    int n_samples = 1000;
    int16_t buffer_in[n_samples];
    sinewave(buffer_in, n_samples);
    int16_t buffer_out[n_samples];
    zeros(buffer_out, n_samples);

    int blocksize = 10;
    for (int i = 0; i < n_samples/blocksize; i++) {
        run_equalizer(&buffer_in[blocksize*i], &buffer_out[blocksize*i], blocksize);
    }
    FILE *f = fopen("output.txt", "w");
    for (int i = 0; i < n_samples; i++) {
        fprintf(f, "%d, %d, %d\n", i, buffer_in[i], buffer_out[i]);
    }
    fprintf(f, "\n");
    fclose(f);
}

int main(int argc, char *argv[]) {
    test1();
    test2();
    return 0;
}

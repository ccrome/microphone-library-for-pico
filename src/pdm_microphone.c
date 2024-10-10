/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "OpenPDM2PCM/OpenPDMFilter.h"

#include "pdm_microphone.pio.h"

#include "pico/pdm_microphone.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "debug_pulse.h"
#include "commandline.h"

#define FIXED_VOLUME         6.33
#define PDM_DECIMATION       128
#define PDM_RAW_BUFFER_COUNT 2

static struct {
    struct pdm_microphone_config config;
    int dma_channel;
    uint8_t* raw_buffer[PDM_RAW_BUFFER_COUNT];
    volatile int raw_buffer_write_index;
    volatile int raw_buffer_read_index;
    uint raw_buffer_size;
    uint dma_irq;
    TPDMFilter_InitStruct filter;
    pdm_samples_ready_handler_t samples_ready_handler;
    float clk_div;
} pdm_mic;

static void pdm_dma_handler();
float pdm_microphone_get_clkdiv() {
    return pdm_mic.clk_div;
}
int pdm_microphone_init(const struct pdm_microphone_config* config) {
    memset(&pdm_mic, 0x00, sizeof(pdm_mic));
    memcpy(&pdm_mic.config, config, sizeof(pdm_mic.config));

    if (config->sample_buffer_size % (config->sample_rate / 1000)) {
        return -1;
    }

    pdm_mic.raw_buffer_size = config->sample_buffer_size * (PDM_DECIMATION / 8);

    for (int i = 0; i < PDM_RAW_BUFFER_COUNT; i++) {
        pdm_mic.raw_buffer[i] = malloc(pdm_mic.raw_buffer_size);
        if (pdm_mic.raw_buffer[i] == NULL) {
            pdm_microphone_deinit();

            return -1;   
        }
    }

    pdm_mic.dma_channel = dma_claim_unused_channel(true);
    if (pdm_mic.dma_channel < 0) {
        pdm_microphone_deinit();

        return -1;
    }

    uint pio_sm_offset = pio_add_program(config->pio, &pdm_microphone_data_program);

    float clk_div = clock_get_hz(clk_sys) / (config->sample_rate * PDM_DECIMATION * 4.0);
    pdm_mic.clk_div = clk_div;
    pdm_microphone_data_init(
        config->pio,
        config->pio_sm,
        pio_sm_offset,
        clk_div,
        config->gpio_data,
        config->gpio_clk
    );

    dma_channel_config dma_channel_cfg = dma_channel_get_default_config(pdm_mic.dma_channel);

    channel_config_set_transfer_data_size(&dma_channel_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_channel_cfg, false);
    channel_config_set_write_increment(&dma_channel_cfg, true);
    channel_config_set_dreq(&dma_channel_cfg, pio_get_dreq(config->pio, config->pio_sm, false));

    pdm_mic.dma_irq = DMA_IRQ_0;

    dma_channel_configure(
        pdm_mic.dma_channel,
        &dma_channel_cfg,
        pdm_mic.raw_buffer[0],
        &config->pio->rxf[config->pio_sm],
        pdm_mic.raw_buffer_size,
        false
    );

    pdm_mic.filter.Fs = config->sample_rate;
    pdm_mic.filter.LP_HZ = config->sample_rate / 2;
    pdm_mic.filter.HP_HZ = 10;
    pdm_mic.filter.In_MicChannels = 1;
    pdm_mic.filter.Out_MicChannels = 1;
    pdm_mic.filter.Decimation = PDM_DECIMATION;
    pdm_mic.filter.MaxVolume = 64;
    pdm_mic.filter.Gain = 10;
}

void pdm_microphone_set_clkdiv_nudge(uint8_t nudge) {
    int div_int = (int)pdm_mic.clk_div;
    int div_frac = (int)((pdm_mic.clk_div - div_int) * 256+0.5);
    // use the fractional setting, and only nudge it up or down by a bit
    pio_sm_set_clkdiv_int_frac(pdm_mic.config.pio, pdm_mic.config.pio_sm, div_int, div_frac+nudge);
}
void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
 
    commandline_print("pll_sys  = %dkHz", f_pll_sys);
    commandline_print("pll_usb  = %dkHz", f_pll_usb);
    commandline_print("rosc     = %dkHz", f_rosc);
    commandline_print("clk_sys  = %dkHz", f_clk_sys);
    commandline_print("clk_peri = %dkHz", f_clk_peri);
    commandline_print("clk_usb  = %dkHz", f_clk_usb);
    commandline_print("clk_adc  = %dkHz", f_clk_adc);
    commandline_print("clk_rtc  = %dkHz", f_clk_rtc);
}

void pdm_microphone_status() {
    float clk_div = clock_get_hz(clk_sys) / (16000 * PDM_DECIMATION * 4.0);
    #define MAX_PRINTF_MSG_LEN 30
    commandline_print("clk div = %f", clk_div);
}

void pdm_microphone_deinit() {
    for (int i = 0; i < PDM_RAW_BUFFER_COUNT; i++) {
        if (pdm_mic.raw_buffer[i]) {
            free(pdm_mic.raw_buffer[i]);
            pdm_mic.raw_buffer[i] = NULL;
        }
    }

    if (pdm_mic.dma_channel > -1) {
        dma_channel_unclaim(pdm_mic.dma_channel);

        pdm_mic.dma_channel = -1;
    }
}
int pdm_microphone_start() {
    irq_set_enabled(pdm_mic.dma_irq, true);
    irq_set_exclusive_handler(pdm_mic.dma_irq, pdm_dma_handler);

    if (pdm_mic.dma_irq == DMA_IRQ_0) {
        dma_channel_set_irq0_enabled(pdm_mic.dma_channel, true);
    } else if (pdm_mic.dma_irq == DMA_IRQ_1) {
        dma_channel_set_irq1_enabled(pdm_mic.dma_channel, true);
    } else {
        return -1;
    }
    Open_PDM_Filter_Init(&pdm_mic.filter);

    pio_sm_set_enabled(
        pdm_mic.config.pio,
        pdm_mic.config.pio_sm,
        true
    );

    pdm_mic.raw_buffer_write_index = 0;
    pdm_mic.raw_buffer_read_index = 0;

    dma_channel_transfer_to_buffer_now(
        pdm_mic.dma_channel,
        pdm_mic.raw_buffer[0],
        pdm_mic.raw_buffer_size
    );

    pio_sm_set_enabled(
        pdm_mic.config.pio,
        pdm_mic.config.pio_sm,
        true
    );
}

void pdm_microphone_stop() {
    pio_sm_set_enabled(
        pdm_mic.config.pio,
        pdm_mic.config.pio_sm,
        false
    );

    dma_channel_abort(pdm_mic.dma_channel);

    if (pdm_mic.dma_irq == DMA_IRQ_0) {
        dma_channel_set_irq0_enabled(pdm_mic.dma_channel, false);
    } else if (pdm_mic.dma_irq == DMA_IRQ_1) {
        dma_channel_set_irq1_enabled(pdm_mic.dma_channel, false);
    }

    irq_set_enabled(pdm_mic.dma_irq, false);
}

static void pdm_dma_handler() {
    // clear IRQ
    if (pdm_mic.dma_irq == DMA_IRQ_0) {
        dma_hw->ints0 = (1u << pdm_mic.dma_channel);
    } else if (pdm_mic.dma_irq == DMA_IRQ_1) {
        dma_hw->ints1 = (1u << pdm_mic.dma_channel);
    }

    // get the current buffer index
    pdm_mic.raw_buffer_read_index = pdm_mic.raw_buffer_write_index;

    // get the next capture index to send the dma to start
    pdm_mic.raw_buffer_write_index = (pdm_mic.raw_buffer_write_index + 1) % PDM_RAW_BUFFER_COUNT;

    // give the channel a new buffer to write to and re-trigger it
    dma_channel_transfer_to_buffer_now(
        pdm_mic.dma_channel,
        pdm_mic.raw_buffer[pdm_mic.raw_buffer_write_index],
        pdm_mic.raw_buffer_size
    );

    if (pdm_mic.samples_ready_handler) {
        pdm_mic.samples_ready_handler();
    }
}

void pdm_microphone_set_samples_ready_handler(pdm_samples_ready_handler_t handler) {
    pdm_mic.samples_ready_handler = handler;
}

void pdm_microphone_set_filter_max_volume(uint8_t max_volume) {
    pdm_mic.filter.MaxVolume = max_volume;
    commandline_print("volume = %d", max_volume);
}

void pdm_microphone_set_filter_gain(uint8_t gain) {
//    pdm_mic.filter.Gain = gain;
    commandline_print("gain = %d", gain);
}

int pdm_microphone_read(int16_t* buffer, size_t samples) {
    int filter_stride = (pdm_mic.filter.Fs / 1000);
    samples = (samples / filter_stride) * filter_stride;

    if (samples > pdm_mic.config.sample_buffer_size) {
        samples = pdm_mic.config.sample_buffer_size;
    }

    if (pdm_mic.raw_buffer_write_index == pdm_mic.raw_buffer_read_index) {
        return 0;
    }

    uint8_t* in = pdm_mic.raw_buffer[pdm_mic.raw_buffer_read_index];
    int16_t* out = buffer;

    pdm_mic.raw_buffer_read_index++;

    for (int i = 0; i < samples; i += filter_stride) {
#if PDM_DECIMATION == 64
        Open_PDM_Filter_64(in, out, FIXED_VOLUME, &pdm_mic.filter);
#elif PDM_DECIMATION == 128
        Open_PDM_Filter_128(in, out, FIXED_VOLUME, &pdm_mic.filter);
#else
        #error "Unsupported PDM_DECIMATION value!"
#endif

        in += filter_stride * (PDM_DECIMATION / 8);
        out += filter_stride;
    }
    return samples;
}

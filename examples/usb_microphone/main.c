/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * This examples creates a USB Microphone device using the TinyUSB
 * library and captures data from a PDM microphone using a sample
 * rate of 16 kHz, to be sent the to PC.
 * 
 * The USB microphone code is based on the TinyUSB audio_test example.
 * 
 * https://github.com/hathach/tinyusb/tree/master/examples/device/audio_test
 */

#include "pico/pdm_microphone.h"
#include "blink.pio.h"
#include "usb_microphone.h"
#include "hardware/clocks.h"
#include "debug_pulse.h"
#include "usb_sync.h"
#include "equalizer.h"

// configuration
const struct pdm_microphone_config config = {
  .gpio_data = 2,
  .gpio_clk = 3,
  .pio = pio0,
  .pio_sm = 0,
  .sample_rate = SAMPLE_RATE,
  .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};

// variables
uint16_t sample_buffer_in[SAMPLE_BUFFER_SIZE];
uint16_t sample_buffer[SAMPLE_BUFFER_SIZE];

// callback functions
void on_pdm_samples_ready();
void on_usb_microphone_tx_ready();

// start blink program 
void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);
    pio->txf[sm] = clock_get_hz(clk_sys) / (2 * freq);
}

int main(void)
{
  // initialize and start the PDM microphone
  pdm_microphone_init(&config);
  pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
  pdm_microphone_start();

  // initialize the USB microphone interface
  usb_microphone_init();
  usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);

  // enable to make LED blink
#if 0
  // start blink program 
  PIO pio = pio0;
  uint offset = pio_add_program(pio, &blink_program);
  printf("Loaded program at %d\n", offset);
  blink_pin_forever(pio, 1, offset, 25, 1);
#endif 

  while (1) {
    // run the USB microphone task continuously
    usb_microphone_task();
  }

  return 0;
}


uint64_t pdm_samples_ready_us = 0;
uint64_t usb_mic_tx_ready_us = 0;
int64_t usb_mic_delta_us = 0;
void on_pdm_samples_ready()
{
  // Callback from library when all the samples in the library
  // internal sample buffer are ready for reading.
  //
  // Read new samples into local buffer.
    debug_pin_high(0);
    pdm_samples_ready_us = to_us_since_boot(get_absolute_time());
    pdm_microphone_read(sample_buffer_in, SAMPLE_BUFFER_SIZE);
    run_equalizer(sample_buffer_in, sample_buffer, SAMPLE_BUFFER_SIZE, 0);
    debug_pin_low(0);
}

void on_usb_microphone_tx_ready()
{
  // Callback from TinyUSB library when all data is ready
  // to be transmitted.
  //
  // Write local buffer to the USB microphone
  usb_mic_tx_ready_us = to_us_since_boot(get_absolute_time());
  usb_mic_delta_us = usb_mic_tx_ready_us - pdm_samples_ready_us;
  usb_sync_update(usb_mic_delta_us);
  usb_microphone_write(sample_buffer, sizeof(sample_buffer));
}

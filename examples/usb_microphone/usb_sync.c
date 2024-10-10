
#include "usb_sync.h"
#include <string.h>
#include "commandline.h"
#include "pico/pdm_microphone.h"

void usb_sync_update(float buffer_level) {
    // First, do a lowpass filter on buffer level
    int bump = 0;
    if (buffer_level > 600)
        bump = 1;
    if (buffer_level < 400)
        bump = -1;
    pdm_microphone_set_clkdiv_nudge(bump);
    return;    
}


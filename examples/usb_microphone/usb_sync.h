// Synchronize the SAI clock to the USB SOF
#ifndef _USB_SYNC_H_
#define _USB_SYNC_H_
#include "pico.h"
enum usb_sync_speed {
    USB_SYNC_UNUSED = 0,
    USB_SYNC_LOWSPEED,
    USB_SYNC_HIGHSPEED,
    USB_SYNC_LAST
};

typedef struct  {
    uint32_t magic;
    uint32_t target;
    uint32_t current;
    uint32_t averaged;
    uint32_t error;
    uint32_t control_signal;
    uint32_t sai_tx_underflow;
} usb_sync_sai_results_t;

void usb_sync_update(float current);
usb_sync_sai_results_t usb_sync_get_stats(void);
void usb_sync_print_stats();
#endif

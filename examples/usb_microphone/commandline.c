#include "tusb.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "commandline.h"
#include "embedded_cli.h"
#include "../../src/OpenPDM2PCM/OpenPDMFilter.h"
#include "pico/bootrom.h"
#include "pico/unique_id.h"

#define ARRAY_SIZE(p) (sizeof(p)/sizeof(p[0]))


#define BUFFERSIZE 300

typedef struct 
{
    int head;
    int tail;
    char buffer[BUFFERSIZE];
} CommandlineQueue_t;

typedef struct {
    EmbeddedCli *cli;
    CommandlineQueue_t queue;
} Commandline_t;
static Commandline_t cmd;


void q_put(CommandlineQueue_t *q, char c) {
    // put a character if there's space
    int next_tail = (q->tail + 1) % BUFFERSIZE;
    if (next_tail != q->head) {
        q->buffer[q->tail] = c;
        q->tail = next_tail;
    }
}

// return the number of characters read
int q_get(CommandlineQueue_t *q, char *c) {
    int current_head = q->head;
    if (current_head == q->tail) {
        return 0;
    }
    *c = q->buffer[current_head];
    q->head = (current_head+1) % BUFFERSIZE;
    return 1;
}

void commandline_task() {
    char c;
    while (tud_cdc_write_available() && q_get(&cmd.queue, &c))
        tud_cdc_write_char(c);
    tud_cdc_write_flush();
}

void write_char(EmbeddedCli * cli, char c) {
    q_put(&cmd.queue, c);
}

void start_ramp(EmbeddedCli *cli, char *args, void *context) {
    Open_PDM_Filter_Ramp(1);
}
void stop_ramp(EmbeddedCli *cli, char *args, void *context) {
    Open_PDM_Filter_Ramp(0);
}

void status(EmbeddedCli *cli, char *args, void *context) {
    commandline_print("Zipline Audio");
    char str_ptr[31];
    pico_get_unique_board_id_string(str_ptr, 30);
    str_ptr[30] = 0;
    commandline_print("Version: %s", APPLICATION_VERSION);
    commandline_print("Serial Number: %s", str_ptr);
    commandline_print("Hardware Revision: 1");
    //Open_PDM_Filter_Print();
}

void bootloader_mode(EmbeddedCli *cli, char *args, void *context) {
    commandline_print("Etnering the bootloader");
    reset_usb_boot(0, 0);
}

static const CliCommandBinding bindings[] = {
        {
            "start-ramp",
            "start sending a ramp",
            false, NULL, start_ramp
        },
        {
            "stop-ramp", "stop sending a ramp",
            false, NULL, stop_ramp
        },
        {
            "status", "show some status",
            false, NULL, status
        },
        {
            "bootloader", "reboot into bootloader mode for updating",
            false, NULL, bootloader_mode
        },
};

void commandline_init() {
    memset(&cmd, 0, sizeof(cmd));
    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->maxBindingCount = ARRAY_SIZE(bindings);
    EmbeddedCli *cli = embeddedCliNew(config);
    cli->writeChar = write_char;
    cmd.cli = cli;
    for (int binding = 0; binding < ARRAY_SIZE(bindings); binding++) 
        embeddedCliAddBinding(cli, bindings[binding]);
}

void commandline_print_str(char *str)
{
    embeddedCliPrint(cmd.cli, str);
}

void cdc_task(void)
{
  uint8_t itf;
  static int initialized = 0;
  if (!initialized) {
      initialized = 1;
      commandline_init();
  }
  
  for (itf = 0; itf < CFG_TUD_CDC; itf++)
  {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_n_connected(itf) )
    {
      if ( tud_cdc_n_available(itf) )
      {
        uint8_t buf[64];

        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
        for (int i = 0; i < count; i++) {
            embeddedCliReceiveChar(cmd.cli, buf[i]);
        }
        embeddedCliProcess(cmd.cli);
      }
    }
  }
  commandline_task();
}

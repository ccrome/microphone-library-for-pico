#ifndef _COMMANDLINE_H_
#define _COMMANDLINE_H_
#include <stdio.h>
void cdc_task(void);

#define MAX_CMDLINE_PRINTF_MSG_LEN 30

#define commandline_print(...) \
{ \
    char msg[MAX_CMDLINE_PRINTF_MSG_LEN]; \
    snprintf(msg, MAX_CMDLINE_PRINTF_MSG_LEN-1, __VA_ARGS__);     \
    msg[MAX_CMDLINE_PRINTF_MSG_LEN-1] = 0; \
    commandline_print_str(msg); \
}

void commandline_print_str(char *str);

#endif

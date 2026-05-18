#ifndef SHELL_LIST_H
#define SHELL_LIST_H

#include "stdint.h"

extern volatile uint8_t g_xModemCommEnable;

void dbg(uint8_t cmdSource, uint32_t instance, unsigned int  argc, unsigned char **argv);
int xbin_boot(uint8_t cmdSource, uint32_t instance, unsigned int  argc, unsigned char **argv);
void software_reset(void);

#endif 
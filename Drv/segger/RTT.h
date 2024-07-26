#ifndef __RTT_H_
#define __RTT_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "SEGGER_RTT.h"

void rtt_init(void);
void rtt_printf(const char *fmt, ...);

#endif
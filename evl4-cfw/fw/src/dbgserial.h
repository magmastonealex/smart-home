#pragma once

// dbgserial is a quick n' dirty debug implementation,
// which when enabled with -DDEBUG will support printf invocations.
// A DBGprintf macro is also defined, which will remove printf calls
// in non-debug builds, significantly reducing code size.

#include <stdio.h>
#include <avr/pgmspace.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG==1
#define DBGprintf(fmt, ...) do { printf_P(PSTR(fmt), ##__VA_ARGS__); } while (0)
#else
#define DBGprintf(fmt, ...) do {} while(0)
#endif
void init_serial();

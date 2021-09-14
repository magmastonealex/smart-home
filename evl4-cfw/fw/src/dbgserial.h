#pragma once

// dbgserial is a quick n' dirty debug implementation,
// which when enabled with -DDEBUG will support printf invocations.
// A DBGprintf macro is also defined, which will remove printf calls
// in non-debug builds, significantly reducing code size.

#include <stdio.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#define DBGprintf(fmt, ...) do { if(DEBUG){ printf(fmt, ##__VA_ARGS__);} } while (0)

void init_serial();

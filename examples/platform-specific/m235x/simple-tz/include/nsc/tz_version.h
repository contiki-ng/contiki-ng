#pragma once

#include "NuMicro.h"

#define TZ_VERSION 0x0100

__NONSECURE_ENTRY
unsigned int tz_version(char *verstr, int l);

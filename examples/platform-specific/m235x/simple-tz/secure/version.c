#include <stdio.h>
#include "nsc/tz_version.h"

__NONSECURE_ENTRY
unsigned int tz_version(char *verstr, int l)
{
    if (verstr && l > 0) {
        snprintf(verstr, l, CONTIKI_VERSION_STRING);
    }
    return TZ_VERSION;
}

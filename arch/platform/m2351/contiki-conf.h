#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#include <stdint.h>
#include <inttypes.h>

/*---------------------------------------------------------------------------*/
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "m2351-def.h"
/*---------------------------------------------------------------------------*/
/* Include CPU-related configuration */
#include "m2351-conf.h"

#include "board.h"
#endif  // end of CONTIKI_CONF_H


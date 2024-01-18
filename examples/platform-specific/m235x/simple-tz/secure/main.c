#include <arm_cmse.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "board.h"

#include "partition_M2354.h"
#define NONSECURE_START_ADDRESS (FMC_NON_SECURE_BASE + 0x1000)

static const char banner[] = \
"\r\n" \
"Secure is running\r\n";

typedef __attribute__( ( cmse_nonsecure_call ) ) void ( *NonSecureResetHandler_t )( uint32_t );

/**
 * @brief Boots into the non-secure code.
 *
 * @param[in] ulNonSecureStartAddress Start address of the non-secure application.
 */
static void prvBootNonSecure( uint32_t ulNonSecureStartAddress );

int main(void)
{
	platform_init_stage_secure();

	puts(banner);

	prvBootNonSecure( NONSECURE_START_ADDRESS );

	while(1);

	return 0;
}

static void prvBootNonSecure( uint32_t ulNonSecureStartAddress )
{
	NonSecureResetHandler_t pxNonSecureResetHandler;

	/* Setup the non-secure vector table. */
	SCB_NS->VTOR = ulNonSecureStartAddress;

	/* Main Stack Pointer value for the non-secure side is the first entry in
	 * the non-secure vector table. Read the first entry and assign the same to
	 * the non-secure main stack pointer(MSP_NS). */
	__TZ_set_MSP_NS( *( ( uint32_t * )( ulNonSecureStartAddress ) ) );

	/* Reset Handler for the non-secure side is the second entry in the
	 * non-secure vector table. Read the second entry to get the non-secure
	 * Reset Handler. */
	pxNonSecureResetHandler = ( NonSecureResetHandler_t )( * ( ( uint32_t * ) ( ( ulNonSecureStartAddress ) + 4U ) ) );

	printf("Jump to non-secure world...\n");

	/* Start non-secure state software application by jumping to the non-secure
	 * Reset Handler. */
	pxNonSecureResetHandler(0);
}


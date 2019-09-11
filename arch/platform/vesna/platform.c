#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "vsn.h"
#include "vsncrc32.h"
#include "vsnpm.h"
#include "vsnsetup.h"
#include "vsnusart.h"
#include "vsntime.h"

#include "contiki.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include "net/netstack.h"
#include "net/mac/framer/frame802154.h"
#include "net/linkaddr.h"
#include "lib/crc16.h"

#include "vesna-conf.h"

#include "sys/platform.h"
#include "sys/node-id.h"

#include "sys/log.h"
#define LOG_MODULE "Platform"
#define LOG_LEVEL (LOG_LEVEL_MAIN)


void
platform_init_stage_one(void)
{
	// justinc, TOP_PAD is incorrect when first calling newlib malloc.
    mallopt(-2, 0); // #define M_TOP_PAD           -2

    // Reset clock configuration
    SystemInit();

    // Enable power management (enable/disable features/pins/hw)
    vsnPM_init();

    // VESNA uses internal clock generator, that's why is limited to 64MHz.
    // For up to 72MHz it would require external oscilator/clock generator.
    // VESNA SNC doesn't have own one. 64MHz is the max on HSI oscilator.

    vsnSetup_intClk(SNC_CLOCK_64MHZ);

    // - Enable GPIO clocks
    // - Enable interrupts
    // - Enable RTC, Tick,
    vsnSetup_initSnc();

    vsnSetup_calibHsi();

    vsnPM_mesureAdcBitVolt();
    vsnCRC32_init();
}


void
platform_init_stage_two(void)
{
#if UART1_ENABLED
	uart1_init(BAUD2UBR(UART1_BAUDRATE));
	LOG_INFO("UART1 enabled\n");

#if SLIP_ENABLED
	slip_arch_init();
	LOG_INFO("UART1 as SLIP interface enabled\n");
#else
	uart1_set_input(serial_line_input_byte);
	process_start(&uart1_rx_process, NULL);
	serial_line_init();
	LOG_INFO("UART1 as serial input enabled\n");
#endif // SLIP_ENABLED

#endif // UART1_ENABLED

	node_id_init(); // node_id_init is triggered twice. Here and between stage 2 & 3.

	// We define unique 64-bit address to represent this device.
	const uint8_t extAddr[8] = { 0, 0x12, 0x4B, 0, 0, 0x06, (node_id & 0xFF), (node_id >> 8) };

	// populate linkaddr_node_addr. Maintain endianess;
	// This sets MAC address
	memcpy(linkaddr_node_addr.u8, extAddr + 8 - LINKADDR_SIZE, LINKADDR_SIZE);
}


void
platform_init_stage_three(void)
{
	NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
	NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, node_id);
	NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, IEEE802154_DEFAULT_CHANNEL);
	NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, linkaddr_node_addr.u8, LINKADDR_SIZE);

	#if AT86RF2XX_BOARD_SNR
	{
		// Go for external output when CLKM is properly configured
		int status = vsnSetup_extClk();
		if (status == SUCCESS) LOG_INFO("Switching to external clock source\n");
	}
	#endif
}


void
platform_idle()
{
	// lpm_enter();
}

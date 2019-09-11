#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"

#include "contiki.h"
#include "sys/int-master.h"
#include <stdbool.h>


void
int_master_enable(void)
{
  __enable_irq();
}


int_master_status_t
int_master_read_and_disable(void)
{
  int_master_status_t primask = __get_PRIMASK();

  __disable_irq();

  return primask;
}


void
int_master_status_set(int_master_status_t status)
{
  __set_PRIMASK(status);
}


bool
int_master_is_enabled(void)
{
  return __get_PRIMASK() ? false : true;
}
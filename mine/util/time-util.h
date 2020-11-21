/*
 * time-util.h
 *
 *  Created on: 2020Äê11ÔÂ13ÈÕ
 *      Author: aliving
 */

#ifndef CONTIKI_NG_MINE_UTIL_TIME_UTIL_H_
#define CONTIKI_NG_MINE_UTIL_TIME_UTIL_H_

#include "contiki.h"
#include <time.h>
#include <stdarg.h>

#define HCX_LONG_TIME 4294967295

void hcx_set_time(struct timer *t){
    timer_set(t,HCX_LONG_TIME);
}
clock_time_t hcx_get_time(struct timer *t){
    clock_time_t temp_stamp  = HCX_LONG_TIME - timer_remaining(t);
    printf("long = %lu \n",temp_stamp);
    return temp_stamp;
}

#endif /* CONTIKI_NG_MINE_UTIL_TIME_UTIL_H_ */

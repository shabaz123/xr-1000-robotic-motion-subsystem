#ifndef __TIMER_HEADER_FILE__
#define __TIMER_HEADER_FILE__

#include "timer.h"
#include "pico/stdlib.h"
#include "femtocli.h"


// timer period, both must be the same, but expressed in msec and usec
#define ALARM_MSEC_PERIOD 20
#define ALARM_USEC_PERIOD 20000

// timer
//void alarm_in_us(uint32_t delay_us); // used to start the timer
//void alarm_in_us_arm(uint32_t delay_us); // used to rearm the timer

#endif // __TIMER_HEADER_FILE__

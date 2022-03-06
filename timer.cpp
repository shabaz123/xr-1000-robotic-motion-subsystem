/*******************************************************
 * timer code
 * rev 1.0 shabaz march 2021
 * 
 * *****************************************************/

// ************ includes ****************************
#include "timer.h"
//#include "hardware/timer.h"
//#include "hardware/irq.h"
#include "femtocli.h"

#ifdef JUNK
// timer
void alarm_in_us_arm(uint32_t delay_us) {
    uint64_t target = timer_hw->timerawl + delay_us;
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
}

static void alarm_irq(void) {
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    //alarm_in_us_arm(alarmPeriod);
    pcui_callback(); // check for any typed characters
    //ledval = !ledval;
    //digitalWrite(7, ledval);
}

void alarm_in_us(uint32_t delay_us) {
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    irq_set_enabled(ALARM_IRQ, true);
    alarm_in_us_arm(delay_us);
}
#endif

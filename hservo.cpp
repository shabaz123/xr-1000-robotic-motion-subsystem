/***********************************
 * hservo.cpp
 * hobby servo controller
 * rev 1 - shabaz - march 2022
 ***********************************/
 
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hservo.h"
#include "hardware/clocks.h"
#include <cstdlib>



HServo::HServo(uint ionum, int initang, int maxang, uint psaveio) {
    mPsaveio = psaveio;
    mMaxang = maxang;
    mCurang = initang;
    mMinpwm = 1000; // 1000 usec = 1 msec
    mMaxpwm = 2000; // 2000 usec = 2 msec
    mMinSleep = 300; // msec time for servo movement
    mMaxSleep = 1000; // msec time for servo movement
    mSleepPerdeg = (mMaxSleep - mMinSleep) / mMaxang; 
    mPerdeg = (mMaxpwm - mMinpwm) / mMaxang;
    if (mPsaveio) {
        gpio_init(mPsaveio);
        gpio_set_dir(mPsaveio, GPIO_OUT);
        
    }
    gpio_set_function(ionum, GPIO_FUNC_PWM);
    mPwmslice = pwm_gpio_to_slice_num(ionum); // get slice number (0-7)
    mPwmchan = pwm_gpio_to_channel(ionum); // get channel number (0-1)
    pwm_set_wrap(mPwmslice, 20000);
    pwm_set_chan_level(mPwmslice, mPwmchan, ang2width(mCurang));
    pwm_set_clkdiv(mPwmslice, (clock_get_hz(clk_sys) / 1E6));

    pwm_set_enabled(mPwmslice, true);
    if (mPsaveio) {
        gpio_put(mPsaveio, 1); // turn on the servo power
    }
    sleep_ms(mMaxSleep + mMinSleep);
    if (mPsaveio) {
        gpio_put(mPsaveio, 0); // turn off the servo power
    }
}

int HServo::ang2width(int ang) {
    return(((180-ang) * mPerdeg) + mMinpwm);
}

void HServo::setAng(int ang) {
    // just return if the user requests zero or very small angle differences
    if (abs(mCurang - ang) < 5)
        return;
    pwm_set_chan_level(mPwmslice, mPwmchan, ang2width(ang));
    if (mPsaveio) {
        gpio_put(mPsaveio, 1); // turn on the servo power
    }
    sleep_ms(mMinSleep + (mSleepPerdeg * abs(ang - mCurang)));
    if (mPsaveio) {
        gpio_put(mPsaveio, 0); // turn off the servo power
    }
    mCurang = ang;
}



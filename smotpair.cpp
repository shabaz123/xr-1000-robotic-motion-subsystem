/******************************************************
 * smotpair.cpp
 * Dual Stepper Motors (Pair) Controller
 * rev 1 - march 2022 - shabaz
 * ****************************************************/

#include "smotpair.h"
#include "stdio.h"
#include <cstdlib>



SMotPair::SMotPair(uint16_t chan1, uint16_t chan2, uint16_t numsteps, int psave) {
    char i;
    uint16_t chans[2];
    chans[0] = chan1;
    chans[1] = chan2;
    this->steps360 = numsteps;
    for (i=0; i<2; i++) {
        switch (chans[i]) {
            case 1:
                this->pin1[i] = 7;
                this->pin2[i] = 6;
                this->pin3[i] = 2;
                this->pin4[i] = 3;
                break;
            case 2:
                this->pin1[i] = 11;
                this->pin2[i] = 10;
                this->pin3[i] = 8;
                this->pin4[i] = 9;
                break;
            case 3:
                this->pin1[i] = 15;
                this->pin2[i] = 14;
                this->pin3[i] = 12;
                this->pin4[i] = 13;
                break;
            case 4:
                this->pin1[i] = 16;
                this->pin2[i] = 17;
                this->pin3[i] = 19;
                this->pin4[i] = 18;
                break;
            default:
                printf("error, invalid chan!\n");
                break;
        }
    }

    this->stepcount[0] = 0;
    this->stepcount[1] = 0;
    this->dir[0] = 0;
    this->dir[1] = 0;
    this->last_step_us_time = 0;
    this->delay = 60L * 1000L * 1000L / this->steps360 / 100; // default speed is 100
    //this->delay = this->delay / 2; // the motion is staggered for the two motors, so halve delay
    this->powersave = psave;
    for(i = 0; i<2; i++) {
        gpio_init(this->pin1[i]);
        gpio_init(this->pin2[i]);
        gpio_init(this->pin3[i]);
        gpio_init(this->pin4[i]);
        gpio_set_dir(this->pin1[i], GPIO_OUT);
        gpio_set_dir(this->pin2[i], GPIO_OUT);
        gpio_set_dir(this->pin3[i], GPIO_OUT);
        gpio_set_dir(this->pin4[i], GPIO_OUT);
    }
}

void SMotPair::speed(long speed) {
    this->delay = 60L * 1000L * 1000L / this->steps360 / speed;
    this->delay = this->delay / 2; // divide by 2 since we want to stagger two motors
}

void SMotPair::step(int n, int direction) {
    int i;
    int current_chan=0;
    int steps_left = n;
    switch(direction) {
        case 0: // fwd
            this->dir[0] = 0; // CCW
            this->dir[1] = 1; // CW
            break;
        case 1: // rev
            this->dir[0] = 1; // CW
            this->dir[1] = 0; // CCW
            break;
        case 2: // left
            this->dir[0] = 1; // CW
            this->dir[1] = 1; // CW
            break;
        case 3: // right
            this->dir[0] = 0; // CCW 
            this->dir[1] = 0; // CCW
            break;
    }

    while ((steps_left > 0) && (current_chan<2)) {
        uint64_t now = to_us_since_boot(get_absolute_time());
        if (now - this->last_step_us_time >= this->delay) {
            this->last_step_us_time = now;
            if (this->dir[current_chan] == 1) {
                this->stepcount[current_chan]++;
                if (this->stepcount[current_chan] == this->steps360) {
                    this->stepcount[current_chan] = 0;
                }
            } else {
                if (this->stepcount[current_chan] == 0) {
                    this->stepcount[current_chan] = this->steps360;
                }

                this->stepcount[current_chan]--;
            }
            stepMotor(current_chan, this->stepcount[current_chan] % 4);
            if (current_chan == 1) {
                steps_left--;
            }
            current_chan++;
            if ((steps_left > 0) && (current_chan>1)) {
                current_chan = 0;
            }
        } else {
            // continue loop and wait for the delay to complete
        }
        // loop back until all steps are complete
    }
    if (this->powersave) { // shut down motor if we are power-saving
        for (i=0; i<2; i++) {
            gpio_put(this->pin1[i], 0);
            gpio_put(this->pin2[i], 0);
            gpio_put(this->pin3[i], 0);
            gpio_put(this->pin4[i], 0);
        }
    }
}

void SMotPair::stepMotor(int chan, int step) {
    switch (step) {
        case 0:  // 1010
            gpio_put(this->pin1[chan], 1);
            gpio_put(this->pin2[chan], 0);
            gpio_put(this->pin3[chan], 1);
            gpio_put(this->pin4[chan], 0);
        break;

        case 1:  // 0110
            gpio_put(this->pin1[chan], 0);
            gpio_put(this->pin2[chan], 1);
            gpio_put(this->pin3[chan], 1);
            gpio_put(this->pin4[chan], 0);
        break;

        case 2:  // 0101
            gpio_put(this->pin1[chan], 0);
            gpio_put(this->pin2[chan], 1);
            gpio_put(this->pin3[chan], 0);
            gpio_put(this->pin4[chan], 1);
        break;

        case 3:  // 1001
            gpio_put(this->pin1[chan], 1);
            gpio_put(this->pin2[chan], 0);
            gpio_put(this->pin3[chan], 0);
            gpio_put(this->pin4[chan], 1);
            break;
    }
}

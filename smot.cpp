/******************************************************
 * smot.cpp
 * Stepper Motor Controller
 * rev 1 - march 2022 - shabaz
 * ****************************************************/

#include "smot.h"
#include "stdio.h"
#include <cstdlib>

SMot::SMot(uint16_t chan, uint16_t numsteps, int psave) {
    this->steps360 = numsteps;
    switch (chan) {
        case 1:
            this->pin1 = 7;
            this->pin2 = 6;
            this->pin3 = 2;
            this->pin4 = 3;
            break;
        case 2:
            this->pin1 = 11;
            this->pin2 = 10;
            this->pin3 = 8;
            this->pin4 = 9;
            break;
        case 3:
            this->pin1 = 15;
            this->pin2 = 14;
            this->pin3 = 12;
            this->pin4 = 13;
            break;
        case 4:
            this->pin1 = 16;
            this->pin2 = 17;
            this->pin3 = 19;
            this->pin4 = 18;
            break;
        default:
            printf("error, invalid chan!\n");
            break;
    }

    this->stepcount = 0;
    this->dir = 0;
    this->last_step_us_time = 0;
    this->delay = 60L * 1000L * 1000L / this->steps360 / 50; // default speed is 50
    this->powersave = psave;
    gpio_init(this->pin1);
    gpio_init(this->pin2);
    gpio_init(this->pin3);
    gpio_init(this->pin4);
    gpio_set_dir(this->pin1, GPIO_OUT);
    gpio_set_dir(this->pin2, GPIO_OUT);
    gpio_set_dir(this->pin3, GPIO_OUT);
    gpio_set_dir(this->pin4, GPIO_OUT);
}

void SMot::speed(long speed) {
    this->delay = 60L * 1000L * 1000L / this->steps360 / speed;
}

void SMot::step(int n, int direction) {
    int steps_left = n;
    this->dir = direction;

    while (steps_left > 0) {
        uint64_t now = to_us_since_boot(get_absolute_time());
        if (now - this->last_step_us_time >= this->delay) {
            this->last_step_us_time = now;
            if (this->dir == 1) {
                this->stepcount++;
                if (this->stepcount == this->steps360) {
                    this->stepcount = 0;
                }
            } else {
                if (this->stepcount == 0) {
                    this->stepcount = this->steps360;
                }

                this->stepcount--;
            }
            stepMotor(this->stepcount % 4);
            steps_left--;
        } else {
            // continue loop and wait for the delay to complete
        }
        // loop back until all steps are complete
    }
    if (this->powersave) { // shut down motor if we are power-saving
        gpio_put(this->pin1, 0);
        gpio_put(this->pin2, 0);
        gpio_put(this->pin3, 0);
        gpio_put(this->pin4, 0);
    }
}

void SMot::stepMotor(int step) {
    switch (step) {
        case 0:  // 1010
            gpio_put(this->pin1, 1);
            gpio_put(this->pin2, 0);
            gpio_put(this->pin3, 1);
            gpio_put(this->pin4, 0);
        break;

        case 1:  // 0110
            gpio_put(this->pin1, 0);
            gpio_put(this->pin2, 1);
            gpio_put(this->pin3, 1);
            gpio_put(this->pin4, 0);
        break;

        case 2:  // 0101
            gpio_put(this->pin1, 0);
            gpio_put(this->pin2, 1);
            gpio_put(this->pin3, 0);
            gpio_put(this->pin4, 1);
        break;

        case 3:  // 1001
            gpio_put(this->pin1, 1);
            gpio_put(this->pin2, 0);
            gpio_put(this->pin3, 0);
            gpio_put(this->pin4, 1);
            break;
    }
}

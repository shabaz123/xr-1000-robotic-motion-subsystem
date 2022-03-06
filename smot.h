#ifndef __SMOT_H_FILE__
#define __SMOT_H_FILE__

#include "pico/stdlib.h"

class SMot {
    public:
        // SMot constructor
        // parameters: chan is 1-4 (up to 4 stepper motors are supported)
        //             steps is number of steps necessary for 360 degrees revolution,
        //                 it is dependant on the motor and any gearing attached.
        //             psave determines if the motor current is switched off after motion
        //                 (defaults to 1, i.e. save power)
        SMot (uint16_t chan, uint16_t numsteps, int psave = 1);
        // Set speed; larger number is faster.
        void speed(long speed);
        // Move motor by n steps (direction is 0 or 1)
        void step(int n, int direction);

    private:
        void stepMotor(int step);
        int dir;
        unsigned long delay;
        int steps360; // number of steps for 360 degree revolution
        int stepcount;
        int pin1;
        int pin2;
        int pin3;
        int pin4;
        int powersave;

        unsigned long last_step_us_time;
};

#endif // __SMOT_H_FILE__

#ifndef __SMOTPAIR_H_FILE__
#define __SMOTPAIR_H_FILE__

#include "pico/stdlib.h"

#define PAIR_FWD 1
#define PAIR_REV 0
#define PAIR_LEFT 2
#define PAIR_RIGHT 3

class SMotPair {
    public:
        // SMot constructor
        // parameters: chan1 and chan2 are 1-4 (up to 4 stepper motors are supported)
        //             steps is number of steps necessary for 360 degrees revolution,
        //                 it is dependant on the motor and any gearing attached.
        //             psave determines if the motor current is switched off after motion
        //                 (defaults to 1, i.e. save power)
        SMotPair (uint16_t chan1, uint16_t chan2, uint16_t numsteps, int psave = 1);
        // Set speed; larger number is faster.
        void speed(long speed);
        // Move motor by n steps (direction is 0,1,2,3 (0=fwd, 1=rev, 2=left, 3=right))
        // The first motor in the pair rotates CCW, and the second motor rotates CW, when viewed from the shaft end,
        // therefore the first motor in the pair should be attached to the left side of the robot chassis, when viewed
        // from the rear of the robot.
        void step(int n, int direction);

    private:
        void stepMotor(int chan, int step);
        int dir[2];
        unsigned long delay;
        int steps360; // number of steps for 360 degree revolution
        int stepcount[2];
        int pin1[2];
        int pin2[2];
        int pin3[2];
        int pin4[2];
        int powersave;

        unsigned long last_step_us_time;
};

#endif // __SMOTPAIR_H_FILE__

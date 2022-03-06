#ifndef __HSERVO_HEADER_FILE__
#define __HSERVO_HEADER_FILE__

#include "pico/stdlib.h"

// pen down and pen up angles
#define PD_ANG 50.0
#define PU_ANG 100.0

class HServo {
    public:
        // HServo constructor
        HServo(uint ionum, int initang=0, int maxang=180, uint psaveio=0);
        // Set hobby servo angle in degrees
        void setAng(int ang);

    private:
        uint mPwmchan;
        uint mPwmslice;
        int mMaxang;
        int mCurang;
        int mPerdeg;
        int mMinSleep; // msec time for servo to respond
        int mMaxSleep; // msec time for servo to complete max angle rotation
        int mSleepPerdeg; // msec time for servo to move by 1 degree
        int mMinpwm; // e.g. 1000 usec (1 msec)
        int mMaxpwm; // e.g. 2000 usec (2 msec)
        uint mPsaveio; // gpio number for servo power enable. If zero, power save is disabled.

        int ang2width(int ang);
};


#endif // __HSERVO_HEADER_FILE__

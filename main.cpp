/*******************************************************
 * motion_controller main code
 * rev 1.0 shabaz march 2021
 * 
 * *****************************************************/

// ************ includes ****************************
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <cstdlib>
#include "stdio.h"
#include "pico/time.h"
#include "smot.h"
#include "smotpair.h"
#include "femtocli.h"
#include "timer.h"
#include "hservo.h"

// *********** function prototypes ****************

//************ defines ****************************
#define DRV_ENA_PIN 22
#define BUTTON_PIN 27
#define HSERVO_POWER_PIN 20
#define HSERVO_CONTROL_PIN 21
#define EXT_PIN 26
#define PICO_LED_ON gpio_put(PICO_DEFAULT_LED_PIN, 1)
#define PICO_LED_OFF gpio_put(PICO_DEFAULT_LED_PIN, 0)
#define BUTTON_PRESSED (gpio_get(BUTTON_PIN) == 0)
#define BUTTON_RELEASED (gpio_get(BUTTON_PIN) != 0)
#define EXT_PWR_ON gpio_put(EXT_PIN, 1)
#define EXT_PWR_OFF gpio_put(EXT_PIN, 0)

// number of motor steps for 360 degree revolution of the output wheel or shaft
#define WHEELSTEPS360 1000
// number of motor steps to rotate the robot by 1 degree.
// use this formula as a baseline and then tweak the value as required:
// WHEELSTEPSDEGREE = (wheel_separation/wheel_diameter) * (WHEELSTEPS360/ 360)
// example: wheel_separation = 86 mm, wheel_diameter = 28 mm, WHEELSTEPS360 = 1000, then result is 8.532
#define WHEELSTEPSDEGREE 8.532

#define BAUD 115200

#define PAIR_FWD 1
#define PAIR_REV 0
#define PAIR_LEFT 2
#define PAIR_RIGHT 3

#define RESP_PROCESSING "PR\n\r"
#define RESP_OK "OK\n\r"

//************ global vars ***********************
char usb_control = 0; // determines if the USB serial is used to control the board or not
SMotPair Wheels(1, 2, WHEELSTEPS360, 1); // drivers 1 and 2 are connected to wheels, 1000 steps per 360 degree revolution, power save mode enabled
SMot Motor3(3, 1000, 1); // driver #3, 1000 steps per 360 deg revolution, powersave on
SMot Motor4(4, 1000, 1);// driver #4, 1000 steps per 360 deg revolution, powersave on
// user interface related params
char uiparam_wheelsaction=0;
char uiparam_motoraction=0;
char uiparam_extaction=0;
double uiparam_valueparam=0.0;
char modechange=0;
char uiparam_adminmode=0;
char uiparam_m2mmode=0;
char uiparam_doaction=ACTION_IDLE;
char uiparam_modifier=MODIFIER_ON;
// repeating timer
uint32_t alarmPeriod;
alarm_pool_t* alarm_pool;
alarm_id_t ui_alarm_id;
// hobby servo
// set initial angle to 0 deg, and max angle to 180 deg, and enable power-saving capability
HServo Servo(HSERVO_CONTROL_PIN, 0, 180, HSERVO_POWER_PIN);

const char* const preset_program1[]={   "fwd 2k",
                                        "right 120",
                                        "fwd 2k",
                                        "right 120",
                                        "fwd 2k",
                                        "right 120",
                                        ""};


//************** extern ****************************
extern char menulevel;

//*********** function prototypes ******************
int init(void); // initialize GPIO, detect if USB is connected
void rotate_wheels(char sub_action_type, double value); // rotate a pair of wheels
void move_servo(int ang); // move servo to ang value
void rotate_motor(char sub_action_type, double value); // rotate motor M3 or M4
void ext_pwr(char subaction); // control external power pin
void run_program(void); // run a preset program
void handle_requests(void); // action requests from the various interfaces

//************** main function *********************
int
main (void)
{
    int sstate = 0;
    int intparam;
    init();
    PICO_LED_ON;

    while(1) {
        sleep_ms(100); // give pico some free time
        handle_requests(); // check if a request is pending from any interface, and handle it
        // check if the user wants to run a program by pressing the operator button:
        if (BUTTON_PRESSED) {
            while(1) {
                sleep_ms(100);
                if (BUTTON_RELEASED)
                    break;
            }
            sleep_ms(1000); // give the user time to move away from the robot : )
            run_program();
        }

    }

}

//*************** other functions ***********************

int init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_init(DRV_ENA_PIN);
    gpio_set_dir(DRV_ENA_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    gpio_init(EXT_PIN);
    gpio_set_dir(EXT_PIN, GPIO_OUT);
    gpio_put(DRV_ENA_PIN, 1); // turn on the motor driver modules
    stdio_init_all();
    menu_init();
    uart_init(uart0, BAUD);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    alarm_pool = alarm_pool_create(2, 16); // create an alarm pool
    irq_set_priority(TIMER_IRQ_2, 0xc0); // larger number is lower priority

    sleep_ms(100);

    if (BUTTON_PRESSED) { // is the Operator button pressed on the board at powerup?
        sleep_ms(2000); // time for USB to be detected by PC
        if (stdio_usb_connected()) {
            // USB mode
            usb_control = 1;
            printf("Motor Subsystem is under USB control\n");
            printf("$ ");
            //add_alarm_in_ms(ALARM_MSEC_PERIOD, pcui_callback, NULL, false);
            ui_alarm_id = alarm_pool_add_alarm_in_ms(alarm_pool, ALARM_MSEC_PERIOD, pcui_callback, NULL, false);
        }
        while(1) {
            if (BUTTON_RELEASED)
                break;
            sleep_ms(100);
        }
        sleep_ms(100); // long debounce period
    } else {
        // operator button is not pressed. Go to UART+M2M mode
        usb_control = 0;
        set_menu(MENU_M2M);
        m2m_response((char *)RESP_OK);
        //add_alarm_in_ms(ALARM_MSEC_PERIOD, pcui_callback, NULL, false);
        ui_alarm_id = alarm_pool_add_alarm_in_ms(alarm_pool, ALARM_MSEC_PERIOD, pcui_callback, NULL, false);
    }

    return(0);
}

// rotate_wheels: wheels action, move robot fwd/back/left/right by specified amount value
// sub_action_type: 0-3 (0=fwd, 1=rev, 2=left, 3=right)
// value: number of motor steps for fwd or reverse, or angle in degrees for left/right rotation
void rotate_wheels(char sub_action_type, double value) {
    int value_int;
    value_int = (int)value;
    switch (sub_action_type) {
        case PAIR_FWD:
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_PROCESSING);
            } else {
                printf("Move fwd %d\n\r", value_int);
            }
            Wheels.step(value_int, PAIR_FWD);
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_OK);
            } else {
                printf("$ ");
            }
            break;
        case PAIR_REV:
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_PROCESSING);
            } else {
                printf("Move back %d\n\r", value_int);
            }
            Wheels.step(value_int, PAIR_REV);
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_OK);
            } else {
                printf("$ ");
            }
            break;
        case PAIR_LEFT:
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_PROCESSING);
            } else {
                printf("Turn left %d deg\n\r", value);
            }
            value_int = (int)(WHEELSTEPSDEGREE * value); // convert degrees to steps
            if (value_int > 0) {
                Wheels.step(value_int, PAIR_LEFT);
            } else {
                Wheels.step(abs(value_int), PAIR_RIGHT);
            }
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_OK);
            } else {
                printf("$ ");
            }
            break;
        case PAIR_RIGHT:
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_PROCESSING);
            } else {
                printf("Turn right %d deg\n\r", value);
            }
            value_int = (int)(WHEELSTEPSDEGREE * value); // convert degrees to steps
            if (value_int > 0) {
                Wheels.step(value_int, PAIR_RIGHT);
            } else {
                Wheels.step(abs(value_int), PAIR_LEFT);
            }
            if (menulevel == MENU_M2M) {
                m2m_response((char *)RESP_OK);
            } else {
                printf("$ ");
            }
            break;
        default:
            break;
    }

}

// move_servo: ang is a value in degrees, typically 0-180 (range is defined in hservo.h/hservo.cpp)
void move_servo(int ang) {
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_PROCESSING);
    } else {
        printf("Move servo to %d deg\n\r", ang);
    }
    Servo.setAng(ang);
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_OK);
    } else {
        printf("$ ");
    }
}

// rotate_motor
void rotate_motor(char sub_action_type, double value) {
    int motornum=0;
    int dir=0;
    int steps = (int)value;
    switch (sub_action_type) {
        case ROT_M3:
            motornum = 3;
            break;
        case ROT_M4:
            motornum = 4;
            break;
        default:
            break;
    }
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_PROCESSING);
    } else {
        printf("Rotate m%d %d steps\n\r", motornum, steps);
    }
    if (steps<0) {
        dir = 1;
        steps = abs(steps);
    } else {
        dir = 0;
    }
    switch(motornum) {
        case 3:
            Motor3.step(steps, dir);
            break;
        case 4:
            Motor4.step(steps, dir);
            break;
        default:
            break;
    }
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_OK);
    } else {
        printf("$ ");
    }
}

// ext_pwr
void ext_pwr(char subaction)
{
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_PROCESSING);
    } else {
        printf("Setting ext power state %d\n\r", subaction);
    }
    switch(subaction) {
        case EXT_ON:
            EXT_PWR_ON;
            break;
        case EXT_OFF:
            EXT_PWR_OFF;
            break;
    }
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_OK);
    } else {
        printf("$ ");
    }
}

// handle_requests
void handle_requests(void) {
    if (modechange)
    {
        switch(uiparam_doaction) {
            case ACTION_WHEELS:
                rotate_wheels(uiparam_wheelsaction, uiparam_valueparam);
                break;
            case ACTION_SERVO:
                move_servo((int)uiparam_valueparam);
                break;
            case ACTION_MOTOR:
                rotate_motor(uiparam_motoraction, uiparam_valueparam);
                break;
            case ACTION_EXT:
                ext_pwr(uiparam_extaction);
                break;
            default:
                break;
        }
        modechange=0;
    }
}

// run_program
// currently runs a preset program, but could be modified in future to run user programs
void run_program(void) {
    char* line;
    int i=0;

    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_PROCESSING);
    } else {
        printf("running preset program\n\r");
    }
    
    line = (char*)preset_program1[0]; // first line
    while(line[0]!='\0')
    {
        if (menulevel == MENU_M2M) {
            //
        } else {
            printf("cmd: %s\n\r", line);
        }
        process_line(line);
        handle_requests(); // execute any request that resulted from the line
        i++;
        line=(char*)preset_program1[i]; // next line
    }

    // finished
    if (menulevel == MENU_M2M) {
        m2m_response((char *)RESP_OK);
    } else {
        printf("$ ");
    }
}

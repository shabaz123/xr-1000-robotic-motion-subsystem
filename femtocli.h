#ifndef FEMTOCLI_HEADER_
#define FEMTOCLI_HEADER_

#include "pico/stdlib.h"

#define RXMAXLEN 100

#define MENU_TOP 1
#define MENU_ADMIN 2
#define MENU_M2M 3

#define ACTION_IDLE 0
#define ACTION_WHEELS 1
#define ACTION_SERVO 2
#define ACTION_MOTOR 3
#define ACTION_EXT 4

#define MODIFIER_NULL 0
#define MODIFIER_ON 1
#define MODIFIER_OFF 2

#define PAIR_FWD 1
#define PAIR_REV 0
#define PAIR_LEFT 2
#define PAIR_RIGHT 3

#define ROT_M3 3
#define ROT_M4 4

#define EXT_ON 1
#define EXT_OFF 0

#define RESP_PROCESSING "PR\n\r"
#define RESP_OK "OK\n\r"
#define RESP_BADREQ "BR\n\r"

//extern Serial pc;

extern char uiparam_wheelsaction;
extern char uiparam_motoraction;
extern char uiparam_extaction;
extern double uiparam_valueparam;
extern char modechange;
extern char uiparam_adminmode;
extern char uiparam_m2mmode;
extern char uiparam_doaction;
extern char uiparam_modifier;
extern uint32_t alarmPeriod;

void menu_init(void);
int64_t pcui_callback(alarm_id_t id, void *user_data);
void clear_hist_buffer(void);
void set_menu(char m);
void m2m_response(char* s);
void process_line(char* line); // non-interactive mode

#endif // FEMTOCLI_HEADER_

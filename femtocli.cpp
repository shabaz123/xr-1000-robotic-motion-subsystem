/***********************************************
 * femtocli.c
 * a miniaturized command line interface
 * rev 1 - shabaz - november 2017
 * rev 2 - shabaz - march 2022
 *
 ***********************************************/

// #includes
#include "femtocli.h"
#include "string.h"
#include "pico/stdio.h"
#include "hardware/irq.h"
#include "timer.h"
#include "pico/time.h"
#include "smotpair.h"
#include "hservo.h"

// #defines
#define DBG_PRINT 0
#define MAXLINEPROMPT 20
#define MAXTOK 5
#define MAXWLEN 20

#ifdef LINUX
#define PRINTF printw
#define PUTCH addch
#else
#define PRINTF printf
#define PUTCH putchar
#endif



// conditional #includes
#ifdef LINUX
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <signal.h>
#else
#include "stdio.h"
#endif

// structs
typedef struct token_s
{
    char idx;
    char len;
} token_t;

// const values
const char default_line_prompt[]="$ ";
const char* const general[]={"exit", "help", "?", "history", ""};
const char* const time_suffix[]={"sec", "msec", ""};
const char* const top_menu[]={"fwd", "back", "left", "right", "pu", "pd", "servo", "m3", "m4", "ext", "admin", "m2m", ""};
const char* const admin_menu[]={"cmd1", "cmd2", ""};
const char* const m2m_menu[]={"fwd", "back", "left", "right", "pu", "pd", "servo", "m3", "m4", "ext", ""};

const char* const general_help[]={  " - exit a sub-menu",
                                    " - get help",
                                    " - see commands available",
                                    " - type !! to repeat a command", ""};
const char* const top_help[]={      "<n>  - go forward n steps",
                                    "<n> - go back n steps",
                                    "<n> - turn left n degrees",
                                    "<n> - turn right n degrees",
                                    " - lift pen up",
                                    " - set pen down",
                                    "<n> - move servo to n degrees",
                                    "<n> <dir> - rotate m3 n steps cw/ccw",
                                    "<n> <dir> - rotate m4 n steps cw/ccw",
                                    "<on/off> - external power",
                                    " - admin menu",
                                    " - M2M mode",
                                    ""};
const char* const admin_help[]={    " - placeholder command 1",
                                    " - placeholder command 2", 
                                    ""};
const char* const m2m_help[]={      "<n>  - go forward n steps",
                                    "<n> - go back n steps",
                                    "<n> - turn left n degrees",
                                    "<n> - turn right n degrees",
                                    " - lift pen up",
                                    " - set pen down",
                                    "<n> - move servo to n degrees",
                                    "<n> <dir> - rotate m3 n steps cw/ccw",
                                    "<n> <dir> - rotate m4 n steps cw/ccw",
                                    "<on/off> - external power",
                                    ""};





// global variables
char rxbuf[RXMAXLEN+1];
char oldrxbuf[RXMAXLEN+1];
char pc_idx=0;
char running=1;
token_t tokens[MAXTOK];
char numtok=0;
char menulevel=MENU_TOP;
char** cmdlist;
char** helplist;

// externs
extern char usb_control;

// function prototypes
void split(char* instring, char delim);

// functions

void set_menu(char m)
{
    menulevel=m;
    switch(menulevel)
    {
        case MENU_TOP:
            cmdlist=(char**)top_menu;
            helplist=(char**)top_help;
            break;
        case MENU_ADMIN:
            cmdlist=(char**)admin_menu;
            helplist=(char**)admin_help;
            break;
        case MENU_M2M:
            cmdlist=(char**)m2m_menu;
            helplist=(char**)m2m_help;
            break;
        default:
            break;
    }
}

void menu_init(void)
{
    set_menu(MENU_TOP);
}

void print_cmdlist(void)
{
    int i=0;
    int nohelp=0;
    char* ctxt;
    char* htxt;
    switch(menulevel)
    {
        case MENU_TOP:
            PRINTF("Main menu");
            break;
        case MENU_ADMIN:
            PRINTF("Admin mode");
            break;
        case MENU_M2M:
            PRINTF("M2M mode");
            break;
        default:
            break;
    }
    PRINTF("\n\r");
    PRINTF("Commands available:\n\r");
    fflush(stdout);
    ctxt=cmdlist[i];
    htxt=helplist[i];
    while(ctxt[0]!='\0')
    {
        PRINTF(" %s", ctxt);
        if ((nohelp==0) && (htxt[0]!='\0'))
        {
            PRINTF(" %s\n\r", htxt);
        }
        else
        {
            nohelp=1;
            PRINTF("\n\r");
        }
        i++;
        ctxt=cmdlist[i];
        if (nohelp==0)
            htxt=helplist[i];
    }
    // print the general commands too:
  i=0;
    ctxt=(char*)general[0];
    htxt=(char*)general_help[0];
    while(ctxt[0]!='\0')
    {
        PRINTF(" %s %s\n\r", ctxt, htxt);
        i++;
        ctxt=(char*)general[i];
        htxt=(char*)general_help[i];
    }
    
}

// searchfor: returns the keyword array index. idx is how far into the input string it was
// at exit if the return value is -1 then no keyword was entirely found.
// parkw is the keyword that was most closely partially matched
int searchfor(char* instring, char** arrp, int* idx, int* parkw)
{
    char* arr;
    int found=0;
    int j=0;
    int si=0;
    int i=0;
    int longi=-1;
    int longsi=-1;
    int longlen=0;
    int ilen=strlen(instring);
    
    //PRINTF("search string is '%s'", instring);
    
    arr=arrp[si];
    while (instring[i] != '\0') // outer loop, through the input string
    {
        while(arr[0]!='\0') // loop through all the words in the array
        {
            while(arr[j]==instring[i+j]) // loop while characters match
            {
                j++;
                if ((i+j)>=ilen) // stop if input string not long enough to do the match
                    break;
            }
            if (j==strlen(arr)) // match found
            {
                found=1;
                break;
            }
            // characters didn't match. Store how much matched (partial match)
            if ((j-1)>longlen)
            {
                //printf("i+j is %d, ilen is %d\n\r", i+j, ilen);
                if (i+j==ilen-1) // we only care about partial match at the end of the string
                {
                    longlen=j-1;
                    longsi=si;
                    longi=i;
                }
            }
            // set up for next word in the array
            si++;
            arr=arrp[si];
            j=0;
        }
        if (found) // match was found, no need to check any more of the input string
            break;
        // characters didn't match, so move along one character in the input string
        i++;
        // set up for the first word in the array
        si=0;
        arr=arrp[si];
        j=0;
    }
    if (found)
    {
        *idx=i;
        *parkw=-1;
    }
    else
    {
        //*idx=-1;
        *idx=longi;
        si=-1;
        *parkw=longsi;
    }
    
    return(si);
}

// very basic sanity checking. It is not bullet-proof
int
idx_notanum(char *ts)
{
    int i=0;
    int found=0;
    
    while(ts[i]!='\0') {
        if(ts[i]>='0' && ts[i]<='9') {
            i++;
            continue;
        }
        if (ts[i]=='.') {
            i++;
            continue;
        }
        if (ts[i]=='-') {
            i++;
            continue;
        }
        found=1;
        break;
    }
    if(ts[i]==' ')
        return(-1);
    if (found)
        return i;
    else
        return(-1);
}

double todouble(char* ds)
{
    int i;
    char tc;
    double mult=1.0;
    double val_double;
    char wbuf[MAXWLEN+2];
    
    i=idx_notanum(ds);
    if (i==0) // we need at least one digit!
        return(0.0);
        
    strcpy(wbuf, ds);
    if (i>0) // there were characters on the end of the number
    {
        tc=wbuf[i];
        wbuf[i]='\0';
    }
    sscanf(wbuf, "%lf", &val_double);
    if (i<0)
    {
        return(val_double); 
    }
    // process the character after the number
    wbuf[i]=tc;
    switch(tc)
    {
        case 'p':
            val_double=val_double*1.0E-12;
            break;
        case 'n':
            val_double=val_double*1.0E-9;
            break;
        case 'u':
            val_double=val_double*1.0E-6;
            break;
        case 'm':
            val_double=val_double*1.0E-3;
            break;
        case 'k':
            val_double=val_double*1.0E3;
            break;
        case 'M':
            val_double=val_double*1.0E6;
            break;
        case 'G':
            val_double=val_double*1.0E9;
            break;
    }
    
    return(val_double);
}

void
dotab(void)
{
    int i;
    int kw;
    int parkw;
    char** kwset;
    char* kwp;
    int a;
    int xi;
    char foundlist=0;
    
    char wbuf[MAXWLEN+2];
    if (pc_idx==0) // nothing to expand!
        return;
    
    if (rxbuf[pc_idx-1]==' ') // we can't expand something already deliminated by the user
        return;

    //tokenise
    // temporarily terminate the string
    if (pc_idx>RXMAXLEN-3)
        return;
    rxbuf[pc_idx]=' ';
    rxbuf[pc_idx+1]='\0';
    split(rxbuf, ' ');
    if (numtok==0) // nothing to expand, just whitespace!
        return;
    
    if (tokens[numtok-1].len >= MAXWLEN) // token is too long to expand!
        return;
    
    //printf("last token idx=%d, len=%d\n\r", tokens[numtok-1].idx, tokens[numtok-1].len);    
    strncpy(wbuf, rxbuf+tokens[numtok-1].idx, tokens[numtok-1].len);
    wbuf[tokens[numtok-1].len]=' ';
    wbuf[tokens[numtok-1].len+1]='\0';
    
    kwset=(char**)general;
    kw=searchfor(wbuf, kwset, &xi, &parkw);
  if (kw>=0)
  {
    // full keyword, nothing to expand
    return;
  }
  // do any menu-specific expansion
  if (parkw<0)
  {
    kw=searchfor(wbuf, cmdlist, &xi, &parkw);
    if (kw>=0)
    {
        // full keyword, nothing to expand
        return;
    }
    foundlist=1;
  }
  
  //PRINTF("examining '%s'\n\r", wbuf);
  if (xi>0) // we are only interested if tab-completing to standalone words for now
  {
    //return;
  }
  
  
  if (parkw>=0)
  {
    // partial keyword!
    //PRINTF("partial!\n\r");
    i=tokens[numtok-1].idx;
    if (xi>=0)
        i=i+xi;
    a=pc_idx-i; // number of characters we already have from the keyword
    
    // select the appropriate list of keywords:
    switch(foundlist)
    {
        case 0:
            kwp=kwset[parkw];
            break;
        case 1:
            kwp=cmdlist[parkw];
            break;
        default:
            break;
    }
    
    while(pc_idx<(RXMAXLEN-1))
    {
        rxbuf[pc_idx]=kwp[a];
        PUTCH(kwp[a]);
        a++;
        pc_idx++;
        if (kwp[a]=='\0')
      {
        break;
      }
    }
    rxbuf[pc_idx]=' ';
    pc_idx++;
    PUTCH(' ');
  }

        
}

int ignore_delim(char* instring, char delim, int startidx)
{
    int i=startidx;
    while (instring[i]!='\0')
    {
        if (instring[i]==delim)
        {
            i++;        
        }
        else
        {
            break;
        }
    }

    return(i);
}

void split(char* instring, char delim)
{
    int ctr=0;
    int i=0;
    int ti=0;
  static int currIndex = 0;
  numtok=0;
  if (*instring == '\0')
  {
    return;
  } 
  
  i=strlen(instring);
  instring[i]=delim; // append a delimeter on the end
  instring[i+1]='\0'; 

    i=ignore_delim(instring, delim, 0); // ignore any initial delimiter
    ti=i;
  while (instring[i] != '\0')
  {
    if (instring[i] != delim)
    {
        i++;        
    }
    else // delimeter found, record its position in the array
    {
        tokens[ctr].idx = ti;
        tokens[ctr].len = i-ti;
        ctr++;
        i=ignore_delim(instring, delim, i);
        ti=i;
    }
  }
    numtok=ctr;
    if (DBG_PRINT)
    {
        PRINTF("numtok is %d\n\r", numtok);
    }
}

void get_line_prompt(char* res)
{
    switch(menulevel)
    {
        case MENU_TOP:
            strncpy(res, default_line_prompt, MAXLINEPROMPT-1);
            break;
        case MENU_ADMIN:
            strncpy(res, "admin$ ", MAXLINEPROMPT-1);
            break;
        case MENU_M2M:
            //strncpy(res, "m2m$ ", MAXLINEPROMPT-1);
            strncpy(res, "", MAXLINEPROMPT-1);
            break;
        default:
            strncpy(res, default_line_prompt, MAXLINEPROMPT-1);
            break;
    }
}

void int_handler(int dummy)
{
  running=0;
}

void parse_input(void)
{
    int i, j;
    int kw;
    int parkw;
    char tstring[MAXLINEPROMPT+1];
    double dvar;
    char numparam;
    // do a carriage return
    PRINTF("\n\r");

  // build array of token indexes
  split(rxbuf, ' ');
  for (i=0; i<numtok; i++)
  {
    if (DBG_PRINT) PRINTF("token %d at index %d, length %d\n\r", i, tokens[i].idx, tokens[i].len);
  }
  numparam=numtok-1;
  // search for general keywords
  kw=searchfor(rxbuf, (char**)general, &i, &parkw);
  if ((i>=0) && (kw>=0))
  {
    if (DBG_PRINT) PRINTF("found general keyword %d, at idx %d\n\r", kw, i);
    // handle the general keywords
    switch(kw)
    {
        case 0: // exit
            switch(menulevel)
            {
                case MENU_TOP:
                    PRINTF("At Main menu\n\r");
                    break;
                case MENU_ADMIN:
                case MENU_M2M:
                    set_menu(MENU_TOP);
                    break;
                default:
                    break;
            }
            break;
        case 1: // help
        case 2: // ?
            print_cmdlist();
            break;
        case 3: // history
            break;
        default:
            break;
    }
  }

    // search for the current menu level keywords
    kw=searchfor(rxbuf, cmdlist, &i, &parkw);
    if ((i>=0) && (kw>=0))
  {
    if (DBG_PRINT) PRINTF("found menu level keyword %d, at idx %d\n\r", kw, i);
    // null-terminate the parameters
    for (j=1; j<=numparam; j++)
    {
        rxbuf[tokens[j].idx+tokens[j].len]='\0';
    }
    // handle the general keywords
    switch(menulevel)
    {
        case MENU_TOP:
            switch(kw)
            {
                case 0: // fwd
                    if (numparam==1)
                    {
                        //rxbuf[tokens[1].idx+tokens[1].len]='\0';
                        uiparam_wheelsaction = PAIR_FWD;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        PRINTF("forward %lf steps\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_WHEELS;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", top_help[kw]);
                    }
                    break;
                case 1: // back
                    if (numparam==1)
                    {
                        uiparam_wheelsaction = PAIR_REV;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        PRINTF("back %lf steps\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_WHEELS;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", top_help[kw]);
                    }
                    break;
                case 2: // left
                    if (numparam==1)
                    {
                        uiparam_wheelsaction = PAIR_LEFT;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        PRINTF("left %lf degrees\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_WHEELS;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", top_help[kw]);
                    }
                    break;
                case 3: // right
                    if (numparam==1)
                    {
                        uiparam_wheelsaction = PAIR_RIGHT;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        PRINTF("right %lf degrees\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_WHEELS;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", top_help[kw]);
                    }
                    break;
                case 4: // pu
                    uiparam_valueparam = PU_ANG;
                    PRINTF("pen up %lf degrees\n\r", uiparam_valueparam);
                    uiparam_doaction=ACTION_SERVO;
                    modechange=1;
                    break;
                case 5: // pd
                    uiparam_valueparam = PD_ANG;
                    PRINTF("pen down %lf degrees\n\r", uiparam_valueparam);
                    uiparam_doaction=ACTION_SERVO;
                    modechange=1;
                    break;
                case 6: // servo
                    if (numparam==1)
                    {
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        PRINTF("servo %lf degrees\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_SERVO;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", top_help[kw]);
                    }
                    break;
                case 7: // m3
                    if (numparam>=1)
                    {
                        uiparam_motoraction = ROT_M3;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        if (numparam>=2) {
                            if (strcmp(&rxbuf[tokens[2].idx], "cw") == 0) {
                                // no change
                            } else if (strcmp(&rxbuf[tokens[2].idx], "ccw") == 0) {
                                uiparam_valueparam = 0 - uiparam_valueparam;
                            }
                        }
                        PRINTF("rotate m3 %lf steps\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_MOTOR;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameters %s\n\r", top_help[kw]);
                    }
                    break;
                case 8: // m4
                    if (numparam>=1)
                    {
                        uiparam_motoraction = ROT_M4;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        if (numparam>=2) {
                            if (strcmp(&rxbuf[tokens[2].idx], "cw") == 0) {
                                // no change
                            } else if (strcmp(&rxbuf[tokens[2].idx], "ccw") == 0) {
                                uiparam_valueparam = 0 - uiparam_valueparam;
                            }
                        }
                        PRINTF("rotate m4 %lf steps\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_MOTOR;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameters %s\n\r", top_help[kw]);
                    }
                    break;
                case 9: // ext
                    if (numparam==1)
                    {
                        uiparam_extaction = EXT_OFF;
                        if ((tokens[1].len==2) && strcmp(&rxbuf[tokens[1].idx], "on")==0) {
                            uiparam_extaction = EXT_ON;
                            PRINTF("external power on\n\r");
                        }
                        else if ((tokens[1].len==3) && strcmp(&rxbuf[tokens[1].idx], "off")==0) {
                            uiparam_extaction = EXT_OFF;
                            PRINTF("external power off\n\r");
                        }
                        uiparam_doaction=ACTION_EXT;
                        modechange=1;
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", admin_help[kw]);
                    }
                    break;
                case 10: // admin
                    PRINTF("Entering admin mode, type exit to quit\n\r");
                    set_menu(MENU_ADMIN);
                    break;
                case 11: // m2m
                    PRINTF("Entering M2M mode, type exit to quit\n\r");
                    set_menu(MENU_M2M);
                    break;
            }
            break;
        case MENU_ADMIN:
            switch(kw)
            {
                case 0: // cmd1
                    if (numparam==1)
                    {
                        if ((tokens[1].len==2) && strcmp(&rxbuf[tokens[1].idx], "on")==0)
                        {
                            //uiparam_xyz=1;
                        }
                        else if ((tokens[1].len==3) && strcmp(&rxbuf[tokens[1].idx], "off")==0)
                        {
                            //uiparam_xyz=0;
                        }
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", admin_help[kw]);
                    }
                    break;
                case 1: // cmd2
                    if (numparam==1)
                    {
                        if ((tokens[1].len==2) && strcmp(&rxbuf[tokens[1].idx], "on")==0)
                        {
                            //uiparam_xyz=1;
                        }
                        else if ((tokens[1].len==3) && strcmp(&rxbuf[tokens[1].idx], "off")==0)
                        {
                            //uiparam_xyz=0;
                        }
                    }
                    else
                    {
                        PRINTF("Error, required parameter %s\n\r", admin_help[kw]);
                    }
                    break;
            }
            break;
        case MENU_M2M:
            switch(kw)
            {
                case 0: // fwd
                case 1: // rev
                case 2: // left
                case 3: // right
                    if (numparam==1)
                    {
                        switch(kw) {
                            case 0:
                                uiparam_wheelsaction = PAIR_FWD;
                                break;
                            case 1:
                                uiparam_wheelsaction = PAIR_REV;
                                break;
                            case 2:
                                uiparam_wheelsaction = PAIR_LEFT;
                                break;
                            case 3:
                                uiparam_wheelsaction = PAIR_RIGHT;
                                break;
                        }
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_WHEELS;
                        modechange=1;
                    }
                    else
                    {
                        m2m_response((char *)RESP_BADREQ);
                    }
                    break;
                case 4: // pu
                    uiparam_valueparam = PU_ANG;
                    uiparam_doaction=ACTION_SERVO;
                    modechange=1;
                    break;
                case 5: // pd
                    uiparam_valueparam = PD_ANG;
                    uiparam_doaction=ACTION_SERVO;
                    modechange=1;
                    break;
                case 6: // servo
                    if (numparam==1) {
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        uiparam_doaction=ACTION_SERVO;
                        modechange=1;
                    }
                    else
                    {
                        m2m_response((char *)RESP_BADREQ);
                    }
                    break;
                case 7: // m3
                    if (numparam>=1)
                    {
                        uiparam_motoraction = ROT_M3;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        if (numparam>=2) {
                            if (strcmp(&rxbuf[tokens[2].idx], "cw") == 0) {
                                // no change
                            } else if (strcmp(&rxbuf[tokens[2].idx], "ccw") == 0) {
                                uiparam_valueparam = 0 - uiparam_valueparam;
                            }
                        }
                        uiparam_doaction=ACTION_MOTOR;
                        modechange=1;
                    }
                    else
                    {
                        m2m_response((char *)RESP_BADREQ);
                    }
                    break;
                case 8: // m4
                    if (numparam>=1)
                    {
                        uiparam_motoraction = ROT_M4;
                        uiparam_valueparam = todouble(&rxbuf[tokens[1].idx]);
                        if (DBG_PRINT) PRINTF("param is %lf\n\r", uiparam_valueparam);
                        if (numparam>=2) {
                            if (strcmp(&rxbuf[tokens[2].idx], "cw") == 0) {
                                // no change
                            } else if (strcmp(&rxbuf[tokens[2].idx], "ccw") == 0) {
                                uiparam_valueparam = 0 - uiparam_valueparam;
                            }
                        }
                        uiparam_doaction=ACTION_MOTOR;
                        modechange=1;
                    }
                    else
                    {
                        m2m_response((char *)RESP_BADREQ);
                    }
                    break;
                case 9: // ext
                    if (numparam==1)
                    {
                        uiparam_extaction = EXT_OFF;
                        if ((tokens[1].len==2) && strcmp(&rxbuf[tokens[1].idx], "on")==0) {
                            uiparam_extaction = EXT_ON;
                        }
                        else if ((tokens[1].len==3) && strcmp(&rxbuf[tokens[1].idx], "off")==0) {
                            uiparam_extaction = EXT_OFF;
                        }
                        uiparam_doaction=ACTION_EXT;
                        modechange=1;
                    }
                    else
                    {
                        m2m_response((char *)RESP_BADREQ);
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
  }
  
  // interpret the keywords as numbers as a test
  for (i=0; i<numtok; i++)
  {
    rxbuf[tokens[i].idx+tokens[i].len]='\0';
    dvar = todouble(&rxbuf[tokens[i].idx]);
    if (DBG_PRINT) printf("double is %lf\n\r", dvar);
    }
    
    
    
  // print the line prompt:
  get_line_prompt(tstring);
  PRINTF("%s", tstring);
}

void rearm_timer()
{
    //alarm_in_us_arm(alarmPeriod);
}

void m2m_response(char* s)
{
    if (usb_control) {
        PRINTF("%s", s);
    } else {
        uart_puts(uart0, s);
    }
}

// non-interactive mode
void process_line(char* line)
{
    strcpy(rxbuf, line);
    strcpy(oldrxbuf, rxbuf); // store the history
    parse_input();
}

// interactive mode callback
int64_t pcui_callback(alarm_id_t id, void *user_data)
{
    int ci;
    char c;
#ifdef LINUX
        char x, y;
        int cc;
        cc=getch();
    c=(char)(cc & 0x00ff);
#else
    if (usb_control) {
        ci=getchar_timeout_us(0); // get char in specified number of microseconds
        if (ci==-1) { // no usb character received
            return(ALARM_USEC_PERIOD);
        }
        c = (char) ci;
    } else {
        if (uart_is_readable(uart0)<1) { // no uart character received
            return(ALARM_USEC_PERIOD);
        }
        c = uart_getc(uart0);
    }
#endif
    // Is carriage return pressed?
#ifdef LINUX
    if (c=='\n')
#else
    if (c=='\r')
#endif
    {
        rxbuf[pc_idx]='\0';
        strcpy(oldrxbuf, rxbuf); // store the history
        parse_input();
        pc_idx=0;
    }
    else // no, something else was pressed. Not really valid for UART mode, to be fixed.
    {
        rxbuf[pc_idx]=c;
        //printf("%d", c);
        // was backspace pressed?
        if ((c==0x08) | (c==0x7f))
        {
            // need to delete a character
            if (pc_idx==0)
            {
                // first char, nothing to do
            }
            else
            {
                // delete a char
                pc_idx--;   
#ifdef LINUX
                                getyx(stdscr, y, x);
                                wmove(stdscr, y, x-1);
                                PUTCH(' ');
                                wmove(stdscr, y, x-1);
#else
                PUTCH(c); // move back a char
                PUTCH(' '); // display a space
                PUTCH(c); // move back a char
#endif
            }
            
        }
        else if (c==0x09) // was tab pressed?
        {
            // do tab completion
            dotab();
        }
        else // something else was pressed
        {
            if (pc_idx<(RXMAXLEN-1)) 
            {
                // print a char
                pc_idx++;
                //printf("%c", c); fflush(stdout); // shabaz
                PUTCH(c);
                // handle special sequences
                if ((rxbuf[0]=='!') && (rxbuf[1]=='!'))
                {
                    strcpy(rxbuf, oldrxbuf);
                    PUTCH(0x08);
                    PUTCH(0x08);
                    PRINTF("%s", rxbuf);
                    parse_input();
                    pc_idx=0;
                }
#ifdef LINUX
                if ((rxbuf[0]=='!') && (rxbuf[1]=='x'))
                {
                    clear();
                    wmove(stdscr, 0, 0);
                    refresh();
                }
#endif
            }
        }
    }
    return(ALARM_USEC_PERIOD);
}



/*---------------------------------------------------------
Purpose: This is a state machine mechanism using table
driven implementation with function pointers for actions.
The purpose is to fallow up after user was identified on
both routines, online and offline. Appropriate events are
generated in order to guide user through features of this
mechanism such as charging, last consumption and expenditure
check, balance check and billing process in online mode. 
In case of offline, features are the same, just readings 
from RFID card are used instead of online data. User is billed
to the RFID card and next time it is synchronized with 
online data.

State transition table:	
				 // No Action			      a1                      b2						   c3                      d4                      e5
			
        0    idl			endSessionm one		retrieve_price  one			draw_menu		charge		charging    consumption getConsumption  balance		getBalance
        1    charge			noAction    one		draw_menu		charge		noAction		charge		noAction    charge		noAction		charge		noAction
        2    consumption	noAction    one		draw_menu		consumption noAction		consumption noAction    consumption noAction		consumption noAction
        3    balance		noAction    one		draw_menu		balance		noAction		balance		noAction    balance		noAction		balance		noAction
        4    idl			noAction    idl		idleWaiting		one			identification  idl			noAction    idl			noAction		idl			noAction

Actions:
void noAction(void);
void retrieve_price(void);
void draw_menu(void);
void navigation(void);
void charging(void);
void getConsumption(void);
void getBalance(void);
void endSessionm(void);
void identification(void);
void idleWaiting(void);

Input: Idied flag is passed from sessionStart module when
user was successfully identified and all the required data 
is collected. Then fallowing events are generated in order 
to properly guide user through features of this mechanism.
Also gets inputs from keypad.

Output: Outputs mainly to the hardware - LCD, writes to
RFID card, sends data packets to server.

Uses: Usual avr libraries as well as other modules such as:
"driverLCD.h"
"driverUSART.h"
"driverKeyPad.h"
"dataReceive.h"
"formPacket.h"
"sessionStart.h"
"adcChargingSimulation.h"
"driverRFID.h"

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include <avr/io.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h> 
#include <avr/wdt.h> 
  
#include "driverLCD.h" 
#include "driverUSART.h" 
#include "driverKeyPad.h" 
#include "dataReceive.h" 
#include "formPacket.h" 
#include "sessionStart.h" 
#include "adcChargingSimulation.h" 
#include "driverRFID.h"

# define F_CPU 1000000UL 
  
typedef enum { 
    one, 
    charge, 
    consumption, 
    balance, 
    idl 
} state_menu; 
  
  
typedef enum { 
    zeroevent, 
    a1, 
    b2, 
    c3, 
    d4, 
    e5 
} event_menu; 
  
int price; 
char price_str[3]; 

/*LCD menu configuration*/  
int menu_position = 1; 
int stMenuItem = 1; 
int lastMenuItem = 3; 
bool default_menu = true; 
bool defChargMenu = false; 
char arrow =    0b00000001; 
char doneChar = 0b00000000;
/*Other global variables and falgs*/ 
char expenseToPayChar[8]; 
char energyStr[8]; 
bool restart = false; 
  
bool charged = false; 
typedef void (*action)(); 
  
typedef struct { 
    state_menu nextState; 
    action actionToDo; 
}  stateElement_menu; 
  
//actions 
  
  
void noAction(void); 
void retrieve_price(void); 
void draw_menu(void); 
void navigation(void); 
void stateTransition_m(event_menu curr); 
bool arrow_mov(void); 
void charging(void); 
void getConsumption(void); 
void getBalance(void); 
bool waitUntilKeyPressed(char key); 
void endSessionm(void); 
void identification(void); 
void idleWaiting(void); 
  
        // No Action            a1                      b2              c3                      d4                      e5                 
  
stateElement_menu stateMatrix_menu[5][6] = { 
/*0 */{ {idl,endSessionm}, {one,retrieve_price}, {one,draw_menu} , {charge,charging}, {consumption,getConsumption}, {balance,getBalance} }, 
/*1 */{ {charge,noAction}, {one,draw_menu}, {charge, noAction},{charge,noAction}, {charge, noAction}, {charge, noAction} }, 
/*2 */{ {consumption, noAction}, {one, draw_menu}, {consumption, noAction}, {consumption, noAction}, {consumption, noAction}, {consumption, noAction} }, 
/*3 */{ {balance, noAction}, {one, draw_menu}, {balance, noAction}, {balance, noAction}, {balance, noAction}, {balance, noAction} }, 
/*4 */{ {idl, noAction}, {idl, idleWaiting}, {one, identification}, {idl, noAction}, {idl, noAction}, {idl, noAction} } 
}; 
  
event_menu  EventOccured_menu = zeroevent; 
  
void  stateEval_menu(event_menu w); 
  
state_menu  currentState_m = idl; 
/* -----------------------------------------------------
void stateEval(event w)
Function that finds next state and corresponding action
according to the event passed as a parameter and current
state variable.
-----------------------------------------------------*/  
void stateEval_menu(event_menu w) { 
    stateElement_menu stateEvaluation = stateMatrix_menu[currentState_m][w];    //suranda sekancia state pagal esama state ir ivyki 
    currentState_m = stateEvaluation.nextState;                     //sauna action'a busimos 
    (*stateEvaluation.actionToDo)(); 
      
} 
/* -----------------------------------------------------
int main(void)
Main function. Initiates USART, LCD, keypad and generates
default event that is passed as parameter to state transition
mechanism.
-----------------------------------------------------*/  
int main(void) 
{ 
    USART_Init(64); 
    lcd_init(); 
    sei(); 
    keypad_init(); 
    stateTransition_m(a1); 
  
}
/* -----------------------------------------------------
void endSessionm(void)
Method that works for both modes and ends a session.
In case of online mode, it just generates data packet 
and sends it. In case of offline mode, it prepares data
(debt, last expense and last consumption) by copying to 
appropriate variables and initiates RFID card writing 
function. In both cases the last action is to restart
controller in order to have a clean start with all the
registers cleared. For that, watchdog timer is set to 15ms.
-----------------------------------------------------*/ 
void endSessionm(void){ 
    lcdClear(); 
    _delay_ms(10); 
      
    if (offline_mode && charged) 
    { 
            GoTo(0,0); 
            LCDPutString("You've been debited"); 
//          GoTo(0,1); 
//          LCDPutString("for 1500dkk"); 
            GoTo(0,2); 
            LCDPutString("Please use RFID card"); 
            GoTo(0,3); 
            LCDPutString("to recalculate"); 
            rfidDone = false; 
            offlineWrite = true; 
        //  int exl = strlen(expenseToPayChar); 
            //int enl = strlen(energyStr); 
              
              
            //make a 16byte memory card block format data + 0000 
            memset(pastEnChar, 0, 16); 
            memset(debtChar, 0, 16); 
            memset(pastExChar, 0, 16); 
              
            memcpy(pastEnChar, energyStr, 8); 
            memcpy(pastExChar, expenseToPayChar, 8); 
            memcpy(debtChar, expenseToPayChar, 8); 
              
              
            while (!RFIDinit(5, "write", 5)); 
            offlineWrite = false; 
            restart = true; 
            lcdClear(); 
            LCDPutString("Logged out"); 
            _delay_ms(100); 
            wdt_enable(WDTO_15MS); 
            while(true){ 
              
            } 
          
    }else{ 
            formPacket("02", "01","99", "EndofSession"); 
            sendStringUSART(formedDataPackageToSend);  
    } 
    /*if (creditDetected) 
    { 
        GoTo(0,1); 
        LCDPutString("Log out with RFID"); 
        while (!RFIDinit(4, "write", 5)); 
    }*/
      
    charged = false; 
    default_menu = true; 
    stMenuItem = 1; 
    menu_position = 1; 
    _delay_ms(100); 
    _delay_ms(100); 
    wdt_enable(WDTO_15MS); 
    while(true); 
} 
/* -----------------------------------------------------
void identification(void)
Function that waits until authentication process is done,
returns true and generates event to shift states and fire
actions.
-----------------------------------------------------*/  
void identification(void){ 
    while(!id()); 
    stateTransition_m(a1); 
} 
/* -----------------------------------------------------
void idleWaiting()
Function that is fired on idle state, has simple string
output on LCD and waits for any key pressed. After it's
pressed, fires action that starts session. If restart 
flag is true, it restarts controller in order to clear
registers. It is in case if it did not restart at 
endSession action.
-----------------------------------------------------*/
void idleWaiting(){ 
    if (restart) 
    { 
        wdt_enable(WDTO_15MS); 
        while(1); 
    } 
    lcdClear(); 
    _delay_ms(10); 
    GoTo(0,1);
    LCDPutString("Ultra 2000 eCharger");
    GoTo(0,2);
    LCDPutString("Press any key"); 
    while (scanKeyPad()!=1); 
    stateTransition_m(b2); 
} 
/* -----------------------------------------------------
void charging(void)
Action that is fired after used has selected menu option
to charge. It initiates adcChargingSimulation module,
waits for it to finish and interprets energy variable
value, counts sum to be paid in double. If not offline,
sends data packages with consumption and expense to 
server. Also puts charging summary screen template on
LCD and waits user to confirm it. Also, adjusts menu
configuration values so that after charging, charging 
option is not available to choose anymore. After confirmation, 
generates event to get back to menu. 
-----------------------------------------------------*/   
void charging(void){ 
	
	memset(expenseToPayChar, '\0', 8);
	memset(energyStr, '\0', 8);
	
    initCharge(); 
    while(!startCharge()); 
	
    double sum = (double)price * (double)energy;
	
	int sizeofArray1 = sprintf(expenseToPayChar, "%.2f \r\n", sum); 
	int sizeofArray2 = sprintf(energyStr, "%.2f \r\n", energy);
	
    snprintf(expenseToPayChar,sizeofArray1, "%.2f", sum); 
      
    snprintf(energyStr,sizeofArray2, "%.2f", (double)energy ); 
    if (!offline_mode){
        formPacket("02", "01", "86", energyStr); 
        sendStringUSART(formedDataPackageToSend);
		_delay_ms(500); 
        formPacket("02", "01", "87", expenseToPayChar); 
        sendStringUSART(formedDataPackageToSend); 
    } 
    charged = true; 
    stMenuItem = 2; 
    menu_position = 2; 
    defChargMenu = true; 
    lcdClear(); 
    _delay_ms(5); 
      
    GoTo(0,0); 
    LCDPutString("Charging's complete"); 
    lcd_data_write(doneChar); 
    GoTo(0,1); 
    LCDPutString("Energy:  "); 
    LCDPutString(energyStr); 
    LCDPutString(" kWs"); 
    GoTo(0,2); 
    LCDPutString("Charged: "); 
    LCDPutString(expenseToPayChar); 
    LCDPutString(" dkk"); 
    GoTo(0,3); 
    lcd_data_write(arrow); 
    LCDPutString(" Accept"); 
    while(!waitUntilKeyPressed('A')); 
    lcdClear(); 
    _delay_ms(5); 
    stateTransition_m(a1); 
} 
/* -----------------------------------------------------
bool waitUntilKeyPressed(char mkey)
Function that initiates keypad, waits for any key to be
pressed, checks if it is equal to the parameter passed.
If it is same, returns true, else - false.
-----------------------------------------------------*/  
bool waitUntilKeyPressed(char mkey){ 
    char keyPressedm; 
    while (scanKeyPad()!=1); 
    keyPressedm = returnKey(); 
      
    if (keyPressedm == mkey) 
    { 
        return true; 
    }  
    else
    { 
        return false; 
    } 
    return false; 
}
/* -----------------------------------------------------
void getConsumption(void)
Action that is fired after menu option "Consumption" is
pressed. If offline, takes the past consumption and past
expenditure values that were read from RFID card and puts
in LCD template. If online, sends request data packets,
interprets them, and puts values in LCD template. 
Consequently, it adjusts menu configuration values and
generates event to shift back to menu.
-----------------------------------------------------*/ 
void getConsumption(void){ 
    char consumedEnergyStr[10]; 
    char consumedTotal[10]; 
    lcdClear(); 
    _delay_ms(10); 
      
    if (offline_mode) 
    { 
        memset(consumedEnergyStr, '\0', 10); 
        memset(consumedTotal, '\0', 10); 
        memcpy(consumedEnergyStr, pastEnergy_, strlen(pastEnergy_)); 
        memcpy(consumedTotal, pastExpense_, strlen(pastExpense_)); 
          
    }else{ 
        formPacket("02", "01", "61", "SendLastConsumedEnergy"); 
		sendStringUSART(formedDataPackageToSend); 
		
		while(!packageReceived()); 
      
		if (  _command == 62    ) 
		{ 
			memset(consumedEnergyStr, '\0', dl+1); 
			memcpy(consumedEnergyStr, actualData, dl); 
		} 
      
		formPacket("02", "01", "63", "SendLastTotal"); 
		sendStringUSART(formedDataPackageToSend); 
		while(!packageReceived()); 
      
		if (_command == 64  ) 
		{ 
			memset(consumedTotal, '\0', dl+1); 
			memcpy(consumedTotal, actualData, dl); 
		}
    } 
      
      
    GoTo(0,0); 
    LCDPutString("The last consumption"); 
    GoTo(0,1); 
    LCDPutString("Energy:  "); 
    LCDPutString(consumedEnergyStr); 
    LCDPutString(" kWs"); 
    GoTo(0,2); 
    LCDPutString("Charged: "); 
    LCDPutString(consumedTotal); 
    LCDPutString(" dkk"); 
    GoTo(14,3); 
    lcd_data_write(arrow); 
    LCDPutString(" Back"); 
    while(!waitUntilKeyPressed('B')); 
    lcdClear(); 
    _delay_ms(10); 
    if (!charged) 
    { 
        default_menu = true; 
        menu_position = 1; 
    }else{ 
        menu_position = 2; 
        defChargMenu = true; 
    } 
    stateTransition_m(a1); 
} 
/* -----------------------------------------------------
void getBalance(void)
Action that is fired after menu option "Balance"(online) of 
"Deposit"(offline) is pressed. If offline, takes the debt 
value that was read from RFID card and puts in LCD template. 
If online, sends request data packet, interprets it, and puts value 
in LCD template.
Consequently, it adjusts menu configuration values and
generates event to shift back to menu.
-----------------------------------------------------*/  
void getBalance(void){ 
    char balanceStr[10]; 
    lcdClear(); 
    _delay_ms(10); 
    if (offline_mode) 
    { 
        memset(balanceStr, '\0', 10); 
        memcpy(balanceStr, credit_, strlen(credit_)); 
        GoTo(0,0); 
        LCDPutString("Account debit"); 
        GoTo(0,1); 
        LCDPutString(balanceStr); 
        LCDPutString(" Dkk"); 
    }else{ 
        formPacket("02", "01", "66", "SendBalance"); 
    sendStringUSART(formedDataPackageToSend); 
    while(!packageReceived()); 
      
    if (  _command == 67    ) 
    { 
        memset(balanceStr, '\0', dl+1); 
        memcpy(balanceStr, actualData, dl); 
    }
    GoTo(0,0); 
    LCDPutString("Account balance"); 
    GoTo(0,1); 
    LCDPutString(balanceStr); 
    LCDPutString(" Dkk"); 
    } 
      
      
    GoTo(14,3); 
    lcd_data_write(arrow); 
    LCDPutString(" Back"); 
    while(!waitUntilKeyPressed('B')); 
    lcdClear(); 
    _delay_ms(10); 
    if (!charged) 
    { 
        menu_position = 1; 
        default_menu = true; 
    }else{ 
        menu_position = 2; 
        defChargMenu = true; 
    } 
    stateTransition_m(a1); 
} 
/* -----------------------------------------------------
bool arrow_mov(void)
Simple mechanism to move arrow symbol on menu options
and wait for user interaction with keyboard. Returns true
if menu option is chosen and generates appropriate event
so that corresponding action could be fired.
-----------------------------------------------------*/  
bool arrow_mov(void) 
{ 
    int old_menu_position; 
      
    if(default_menu){ 
        default_menu = false; 
        GoTo(0,1); 
        lcd_data_write(arrow); 
        menu_position = 1; 
    } 
    if(charged){ 
        GoTo(0,1); 
        lcd_data_write(doneChar); 
    } 
    if (defChargMenu){ 
        defChargMenu = false; 
        GoTo(0,2); 
        lcd_data_write(arrow); 
    } 
    old_menu_position = menu_position; 
  
    while(scanKeyPad()!=1); 
    char key = returnKey(); 
      
    switch(key){ 
        case 'F'  : 
          
            if (menu_position == stMenuItem){ 
                menu_position = lastMenuItem; 
            }else{ 
                menu_position--; 
            } 
          
            break; 
          
        case 'C'  : 
          
            if (menu_position == lastMenuItem){ 
                menu_position = stMenuItem; 
            }else{ 
                menu_position++; 
            } 
            break;  
        case 'A'  : 
            if (menu_position == 1) 
            { 
                stateTransition_m(c3); 
                return true; 
            }  
            else if (menu_position == 2) 
            { 
                stateTransition_m(d4); 
                return true; 
            } 
            else if (menu_position == 3) 
            { 
                stateTransition_m(e5); 
                return true; 
            } 
            break; 
        case 'B'    : 
            stateTransition_m(zeroevent); 
            return true; 
              
        default :  
            menu_position = 1; 
    } 
    GoTo(0,old_menu_position); 
    lcd_data_write(' '); 
    GoTo(0,menu_position); 
    lcd_data_write(arrow); 
    return false; 
} 
 /* -----------------------------------------------------
void stateTransition_m(event_menu curr)
Function that passes event to "engine" and saves a 
generated event that is passed as a parameter. It is very
handy at testing stages and as generated event is stored, 
let's stimulate state and action change from anywhere from
this module.
 -----------------------------------------------------*/
void stateTransition_m(event_menu curr){ 
    EventOccured_menu = curr; 
    stateEval_menu((event_menu)curr); 
} 
 /* -----------------------------------------------------
void retrieve_price(void)
Method that retrieves price value in both cases. If it is 
offline, it takes fixed price which value is stored in 
OFFLINE_PRICE variable. Else, sends data packet with request
to retrieve online price form server. Latter stored in 
price_str variable and used in menu template as well as
to calculate totals.
 -----------------------------------------------------*/  
void retrieve_price(void){ 
    if (offline_mode) 
    { 
        price = OFFLINE_PRICE; 
        itoa(price, price_str, 10); 
        //snprintf(price_str,2, "%i \r\n", price); 
        stateTransition_m(b2); 
    } 
    else{ 
    formPacket("02","01","50", "SendCurrentPrice"); 
    sendStringUSART(formedDataPackageToSend); 
    while(!packageReceived()); 
    if ( _command == 51   ) 
    { 
        price = atoi(actualData); //retrieve price, conv string to int and assign to the field 
        memset(price_str, '\0', dl);  
        memcpy(price_str, actualData, 2);//init known size string with price value as char 
        stateTransition_m(b2); 
    } 
      
    } 
} 
 /* -----------------------------------------------------
void draw_menu(void)
Method that draws a menu template. Puts the value of price_str
as price and calls arrow_mov function to initiate interactive
menu for user. In case it is offline, instead of balance
check option, there is deposit check option where the 
value is taken from RFID.
 -----------------------------------------------------*/    
void draw_menu(void){ 
    lcdClear(); 
    _delay_ms(10); 
    //Header 
    GoTo(5,0); 
    LCDPutString("Price "); 
    GoTo(11,0); 
    LCDPutString(price_str); 
    GoTo(13,0); 
    LCDPutString("dkk/kWs"); 
      
    //Second line 
    GoTo(1,1); 
    LCDPutString("Charge"); 
  
    //Third line 
    GoTo(1,2); 
    LCDPutString("Consumption"); 
      
    //fourth line 
    GoTo(1,3); 
      
    if (offline_mode) 
    { 
        LCDPutString("Debit");   
    }else{ 
      
    LCDPutString("Balance"); 
    } 
    while(!arrow_mov()); 
}
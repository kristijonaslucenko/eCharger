/*---------------------------------------------------------
Purpose: This is a state machine mechanism using table
driven implementation with function pointers for actions.
The purpose is to determine whether routine follows offline 
or online mode and uses corresponding actions to initiate
user identification. 

State transition table:

State/Event			nilEvent				a event							b event					c event						d event				e event					f event					g event					h event
			
			0    init		noAction    init		Welcome					Session StartSession	KeyPad		KeyPadRead		init	 noAction    init		noAction    init	 noAction		init	noAction		init	 noAction
			1    LCD		noAction    LCD			Welcome					LCD		noAction		LCD			noAction		LCD		 noAction    LCD		noAction    LCD		 noAction		LCD		noAction		LCD		 noAction
			2    RFID		noAction    Terminal	Send					RFID	noAction		RFID		noAction		RFID	 noAction    RFID		noAction    RFID	 noAction		RFID	noAction		RFID	 noAction
			3    KeyPad		noAction    Terminal	Send					KeyPad	noAction		KeyPad		noAction		KeyPad   noAction    KeyPad		noAction    KeyPad	 noAction		KeyPad  noAction		KeyPad	 noAction
			4    Terminal	noAction    Terminal	Receive					KeyPad	KeyPadRead		Terminal	repeatPacket    Session  EndSession  Terminal	Receive		Session  idSessionDone  KeyPad  KeyPadRead		Terminal repeatPacket
			5    Session	noAction    offline		idOfflineWelcome		Session repeatPacket    RFID		RFIDidRead		Session  EndSession  init		Welcome		Session  StartSession   Session idleWaitingf    Session  noAction
			6    offline	noAction    offline		idOfflineGetMifareInfo  offline KeyPadRead		offline		offlinePINcheck offline  noAction    offline	noAction    offline  noAction		offline idSessionDone   offline  noAction
Actions:

void noAction(void);
void Welcome(void);
void Send(void);
void Receive(void);
void KeyPadRead(void);
void RFIDidRead(void);
void StartSession(void);
void EndSession(void);
void repeatPacket(void);
void idSessionDone(void);
void idOfflineWelcome(void);
void idOfflineGetMifareInfo(void);
void offlinePINcheck(void);
void idleWaitingf(void);
		
Input: Many inputs from modules:
"driverLCD.h"
"driverUSART.h"
"driverKeyPad.h"
"driverRFID.h"
"dataReceive.h"
"formPacket.h"

Output: 
extern bool idied;
extern char id_[14];
extern char pastEnergy_[17];
extern char pastExpense_[17];
extern char credit_[17];
extern char pin_s[5];
extern bool offline_mode;
extern int OFFLINE_PRICE;
extern bool credited;
extern bool deleteCreditRFID;

Uses: usual avr libraries such as io.h and interrupt.h etc.

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include "driverLCD.h" 
#include "driverUSART.h" 
#include "driverKeyPad.h" 
#include "driverRFID.h" 
#include "dataReceive.h" 
#include "formPacket.h" 
#include <util/delay.h> 
#include <avr/io.h> 
#include <stdio.h> 
#include <stdint.h> 
#include <math.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <float.h> 
#include <util/delay.h> 
  
typedef enum { 
    init, 
    LCD, 
    RFID, 
    KeyPad, 
    Terminal, 
    Session, 
    offline 
} state; 
  
  
typedef enum { 
    NULLevent, 
    a, 
    b, 
    c, 
    d, 
    e, 
    f, 
    g, 
    h 
} event; 
  
char pin[5]; 
bool pinReceived = false; 
bool rfidIdArrived = false; 
bool requestRepeatPacket = false;    
bool idied = false; 
bool offline_mode = false; 
char arrowf =   0b00000001; 
char doneCharf = 0b00000000; 
int keyPressed = 0; 
bool incorrectPIN = false; 
int OFFLINE_PRICE = 15; //dkk/kWs 
  
  
int back; 
char id_[14]; 
char pastEnergy_[17]; 
char pastExpense_[17]; 
char credit_[17]; 
char pin_s[5]; 
  
typedef void (*action)(); 
  
typedef struct { 
    state nextState; 
    action actionToDo; 
}  stateElement; 
  
/* -----------------------------------------------------
Actions:
-----------------------------------------------------*/ 
  
void noAction(void); 
void Welcome(void); 
void LCDStringBye(void); 
void Send(void); 
void LCDclear(void); 
void Receive(void); 
void KeyPadRead(void); 
void RFIDidRead(void); 
void StartSession(void); 
void EndSession(void); 
void repeatPacket(void); 
void stateTransition(event curr); 
void idSessionDone(void); 
void idOfflineWelcome(void); 
void idOfflineGetMifareInfo(void); 
void offlinePINcheck(void); 
bool waitUntilKeysPressed(char fkey, char fkey1); 
void idleWaitingf(void); 
  
  
// No Action            a1                      b2                      c3              d4              e5                  f6                  g7              h8 
  
stateElement stateMatrix[7][9] = { 
/*0 */{ {init,noAction}, {init,Welcome}, {Session,StartSession} , {KeyPad,KeyPadRead}, {init,noAction}, {init,noAction}, {init, noAction}, {init, noAction}, {init, noAction} }, 
/*1 */{ {LCD,noAction}, {LCD,Welcome}, {LCD, noAction},{LCD,noAction}, {LCD, noAction}, {LCD, noAction}, {LCD, noAction}, {LCD, noAction}, {LCD, noAction} }, 
/*2 */{ {RFID, noAction}, {Terminal, Send}, {RFID, noAction}, {RFID, noAction}, {RFID, noAction}, {RFID, noAction}, {RFID, noAction}, {RFID, noAction}, {RFID, noAction} }, 
/*3 */{ {KeyPad,noAction}, {Terminal,Send}, {KeyPad,noAction}, {KeyPad,noAction}, {KeyPad, noAction}, {KeyPad,noAction}, {KeyPad, noAction}, {KeyPad, noAction}, {KeyPad, noAction} }, 
/*4 */{ {Terminal, noAction}, {Terminal, Receive}, {KeyPad,KeyPadRead}, {Terminal, repeatPacket}, {Session,EndSession}, {Terminal, Receive}, {Session, idSessionDone}, {KeyPad, KeyPadRead}, {Terminal, repeatPacket} }, 
/*5 */{ {Session, noAction}, {offline, idOfflineWelcome}, {Session, repeatPacket}, {RFID, RFIDidRead}, {Session, EndSession}, {init, Welcome}, {Session, StartSession}, {Session, idleWaitingf}, {Session, noAction} }, 
/*6 */{ {offline, noAction}, {offline, idOfflineGetMifareInfo}, {offline, KeyPadRead}, {offline, offlinePINcheck}, {offline, noAction}, {offline, noAction}, {offline, noAction}, {offline, idSessionDone}, {offline, noAction} } 
      
}; 
  
event   EventOccured = a; 
  
void  stateEval(event w); 
  
state   currentState = init; 
/* -----------------------------------------------------
void stateEval(event w)
Function that finds next state and corresponding action
according to the event passed as a parameter and current
state variable.
-----------------------------------------------------*/  
void stateEval(event w) { 
    stateElement stateEvaluation = stateMatrix[currentState][w];    //suranda sekancia state pagal esama state ir ivyki 
    currentState = stateEvaluation.nextState;                       //sauna action'a busimos 
    (*stateEvaluation.actionToDo)(); 
      
} 
 /* -----------------------------------------------------
void offlinePINcheck(void)
Function that takes external array with pin values that 
were read from RFID card and checks with one that is 
entered by user.
 -----------------------------------------------------*/ 
void offlinePINcheck(void){ 
    if ((pinChar[0] == pin[0]) && (pinChar[1] == pin[1]) && (pinChar[2] == pin[2]) && (pinChar[3] == pin[3])) 
    { 
        stateTransition(g); //id session is done 
    }else{ 
        GoTo(0,3); 
        incorrectPIN = true; 
        stateTransition(b); 
    } 
      
} 
 /* -----------------------------------------------------
void idleWaitingf()
Function that is fired on idle state, has simple string
output on LCD and waits for any key pressed. After it's
pressed, fires action that starts session.
 -----------------------------------------------------*/   
void idleWaitingf(){ 
        lcdClear(); 
        _delay_ms(10); 
        GoTo(0,1);
		LCDPutString("Ultra 2000 eCharger"); 
        GoTo(0,2); 
        LCDPutString("Press any key"); 
        while (scanKeyPad()!=1); 
        stateTransition(e); 
} 
 /* -----------------------------------------------------
void idOfflineGetMifareInfo(void)
Function that gets RFID card info such as card id,
debt that is on card, last consumption and last expense 
and pin code. Char arrays are converted to floats in order
to lose any trailing zeros and then stored to char arrays
to be used at later actions and states.
 -----------------------------------------------------*/  
void idOfflineGetMifareInfo(void){ 
     offlineFirstRead = true; 
     rfidDone = false; 
     while(!RFIDinit(1, "64.76", 5)); 
     //convert 16byte data to float 
     double creditD = atof (debtChar); 
     double pastEnergyD = atof (pastEnChar); 
     double pastExpenseD = atof (pastExChar); 
     //convert float to string so zeros are lost ant only float number is stored 
    snprintf(credit_,8, "%.2f \r\n", creditD); 
    snprintf(pastEnergy_,8, "%.2f \r\n", pastEnergyD); 
    snprintf(pastExpense_,8, "%.2f \r\n", pastExpenseD); 
      
    stateTransition(b); 
} 
 /* -----------------------------------------------------
void idOfflineWelcome(void)
Simple function that initiates welcome screen whenever
there is evidence of lost connection with server and 
offline mode is on.
 -----------------------------------------------------*/    
void idOfflineWelcome(void){ 
        lcdClear(); 
        _delay_ms(3); 
        LCDPutString("Ultra 2000 eCharge"); 
        GoTo(0,1); 
        LCDPutString("System's offline"); 
        GoTo(0,2); 
        LCDPutString("Use your RFID card"); 
        stateTransition(a); 
      
}
 /* -----------------------------------------------------
bool id(void) 
This is an "engine" function of this mechanism. It returns 
true when id process is done, offline or online mode.
Also initiates USART, LCD and keypad modules. Stimulates
actions and states according to generated events.
 -----------------------------------------------------*/     
bool id(void) 
{ 
    USART_Init(64); 
    lcd_init(); 
    sei(); 
    keypad_init(); 
      
    while(!idied){ 
    stateTransition(EventOccured); 
    } 
    return true; 
// 
//stateTransition(a); 
} 
 /* -----------------------------------------------------
void idSessionDone()
Function that clears the screen and sets idied flag true.
The mechanism above stops and returns true for inheriting
module to flag id process done.
 -----------------------------------------------------*/   
void idSessionDone(){ 
    lcdClear(); 
    _delay_us(5); 
    idied = true; 
} 
 /* -----------------------------------------------------
void stateTransition(event curr)
Function that passes event to "engine" and saves a 
generated event that is passed as a parameter. It is very
handy at testing stages and as generated event is stored, 
let's stimulate state and action change from anywhere from
this module.
 -----------------------------------------------------*/   
void stateTransition(event curr){ 
    EventOccured = curr; 
    stateEval((event)curr); 
} 
 /* -----------------------------------------------------
bool waitUntilKeysPressed(char fkey, char fkey1)
Function that gets two chars, ASCII representation of keys
and waits for one of them to be pressed, stores 1 in global
variable if first key is pressed, 2 if second pressed.
 -----------------------------------------------------*/    
bool waitUntilKeysPressed(char fkey, char fkey1){ 
    char keyPressedf; 
    while (scanKeyPad()!=1); 
    keyPressedf = returnKey(); 
      
    if (keyPressedf == fkey) 
    { 
        keyPressed = 1; 
        return true; 
    } 
    else if(keyPressedf == fkey1) 
    { 
        keyPressed = 2; 
        return true; 
    } 
    return false; 
} 
 /* -----------------------------------------------------
void repeatPacket(void)
Function that sends request to repeat last packet if 
receiving mechanism failed to interpret it.
 -----------------------------------------------------*/    
void repeatPacket(void){ 
    formPacket("02", "01", "09", "RepeatLastPacket"); 
    sendStringUSART(formedDataPackageToSend); 
    if ((back == 1)) 
    { 
        back = 0; 
        stateTransition(f); //start session 
    }  
    else
    { 
        stateTransition(a); 
    } 
} 
 /* -----------------------------------------------------
void StartSession(void)
Function that initiates session start with server by sending
data package. If answered command is 23, it is online mode,
if server is off and sent command is 24 - it is offline mode
and appropriate indications are put on LCD.
 -----------------------------------------------------*/      
void StartSession(void){ 
    formPacket("02", "01", "22", "StartSession"); 
    sendStringUSART(formedDataPackageToSend); 
    while(!packageReceived()); 
      
    if (_command == 23) 
    { 
        GoTo(0,2); 
        LCDPutString("Connected");//To RFID 
        stateTransition(c); 
    } 
      
    else if (_command == 24) 
    { 
        lcdClear(); 
        _delay_ms(5); 
        GoTo(0,0); 
        LCDPutString("No connection"); 
        GoTo(0,1); 
        _delay_us(10); 
        LCDPutString("Offline, fixed price"); 
        GoTo(0,2); 
        char fixedPrice[3]; 
        itoa(OFFLINE_PRICE, fixedPrice, 10); 
        LCDPutString(fixedPrice); 
        LCDPutString("dkk/kWs"); 
        GoTo(0,3); 
        lcd_data_write(doneCharf); 
        LCDPutString("Accept        "); 
        lcd_data_write(arrowf); 
        LCDPutString("Back"); 
        while(!waitUntilKeysPressed('A', 'B')); 
        offline_mode = true; 
        _delay_ms(3000); 
        //To RFID, fixed price, offline mode 
        if (keyPressed == 1) 
        { 
            stateTransition(a); 
        }else if (keyPressed == 2) 
        { 
            stateTransition(d); 
        } 
          
    } 
    else
    { 
        //Repead data packet 
        stateTransition(b); 
        back = 1; 
    } 
} 
 /* -----------------------------------------------------
void EndSession(void)
Function that sends data package to server to initiate
end of session and shifts to idle state.
 -----------------------------------------------------*/     
void EndSession(void){ 
    formPacket("02", "01", "99", "EndofSession"); 
    sendStringUSART(formedDataPackageToSend); 
    //Welcome 
    stateTransition(g); 
} 
  
void noAction(void){ 
  
    } 
 /* -----------------------------------------------------
void Welcome(void)
Welcome screen function that puts a couple of text lines
on welcome screen and generates event to shift states.
 -----------------------------------------------------*/   
void Welcome(void){ 
    lcdClear(); 
    _delay_ms(5); 
    LCDPutString("Ultra 2000 eCharge"); 
    GoTo(0,1); 
    LCDPutString("Use your RFID card"); 
    stateTransition(b); 
} 
void LCDStringBye(void){ 
      
    LCDPutString("Good Bye"); 
} 
void LCDclear(void){ 
      
    lcdClear(); 
}
 /* -----------------------------------------------------
void KeyPadRead(void)
Method that reads pin code from keypad input. Has simple
graphics, stores entered pin code in char array and works
for both modes - offline and online. After pin is read,
corresponding event is generated and appropriate actions
are fired - to check pin code.
 -----------------------------------------------------*/    
void KeyPadRead(void){ 
    //sendStringUSART("KeypadRead\n"); 
        int i = 0; 
        lcdClear(); 
        _delay_ms(5); 
        GoTo(0,0); 
        LCDPutString("Enter PIN:"); 
        if (incorrectPIN) 
        { 
            GoTo(0,3); 
            LCDPutString("Incorrect PIN"); 
        } 
          
        memset(pin, '\0', 5); 
          
        while (i!=4) 
        { 
            if ((scanKeyPad()==1) && (i!=4)){ 
                GoTo(i,1); 
                char jj = returnKey(); 
                pin[i] = jj; 
                lcd_data_write(pin[i]); 
                _delay_ms(100); 
                GoTo(i,1); 
                lcd_data_write('*'); 
                i++; 
            } 
        } 
        i=0; 
        pinReceived = true; 
        if (currentState == offline) 
        { 
            stateTransition(c); 
        }else{ 
        stateTransition(a);  
        } 
  
} 
 /* -----------------------------------------------------
void Send(void)
Method that is responsible to send data packets according
to the events that happened. If rfidIdArrived flag is true,
it sends packet with RFID card id, if pinReceived flag is
up, sends packet with pin code to be checked.
 -----------------------------------------------------*/  
void Send(void){ 
  
    if (rfidIdArrived) 
    { 
        /* double creditgg = atof (debtChar); 
         //convert float to string so zeros are lost ant only float number is stored 
         if(snprintf(credit_,8, "%.2f \r\n", creditgg)){ 
             sendStringUSART("detectedcredit"); 
         }*/
        formPacket("02", "01", "10", RfidBufferToRead); 
        sendStringUSART(formedDataPackageToSend); 
        rfidIdArrived = false; 
        //To Receive 
        stateTransition(a); 
    }  
    if (pinReceived) 
    { 
        pinReceived = false; 
        GoTo(0,2); 
        LCDPutString("PIN is being checked"); 
      
        formPacket("02", "01", "00", pin); 
        sendStringUSART(formedDataPackageToSend); 
          
        stateTransition(e); 
    } 
}
 /* -----------------------------------------------------
void Receive(void)
Method that is responsible for received package interpretation
and appropriate event generation. If command is 1,
it means that pin code is true and id session is done,
if command is 2, pin code is incorrect and it keypad is 
initiated again, if it is 3, pin has been entered three
times incorrectly and session is done. If command is 11,
RFID is authorized and pin code is to be entered, if 
command is 12, RFID card is unknown and it ends session.
If any other command, it is not interpreted and action
that asks to repeat data is fired.
 -----------------------------------------------------*/   
void Receive(void){ 
    //sendStringUSART("USARTreceive\n"); 
      
    while(!packageReceived()); 
      
    if ( _command == 1) 
    { 
        GoTo(0,3); 
        LCDPutString("PIN is OK"); 
        _delay_ms(2000); 
        // End Session 
        stateTransition(f); 
    }  
    else if (  _command == 2) 
    { 
        GoTo(0,3); 
        LCDPutString("Incorrect PIN"); 
        _delay_ms(2000); 
        //To Keypad 
        stateTransition(g); 
    } 
      
    else if (  _command == 3) 
    { 
        lcdClear(); 
        _delay_ms(5); 
        GoTo(0,0); 
        LCDPutString("Access is blocked"); 
        GoTo(0,2); 
        LCDPutString("Wrong PIN input 3x"); 
        _delay_ms(1000); 
        //End Session 
        stateTransition(f); 
    } 
      
    else if (_command == 11) 
    { 
        //To keypad 
        stateTransition(b); 
    }  
    else if ( _command == 12) 
    { 
        GoTo(0,3); 
        LCDPutString("Unknown card"); 
        _delay_ms(1000); 
        //End Session 
        stateTransition(d); 
    } 
    else{ 
        //Repeat data 
        stateTransition(c); 
    } 
}
/* -----------------------------------------------------
void RFIDidRead(void)
Action that is fired while in online mode and reads RFID 
id and debt if there was one.
 -----------------------------------------------------*/    
void RFIDidRead(void) 
{ 
    //sendStringUSART("RFID \n"); 
    GoTo(0,3); 
    onlineFirstREad = true; 
    while (!RFIDinit(1, "uid", 3));  
    rfidDone = false; 
      
    rfidIdArrived = true; 
    LCDPutString("Wait"); 
    //To Send 
    stateTransition(a); 
} 
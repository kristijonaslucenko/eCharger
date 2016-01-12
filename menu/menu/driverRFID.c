/*---------------------------------------------------------
Purpose: The purpose of this module is to interface RFID reader
with micro controller. It has three main modes that are designed
accordingly to the main purpose. Those are OfflineFirstReading,
OfflineWriting and OnlineFirstReading. Basically it is a state 
machine with a three different routines to read or write from
to RFID card.

State transition table:

cur. St/ Event			nilEvent			cardEvent				cardDatareadyEvent			startTransmitEvent


idle				idle, nilAction		commanding,send_command		reading,nilAction			idle,nilAction
commanding			idle, nilAction		wait_data,nilAction			reading,nilAction			idle,nilAction
wait_data			idle, nilAction		wait_data,nilAction			reading,nilAction			idle,nilAction
reading				idle, nilAction		wait_data,nilAction			reading,readBuffer			presenting,nilAction
presenting			idle, nilAction		wait_RFID_removed,nilAction	reading,nilAction			presenting,transmitString
wait_RFID_removed	idle, nilAction		wait_RFID_removed,nilAction	wait_RFID_removed,nilAction	wait_RFID_removed,nilAction

Actions
nilAction
send_command
readBuffer
transmitString

Input: Input comes RFID reader through SPI serial comm. Also,
it uses two external interrupt routines to detect card entrance
and flag data availability.
Other soft input comes from inheriting module and set in external
global variables:
extern bool offlineFirstRead	-- set in case of this mode
extern bool offlineWrite;		-- set in case of this mode
extern bool onlineFirstREad;	-- set in case of this mode
extern int writeAction;			-- set in case of offlineWrite action

Output: 
case onlineFirstREad:
extern char RfidBufferToRead[MAX];	--card id in ASCII representation
extern char debtChar[17];			--debit if there was any

case onlineFirstREad:

extern char RfidBufferToRead[MAX];	--card id in ASCII representation
extern char pinChar[5];				--PIN code in ASCII
extern char debtChar[17];			--debit if there was one
extern char pastEnChar[17];			--past consumption if there was one
extern char pastExChar[17];			--past total if there was one


Uses: Self-sufficient, USART driver is here for testing purposes

Author: Ultra 2000, foundation by Ole Shultz 
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include "driverSPI.h"
#include "driverUSART.h"
#define  F_CPU 10000000L
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX 14  //ascii representation for the 7 bytes id
					
typedef enum {
	nilEvent,
	cardEvent,
	cardDatareadyEvent,
	startTransmitEvent	
	}eventRFID;

typedef enum {
	idle,
	commanding,	
	wait_data,			
	reading,				
	presenting,		
	wait_RFID_removed
	}stateRFID;

typedef void (*action)();
void stateEvalRFID(eventRFID e);

void nilAction();
void sendCommand();
void readBuffer();
void transmitString();


typedef struct {
	stateRFID nextstateRFID;
	action actionToDoRFID;
} stateElementRFID;

																														   
stateElementRFID stateMatrixRFID[6][4] ={{{idle, nilAction} ,{	commanding,sendCommand} ,	{	reading,nilAction},			{idle,nilAction}},
								{{idle, nilAction},{	wait_data,nilAction	},	{		reading,nilAction},			{idle,nilAction}},
								{{idle, nilAction},{	wait_data,nilAction	},	{		reading,nilAction},			{idle,nilAction}},
								{{idle, nilAction},{	wait_data,nilAction  },	{		reading,readBuffer},		{presenting,nilAction }},
								{{idle, nilAction},{	wait_RFID_removed,nilAction},{	reading,nilAction},			{presenting,transmitString}},
								{{idle, nilAction},{	commanding,sendCommand},{	wait_RFID_removed,nilAction},{wait_RFID_removed,nilAction}}};
char RfidBufferToRead[MAX];
char superBuffer[100];
int index_ = 0;
volatile int i=MAX;
//global flags to guide state machine 
bool uid = false;
bool pinB = false;
bool debt = false;
bool writeCredit = false;
bool deleteCredit = false;
bool pastConsB = false;
bool pastExpB = false;

bool offlineFirstRead = false;
bool offlineWrite = false;
bool onlineFirstREad = false;
bool creditDetected = false;
//Global variables to store read values
int pl = 0;
char pinChar[5];
char debtChar[17];
char pastEnChar[17];
char pastExChar[17];
int writeAction = 1;
char parammeter[100];

volatile char data_ready=0;
volatile char RFID_present=0;
volatile char bufferRead=0;
bool rfidDone = false;

eventRFID eventOccuredRFID=nilEvent;
stateRFID currentStateRFID=idle;
void OfflineFirstReading();
void OfflineWriting();
void OnlineFirstReading();

/* -----------------------------------------------------
void initCharge(void)
Initializes charging simulation by initializing other
modules and drawing graphical template.
-----------------------------------------------------*/
void init ()
{
	DDRB |= (1<< DDB0);
	USART_Init(64);
	GICR |=(1<<INT0)|(1<<INT1)|(1<<INT1);//
	MCUCR|=(1<<ISC00)|(1<<ISC10); //(1<<ISC11)|(1<<ISC01)| rising edge and falling edge int0 card present/removed
	asm("SEI");
}
/* -----------------------------------------------------
ISR(INT1_vect)
Uses external interrupt to shift between states and determine
an appropriate event. This routine determines whenever card data 
is ready, on sequential initiation it starts the command transmission.
-----------------------------------------------------*/
ISR(INT1_vect) {
static char togle=0;
	if(togle==0) {
		eventOccuredRFID=cardDatareadyEvent;
		togle=1;
	}
	else {
		eventOccuredRFID=startTransmitEvent;
		togle=0;
	}
}
/* -----------------------------------------------------
ISR(INT0_vect)
Uses external interrupt to shift between states and determine
an appropriate event. This routine determines whenever card is
put, on sequential initiation it goes to idle state.
-----------------------------------------------------*/
ISR(INT0_vect) {
static char togle=0;
   
   if (togle==0){
	eventOccuredRFID=cardEvent;
	 togle=1;
   }
	 else {
		 togle=0;
		 eventOccuredRFID=nilEvent;
	 }
	
}
/* -----------------------------------------------------
bool RFIDinit(int command, char _parammeter[], int sizeOfPar)
This function determines the mode that desired, passes parameter
in case it is offlineWrite and parameter size. It also initiates
serial communication with RFID reader as well as sets up interface
and stimulates event change.
-----------------------------------------------------*/
bool RFIDinit(int command, char _parammeter[], int sizeOfPar)
{
	pl = sizeOfPar;
	memset(parammeter, '\0', sizeOfPar+1);
	memcpy(parammeter, _parammeter, sizeOfPar);
	memset(pinChar, '\0', 5);
	memset(superBuffer, '\0', 100);
	
	switch(command){
		case 1 :
			uid = true;
		break;
		
		case 2 :
			debt = true;
		break;
		
		case 3 :
			pinB = true;
			
		break;
		
		case 4 :
			deleteCredit = true;
		break;
		
		case 5 :
			writeCredit = true;
		break;
		
		case 6 :
			pastConsB = true;
		break;
		
		case 7 :
			pastExpB = true;
		break;
		
	}
	 init();
	 SPIinit();
	

while (!rfidDone)
{
	  stateEvalRFID(eventOccuredRFID);
}	
return true;	   
}
/* -----------------------------------------------------
void stateEvalRFID(eventRFID e)
This function is being passed an event as a parameter and 
determines a state and action according to the a new event
and current state. 
-----------------------------------------------------*/
void stateEvalRFID(eventRFID e){
	 stateElementRFID stateEvaluationRFID = stateMatrixRFID[currentStateRFID][e];
	 currentStateRFID=stateEvaluationRFID.nextstateRFID;
	 (*stateEvaluationRFID.actionToDoRFID)();
	 
	 
 }
 
 void nilAction(){
};
/* -----------------------------------------------------
void sendCommand()
This function sends commands and parameters to the RFID 
reader through SPI according to the flags that are set.
-----------------------------------------------------*/	 
 void sendCommand(){
	
if (uid)
{
	SPItransmit(0x55);
}

if (pinB)
{
	SPItransmit(0x52);
	SPItransmit(0x01);
	SPItransmit(0x01);
}
if (debt)
{
	SPItransmit(0x52);
	SPItransmit(0x02);
	SPItransmit(0x01);
}
if (pastExpB)
{
	SPItransmit(0x52);
	SPItransmit(0x04);
	SPItransmit(0x01);
}
if (pastConsB)
{
	SPItransmit(0x52);
	SPItransmit(0x05);
	SPItransmit(0x01);
}
if (writeCredit)
{
	char blockAddress = 0x02;
	if (writeAction == 1)
	{
		blockAddress = 0x02; //debt
		memset(parammeter, '\0', 17);
		memcpy(parammeter, debtChar, 16);
		//usart_transmit('1');
	}else if (writeAction == 2)
	{
		blockAddress = 0x05; //exp
		memset(parammeter, '\0', 17);
		memcpy(parammeter, pastEnChar, 16);
		//usart_transmit('2');
	}else if (writeAction == 3)
	{
		blockAddress = 0x04; //cons
		memset(parammeter, '\0', 17);
		memcpy(parammeter, pastExChar, 16);
		//usart_transmit('3');
	}
	SPItransmit(0x57);
	SPItransmit(blockAddress);
	SPItransmit(0x01);
	
	for (int i = 0; i < 16; i++)
	{
		SPItransmit(parammeter[i]);
	}
}
if (deleteCredit)
{
	SPItransmit(0x57);
	SPItransmit(0x02);
	SPItransmit(0x01);
	
	for (i = 0; i < 6; i++)
	{
		SPItransmit('0');
	}

}
}
/* -----------------------------------------------------
void SPItransmit_(unsigned char byte)
This is simple function to shift bytes through SPI. It receives
char byte as a parameter, shifts it to RFID and puts received
one to the superBuffer array. Acts as a usual buffer.
-----------------------------------------------------*/
 void SPItransmit_(unsigned char byte)
 {
	 SPDR = byte;					//Load byte to Data register
	 while(!(SPSR & (1<<SPIF))); 	// Wait for transmission complete
	 char dd = SPDR;
	 superBuffer[index_] = dd;
// 	 usart_transmit('f');
// 	 usart_transmit(dd);
	 index_++;
 }
/* -----------------------------------------------------
void readBuffer()
Function that reads the buffer of data RFID reader has
sent through SPI. It has two cases, one when boolean uid
is set true and second when it's not. First case is always
true for card id reading and converts id bit value to ASCII
representation and stores it to RfidBufferToRead array. On
second case it calls SPItransmit_ where register is shift
and bytes are stored to superBuffer array.
-----------------------------------------------------*/ 
 void readBuffer(){ 
	 if (uid)
	 {
			 _delay_ms(2);
			 SPItransmit(0xF5);
			 char data = SPDR;
			 char HEXbuffer[2];
			 sprintf(HEXbuffer,"%02X", data);
			 //RfidBufferToRead[15]='\0';
			 RfidBufferToRead[i]=HEXbuffer[1];
			 i--;
			 RfidBufferToRead[i]=HEXbuffer[0];
			 i--;
	 }else{
			_delay_ms(2);
			SPItransmit_(0xF5);
			//usart_transmit('r');
	 }
}
/* -----------------------------------------------------
 void transmitString()
This function interprets logical flags and calls 
corresponding functions to interpret buffer value.
Logical flags correspond to three modes this mechanism
has. 
offlineFirstRead ->OfflineFirstReading()
offlineWrite->OfflineWriting()
onlineFirstREad->OnlineFirstReading()
-----------------------------------------------------*/
 void transmitString(){
	
	memset(parammeter, '\0', 100);
	pl = 0;
	i = MAX;
	index_ = 0;
	bufferRead=0;
	
	if (offlineFirstRead)
	{
		OfflineFirstReading();
	}
	else if (offlineWrite)
	{
		OfflineWriting();
		
	}else if(onlineFirstREad)
	{
		OnlineFirstReading();
		
	}else if(deleteCredit){
		
		rfidDone = true;
		deleteCredit = false;
		//putString("deletedCredit");
		eventOccuredRFID=nilEvent;
		
	}else{
		eventOccuredRFID=nilEvent;
	}
 }
 /* -----------------------------------------------------
 void OnlineFirstReading(void)
This function interprets and stores the values from RFID
reader buffer according to the logical sequence. First it 
catches card id and stores it to RfidBufferToRead. Then it 
sets debt flag true and stores buffer value to debtChar.
-----------------------------------------------------*/
 void OnlineFirstReading(void){
	/* if (onlineFirstREad && deleteCredit)
	 {
		 deleteCredit = false;
		 rfidDone = true;
		 //creditDetected = true;
		 onlineFirstREad = false;
		 putString("deleted");
		 eventOccuredRFID=nilEvent;
	 }*/
	 
	 if (onlineFirstREad && debt)
	 {
		 rfidDone = true;
		 deleteCredit = true;
		 debt = false;
		 //creditDetected = true;
		 onlineFirstREad = true;
		 memcpy(debtChar, superBuffer + 1, 16);
		 memset(superBuffer, '\0', 100);
		 eventOccuredRFID=nilEvent;
	 }
	 
	 
	 if (onlineFirstREad && uid)
	 {
		uid = false;
		debt = true;
		onlineFirstREad = true;
		RfidBufferToRead[16]='\0';
		//putString(RfidBufferToRead);
		//putString("\nfirstonline\n");
		eventOccuredRFID=cardEvent;
	 }
	
	 
 }
  /* -----------------------------------------------------
 void OfflineWriting()(void)
This function sets writing flags in order to write parameters
to the RFID card at consecutive states and events. By default
first write action is to write debt, then it automatically sets
flag to write last consumption and then - last expense.
-----------------------------------------------------*/
 void OfflineWriting(){
	  if (offlineWrite && writeCredit && writeAction == 3)
	  {
		  rfidDone = true;
		  offlineWrite = false;
		  writeCredit = false;
		  //putString("\ndoneOfflinewrite\n");
		  eventOccuredRFID=nilEvent;
	  }
	  
	   if (offlineWrite && writeCredit && writeAction == 2)
	   {
		   writeCredit = true;
		   writeAction = 3;
		   
		   eventOccuredRFID=cardEvent;
	   }
	   
	    if (offlineWrite && writeCredit && writeAction == 1)
	    {
		    writeCredit = true;
			writeAction = 2;
		    
		    eventOccuredRFID=cardEvent;
	    }
 }
  /* -----------------------------------------------------
void OfflineFirstReading(void)
This function interprets and stores the values from RFID
reader buffer according to the logical sequence. First it 
catches card id and stores it to RfidBufferToRead. Then it 
sets debt flag true and stores buffer value to debtChar.
Consecutively it performs same action for storing pin, 
past expenditure and past expense values.
-----------------------------------------------------*/
 void OfflineFirstReading(){
	 
	 if (offlineFirstRead && pastExpB)
	 {
		 rfidDone = true;
		 pastConsB = false;
		 pastExpB = false;
		 offlineFirstRead = false;
		 memcpy(pastExChar, superBuffer + 1, 16);
		// putString(pastExChar);
		//  putString("\ndoneoffline\n");
		 memset(superBuffer, '\0', 100);
		 eventOccuredRFID=nilEvent;
	 }
	 
	 if (offlineFirstRead && pastConsB)
	 {
		 pastConsB = false;
		 pastExpB = true;
		 memcpy(pastEnChar, superBuffer + 1, 16);
	//	 putString(pastEnChar);
		 memset(superBuffer, '\0', 100);
		 eventOccuredRFID=cardEvent;
	 }
	 
	 if (offlineFirstRead && debt)
	 {
		 rfidDone = false;
		 debt = false;
		 pastConsB = true;
		 memcpy(debtChar, superBuffer + 1, 16);
	//	 putString(debtChar);
		 memset(superBuffer, '\0', 100);
		 eventOccuredRFID=cardEvent;
	 }
	 
	 
	 if (offlineFirstRead && pinB)
	 {
		 rfidDone = false;
		 pinB = false;
		 debt = true;
		 memcpy(pinChar, superBuffer + 1, 4);
	//	 putString(pinChar);
		 memset(superBuffer, '\0', 100);
		 eventOccuredRFID=cardEvent;
	 }
	 
	 if (offlineFirstRead && uid)
	 {
		 uid = false;
		 pinB = true;
		 RfidBufferToRead[16]='\0';
	//	 putString(RfidBufferToRead);
		 eventOccuredRFID=cardEvent;
	 }
 }
 

/*---------------------------------------------------------
Purpose: The purpose of this module is to interface a LCD
to controller. In short, this is a LCD driver. 

Input: Most of the functions are being passed some variables
as this project mostly writes to LCD and does not read.

Output: See the specific outputs of specific functions

Uses: uses it's header file for stored and defined values

Author: Ole Shultz, edited by Ultra 2000
Company: DTU Dipom
Version: 1.01
Date and year: 2014/05/26
-----------------------------------------------------*/

#include <avr/io.h>    
#include <avr/interrupt.h>
#include <stdio.h> 


#include "driverLCD.h"  // File with #define statements and prototypes for the lcdm.c module


//#define FUNCTION_SET  0x28  // etc......
#define OFFSET        0x40   //2nd line
#define lcd_RS 2
//#define lcd_RW 1
#define lcd_E 3
//#define lcd_BUSY 7




char disp_buffer[32];
unsigned char indx=0;
unsigned char inx=0;
unsigned char tmp;
unsigned char lcd_x=0;
unsigned char lcd_y=0;
unsigned char lcd_maxx=16;


//*******************************************************************
// LCD initialization sequence (works somewhat like a constructor)
//*******************************************************************

void lcd_init()    // Works like a constructor
   
   {

   // Power on delay
	lcd_direction |= 0xfc;							//	set port a as output
   lcd_wait( 20000 );                                   // Power on wait 
   lcd_wait( 20000 );
	lcd_port &= ~((1<<lcd_E) | (1<<lcd_RS)); // EN=0, RS=0
//	lcd_port &= ~(1<<lcd_RW);               // Set RW = 0 in case it is connected
 
   lcd_nibble_transfer(SET_FUNCTION+IN8_BIT);
   //lcd_wait(longdelay );  						 // wait 24 ms
   _delay_ms(24);
   lcd_nibble_transfer(SET_FUNCTION+IN8_BIT);
   //lcd_wait(longdelay);
    _delay_ms(24);
	lcd_nibble_transfer(SET_FUNCTION+IN8_BIT);
   //lcd_wait(longdelay);
    _delay_ms(24);
	lcd_nibble_transfer(SET_FUNCTION);
  // lcd_wait(longdelay);
   _delay_ms(24);
   lcd_cmd_write(SET_FUNCTION+LN2_BIT);                 // Function set: 0x28  2 lines
  // lcd_wait(WAIT_15m);                              // Wait 39 uS
 _delay_ms(2);
    lcd_cmd_write(SET_DISPLAY);						//display off
 //lcd_wait(WAIT_15m);
_delay_ms(2);	
	lcd_cmd_write(CLR_DISPLAY);       // Display clear: 0x01 clear data                 	 
	
  // lcd_wait(WAIT_15m);                // 1.53 mS  
     _delay_ms(2);                  
                               

   lcd_cmd_write(SET_ENTRY_MODE+INC_BIT);          //  +INC_BIT  // Entry mode set: shift cursor 1 position to right
  //   lcd_wait(WAIT_15m);      
  _delay_ms(2);
    lcd_cmd_write(SET_DISPLAY+ON_BIT+CUR_BIT+BLK_BIT); //+CUR_BIT+BLK_BIT);   Display ON/OFF control: 0x0f
  //   lcd_wait(WAIT_15m);                               // Wait 39 uS                      // 1.53 mS     

  //lcd_wait(WAIT_15m);                            // 1.53 mS  
_delay_ms(2);
  createCustomFont();
 _delay_ms(2);
 lcd_cmd_write(RTN_HOME);
   } // end lcd_init()
/* -----------------------------------------------------
void createCustomFont()
Writes a couple of custom made fonts to CGRAM that later
are being used.
-----------------------------------------------------*/

void createCustomFont(){
	lcd_cmd_write(0x40); //CGRAM
	
	_delay_ms(4);
	lcd_data_write(0);
	_delay_ms(2);
	lcd_data_write(1);
	_delay_ms(2);
	lcd_data_write(3);
	_delay_ms(2);
	lcd_data_write(22);
	_delay_ms(2);
	lcd_data_write(28);
	_delay_ms(2);
	lcd_data_write(8);
	_delay_ms(2);
	lcd_data_write(0);
	_delay_ms(2);
	lcd_data_write(0);
	_delay_ms(4);

	lcd_data_write(8);
	_delay_ms(2);
	lcd_data_write(12);
	_delay_ms(2);
	lcd_data_write(10);
	_delay_ms(2);
	lcd_data_write(9);
	_delay_ms(2);
	lcd_data_write(10);
	_delay_ms(2);
	lcd_data_write(12);
	_delay_ms(2);
	lcd_data_write(8);
	_delay_ms(2);
	lcd_data_write(0);
	_delay_ms(4);

// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(2);
// 	lcd_data_write(0x1c);
// 	_delay_ms(4);
// 
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(2);
// 	lcd_data_write(0x1e);
// 	_delay_ms(10);

	lcd_data_write(0x80); //DD RAM	
	
}
//******************************************************************************************
// Medium level functions
// Select RS / RW mode and call lower level funtion to complete the transfer

void lcd_cmd_write(unsigned char cmd)
   { 
    lcd_direction |= 0xfc;
	lcd_port &= ~(1<<lcd_RS);

	asm volatile("NOP");  // Slow down timing 100 nS
	asm volatile("NOP");   // Slow down timing 100 nS
	lcd_transfer(cmd);
   } 


void lcd_data_write(unsigned char d)
{
   lcd_direction |= 0xfc;
   lcd_port|=(1<<lcd_RS);				//rs=1 when writing data
   

    asm volatile("NOP");   // Slow down timing 100 nS
   	asm volatile("NOP");   // Slow down timing 100 nS

   lcd_transfer(d); 
} 


//********************************************************************************************
// Low level functions
// Write to the lcd data bus - generate E pulse 

void lcd_transfer (unsigned char d)
{
	lcd_port|= (1<<lcd_E);
	asm volatile("NOP");  // Slow down timing 100 nS
    asm volatile("NOP"); // Slow down timing 100 nS
	lcd_nibble_transfer(d);   //(msn)
  
	asm volatile("NOP");   // Slow down timing 100 nS
	lcd_port |= (1<<lcd_E);

	lcd_nibble_transfer(d<<4);   //lsn);
 
} 


void lcd_nibble_transfer( unsigned char d )  
{ 
    lcd_port|= (1<<lcd_E);
	asm volatile("NOP");   // Slow down timing 100 nS
   	lcd_port= (lcd_port & 0x0f)| (d &0xf0);   //(always msn as 4 bit data bus)
  
  	
	_delay_us(60);
	lcd_port &= ~(1<<lcd_E);
	_delay_us(60);
   } 

//clear the display
void lcdClear(void) {
	lcd_cmd_write(CLR_DISPLAY);
	lcd_wait(WAIT_15m);
	lcd_cmd_write(CLR_DISPLAY+RTN_HOME);
	lcd_wait(WAIT_15m);
}

void clearLine(unsigned char x, unsigned char y){

	unsigned char max_x=20;

	GoTo(x,y);
	for (int i=x; i<max_x; i++)
	{
		lcd_data_write(0x20);
		_delay_ms(1);
	}
	GoTo(x,y);
}

// Software function for delay insertion after commands/data tranfers

void lcd_wait(unsigned int count)
{ 
   unsigned int i;

   for ( i = 0 ; i < count ; i++ );    //4 cycles per count
 
}


//! write a zero-terminated ASCII string to the display
void LCDPutString(char *str) {
   char c,index=0;
	for (; (c = *str) != 0; str++){
	
		if((c=='\r') || c=='\n');
		else
		lcd_data_write(c);

		index++;

		if (index>=20) {
			lcd_cmd_write(SET_DRAM_ADDR+line_3);
			lcd_wait(WAIT_15m);
			index=0;
		}

	}
}

//*goto x-position and y-line called by parameters x, y used in main() and internally LCDPutChar()*/

void GoTo(unsigned char x, unsigned char y){
	switch (y) {
		case 0: 

		lcd_cmd_write(SET_DRAM_ADDR+x);
		lcd_wait(WAIT_15m);  
		break;

		case 1: 

		lcd_cmd_write(SET_DRAM_ADDR+line_1+x);
		lcd_wait(WAIT_15m);  
		break;

		case 2:
		lcd_cmd_write(SET_DRAM_ADDR+line_2+x);
		lcd_wait(WAIT_15m);  
		break;

		case 3:
		lcd_cmd_write(SET_DRAM_ADDR+line_3+x);
		lcd_wait(WAIT_15m);  
		break;
		
		default:
		break;
	}

}


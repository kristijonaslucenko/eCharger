/*---------------------------------------------------------
Purpose: This is a driver for keypad. It main purpose is to
correctly interpret values of decoder and encoder.

Input: Input comes from decoder cd4532b and encoder cd4028b.

Output: Function scanKeyPad() scans a keypad and 
returns 1 if any key is pressed.
Function returnKey() returns a ASCII value of 
corresponding key that was recently pressed.

Uses: Self-sufficent

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

int count;
int keyfound;
char key;
char temp;

enum{idle, keyPresses, debounce, getKey, waitRelease, releasedDebounce,released}state1;

char key_table[4][4]=  
{{'1','2','3','F'},
{'4','5','6','E'},
{'7','8','9','D'},
{'A','0','B','C'}};
	
/* -----------------------------------------------------
void setRow(int cnt)
This function sets a row to be checked for key press.
Four button rows, four cases. Parameter is int cnt, that 
is passed from main state machine.
-----------------------------------------------------*/	
void setRow(int cnt){
	switch(cnt){
		
		case 2:  PORTC|= (1<<PC6); PORTC&=~(1<<PC7);  break; //10
		
		case 3:  PORTC&=~(1<<PC6); PORTC|= (1<<PC7);  break; //01
		
		case 4:  PORTC|= (1<<PC6); PORTC|= (1<<PC7);  break; //11
		
		case 1:  PORTC&=~(1<<PC6); PORTC&=~(1<<PC7);  break; //00
		
		default: break;
	}
}
/* -----------------------------------------------------
unsigned char RawKeyPressed()
This function checks whether any key was pressed or not.
Returns 0x02 for pressed and 0x00 for no press.
-----------------------------------------------------*/
unsigned char RawKeyPressed()
{
	char temp1=(PINB&0x04)>>1|(PINB&0x08)>>3|(PINB&0x02)<<1;
	return(temp1);
}
/* -----------------------------------------------------
char findKey(int row, char temp)
This function maps the ASCII value to the key that was pressed.
Parameters passed are int row for row number and char temp
for character value. It looks up a table and returns a 
key_d variable with corresponding ASCII value.
-----------------------------------------------------*/
char findKey(int row, char temp){
	char key_d=0;
	switch(temp)
	{
		case 0b0000010: key_d=key_table[row-1][0]; break;
		
		case 0b0000110: key_d=key_table[row-1][1]; break;
		
		case 0b0000011: key_d=key_table[row-1][2]; break;
		
		case 0b0000111: key_d=key_table[row-1][3]; break;
		
		default: break;
	}
	return key_d;
	
}
/* -----------------------------------------------------
bool delay(char ms)
This function was written because the use of timers was
not reliable for constant use in main routine. It returns
a true whenever _delay_ms(1) was executed ms number of
times. 
-----------------------------------------------------*/
bool delay(char ms)
{
	int i;
	for (i=0;i<ms;i++)
	{
		_delay_ms(1);
	}
	return true;
}
/* -----------------------------------------------------
char scanKeyPad()
This function that is a little and nice state machine that
implements the use of functions above. It has a number of states:
idle, keyPresses, debounce, getKey, waitRelease, 
releasedDebounce,released. All of them are explained above. 
In short, it sets the row, checks if there was a key press,
if yes, waits for debounce and looks up for ASCII value, else
sets another row and continues routine. Returns a 1 after
key press was detected, encoded and ASCII value found.
ASCII value is stored in global value key.
-----------------------------------------------------*/
char scanKeyPad(){
	
	switch(state1)
	{
		case idle:
		if(keyfound==1)
		{
			keyfound=0;
		} if (count==5)
		{ 
			count=1; 
			setRow(count);
		} if ((RawKeyPressed()&0x02)==(0x02)) 
		{ 
			state1=keyPresses;
		} else {
			state1=idle; 
			count++; 
			setRow(count);
		} break;
		
		case keyPresses: 
		temp=RawKeyPressed(); 
		state1=debounce; 
		break;
		
		case debounce: 
		if((delay(10)) && (temp==RawKeyPressed()))
		{ 
			state1=getKey; 
		}else { 
			state1=debounce;
		}  break;
		
		case getKey:state1=waitRelease; key=findKey(count,temp); keyfound=1; break;

		case waitRelease: if(keyfound==1){keyfound=0;} if ((RawKeyPressed()&0x02)==(0x02)){state1=releasedDebounce;} else {state1=released;} break;
		
		case releasedDebounce: 
		if((delay(35)) && (RawKeyPressed()==0b0000000))
		{
			state1=released;
		} else {
			state1=releasedDebounce;
		} break;
		
		case released: state1=idle; count=1; break;
		
		default:state1=idle; break;
	}
	

	return keyfound;
}
/* -----------------------------------------------------
char returnKey()
Simple getter for ASCII value of key pressed. Returns 
a char with ASCII value.
-----------------------------------------------------*/
char returnKey()
{
	return key;
}
/* -----------------------------------------------------
void keypad_init()
Function that sets initial values for keypad scanning
mechanism.
-----------------------------------------------------*/
void keypad_init()
{	 
	 DDRC |= 1 << PINC6;
	 DDRC |= 1 << PINC7;

	 state1=idle;
}

/*---------------------------------------------------------
Purpose: The purpose is to set up a timers for counting 
up a seconds and milliseconds. It's to be used as a sampling
rate for adc module. Detailed comments are to be found below.

Input: void init_timer1(char flagA, char flagB)
Chars flagA nad flagB are used to set up counter. If flagA is
1, timer 1 is enabled while disable timer 0 compare interrupt.
If flagB is set to 1, timer 0 is enabled while timer 1 compare
interrupt is disabled.

Output: 
extern char ms;			--> ++ every ms
extern char second;		--> ++ every s

Uses: usual avr libraries such as io.h and interrupt.h etc.

Author: Ole Shultz
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 10000000L

volatile char ms=0;
volatile int timeOut=0;
volatile char second=0;
/* -----------------------------------------------------
void init_timer1(char flagA, char flagB)
Chars flagA and flagB are used to set up counter. If flagA is
1, timer 1 is enabled else timer 1 compareA interrupt is disabled.
If flagB is set to 1, timer 0 is enabled else timer 1 compareB
interrupt is disabled.
-----------------------------------------------------*/
void init_timer1(char flagA, char flagB){
	
	TCCR1A=(0<<COM1A1)|(0<<COM1B1)|(0<<COM1A0)|(0<<WGM11)|(0<<WGM10);   //enable timer1 out for simulation of pedal speed - phase and frequency correct mode
	TCCR1B=(1<<WGM10)|(3<<CS10); /*prescaling by 64 - 1250000 Hz*/ // (1<<WGM12) CTC

	OCR1A=0x04E2;  //04E2gives 1 msec compare og 30D4 gives 10ms 
	TCCR1A=0x0000;
	if (flagA==1)
		TIMSK|=(1<<OCIE1A);   //enable timer 1 and disable timer 0 compare interrupt
	else
		TIMSK&=~(1<<OCIE1A);

	if (flagB==1) {
		TIMSK|=(1<<OCIE1B);   //enable timer 1 and disable timer 0 compare interrupt
		OCR1B =14375;  //count to 5 => 1 seconds - 1 ms is counted by compare A
	}
	else
	TIMSK&=~(1<<OCIE1B);
}
/* -----------------------------------------------------
ISR(TIMER1_COMPA_vect)
ISR timer1 compare interrupt that is executed every ms
and increments ms variable value.
-----------------------------------------------------*/
ISR(TIMER1_COMPA_vect)
{
	timeOut++;
	ms++;
}
/* -----------------------------------------------------
ISR(TIMER1_COMPB_vect)
ISR timer1 compare interrupt that is executed every s
and increments second variable value.
-----------------------------------------------------*/
ISR(TIMER1_COMPB_vect)
{
	second++;
}

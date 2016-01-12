/*---------------------------------------------------------
Purpose: The purpose of this module is to simulate charging
process by constant sampling of ADC values.

Input: No input is being passed from inheriting module.

Output: There are two outputs for this module, that are being
passed to the inheriting module main:
_charge function returns true whenever simulation's reached the
desired value or user has cancelled it.
The simulation value then is stored in external variable
double energy that is being used by main module menu.

Uses: It is self sufficient module, thus it uses other
driver modules such as:
LCD, Timer, Keypad and USART (debugging purpose) drivers.

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/


#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <util/delay.h>

#include "driverTimer.h"
#include "driverLCD.h"
#include "driverUSART.h"
#include "driverKeyPad.h"

#define NsampleSeconds 1
#define F_CPU 10000000L

/* -----------------------------------------------------
Global parameters used:

char buffer[12];			Buffer to store ADC read values
double energy = 0;			decimal value of sampled energy v
double Last_power = 0;		"-"
uint16_t data = 0;			sample readings from ADC
uint16_t lastData = 0;		"-"
bool cancelled = false;		Bool variable for flagging simulation cancellation by user
-----------------------------------------------------*/

char buffer[12];
//int n=0;
double energy = 0;
double Last_power = 0;
uint16_t data = 0;
uint16_t lastData = 0;
bool cancelled = false;

// char arrow = 0b01111110;
// char CustomChar1 = 0b00000000;
// char CustomChar2 = 0b00000001;
// char CustomChar3 = 0b00000010;
// char CustomChar4 = 0b00000011;
char ful5x8font = 0b11111111;

typedef unsigned int uint16_t;

void initADC();
unsigned doSample();
void onADC();
void offADC();
bool chargeADC();
void progressBar(double _energy);
bool waitAndScanKeyPad();

/* -----------------------------------------------------
void initCharge(void)
Initializes charging simulation by initializing other 
modules and drawing graphical template.
-----------------------------------------------------*/

void initCharge(void)
{
	lcd_init();
	keypad_init();
	initADC();
	init_timer1(0,1);
	USART_Init(64);
	onADC();
	
	lcdClear();
	_delay_ms(15);
	GoTo(0,0);
	LCDPutString("Energy:          mWs");
	_delay_ms(10);
	GoTo(0,1);
	_delay_ms(10);
	LCDPutString("Power:           mW");
	_delay_ms(10);
	GoTo(0,2);
	LCDPutString("Charging progress:");
	sei();
}

/* -----------------------------------------------------
void startCharge(void)
Initiates charge() function and returns true when simulation
is done
-----------------------------------------------------*/

bool startCharge(){
	//initCharge();
	while(!chargeADC());
	return true;
}
/* -----------------------------------------------------
bool waitAndScanKeyPad()
The function uses the time ADC samples it's values and 
scans keypad for user interaction (cancels charging sim)
meanwhile. Let's say, ADC readings are being sampled every
4 seconds, thus keypad is scanned four seconds and then 
sample is made.
-----------------------------------------------------*/
bool waitAndScanKeyPad(){
	if(second == NsampleSeconds) {
 		second = 0;
		return true;
	}else{
		if (scanKeyPad() == 1)
		{
			if (returnKey() == 'B')
			{
				cancelled = true;
			}
		}
		return false;
	}
}
/* -----------------------------------------------------
bool charge()
Main function that returns true and stores energy used value
in energy variable. It returns true whenever energy value 
reached desired one or user's cancelled it.
-----------------------------------------------------*/
bool chargeADC(){

 	while(!waitAndScanKeyPad());
	if (cancelled)
	{
		offADC();
		return true;
	}
		data = doSample();
		
		double power = data/(0.4*1023);   //mW  uW 0.4*1.023
		if (power == 0)
		{
			energy = 0;
		}
		else
		{
			energy = energy + power;
		}
		
		//Only update lcd if needed
		if(Last_power!= power) {
			if(snprintf(buffer,8, "%.3f \r\n", (double) power )) {
				GoTo(10,1);
			
				_delay_ms(10);
				LCDPutString("       ");
				_delay_ms(10);
				GoTo(10,1);
				_delay_ms(10);
				LCDPutString(buffer);
				Last_power = power;
				}else{
				LCDPutString("not converted");
			}
		}
		
		
		
		if (buffer == ultoa(energy, buffer, 10)) {  //last number is the radix
			snprintf(buffer,8, "%.1f \r\n", (double) energy );
			GoTo(10,0);
			LCDPutString("      ");
			_delay_ms(10);
			GoTo(10,0);
			LCDPutString(buffer);
			_delay_ms(10);
			}else{
			LCDPutString("not converted correct");
		}
		//draw a progress bar along with value sampling
		if (energy <= 100)
		{
			progressBar(energy);
		}
		else //simulation has reached desired value energy <= 100 and true value returned
		{
			GoTo(0,3);
			LCDPutString("Charging is complete");
			offADC();
			return true;
		
	}
	return false;
}
/* -----------------------------------------------------
void progressBar(double _energy)
Simple function to draw some graphical simulation
representation. Uses _energy as input
-----------------------------------------------------*/
void progressBar(double _energy){
	int pos_value = (int)floor(_energy/5);

	GoTo(pos_value,3);
	lcd_data_write(ful5x8font);

}
/* -----------------------------------------------------
void initADC()
ADC initialization function
-----------------------------------------------------*/
void initADC(){
	ADMUX|=  (1<<REFS0);  //(1<<REFS1) |(1<<REFS0) the internal adcref: external ref  (1<<REFS0); and adc0 selected
	ADCSRA|= (1<<ADEN);   //enable triggered sampling  |(1<<ADATE)
}
/* -----------------------------------------------------
unsigned int doSample()
Samples ADC readings of ADCL and ADCH 
and returns it's value
-----------------------------------------------------*/
unsigned int doSample(){
	
	unsigned int value=0;
	ADCSRA|=(1<<ADSC);
	while((ADCSRA & (1<<ADSC)));
	
	value= ADCL+ ADCH*256;
	return value&0x3FF;
}
/* -----------------------------------------------------
void onADC()
Turns on ADC.
-----------------------------------------------------*/
void onADC(){
	ADCSRA|= (1<<ADEN);
	
}
/* -----------------------------------------------------
void offADC()
Turns off ADC.
-----------------------------------------------------*/
void offADC() {
	ADCSRA &=~(1<<ADEN);
	
}
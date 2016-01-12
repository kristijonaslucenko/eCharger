/*---------------------------------------------------------
Purpose: This is simple SPI initiation and buffer reading
module. Has SPIinit() function to set apropriate bits as 
well as void SPItransmit(unsigned char byte) function that 
acts as shift register for serial communication.

Input: --

Output: void SPItransmit(unsigned char byte) is function that
acts as shift register for serial communication. 

Uses: usual avr libraries such as io.h and interrupt.h

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
/* -----------------------------------------------------
void SPIinit(void)
Initializes SPI.
-----------------------------------------------------*/
void SPIinit(void)
{
	DDRB = (1<<PB4) | (1<<PB5) | (1<<PB7);								// Set MOSI , SCK , and SS output
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPI2X);				// Enable , SPI itself, Master, set clock rate fck/32
}
/* -----------------------------------------------------
void SPItransmit(unsigned char byte)
Shifts a byte through SPI.
-----------------------------------------------------*/
void SPItransmit(unsigned char byte)
{
	SPDR = byte;					//Load byte to Data register
	while(!(SPSR & (1<<SPIF))); 	// Wait for transmission complete
}


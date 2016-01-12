/*---------------------------------------------------------
Purpose: The purpose is to set up a USART for server - controller
communication. Set frame format is:
1 stop bit, 8 data bit, no parity, 19200 baud, double full duplex.
Module consists of initiation function, byte transmit, string transmit
and byte receive functions.

Input: A byte could be received through char receiveUSART().

Output: A byte and array of chars could be sent through 
void usart_transmit( char data) and 
void sendStringUSART (char *s) respectively. 


Uses: usual avr libraries such as io.h and interrupt.h etc.

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
/* -----------------------------------------------------
char receiveUSART()
Waits for RXC flag and writes UDR byte value to received
variable. Returns latter.
-----------------------------------------------------*/
char receiveUSART()
{
	char received;
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)));
	/* Get and return received data from buffer */
	received = UDR;

	return received;
}
/* -----------------------------------------------------
void usart_transmit( char data)
Waits for UDRE flag and writes to data parameter value
to UDR register. Sends data byte.
-----------------------------------------------------*/
void transmitUSART( char data)
{
	/* Wait for data to be transmitted */
	while ( !(UCSRA & (1<<UDRE)));

	UDR = data;
}
/* -----------------------------------------------------
void sendStringUSART (char *s)
Sends a string through USART. Shifts bytes of array
passed by *s pointer until terminator.
-----------------------------------------------------*/
void sendStringUSART (char *s)
{
	char c = 0;
	for (;(( c=*s)!=0);s++){
		transmitUSART(*s); 
	}
}
/* -----------------------------------------------------
void sendStringUSART (char *s)
Enables USART receiver and transmitter, sets up frame
format: 1 stop bit, 8 data bits, no parity, full duplex.
Uses int baud parameter to set baud rate while casted.
-----------------------------------------------------*/
void USART_Init( unsigned int baud )
{
	UBRRH = (unsigned char)(baud>>8); 
	UBRRL = (unsigned char)baud;
	/* Enable receiver and transmitter */
	UCSRB|= (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); 
	/* Set frame format:enable the UCSRC for writing 1 stop bit, 8 data bit */
	UCSRC|= (1<<URSEL)|(0<<USBS)|(3<<UCSZ0);     
	UCSRA =(1<<U2X);        //double speed full duplex
}

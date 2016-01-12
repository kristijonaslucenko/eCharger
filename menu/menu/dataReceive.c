/*---------------------------------------------------------
Purpose: The purpose of this module is to receive communication
packages from server and correctly interpret as well as to pass
interpreted values to inheriting modules.

Input: Input comes from ISR where USART_RXC_vect vector is passed
as a parameter.

Output: The output is a number of variables that contain
interpreted values from data package after reception.
extern int dl; int that has a number of bytes of received actual data
extern char actualData[100]; string or char array with actual data in it
extern int _source;	int with source value
extern int _destination; int with destination value
extern int _command;	int with command value

Uses: USARTdriver for testing purposes, else it's self-sufficent

Author: Ultra 2000
Company: DTU Dipom
Version: 1.0
Date and year: 2014/05/26
-----------------------------------------------------*/
#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "driverUSART.h"

#define STOP_CHAR1 '-'
#define STOP_CHAR2 '*'
/*			Global variables
	Developer believes that 
	variable names speak for
	themselves. 
*/
volatile char arrivedByte;
volatile bool packageArrival = false;
volatile bool packageArrived = false;
bool datastart = false;

char dataArrived[100];
char source[3];
char destination[3];
char command[3];
char dataLenght[5];
char checksum[4];
int dl;
char actualData[100];
int _source;
int _destination;
int _command;
int _status;
static char prevByteisr = 0;
static char currByteisr = 0;


bool receivePackage(void);
bool packageReceived(void);

/* -----------------------------------------------------
ISR(USART_RXC_vect)
Interrupt service routine is used by passing USART receive
vector. It receives byte and stores it's value in currByteisr
variable and also determines the beginning of data package by
detecting consecutive bytes to be stop char 2 and stop char 1
and setting datastart true and start to store arrived data byte
to arrivedByte variable.
-----------------------------------------------------*/
ISR(USART_RXC_vect)
{
	prevByteisr = currByteisr;
	currByteisr = UDR;
	if (datastart)
	{
		packageArrival = true;
		arrivedByte = currByteisr;
	}
	if ((currByteisr == STOP_CHAR1) && (prevByteisr == STOP_CHAR2))
	{
		datastart = true;
	}
	
}
/* -----------------------------------------------------
bool packageReceived(void)
Simple function to determine whether the package is 
received yet. It returns true whenever it is received.
-----------------------------------------------------*/
bool packageReceived(void)
{
	//USART_Init(64);
    while(!receivePackage());
	return true;
}
/* -----------------------------------------------------
bool receivePackage(void)
Main module function that after getting flag packageArrival
starts to store arrived data in dataArrived array and look
for consecutive bytes to be stop char 1 and stop char 2.
As latter happens it sets a flag packageArrived and the 
proceeds to interpret data according to a following format:
*-02012400011001-* 
Where 2 first bytes are stop chars 1 and 2,
next 2 bytes are destination, then 2 bytes are source, then
2 bytes are command, then 4 bytes are data lenght, next undefined
number of bytes are actual data, 3 bytes for check sum and 2
for stop chars 1 and 2.
The interpreted data is stored in external and global 
variables:
extern int dl;
extern char actualData[100];
extern int _source;
extern int _destination;
extern int _command;
-----------------------------------------------------*/
bool receivePackage(void){
	
static char prevByte = 0;
static char currByte = 0;
static int countBits = 0;

	if ((packageArrival) && (!packageArrived))
	{
		
		packageArrival = false;
		
		prevByte = currByte;
		currByte = arrivedByte;
		
		if ((prevByte == STOP_CHAR1) && (currByte == STOP_CHAR2)) //The end of package is detected and flag packageArrived is set
		{
			packageArrived = true;
			dataArrived[countBits++] = '\0';
			countBits = 0;
			prevByte = 0;
			currByte = 0;
			datastart = false;
		}else{
			packageArrived = false;
		}

		dataArrived[countBits++] = currByte;		
	}
	
	if ((!packageArrival) && packageArrived) //package data interpretation
	{
		_source = 0;
		_destination = 0;
		_command = 0;
		//set a null terminators so the arrays later could be interpreted as a string
		memset(source, '\0', 3);
		memset(destination, '\0', 3);
		memset(command, '\0', 3);
		memset(dataLenght, '\0', 5);
		memset(actualData, '\0', 100);
		memset(checksum, '\0', 4);
		
		//copy package contents to package fields
		memcpy(source, dataArrived, 2);
		memcpy(destination, dataArrived + 2, 2);
		memcpy(command, dataArrived + 4, 2);
		memcpy(dataLenght, dataArrived + 6, 4);
				
		//count data bytes and copy data to package data field
		dl = atoi(dataLenght);
				
		memcpy(actualData, dataArrived + 10, dl);
		memcpy(checksum, dataArrived + (10 + dl), 3);
		
//  		putString("Data lenght: \n");
//  		putString(dataLenght);
//  		usart_transmit('\n');
//  		putString("Data: \n");
//  		putString(actualData);
//  		usart_transmit('\n');
//  		putString("CheckSum: \n");
//  		putString(checksum);
//  		usart_transmit('\n');
// 			putString(command);
		_source = atoi(source);
		_destination = atoi(destination);
		_command = atoi(command);
		packageArrived = false;
		packageArrival = false;
		memset(dataArrived, '\0', 100);
		countBits = 0;
		return true;
	}
	return false;
}

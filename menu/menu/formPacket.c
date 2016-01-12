/*---------------------------------------------------------
Purpose: The purpose of this module is to form a packet
that is being used for server - controller communication.

Input: char _source[], char _destination[], char _command[], char _data[]
First parameter is char array, that contains source value
Second param is the same, contains destination value
Third is the same as two previous and contains command value
Fourth parameter is actual data that is to be sent.

Output: The output is formed package which value is stored
in global and external variable formedDataPackageToSend which
although is defined size (100), is actually a string, so later
it's size is determined as usual - by null terminator '\0'.

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

char formedDataPackageToSend[100] = {0};

/* -----------------------------------------------------
bool formPacket(char _source[], char _destination[], char _command[], char _data[])

Parameters:
First parameter is char array, that contains source value
Second param is the same, contains destination value
Third is the same as two previous and contains command value
Fourth parameter is actual data that is to be sent.

Output: 
Actual return is boolean to indicate the success of instructions.
The output is formed package which value is stored
in global and external variable formedDataPackageToSend which,
although is defined size (100), is actually a string, so later
it's size is determined as usual - by null terminator '\0'.

_source(2 bytes) _destination(2 bytes) _command (2 bytes) _data (undefined size)

For input:
formPacket("02", "01", "86", energyStr); where energyStr is "102.20"
The output is:
0201860006102.2000(xor value)-*
-----------------------------------------------------*/
	
bool formPacket(char _source[], char _destination[], char _command[], char _data[]){
	
	memset(formedDataPackageToSend, 0, 100);
	
	int datalint = strlen(_data);

	int sizeofpacket = datalint + 10;
	
	char _dataLenght[5];
	
	memset(_dataLenght, '\0', 5);
	
	if (datalint < 10)
	{
		char datal = (char)(((int)'0')+datalint);
		_dataLenght[0] = '0';
		_dataLenght[1] = '0';
		_dataLenght[2] = '0';
		_dataLenght[3] = datal;
	}
	if ((datalint < 100) && (datalint >= 10))
	{
		_dataLenght[0] = '0';
		_dataLenght[1] = '0';
		char datal[3];
		itoa(datalint, datal, 10);
		_dataLenght[2] = datal[0];
		_dataLenght[3] = datal[1];
	}
	if ((datalint < 999) && (datalint >= 100))
	{
		_dataLenght[0] = '0';
		char datal[4];
		itoa(datalint, datal, 10);
		_dataLenght[1] = datal[0];
		_dataLenght[2] = datal[1];
		_dataLenght[3] = datal[2];
	}
	
	memcpy(formedDataPackageToSend, _source, 2);
	memcpy(formedDataPackageToSend + 2, _destination, 2);
	memcpy(formedDataPackageToSend + 4, _command, 2);
	memcpy(formedDataPackageToSend + 6, _dataLenght, 4);
	memcpy(formedDataPackageToSend + 10, _data, datalint);
		
	char xor = 0;
	
	for ( int i = 0 ; i < (sizeofpacket); i ++ ) {
		xor = xor ^ formedDataPackageToSend[i];
	}
	
	formedDataPackageToSend[(10 + datalint)] = '0';
	formedDataPackageToSend[(11 + datalint)] = '0';
	formedDataPackageToSend[(12 + datalint)] = xor;
	formedDataPackageToSend[(13 + datalint)] = STOP_CHAR1;
	formedDataPackageToSend[(14 + datalint)] = STOP_CHAR2;
	formedDataPackageToSend[(15 + datalint)] = '\0';
	
	return true;
}
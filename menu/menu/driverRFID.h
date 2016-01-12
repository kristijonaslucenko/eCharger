#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define MAX 14

extern char RfidBufferToRead[MAX];
extern bool rfidDone;
extern char pinChar[5];
extern char debtChar[17];
extern char pastEnChar[17];
extern char pastExChar[17];
extern bool offlineFirstRead;
extern bool offlineWrite;
extern bool onlineFirstREad;
extern int writeAction;
extern bool creditDetected;


extern bool RFIDinit(int command, char _parammeter[], int sizeOfPar);

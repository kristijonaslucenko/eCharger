#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern int dl;
extern char actualData[100];
extern int _source;
extern int _destination;
extern int _command;

extern bool packageReceived(void);
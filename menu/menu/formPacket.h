#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern char formedDataPackageToSend[100];

extern bool formPacket(char _source[], char _destination[], char _command[], char _data[]);
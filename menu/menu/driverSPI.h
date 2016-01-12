#include <avr/io.h>
#include <avr/interrupt.h>

extern void SPIinit(void);
extern void SPItransmit(unsigned char byte);
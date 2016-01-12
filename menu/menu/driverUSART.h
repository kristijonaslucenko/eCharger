#include <avr/io.h>
#include <avr/interrupt.h>

extern void USART_Init( unsigned int baud );
extern void sendStringUSART (char *s);
extern void transmitUSART(unsigned char data);
extern char receiveUSART();
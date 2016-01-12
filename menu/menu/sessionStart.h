#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern bool idied;
extern char id_[14];
extern char pastEnergy_[17];
extern char pastExpense_[17];
extern char credit_[17];
extern char pin_s[5];
extern bool offline_mode;
extern int OFFLINE_PRICE;
extern bool credited;
extern bool deleteCreditRFID;

extern bool id(void); 
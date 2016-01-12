#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>

#include "driverTimer.h"
#include "driverLCD.h"
#include "driverUSART.h"

extern double energy;

extern bool startCharge(void);
extern void initCharge(void);
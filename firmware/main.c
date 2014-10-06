#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "softuart.h"

// Pin 1 - ROM Bank Switch (Low = Orig/High = JiffyDOS)
// Pin 2 - ATX Power Remote (Low = ON)
// Pin 3 - TOD Clock Out (60Hz) (R7/U16 Pin 5)     
// Pin 4 - GND                      
// Pin 5 - Rx -> U.P. Pin M (PA2)
// Pin 6 - Tx -> U.P. Pin B+C (FLAGS2+PB0)
// Pin 7 - Restore Key (R10/U16 Pin 9)
// Pin 8 - VCC

uint8_t g_power_enabled = 0;

void setup(void);

void hibernate(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_bod_disable();
    TIMSK = 0;
    sei();                              // Enable global interrupts 
    sleep_mode();
    sleep_disable();
    setup();
}


void setup(void)
{
    TCCR0A = 0;
    TCCR0B = (1 << CS02) | (0 << CS01) | (0 << CS00);  // 1Mhz/1024
    TIMSK |= (1 << TOIE0);
    GIMSK |= (1 << PCIE);               // Set general interrupt pin change
    sei();                              // Enable interrupts 
}

int main(void)
{
    setup();
    return 0;
}

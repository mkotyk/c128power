#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "softuart.h"

// Pin 1 (PB5) - ROM Bank Switch (Low = Orig/High = JiffyDOS)
// Pin 2 (PB3) - ATX Power Remote (Low = ON)
// Pin 3 (PB4|OCB1) - TOD Clock Out (60Hz) (R7/U16 Pin 5)     
// Pin 4 - GND                      
// Pin 5 (PB0) - Rx -> U.P. Pin M (PA2)
// Pin 6 (PB1) - Tx -> U.P. Pin B+C (FLAGS2+PB0)
// Pin 7 (PB2) - Restore Key (R10/U16 Pin 9)
// Pin 8 - VCC

const uint8_t UART_RX_PIN      = (1 << PB0);
const uint8_t UART_TX_PIN      = (1 << PB1);
const uint8_t RESTORE_KEY_PIN  = (1 << PB2);
const uint8_t POWER_ENABLE_PIN = (1 << PB3);
const uint8_t TOD_CLOCK_PIN    = (1 << PB4);
const uint8_t ROM_SELECT_PIN   = (1 << PB5);

#define SHORT_PRESS  30    // 1 Second
#define LONG_PRESS   300   // 10 Seconds
#define IDLE_TIMEOUT 3000  // 5 Mins

#define CMD_OK       0
#define CMD_ERROR    1

uint8_t g_power_enabled = 1;
uint8_t g_rom_select = 0;
uint16_t g_press_down_timer = 0;
uint16_t g_idle_timer = 0;

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

static void power_down(void)
{
    g_power_enabled = 0;
    PORTB |= POWER_ENABLE_PIN;
}

static void power_up(void)
{
    g_power_enabled = 1;
    PORTB &= ~POWER_ENABLE_PIN;
}

// Overflow at 30Hz
ISR(TIMER1_OVF_vect)
{
    if((g_idle_timer % 300) == 0)
        PORTB ^= POWER_ENABLE_PIN;
    // Check if the RESTORE key is being pressed.
    if(!(PINB & RESTORE_KEY_PIN))
    {
        g_press_down_timer++;
        g_idle_timer = 0;
    }
    else
    {
        if(g_power_enabled)
        {
            if(g_press_down_timer > LONG_PRESS)
                power_down();
        }
        else
        {
            if(g_press_down_timer > SHORT_PRESS)
                power_up();
        }
        g_press_down_timer = 0;
        g_idle_timer++;

//        if(!g_power_enabled && (g_idle_timer > IDLE_TIMEOUT))
//        {
//            hibernate();
//        }
    }
}

ISR(PCINT0_vect)
{
}

void setup(void)
{
    g_press_down_timer = 0;
    g_idle_timer = 0;

    DDRB = TOD_CLOCK_PIN | ROM_SELECT_PIN | POWER_ENABLE_PIN | UART_TX_PIN;
    PORTB = (g_rom_select * ROM_SELECT_PIN) | (g_power_enabled?0:POWER_ENABLE_PIN) | RESTORE_KEY_PIN;
    PCMSK = RESTORE_KEY_PIN;

    softuart_init();

    // Configure OCR1B output for 60Hz
    TCCR1 = _BV(CTC1) | _BV(CS13) | _BV(CS11) | _BV(CS10);
    GTCCR = _BV(PWM1B) | _BV(COM1B0);
    OCR1A = 0;
    OCR1B = 66;
    OCR1C = 132;

    TIMSK |= _BV(TOV1);
    GIMSK |= _BV(PCIE);

    sei();                              // Enable interrupts 
}

static void select_rom_low(void)
{
    g_rom_select = 0;
}

static void select_rom_high(void)
{
    g_rom_select = 1;
}

typedef struct 
{
    const char *name;
    void (*callback)(void);
} command_t;

static command_t cmds[5] =  {
    { "powerdown", &power_down },
    { "romlow", &select_rom_low },
    { "romhigh", &select_rom_high },
    { 0, 0 }
};

static int execute_cmd(char *cmd)
{
    uint8_t i;
    for(i = 0; cmds[i].name != 0; i++)
    {
        if(strcasecmp(cmd, cmds[i].name) == 0)
        {
            cmds[i].callback();
            return CMD_OK;
        }
    }
    return CMD_ERROR;
}

int main(void)
{
    static char cmd[10];
    char c, *p = cmd;
    setup();
    softuart_puts(PSTR("c128power v1.0"));
    while(1)
    {
        if(softuart_kbhit()) {
            c = softuart_getchar();
            switch(c)
            {
                case '\n':
                    {
                        *p = 0;
                        p = cmd;
                        if(execute_cmd(cmd) == CMD_OK)
                            softuart_puts(PSTR("ok\n"));
                        else
                            softuart_puts(PSTR("error\n"));
                        break; 
                    }
                default:
                    {
                        if(p < cmd+(sizeof(cmd)))
                        {
                            *p++ = c;
                            softuart_putchar(c);
                        }
                        else
                        {
                            softuart_putchar(7); // bell
                        }
                    }
            } 
        }
    }
    return 0;
}

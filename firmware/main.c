#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
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
#define IDLE_TIMEOUT 3000  // 100 Seconds

#define CMD_OK       0
#define CMD_ERROR    1

#define EEPROM_DEFAULT_ROM_SEL_ADDR 0

uint8_t g_power_enabled = 0;
uint8_t g_rom_select = 0;
uint16_t g_press_down_timer = 0;
uint16_t g_idle_timer = 0;

void setup(void);

void hibernate(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_bod_disable();
    softuart_turn_rx_off();
    TIMSK = 0;
    sei();                      // Enable global interrupts 
    sleep_mode();
    sleep_disable();
    setup();
}

static void power_down(void)
{
    g_power_enabled = 0;
    PORTB &= ~(ROM_SELECT_PIN); // Remove power from rom select
    PORTB |= POWER_ENABLE_PIN;  // Power enable pin high (OFF)
    DDRB &= ~TOD_CLOCK_PIN;     // Turn off OCB1 output
    
}

static void power_up(void)
{
    g_power_enabled = 1;
    PORTB |= (g_rom_select * ROM_SELECT_PIN); // Set ROM select at last known value
    PORTB &= ~POWER_ENABLE_PIN; // Power enable pin low (ON)
    DDRB |= TOD_CLOCK_PIN;      // Re-enable TOD clock
}

// Overflow at 30Hz
ISR(TIMER1_OVF_vect)
{
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

        if(!g_power_enabled && (g_idle_timer > IDLE_TIMEOUT))
        {
            hibernate();
        }
    }
}

ISR(PCINT0_vect)
{
}

void setup(void)
{
    g_press_down_timer = 0;
    g_idle_timer = 0;

    g_rom_select = eeprom_read_word(EEPROM_DEFAULT_ROM_SEL_ADDR);

    DDRB =  ROM_SELECT_PIN | POWER_ENABLE_PIN | UART_TX_PIN;
    PORTB = (g_power_enabled?0:POWER_ENABLE_PIN) | RESTORE_KEY_PIN;
    PCMSK = RESTORE_KEY_PIN;

    softuart_init();

    // Configure OCR1B output for 60Hz
    TCCR1 = _BV(CTC1) | _BV(CS13) | _BV(CS11) | _BV(CS10);  // Prescale select CK/1024
    GTCCR = _BV(PWM1B) | _BV(COM1B1);                       // Enable PWM on OCB1
    OCR1A = 0;
    OCR1B = 66;
    OCR1C = 132;

    TIMSK |= _BV(TOV1);  // Interrupt on Timer 1 Overflow (used for button)
    GIMSK |= _BV(PCIE);  // Interrupt on pin change for Restore Key (used to wake from sleep)

    sei();               // Enable interrupts 
}

static void select_rom_low(void)
{
    g_rom_select = 0;
    PORTB &= ~ROM_SELECT_PIN;
}

static void select_rom_high(void)
{
    g_rom_select = 1;
    PORTB |= ROM_SELECT_PIN;
}

static void default_rom_low(void)
{
    eeprom_write_word(EEPROM_DEFAULT_ROM_SEL_ADDR, 0);
}

static void default_rom_high(void)
{
    eeprom_write_word(EEPROM_DEFAULT_ROM_SEL_ADDR, 1);
}

static void show_version(void)
{
    softuart_puts_p(PSTR("c128power v1.0\r\n"));
}

static void show_help(void);

typedef struct 
{
    const char *name;
    const char *help;
    void (*callback)(void);
} command_t;

static command_t cmds[] =  {
    { "powerdown", "turn power off", &power_down },
    { "powerup", "turn power on (debug)", &power_up },
    { "roml", "select kernel rom in high bank", &select_rom_low },
    { "romh", "select kernel rom in low bank", &select_rom_high },
    { "defroml", "default to kernel rom in low bank", &default_rom_low },
    { "defromh", "default to kernel rom in high bank", &default_rom_high },
    { "version", "show version", &show_version },
    { "help", "this screen", &show_help },
    { 0, 0 }
};

static void show_help(void)
{
    uint8_t i;
    for(i = 0; cmds[i].name != 0; i++)
    {
        softuart_puts(cmds[i].help);
        softuart_puts_p(PSTR("\r\n"));
    }
}

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
    while(1)
    {
        if(softuart_kbhit()) {
            c = softuart_getchar();
            switch(c)
            {
                case '\n':
                case '\r':
                {
                    *p = 0;
                    p = cmd;
                    if(execute_cmd(cmd) == CMD_OK)
                        softuart_puts_p(PSTR("ok\r\n"));
                    else
                        softuart_puts_p(PSTR("error\r\n"));
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

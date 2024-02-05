#include "msp.h"
#include "lcd.h"
#include "delays.h"
#include "csLFXT.h"
#include <stdio.h>

#define LCD_CLK 48000000
#define period 16384
#define HALL_PORT P2
#define HALL_MASK BIT0
/**
 * main.c
 */
extern unsigned int TOTAL_REVOLUTIONS = 0;
extern int CURRENT_REVOLUTIONS = 0;
extern int RADIUS_IN_INCHES = 13; //user input?
typedef enum _HallState
{
    HIGH, LOW
} HallState;
void configTimer()
{
    //configure A0 for SMCLK, UP mode, interrupt disabled, will interrupt every 0.5 seconds
    TIMER_A0->CTL = 0b0000000100010000;
    //CCR0 -> Compare, interrupt enabled
    TIMER_A0->CCTL[0] = 0b0000000000010000;
    TIMER_A0->CCR[0] = period;

    //enable global interrupts
    NVIC->ISER[0] |= 0x00000080;
    __enable_irq();
}

void configHallPort()
{
    HALL_PORT->DIR |= HALL_MASK;
    HALL_PORT->SEL0 &= ~HALL_MASK;
    HALL_PORT->SEL1 &= ~HALL_MASK;
    HALL_PORT->REN |= HALL_MASK;
    HALL_PORT->OUT |= HALL_MASK;
}

HallState checkHall()
{
    char hallValue = HALL_PORT->IN & HALL_MASK;
    if (hallValue == HALL_MASK)
    {
        return HIGH;
    }
    else
    {
        return LOW;
    }
}
void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

    configHFXT(); //config 48MHz SMCLK
    configLFXT(); //config ~32kHz ACLK
    configLCD(LCD_CLK); //config lcd ports
    initLCD(); //initialize lcd
    configTimer(); //config timer registers
    configHallPort(); //config hall sensor input port

    //lcd update done in interrupt handler
    while (1)
    {
        HallState = checkHall();
        if (HallState == LOW)
        {
            TOTAL_REVOLUTIONS++;
            CURRENT_REVOLUTIONS++;
        }
    }

}

void TA0_0_IRQHandler(void)
{
    int rpm = CURRENT_REVOLUTIONS * 120;
    double mph = rpm * 2 * 3.14 * 63360;
    lcd_SetLineNumber(LINE1_OFFSET);
    sprintf(Buffer, "Speed: %d MPH", mph);

    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

}

#include "msp.h"
#include "lcd4bits_ece230winter23_24.h"
#include "delays.h"
#include "csLFXT.h"
#include "csHFXT.h"
#include <stdio.h>
#include <keypadscan_subroutines.h>
#include "string.h"
#include  <stdlib.h>
#include <math.h>

#define period 16384*2 //interrupt every second
#define HALL_PORT P2
#define HALL_MASK BIT3
#define inputSize 2 //size of user input
/**
 * main.c
 */
unsigned int TOTAL_REVOLUTIONS = 0;
float CURRENT_REVOLUTIONS = 0.0;
float RADIUS_IN_MILES; //user input?
char Buffer[200]; //for lcd print
extern enum Status
{
    NO, YES
};
extern char NewKeyPressed;
extern char FoundKey;
char input[inputSize];
int calc = 0;
float rpm;
float mph;
float distance;

void configTimer()
{
    //configure A0 for ACLK, UP mode, interrupt disabled, will interrupt every 0.5 seconds
    TIMER_A0->CTL = 0b0000000100010000;
    //CCR0 -> Compare, interrupt enabled
    TIMER_A0->CCTL[0] = 0b0000000000010000;
    TIMER_A0->CCR[0] = period;

    //enable global interrupts
    NVIC->ISER[0] |= (1 << TA0_0_IRQn);
    __enable_irq();
}

void configHallPort()
{
    HALL_PORT->DIR &= ~HALL_MASK;
    HALL_PORT->SEL0 &= ~HALL_MASK;
    HALL_PORT->SEL1 &= ~HALL_MASK;
    HALL_PORT->REN |= HALL_MASK;
    HALL_PORT->OUT |= HALL_MASK;
    //select interrupt edge on high to low transition on input pins
    HALL_PORT->IES |= HALL_MASK; //bit = 1 in PxIES register
    //enable pin interrupts
    HALL_PORT->IE |= HALL_MASK; //bit = 1 to enable pin in PxIE register
    //clear interrupt flags
    HALL_PORT->IFG &= ~HALL_MASK;
    //enable PORT interrupt
    NVIC->ISER[1] |= (1) << (PORT2_IRQn - 32);
}

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
    configHFXT(); //config 48MHz SMCLK
    configLFXT(); //config ~32kHz ACLK
    lcd4bits_init();
    keypadconfiguration();
    configHallPort(); //config hall sensor input port
    configTimer(); //config timer registers
    int i = 0;
    while (1)
    {
        if (NewKeyPressed == YES)
        {
            NewKeyPressed = NO;
            //translate binary sequence into an actual character to be either displayed or interpreted
            switch (FoundKey)
            {
            case 1:
                FoundKey = '1';
                break;
            case 2:
                FoundKey = '2';
                break;
            case 3:
                FoundKey = '3';
                break;
            case 5:
                FoundKey = '4';
                break;
            case 6:
                FoundKey = '5';
                break;
            case 7:
                FoundKey = '6';
                break;
            case 9:
                FoundKey = '7';
                break;
            case 10:
                FoundKey = '8';
                break;
            case 11:
                FoundKey = '9';
                break;
            case 13:
                FoundKey = '*';
                break;
            case 14:
                FoundKey = '0';
                break;
            case 15:
                FoundKey = '#';
                break;
            default:
                FoundKey = '*';
                break;
            }
            if (FoundKey == '#')
            {
                break;
            }
            else if (FoundKey == '*')
            {
                input[0] = '\0';
                input[1] = '\0';
                lcd_clear();
                i = 0;
            }
            else
            {
                input[i % 2] = FoundKey;
                i++;
            }
        }
        //user interface for inputing radius of wheel
        lcd_SetLineNumber(FirstLine);
        sprintf(Buffer, "INPUT RADIUS IN");
        lcd_puts(Buffer);
        lcd_SetLineNumber(SecondLine);
        int temp = atoi(input);
        sprintf(Buffer, "INCHES : %d", temp);
        lcd_puts(Buffer);
    }
    RADIUS_IN_MILES = atoi(input) * 0.00001578; //convert radius to miles
    lcd_clear();
    while (1)
    {
        if (calc == 1)
        {
            rpm = CURRENT_REVOLUTIONS / 9.0 * 120.0; // divide by number of magnets, multiply by 60/samepling rate
            mph = rpm * 2.0 * 3.14 * RADIUS_IN_MILES * 60.0; //RPM * CIRCUMFERENCE * 60 MINUTES
            distance = (TOTAL_REVOLUTIONS / 9) * 2 * 3.14 * RADIUS_IN_MILES; //divide by number of magnets, multiply by circumference
            calc = 0; //reset flag
            CURRENT_REVOLUTIONS = 0; //reset current revolutions
        }
        //print info to lcd
        lcd_SetLineNumber(FirstLine);
        sprintf(Buffer, "Speed: %.2f MPH", mph);
        lcd_puts(Buffer);
        lcd_SetLineNumber(SecondLine);
        sprintf(Buffer, "DIST: %.2f MILES", distance);
        lcd_puts(Buffer);
    }

}

void TA0_0_IRQHandler(void)
{
    calc = 1; //tell main to recalculate distance and speed
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; //clear flag
}

void PORT2_IRQHandler(void)
{
    CURRENT_REVOLUTIONS++;
    TOTAL_REVOLUTIONS++;
    uint32_t status;
    status = HALL_PORT->IFG;
    HALL_PORT->IFG &= ~status; //clear flag
} // end PORT2_IRQHandler(void)



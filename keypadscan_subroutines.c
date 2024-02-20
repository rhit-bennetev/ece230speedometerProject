//File Name: keypadscan_subroutines.c
//Author: Jianjian Song
//ECE230-01/02 Winter 2023-2024
//Date: January 28 2024
//Purpose: Scan a 4x4 keypad to provide key numbers from 1 to 16
//         if a corresponding key is pressed
//         key_number is 0 if no key is pressed
/*
 * MSP432P111 GPIO -
 * Port 4 Input Interrupt on input pins 4, 5, 6, 7
 *
 *                MSP432P4111
 *             ------------------
 *         /|\|                  |
 *          | |                  |
 *          --|RST    output P4.0|---> key0
 *            |       output P4.1|---> key1
 *            |       output P4.2|---> key2
 *            |       output P4.3|---> key3
 *            | pullup input P4.4|---> key4
 *            | pullup input P4.5|---> key5
 *            | pullup input P4.6|---> key6
 *            | pullup input P4.7|---> key7
 *            |                  |
 *            |                  |
 ******************************************************************************/
/* Standard Includes */
#include <msp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <keypadscan_subroutines.h>

//configure Port 4 pins 3-0 as output
//configure pins 7-4 as input with pullup enabled
//configure pins 7-4 interrupt enabled on high to low pin change
void keypadconfiguration(void) {
//Configuring P4.3-0 as output
    KeypadPort->DIR |= KeypadOutputPins;
    KeypadPort->SEL0 &= ~KeypadOutputPins;
    KeypadPort->SEL1 &= ~KeypadOutputPins;

//P4.7-4 as pulled-up input to interface a keypad
//Pins 7 to 4 cause input change interrupt
    KeypadPort->DIR &= ~KeypadInputPins;
    KeypadPort->SEL0 &= ~KeypadInputPins;
    KeypadPort->SEL1 &= ~KeypadInputPins;
//Pullup resistors on input pins
    KeypadPort->OUT |= KeypadInputPins;
    KeypadPort->REN |= KeypadInputPins;

//select interrupt edge on high to low transition on input pins
    KeypadPort->IES |= KeypadInputPins; //bit = 1 in PxIES register
//enable pin interrupts
    KeypadPort->IE |= KeypadInputPins; //bit = 1 to enable pin in PxIE register
//clear interrupt flags
    KeypadPort->IFG &= ~KeypadInputPins;
//enable PORT interrupt
    NVIC->ISER[1] |= (1)<<(PORT4_IRQn-32);

}

const char KeyPatterns[16]={0b01110111,0b01111011,0b01111101,0b01111110,
0b10110111,0b10111011,0b10111101,0b10111110,
0b11010111,0b11011011,0b11011101,0b11011110,
0b11100111,0b11101011,0b11101101,0b11101110};

enum Status {NO, YES};
char NewKeyPressed=NO;
char FoundKey;
//return key numbers from 1 to 16 if a key is found to be pressed
//return 0 if no key is pressed
char FindKey(void) {
#define DEBOUNCETIME 500
char i;
char KeyNumber;
char PortValue;
uint16_t delayloop;
    for(delayloop=0; delayloop<DEBOUNCETIME; delayloop++); //debounce the key
    for(i=0;i<=15;i++) {
//output key pattern
        KeypadPort->OUT = (KeypadPort->OUT & ~KeypadOutputPins) | (KeyPatterns[i] & KeypadOutputPins);
        PortValue = KeypadPort->IN;
        if((PortValue & KeypadInputPins) ==(KeyPatterns[i] & KeypadInputPins))  {   //check key press
            for(delayloop=0; delayloop<DEBOUNCETIME; delayloop++); //debounce the key
            if((PortValue & KeypadInputPins) ==(KeyPatterns[i] & KeypadInputPins))  {   //still pressed
                KeyNumber = i+1;
                while((PortValue & KeypadInputPins) ==(KeyPatterns[i] & KeypadInputPins))
                    PortValue=KeypadPort->IN; //wait for release
                break;
        }
        for(delayloop=0; delayloop<DEBOUNCETIME; delayloop++); //debounce the key
    }
}
    return KeyNumber;
} //end FindKey()

/* GPIO ISR */

void PORT4_IRQHandler(void)
{
    uint32_t status;

    status = KeypadPort->IFG;
    KeypadPort->IFG &=~status;

    FoundKey = FindKey();
//set up for next key interrupt. Set all output pins to 0
//do not change input values
    KeypadPort->OUT=(KeypadPort->OUT & ~KeypadOutputPins) | (0b11110000 & KeypadOutputPins);
    if(FoundKey!=0) {
        NewKeyPressed=YES;
    }
    else NewKeyPressed=NO;
} // end PORT4_IRQHandler(void)

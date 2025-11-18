// This code was ported from TI's sample code. See Copyright notice at the bottom of this file.

#include <LcdDriver/lower_driver.h>
#include "msp430fr6989.h"
#include "Grlib/grlib/grlib.h"
#include <stdint.h>

void HAL_LCD_PortInit(void)
{
    /////////////////////////////////////
    // Configuring the SPI pins (UCB0 on P1.4 / P1.6)
    /////////////////////////////////////

    // Route P1.4 and P1.6 to the eUSCI_B0 peripheral: CLK and SIMO.
    P1SEL0 |= (BIT4 | BIT6);
    P1SEL1 &= ~(BIT4 | BIT6);

    ///////////////////////////////////////////////
    // Configuring the display's other pins
    ///////////////////////////////////////////////

    // RESET (P9.4), DC (P2.3), and CS (P2.5) are simple GPIO outputs.
    P9DIR |= BIT4;                 // Reset pin
    P2DIR |= BIT3 | BIT5;          // Data/Command and Chip Select pins

    return;
}

void HAL_LCD_SpiInit(void)
{
    //////////////////////////
    // SPI configuration
    //////////////////////////

    // Hold eUSCI_B0 in reset while we configure it.
    UCB0CTLW0 = UCSWRST;

    // Clocking and frame format:
    //  - Capture on first edge, change on following edge  (UCCKPH = 1)
    //  - Clock idle low                                  (UCCKPL = 0)
    //  - MSB first                                      (UCMSB  = 1)
    //  - 8-bit data                                     (UC7BIT = 0)
    //  - Master, 3‑pin SPI, synchronous, SMCLK source
    UCB0CTLW0 |= UCCKPH;          // phase: capture on first edge
    UCB0CTLW0 &= ~UCCKPL;         // polarity: idle low
    UCB0CTLW0 |= UCMSB;           // transmit MSB first
    UCB0CTLW0 &= ~UC7BIT;         // 8‑bit characters
    UCB0CTLW0 |= UCMST;           // master mode
    UCB0CTLW0 |= UCSYNC;          // synchronous (SPI)
    UCB0CTLW0 |= UCMODE_0;        // 3‑wire SPI (no STE)
    UCB0CTLW0 |= UCSSEL_2;        // use SMCLK as the source

    // SMCLK is configured for 16 MHz in main.c.
    // Divide by 2 so the SPI clock is 8 MHz (within the LCD's 10 MHz max).
    UCB0BRW = 2;

    // Leave reset and start the SPI state machine.
    UCB0CTLW0 &= ~UCSWRST;

    // Tie the LCD's chip-select permanently low (active)
    // and default DC high (data mode).
    P2OUT &= ~BIT5;               // CS' low  -> display selected
    P2OUT |= BIT3;                // DC' high -> data by default

    return;
}


void HAL_LCD_writeCommand(uint8_t command)
{
    // Wait as long as the module is busy
    while (UCB0STATW & UCBUSY);

    // For command, set the DC' bit to low before transmission
    P2OUT &= ~BIT3;

    // Transmit data
    UCB0TXBUF = command;

    return;
}


//*****************************************************************************
// Writes a data to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//*****************************************************************************
void HAL_LCD_writeData(uint8_t data)
{
    // Wait as long as the module is busy
    while (UCB0STATW & UCBUSY);

    // Set DC' bit back to high
    P2OUT |= BIT3;

    // Transmit data
    UCB0TXBUF = data;

    return;
}






/* --COPYRIGHT--,BSD
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

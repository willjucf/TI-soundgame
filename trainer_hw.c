// trainer_hw.c
// hardware Music Trainer

#include <msp430fr6989.h>
#include <stdint.h>

#include "LcdDriver/lcd_driver.h"
#include "Grlib/grlib/grlib.h"
#include "music_trainer.h"

Graphics_Context g_sContext;

// delay
void delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_cycles(16000);
    }
}

//  Clock
void Init_ClockSystem(void)
{
    FRCTL0 = FRCTLPW | NWAITS_1;

    CSCTL0_H = CSKEY_H;

    CSCTL1 &= ~DCOFSEL_7;
    CSCTL1 |= DCOFSEL_4;
    CSCTL1 |= DCORSEL;

    // MCLK = fDCO/1, SMCLK = fDCO/1
    CSCTL3 &= ~(DIVS2 | DIVS1 | DIVS0);   // SMCLK divider 1
    CSCTL3 &= ~(DIVM2 | DIVM1 | DIVM0);   // MCLK divider 1

    CSCTL0_H = 0;
}

//  LCD + grlib
void LCD_InitGraphics(void)
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(0);

    // Turn LCD backlight on
    P2DIR |= BIT6;
    P2OUT |= BIT6;

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);
}

// joystick thresholds
#define JOY_CENTER      2048
#define JOY_DEAD_BAND   400
#define JOY_HIGH_THRESH (JOY_CENTER + JOY_DEAD_BAND)
#define JOY_LOW_THRESH  (JOY_CENTER - JOY_DEAD_BAND)

void Joystick_Init(void)
{
    // X axis
    P9SEL1 |= BIT2;
    P9SEL0 |= BIT2;

    // Y axis
    P8SEL1 |= BIT7;
    P8SEL0 |= BIT7;

    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 |= ADC12ON;

    ADC12CTL0 = (ADC12CTL0 & ~ADC12SHT0_15) | ADC12SHT0_4 | ADC12MSC;

    ADC12CTL1 = ADC12SHS_0
              | ADC12SHP
              | ADC12DIV_0
              | ADC12SSEL_0
              | ADC12CONSEQ_1;

    ADC12CTL2 = ADC12RES_2;

    ADC12CTL3 = ADC12CSTARTADD_0;

    ADC12MCTL0 = ADC12VRSEL_0 | ADC12INCH_10;
    ADC12MCTL1 = ADC12VRSEL_0 | ADC12INCH_4 | ADC12EOS;

    ADC12CTL0 |= ADC12ENC;
}

void Joystick_ReadAxes(uint16_t *py, uint16_t *px)
{
    ADC12CTL0 |= ADC12SC;
    while (ADC12CTL1 & ADC12BUSY) {
        ; // wait
    }

    uint16_t x = ADC12MEM0;
    uint16_t y = ADC12MEM1;

    if (px) *px = x;
    if (py) *py = y;
}

JoystickDir Joystick_GetDir(void)
{
    uint16_t x, y;
    Joystick_ReadAxes(&y, &x);  // y first x second

    if (y > JOY_HIGH_THRESH) {
        return DIR_UP;
    }
    if (y < JOY_LOW_THRESH) {
        return DIR_DOWN;
    }
    if (x > JOY_HIGH_THRESH) {
        return DIR_RIGHT;
    }
    if (x < JOY_LOW_THRESH) {
        return DIR_LEFT;
    }
    return DIR_CENTER;
}

void Buzzer_Init(void)
{
    P2DIR  |= BIT7;
    P2SEL0 |= BIT7;
    P2SEL1 &= ~BIT7;

    TB0CTL   = TBSSEL__SMCLK | MC__STOP | TBCLR;   // SMCLK source
    TB0CCTL6 = OUTMOD_7;
    TB0CCR0  = 0;
    TB0CCR6  = 0;
}

static void Buzzer_SetFrequency(uint16_t freq)
{
    if (freq == 0) {
        // Stop timer to stop buzzer
        TB0CTL = TBSSEL__SMCLK | MC__STOP | TBCLR;
        return;
    }

    uint16_t period = (uint16_t)(16000000UL / freq);

    TB0CCR0  = period;
    TB0CCR6  = period / 2;
    TB0CCTL6 = OUTMOD_7;

    TB0CTL   = TBSSEL__SMCLK | MC__UP | TBCLR;   // Up mode  start timer
}

void Buzzer_PlayNote(uint16_t freq, uint16_t duration_ms)
{
    Buzzer_SetFrequency(freq);
    delay_ms(duration_ms);
    Buzzer_SetFrequency(0);
    delay_ms(40);
}

void Buzzer_Stop(void)
{
    Buzzer_SetFrequency(0);
}

// main.c
// Music Trainer

#include <msp430fr6989.h>
#include "music_trainer.h"

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog
    PM5CTL0 &= ~LOCKLPM5;       // Unlock GPIO

    MusicTrainer_Init();
    MusicTrainer_Run();

    // Idle forever
    while (1) {
        __no_operation();
    }
}

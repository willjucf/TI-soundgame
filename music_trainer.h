#ifndef MUSIC_TRAINER_H_
#define MUSIC_TRAINER_H_

#include <stdint.h>

// Joystick direction
typedef enum {
    DIR_CENTER = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} JoystickDir;

void delay_ms(uint16_t ms);

// Clock and LCD
void Init_ClockSystem(void);
void LCD_InitGraphics(void);

// Joystick
void Joystick_Init(void);
void Joystick_ReadAxes(uint16_t *py, uint16_t *px);
JoystickDir Joystick_GetDir(void);

// Buzzer
void Buzzer_Init(void);
void Buzzer_PlayNote(uint16_t freq, uint16_t duration_ms);
void Buzzer_Stop(void);

// game interface
void MusicTrainer_Init(void);
void MusicTrainer_Run(void);

#endif // MUSIC_TRAINER_H_

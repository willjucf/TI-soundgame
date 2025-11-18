// music_trainer.c
// logic Music Trainer

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "Grlib/grlib/grlib.h"
#include "music_trainer.h"

// from logo.c
extern const Graphics_Image UCF_Logo;

// defined in trainer_hw.c
extern Graphics_Context g_sContext;

//  Note table C major

typedef struct {
    uint16_t freq;
    const char *name;
} NoteInfo;

static const NoteInfo g_notes[] = {
    {262,  "C"},
    {294,  "D"},
    {330,  "E"},
    {349,  "F"},
    {392,  "G"},
    {440,  "A"},
    {494,  "B"}
};

#define NUM_NOTES (sizeof(g_notes)/sizeof(g_notes[0]))

// declarations

static void draw_title_screen(void);
static void draw_selection_screen(uint8_t questions);
static void draw_question_header(uint8_t qIndex, uint8_t total, uint8_t score);
static void draw_sequence_info(uint8_t notes[3]);
static void draw_feedback_screen(uint8_t qIndex, uint8_t total,
                                 uint8_t score, uint8_t correct,
                                 const char *correctText);
static void draw_final_screen(uint8_t score, uint8_t total);

static uint8_t select_num_questions(void);
static void play_startup_tune(void);
static void run_quiz(uint8_t numQuestions);
static void generate_question(uint8_t notes[3]);
static uint8_t wait_for_updown_choice(const char *line1, const char *line2);

void MusicTrainer_Init(void)
{
    Init_ClockSystem();
    LCD_InitGraphics();
    Joystick_Init();
    Buzzer_Init();

    // Seed RNG from joystick noise
    uint16_t seedX, seedY;
    Joystick_ReadAxes(&seedY, &seedX);
    unsigned int seed = ((unsigned int)seedX << 5)
                      ^ ((unsigned int)seedY << 1)
                      ^  (unsigned int)seedX;

    if (seed == 0) {
        seed = 1;
    }
    srand(seed);
}

void MusicTrainer_Run(void)
{
    draw_title_screen();
    play_startup_tune();

    uint8_t numQuestions = select_num_questions();
    run_quiz(numQuestions);
}

// helpers

static void draw_title_screen(void)
{
    Graphics_clearDisplay(&g_sContext);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"EEL 4742C",
                                AUTO_STRING_LENGTH,
                                64, 10, TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Music Trainer",
                                AUTO_STRING_LENGTH,
                                64, 25, TRANSPARENT_TEXT);

    Graphics_drawImage(&g_sContext, &UCF_Logo, 32, 40);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_CYAN);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Use the joystick",
                                AUTO_STRING_LENGTH,
                                64, 100, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"to play!",
                                AUTO_STRING_LENGTH,
                                64, 110, TRANSPARENT_TEXT);
}

static void draw_selection_screen(uint8_t questions)
{
    char buf[20];

    Graphics_clearDisplay(&g_sContext);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Select # Questions",
                                AUTO_STRING_LENGTH,
                                64, 20, TRANSPARENT_TEXT);

    snprintf(buf, sizeof(buf), "%d", (int)questions);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buf,
                                AUTO_STRING_LENGTH,
                                64, 45, TRANSPARENT_TEXT);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_CYAN);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"LEFT: -1   RIGHT: +1",
                                AUTO_STRING_LENGTH,
                                64, 75, TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"UP: start",
                                AUTO_STRING_LENGTH,
                                64, 90, TRANSPARENT_TEXT);
}

static void draw_question_header(uint8_t qIndex, uint8_t total, uint8_t score)
{
    char buf[20];

    Graphics_clearDisplay(&g_sContext);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Music Trainer",
                                AUTO_STRING_LENGTH,
                                64, 8, TRANSPARENT_TEXT);

    snprintf(buf, sizeof(buf), "Q %d / %d", (int)qIndex, (int)total);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buf,
                                AUTO_STRING_LENGTH,
                                64, 22, TRANSPARENT_TEXT);

    snprintf(buf, sizeof(buf), "Score: %d", (int)score);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buf,
                                AUTO_STRING_LENGTH,
                                64, 36, TRANSPARENT_TEXT);
}

static void draw_sequence_info(uint8_t notes[3])
{
    (void)notes;

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"3 notes played: 1,2,3",
                                AUTO_STRING_LENGTH,
                                64, 50, TRANSPARENT_TEXT);
}

static void draw_feedback_screen(uint8_t qIndex, uint8_t total,
                                 uint8_t score, uint8_t correct,
                                 const char *correctText)
{
    char buf[20];

    Graphics_clearDisplay(&g_sContext);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    snprintf(buf, sizeof(buf), "Q %d / %d", (int)qIndex, (int)total);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buf,
                                AUTO_STRING_LENGTH,
                                64, 15, TRANSPARENT_TEXT);

    snprintf(buf, sizeof(buf), "Score: %d", (int)score);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buf,
                                AUTO_STRING_LENGTH,
                                64, 30, TRANSPARENT_TEXT);

    if (correct) {
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"Correct!",
                                    AUTO_STRING_LENGTH,
                                    64, 55, TRANSPARENT_TEXT);
    } else {
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"Wrong",
                                    AUTO_STRING_LENGTH,
                                    64, 55, TRANSPARENT_TEXT);
    }

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_CYAN);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Answer:",
                                AUTO_STRING_LENGTH,
                                64, 75, TRANSPARENT_TEXT);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)correctText,
                                AUTO_STRING_LENGTH,
                                64, 90, TRANSPARENT_TEXT);
}

static void draw_final_screen(uint8_t score, uint8_t total)
{
    char buf[20];

    Graphics_clearDisplay(&g_sContext);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Game Over",
                                AUTO_STRING_LENGTH,
                                64, 20, TRANSPARENT_TEXT);

    snprintf(buf, sizeof(buf), "Score: %d / %d", (int)score, (int)total);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buf,
                                AUTO_STRING_LENGTH,
                                64, 40, TRANSPARENT_TEXT);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_CYAN);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Press RESET",
                                AUTO_STRING_LENGTH,
                                64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"to play again",
                                AUTO_STRING_LENGTH,
                                64, 82, TRANSPARENT_TEXT);
}

//  Startup tune

static void play_startup_tune(void)
{
    static const uint8_t seq[] = {0, 2, 4, 5, 4, 2, 0, 4, 2, 0, 4, 5, 4};
    uint8_t length = sizeof(seq) / sizeof(seq[0]);
    uint8_t i;

    for (i = 0; i < length; i++) {
        uint8_t idx = seq[i];
        if (idx < NUM_NOTES) {
            Buzzer_PlayNote(g_notes[idx].freq, 400);
        } else {
            delay_ms(400);
        }
    }
    Buzzer_Stop();
}

//  Question-selection and random note

static uint8_t select_num_questions(void)
{
    uint8_t questions = 10;
    JoystickDir lastDir = DIR_CENTER;

    // Draw once at start
    draw_selection_screen(questions);

    while (1) {
        JoystickDir dir = Joystick_GetDir();

        if (dir != lastDir) {
            if (lastDir == DIR_CENTER) {
                if (dir == DIR_LEFT && questions > 5) {
                    questions--;
                    draw_selection_screen(questions);
                } else if (dir == DIR_RIGHT && questions < 20) {
                    questions++;
                    draw_selection_screen(questions);
                } else if (dir == DIR_UP) {
                    // Debounc
                    while (Joystick_GetDir() == DIR_UP) {
                        ;
                    }
                    return questions;
                }
            }
            lastDir = dir;
        }
    }
}

static uint8_t wait_for_updown_choice(const char *line1, const char *line2)
{
    // Make sure joystick is centered
    while (Joystick_GetDir() != DIR_CENTER) {
        ;
    }
    delay_ms(80);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_CYAN);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)line1,
                                AUTO_STRING_LENGTH,
                                64, 75, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)line2,
                                AUTO_STRING_LENGTH,
                                64, 90, TRANSPARENT_TEXT);

    while (1) {
        JoystickDir dir = Joystick_GetDir();
        if (dir == DIR_UP) {
            // wait
            while (Joystick_GetDir() == DIR_UP) {
                ;
            }
            return 1;   // "Up" = later note higher
        } else if (dir == DIR_DOWN) {
            while (Joystick_GetDir() == DIR_DOWN) {
                ;
            }
            return 0;   // "Down" = later note lower
        }
    }
}

static void generate_question(uint8_t notes[3])
{
    // Pick three random notes and make sure they are different
    notes[0] = (uint8_t)(rand() % NUM_NOTES);

    do {
        notes[1] = (uint8_t)(rand() % NUM_NOTES);
    } while (notes[1] == notes[0]);

    do {
        notes[2] = (uint8_t)(rand() % NUM_NOTES);
    } while (notes[2] == notes[1]);
}


static void run_quiz(uint8_t numQuestions)
{
    uint8_t q;
    uint8_t score = 0;

    for (q = 1; q <= numQuestions; q++) {
        uint8_t notes[3];
        uint8_t correctUp1, correctUp2;
        uint8_t ansUp1, ansUp2;
        char correctText[16];

        generate_question(notes);

        // "Up" means the LATER note in the pair has HIGHER pitch
        correctUp1 = (g_notes[notes[1]].freq > g_notes[notes[0]].freq);
        correctUp2 = (g_notes[notes[2]].freq > g_notes[notes[1]].freq);

        draw_question_header(q, numQuestions, score);
        draw_sequence_info(notes);

        // Play the notes
        uint8_t i;
        for (i = 0; i < 3; i++) {
            Buzzer_PlayNote(g_notes[notes[i]].freq, 300);
        }
        Buzzer_Stop();

        // Q1. 2nd note vs first.
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"1) 2nd vs 1st note",
                                    AUTO_STRING_LENGTH,
                                    64, 65, TRANSPARENT_TEXT);

        ansUp1 = wait_for_updown_choice("UP = later higher",
                                        "DOWN = later lower");

        // Q2. 3rd note vs 2nd.
        Graphics_clearDisplay(&g_sContext);
        draw_question_header(q, numQuestions, score);
        draw_sequence_info(notes);

        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"2) 3rd vs 2nd note",
                                    AUTO_STRING_LENGTH,
                                    64, 65, TRANSPARENT_TEXT);

        ansUp2 = wait_for_updown_choice("UP = later higher",
                                        "DOWN = later lower");

        // both answers must be right
        uint8_t correct = (ansUp1 == correctUp1) && (ansUp2 == correctUp2);
        if (correct) {
            score++;
            Buzzer_PlayNote(523, 200);   // success
        } else {
            Buzzer_PlayNote(196, 200);   // fail
        }

        const char *s1 = correctUp1 ? "Up" : "Down";
        const char *s2 = correctUp2 ? "Up" : "Down";
        snprintf(correctText, sizeof(correctText), "1:%s 2:%s", s1, s2);

        draw_feedback_screen(q, numQuestions, score, correct, correctText);
        delay_ms(1200);
    }

    draw_final_screen(score, numQuestions);
}

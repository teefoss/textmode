#ifndef textmode_h
#define textmode_h

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOS_NUMCOLORS       16
#define DOS_CHAR_WIDTH      8

typedef enum
{
    DOS_MODE40 = 8,  // 'wide' characters (8 x 8)
    DOS_MODE80 = 16, // normal characters (8 x 16, default)
} DOS_Mode;

typedef struct
{
    uint8_t fg_color      : 4; // foreground color
    uint8_t bg_color      : 4; // background color
    uint8_t transparent   : 1; // background is transparent
    uint8_t blink         : 1; // text blinks
} DOS_Attributes;

typedef struct
{
    uint8_t character;
    DOS_Attributes attributes;
} DOS_CharInfo;

typedef struct DOS_Console DOS_Console;

typedef enum
{
    DOS_BLACK,
    DOS_BLUE,
    DOS_GREEN,
    DOS_CYAN,
    DOS_RED,
    DOS_MAGENTA,
    DOS_BROWN,
    DOS_WHITE,
    DOS_GRAY,
    DOS_BRIGHT_BLUE,
    DOS_BRIGHT_GREEN,
    DOS_BRIGHT_CYAN,
    DOS_BRIGHT_RED,
    DOS_BRIGHT_MAGENTA,
    DOS_YELLOW,
    DOS_BRIGHT_WHITE
} DOS_Color;

// console cursor appearance options
typedef enum
{
    DOS_CURSOR_NONE,    // don't show the cursor
    DOS_CURSOR_NORMAL,  // bottom 20% cell
    DOS_CURSOR_FULL     // cursor fills entire cell
} DOS_CursorType;

extern const SDL_Color dos_palette[DOS_NUMCOLORS + 1];

void DOS_SetColor(SDL_Renderer * renderer, DOS_Color color);
void DOS_SetColorAlpha(SDL_Renderer * renderer, DOS_Color color, uint8_t alpha);
SDL_Color DOS_CGAToSDL(DOS_Color color);

void DOS_RenderChar(SDL_Renderer * renderer, int x, int y, DOS_Mode mode, uint8_t character);
int DOS_RenderString(SDL_Renderer * renderer, int x, int y, DOS_Mode mode, const char * format,...);
int DOS_StringWidth(const char * format, ...);
DOS_Attributes DOS_DefaultAttributes(void);

// CONSOLE

DOS_Console * DOS_CreateConsole(int w, int h, DOS_Mode text_style);
void DOS_FreeConsole(DOS_Console * console);
void DOS_SetActiveConsole(DOS_Console * console);
void DOS_ClearScreen();
void DOS_ClearBackground(void);
void DOS_SetTransparentBackground(void);
void DOS_RenderConsole(SDL_Renderer * renderer, DOS_Console * console, int x, int y);
void DOS_GotoXY(int x, int y);
void DOS_SetForeground(int color);
void DOS_SetBackground(int color);
void DOS_PrintChar(uint8_t ch);
void DOS_PrintString(const char * format, ...);
int  DOS_GetX(void);
int  DOS_GetY(void);
DOS_CharInfo DOS_GetChar();
void DOS_SetChar(DOS_CharInfo * char_info);
void DOS_SetBlink(bool blink);
void DOS_SetTabSize(int tab_size);
void DOS_SetCursorType(DOS_CursorType type);
void DOS_SetScale(int scale);
void DOS_SetMargin(int margin);

// SCREEN
// TODO: border color?

void DOS_InitScreen(const char * window_name, int console_w, int console_h, DOS_Mode text_style, int border_size);
void DOS_DrawScreen(void);
void DOS_DrawScreenEx(void (* user_function)(void * data), void * user_data);
void DOS_SwitchPage(int new_page);
int DOS_CurrentPage(void);
SDL_Window * DOS_GetWindow(void);
SDL_Renderer * DOS_GetRenderer(void);
void DOS_SetFullscreen(bool fullscreen);
void DOS_ToggleFullscreen(void);
void DOS_SetScreenScale(int scale);
void DOS_IncreaseScreenScale(void);
void DOS_DecreaseScreenScale(void);
float DOS_LimitFrameRate(int fps);

// SOUND
// PC beeper emulation. (Monophonic square wave playback).
// All sound is played asynchronously.

/**
 *  Initialize sound. Must be called before using other sound functions.
 */
void DOS_InitSound(void);

/** 
 *  Set the volume for all playback.
 *  Valid range: 1-15 (default: 5)
 */
void DOS_SetVolume(unsigned value);

/**
 *  Play a frequency of given duration, stopping any currently playing sound.
 */
void DOS_Sound(unsigned frequency, unsigned milliseconds);

/** 
 *  Play a pitch of 800 Hz for 200 milliseconds
 */
void DOS_Beep(void);

/**
 *  Add a frequency to a queue to be played. If sound is not already playing,
 *  playback is immediate. Can be called multiple times successtion to play
 *  multiple pitches.
 */
void DOS_AddSound(unsigned frequency, unsigned milliseconds);

/**
 *  Stops any sound that is currently playing.
 */
void DOS_StopSound(void);

/**
 *  Mute or unmute all sound.
 */
void DOS_Mute(bool muted);

/** Returns true if sound is currently playing.
 *  - Note: This is useful if you wish to suspend the program while sound
 *  plays, for example
 */
bool DOS_SoundIsPlaying(void);

/**
 *  Play musical notes.
 *
 *  (Spaces in play strings are optional and are ignored.)
 *
 *                                   EXAMPLE
 *    notes:  a b c d e f g          "ccggaag" (beginning of Twinkle, Twinkle)
 *     flat:  [note]-                "a- b-" (plays A-flat, B-flat)
 *    sharp:  [note]+                "c+ d+" (plays C-sharp, D-sharp)
 *   length:  l[int(1, 2, 4...128)]  "l8 cdef l2 g" (plays eight-notes, etc)
 *    tempo:  t[bmp]                 "t80 l2 g+ef+g+"
 *   octave:  o[int(1-6)]            "o6 ba+ba+b" (plays notes in octave 6)
 *            > increase octave      "a b > c"
 *            < decrease octave      "l2 e l4 dc < b"
 *    music:  m[s, n, l]             "ms l16 cdef ml l8 g b > ms c < c"
 *            s: staccato (6/8 length)
 *            n: normal   (7/8 length, default)
 *            l: legato   (8/8 length)
 */
void DOS_Play(const char * string, ...);


// -----------------------------------------------------------------------------
// The MS-DOS ASCII Character Set

enum
{
    DOS_NUL               = 0x00,
    DOS_FACE1             = 0x01,
    DOS_FACE2             = 0x02,
    DOS_HEART             = 0x03,
    DOS_DIAMOND           = 0x04,
    DOS_CLUB              = 0x05,
    DOS_SPADE             = 0x06,
    DOS_DOT1              = 0x07,
    DOS_DOT2              = 0x08,
    DOS_RING1             = 0x09,
    DOS_RING2             = 0x0A,
    DOS_MALE              = 0x0B,
    DOS_FEMALE            = 0x0C,
    DOS_NOTE1             = 0x0D,
    DOS_NOTE2             = 0x0E,
    DOS_STAR              = 0x0F,
    DOS_TRI_RIGHT         = 0x10,
    DOS_TRI_LEFT          = 0x11,
    DOS_UPDOWNARROW1      = 0x12,
    DOS_DBL_EXCLAM        = 0x13,
    DOS_PARAGRAPH         = 0x14,
    DOS_SECTION           = 0x15,
    DOS_CURSOR            = 0x16,
    DOS_UPDOWNARROW2      = 0x17,
    DOS_UPARROW           = 0x18,
    DOS_DOWNARROW         = 0x19,
    DOS_RIGHTARROW        = 0x1A,
    DOS_LEFTARROW         = 0x1B,
    DOS_REV_INV_NOT       = 0x1C,
    DOS_RIGHTLEFTARROW    = 0x1D,
    DOS_TRI_UP            = 0x1E,
    DOS_TRI_DOWN          = 0x1F,
    
    // printable characters omitted
    
    DOS_DELETE            = 0x7F,
    DOS_C_CEDI_UPPER      = 0x80,
    DOS_U_UMLT_LOWER      = 0x81,
    DOS_E_ACUT_LOWER      = 0x82,
    DOS_A_CIRC_LOWER      = 0x83,
    DOS_A_UMLT_LOWER      = 0x84,
    DOS_A_GRAV_LOWER      = 0x85,
    DOS_A_RING_LOWER      = 0x86,
    DOS_C_CEDI_LOWER      = 0x87,
    DOS_E_CIRC_LOWER      = 0x88,
    DOS_E_UMLT_LOWER      = 0x89,
    DOS_E_GRAV_LOWER      = 0x8A,
    DOS_I_UMLT_LOWER      = 0x8B,
    DOS_I_CIRC_LOWER      = 0x8C,
    DOS_I_GRAV_LOWER      = 0x8D,
    DOS_A_UMLT_UPPER      = 0x8E,
    DOS_A_RING_UPPER      = 0x8F,
    DOS_E_ACUT_UPPER      = 0x90,
    DOS_AE_LOWER          = 0x91,
    DOS_AE_UPPER          = 0x92,
    DOS_O_CIRC_LOWER      = 0x93,
    DOS_O_UMLT_LOWER      = 0x94,
    DOS_O_GRAV_LOWER      = 0x95,
    DOS_U_CIRC_LOWER      = 0x96,
    DOS_U_GRAV_LOWER      = 0x97,
    DOS_Y_UMLT_LOWER      = 0x98,
    DOS_O_UMLT_UPPER      = 0x99,
    DOS_U_UMLT_UPPER      = 0x9A,
    DOS_CENT              = 0x9B,
    DOS_POUND             = 0x9C,
    DOS_YEN               = 0x9D,
    DOS_PESETA            = 0x9E,
    DOS_F_HOOK            = 0x9F,
    DOS_A_ACUT_LOWER      = 0xA0,
    DOS_I_ACUT_LOWER      = 0xA1,
    DOS_O_ACUT_LOWER      = 0xA2,
    DOS_U_ACUT_LOWER      = 0xA3,
    DOS_N_TILD_LOWER      = 0xA4,
    DOS_N_TILD_UPPER      = 0xA5,
    DOS_FEM_ORD           = 0xA6,
    DOS_MASC_ORD          = 0xA7,
    DOS_INV_QUESTION      = 0xA8,
    DOS_NOT_REVERSED      = 0xA9,
    DOS_NOT               = 0xAA,
    DOS_FRAC_HALF         = 0xAB,
    DOS_FRAC_QUARTER      = 0xAC,
    DOS_INV_EXCLAMATION   = 0xAD,
    DOS_LEFT_ANG_QUOTE    = 0xAE,
    DOS_RIGHT_AND_QUOTE   = 0xAF,
    DOS_BLOCK_LIGHT       = 0xB0,
    DOS_BLOCK_MEDIUM      = 0xB1,
    DOS_BLOCK_DARK        = 0xB2,

    // box-drawing characters omitted
    
    DOS_BLOCK_FULL        = 0xDB,
    DOS_BLOCK_BOTTOM      = 0xDC,
    DOS_BLOCK_LEFT        = 0xDD,
    DOS_BLOCK_RIGHT       = 0xDE,
    DOS_BLOCK_TOP         = 0xDF,
    DOS_ALPHA             = 0xE0,
    DOS_BETA              = 0xE1,
    DOS_GAMMA             = 0xE2,
    DOS_PI                = 0xE3,
    DOS_SIGMA_UPPER       = 0xE4,
    DOS_SIGMA_LOWER       = 0xE5,
    DOS_MU                = 0xE6,
    DOS_TAU               = 0xE7,
    DOS_PHI_UPPER         = 0xE8,
    DOS_THETA             = 0xE9,
    DOS_OMEGA             = 0xEA,
    DOS_DELTA             = 0xEB,
    DOS_INFINITY          = 0xEC,
    DOS_PHI_LOWER         = 0xED,
    DOS_EPSILON           = 0xEE,
    DOS_INTERSECTION      = 0xEF,
    DOS_IDENTICAL         = 0xF0,
    DOS_PLUSMINUS         = 0xF1,
    DOS_GT_OR_EQ          = 0xF2,
    DOS_LT_OR_EQ          = 0xF3,
    DOS_INTEGRAL_TOP      = 0xF4,
    DOS_INTEGRAL_BOTTOM   = 0xF5,
    DOS_DIVISION          = 0xF6,
    DOS_ALMOST_EQUALS     = 0xF7,
    DOS_DEGREE            = 0xF8,
    DOS_INTERPUNCT1       = 0xF9,
    DOS_INTERPUNCT2       = 0xFA,
    DOS_RADICAL           = 0xFB,
    DOS_SUPER_N           = 0xFC,
    DOS_SUPER_2           = 0xFD,
    DOS_SQUARE            = 0xFE,
    DOS_NBSP              = 0xFF
};

#ifdef __cplusplus
}
#endif

#endif /* textmode_h */

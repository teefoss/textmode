# TextMode

Used in conjuction with SDL 2, TextMode is a library for creating programs that use the IBM code page 437 character set and CGA color. This library would be useful for:

* Creating MS-DOS-looking programs.
* Creating retro-looking text-based games.
* An easy way to integrate retro-looking text, color, and sound into a retro-looking game.
* Learning programming in a way that includes a simple interface (use `DOS_Screen`) and has more interesting output (color and sound) than a modern terminal.

Simply include `textmode.h` and link with the TextMode library and SDL2.

```c
#include <textmode.h>
```

```bash
cc main.c -lSDL2 -ltextmode
```

This library provides levels of flexibility, allowing simple rendering of CP437 characters, the creation of consoles that can be used within a regular program, to full, text mode programs.



## Color

TextMode defines the symbols for the 16 CGA Colors, `DOS_BLUE`, `DOS_BRIGHT_GREEN`, etc, and general functions `DOS_SetColor()` and `DOS_SetColorAlpha()` that can be used for any rendering where CGA Color is needed.

```c
#include <textmode.h>
...
SDL_Renderer * renderer;
...
DOS_SetColor(renderer, DOS_BLUE);
SDL_RenderDrawRect(renderer, &rect);
```



## Sound

TextMode provides several functions to play PC-speaker-like beeps. To use sound in a program, first call `DOS_InitSound()`.

```c
DOS_InitSound();
...
DOS_Beep(); // just a basic beep
DOS_Sound(440, 1000); // A440 for 1000 ms
...
DOS_SoundAsync(440, 1000); // play a single pitch asychronously
...
// or, multiple pitches asychronously
DOS_QueueSound(440, 200);
DOS_QueueSound(550, 200);
DOS_QueueSound(660, 200);
DOS_QueueSound(880, 500);
DOS_PlayQueuedSound();
...
// play a series of musical notes with a BASIC-style PLAY string:
DOS_Play("t132 o5 l2 c l4 eg < b l8 b l16 > c d l4 c");
```



## Basic Characters

For programs that need nothing more than some text here and there, TextMode provide functions for basic rendering of CP437 characters: `DOS_RenderChar()` and `DOS_RenderString()`.

There are two styles of characters, normal (`DOS_MODE80`) and wide (`DOS_MODE40`).

```c
#include <textmode.h>
...
SDL_Renderer * renderer;
...
DOS_SetColor(renderer, DOS_BRIGHT_GREEN);
DOS_RenderString(renderer, 0, 0, DOS_MODE80, "Hello, world");
SDL_RenderPresent(renderer);
```



## Character Sprite Sheet

If you're doing a lot of rendering of text, or have a custom color palette, `DOS_Text` might provide better performance. Creating a `DOS_Text` generates a sprite sheet of CP437 characters.

```c
#include <textmode.h>
...
SDL_Color my_palette[3] = {
    { 255, 0, 0, 255 },
    { 0, 255, 0, 255 },
    { 0, 0, 255, 255 }
};
SDL_Renderer * renderer;
...
DOS_Text * text = DOS_MakeText(renderer, DOS_MODE40, my_palette, 3);
DOS_CharInfo attr = DOS_DefaultAttributes(); // white text on black background
DOS_TRenderString(text, 0, 0, &attr, "Hello, galaxy");
...
DOS_DestroyText(text);
```

To create text with the default CGA palette, use the global variable `dos_palette`, which has 16 colors (`DOS_NUMCOLORS`).

```c
DOS_Text * text = DOS_MakeText(renderer, DOS_MODE80, dos_palette, DOS_NUMCOLORS);
```



## Console

If you need a console that can be rendered anywhere within a window, use a `DOS_Console`.

```c
SDL_Renderer * renderer;
...
#define ROWS 10
#define COLS 15
DOS_Console * console = DOS_NewConsole(renderer, COLS, ROWS, DOS_MODE80);
DOS_CSetCursorType(console, DOS_CURSOR_FULL);
...
DOS_ClearConsole(console); // clear to default attributes, cursor at 0, 0
DOS_CGotoXY(console, 2, 2);
DOS_CSetForeground(DOS_CYAN);
DOS_CPrintString(console, "console size: %d x %d", COLS, ROWS);
...
DOS_RenderConsole(console, 0, 0);
SDL_RenderPresent(renderer);
```



## Screen

Finally, if you need a full MS-DOS-like program with a multiple page console and colored border, use a `DOS_Screen`. This use case is the least flexible, but provides the simplest interface for writing a text mode program.

```c
#include <textmode.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#define ROWS            25
#define COLS            80
#define BORDER_SIZE     4

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    DOS_InitScreen("My App", COLS, ROWS, DOS_MODE80, BORDER_SIZE);

    DOS_ClearScreen();
    DOS_GotoXY(5, 5);
    DOS_SetForeground(DOS_YELLOW);
    DOS_PrintString("Hey, Earth!");
    
    bool run = true;
    while ( run ) {
        SDL_Event event;
        while ( SDL_PollEvent(&event) )
            if ( event.type == SDL_QUIT )
                run = false;

        DOS_RenderScreen();
    }
    
    SDL_Quit();
    return 0;
}

```


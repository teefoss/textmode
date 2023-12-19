#include <SDL2/SDL.h>
#include <stdlib.h>
#include "textmode.h"

int HandelKey(SDL_Keycode key)
{
    switch ( key )
    {
        case SDLK_1:
            DOS_SwitchPage(1);
            break;
        case SDLK_2:
            DOS_SwitchPage(2);
            break;
        case SDLK_0:
            DOS_SwitchPage(0);
            break;

        case SDLK_s:
            for ( int i = 0; i < 20; i++ ) {
                DOS_AddSound(arc4random_uniform(400)+400, 100);
            }
            break;
        case SDLK_p:
            DOS_Play("t160 l16 cdefgfed l32 cdefgfed l4 c");
            break;
        case SDLK_ESCAPE:
            return 0;
        case SDLK_c:
            DOS_ClearScreen();
            break;
        case SDLK_BACKSLASH:
            DOS_ToggleFullscreen();
            break;
        case SDLK_EQUALS:
            DOS_IncreaseScreenScale();
            break;
        case SDLK_MINUS:
            DOS_DecreaseScreenScale();
            break;
        default:
            break;
    }
    
    return 1;
}


int main()
{
    puts("\nDOSApp Test Program");
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    atexit(SDL_Quit);
    
    DOS_InitSound();
    
    int w = 80;
    int h = 35;
    
    DOS_InitScreen("test", w, h, DOS_MODE80, 8);
    //DOS_ToggleFullscreen();
//    DOS_SetBorderColor(DOS_BLUE);
    
    // debug corners
    DOS_GotoXY(0, 0);
    DOS_PrintChar('X');
    DOS_GotoXY(w - 1, 0);
    DOS_PrintChar('X');
    DOS_GotoXY(0, h - 1);
    DOS_PrintChar('X');
    DOS_GotoXY(w - 1, h - 1);
    DOS_PrintChar('X');
    
    // debug background
    DOS_GotoXY(2, 2);
    for ( int ch = 'A'; ch <= 'Z'; ch++ ) {
        DOS_SetBackground(ch % DOS_NUMCOLORS);
        DOS_PrintChar(ch);
    }
    
    // debug foreground
    DOS_GotoXY(2, 3);
    DOS_SetBackground(DOS_BLACK);
    for ( int ch = 'A'; ch <= 'Z'; ch++ ) {
        DOS_SetForeground(ch % DOS_NUMCOLORS);
        DOS_PrintChar(ch);
    }

    // debug PrintString
    DOS_GotoXY(2, 4);
    DOS_PrintString("Hello there, %d", 10);
    
    DOS_SwitchPage(1);
    DOS_GotoXY(10, 10);
    DOS_SetForeground(DOS_BRIGHT_MAGENTA);
    DOS_PrintChar(DOS_FACE1);
    DOS_SwitchPage(0);
    
    int run = 1;
    while ( run )
    {
        DOS_LimitFrameRate(30);
        
        SDL_Event ev;
        while ( SDL_PollEvent(&ev) )
        {
            switch ( ev.type )
            {
                case SDL_QUIT:
                    run = 0;
                    break;
                case SDL_KEYDOWN:
                    run = HandelKey(ev.key.keysym.sym);
                    break;
                default: break;
            }
        }
        
        DOS_GotoXY(20, 20);
        DOS_SetForeground(arc4random_uniform(8) + 7);
        DOS_PrintChar(DOS_FACE2);
        
        DOS_DrawScreen();
    }
    
    return EXIT_SUCCESS;
}

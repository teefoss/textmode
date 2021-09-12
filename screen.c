#include "textmode.h"

#define DOS_NUM_PAGES   16
#define ACTIVE_PAGE     screen.pages[screen.active_page]

struct DOS_Screen
{
    SDL_Window *    window;
    int             window_scale;
    bool            fullscreen;
    
    SDL_Renderer *  renderer;
    
    int             border_size;
    int             border_color;
    
    bool            blink;
    
    DOS_Console *   pages[DOS_NUM_PAGES];
    int             active_page;
    int             width;  // console size
    int             height;
    DOS_Mode        mode;
    int             render_x; // render position of the console
    int             render_y;
};

static DOS_Screen screen;

// -----------------------------------------------------------------------------

static void FreeScreen()
{
    
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
    
    for ( int i = 0; i < DOS_NUM_PAGES; i++ ) {
        DOS_FreeConsole(screen.pages[i]);
    }
}


static void NewScreenError(const char * message)
{
    fprintf(stderr, "DOS_InitScreen error: %s\n", message);
    FreeScreen();
    
    exit(EXIT_FAILURE);
}


static SDL_Rect ConsoleSizeInPixels()
{
    SDL_Rect rect;
    rect.w = screen.width * DOS_CHAR_WIDTH;
    rect.h = screen.height * screen.mode;
    
    return rect;
}

#if 0 // TODO: remove
static SDL_Rect WindowSize()
{
    SDL_Rect rect;
    SDL_GetWindowSize(screen.window, &rect.w, &rect.h);
    
    return rect;
}
#endif

static SDL_Rect UnscaledWindowRect()
{
    SDL_Rect rect = ConsoleSizeInPixels();
    
    rect.x = SDL_WINDOWPOS_CENTERED;
    rect.y = SDL_WINDOWPOS_CENTERED;
    rect.w += screen.border_size * 2;
    rect.h += screen.border_size * 2;
    
    return rect;
}

// -----------------------------------------------------------------------------

void
DOS_InitScreen
(   const char * window_name,
    int console_w, // TODO: window x, y?
    int console_h,
    DOS_Mode mode,
    int border_size )
{
    screen.width        = console_w;
    screen.height       = console_h;
    screen.mode         = mode;
    screen.border_size  = border_size;
    screen.border_color = DOS_BLACK;
    screen.active_page  = 0;
    screen.blink        = false;
    screen.fullscreen   = false;
    screen.window_scale = 1;
    
    SDL_Rect w = UnscaledWindowRect();
    uint32_t flags = 0;
    //flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    screen.window = SDL_CreateWindow(window_name, w.x, w.y, w.w, w.h, flags);
    
    if ( screen.window == NULL ) {
        return NewScreenError("could not create SDL window");
    }
    
    screen.renderer = SDL_CreateRenderer(screen.window, -1, 0);
    
    if ( screen.renderer == NULL ) {
        return NewScreenError("could not create SDL renderer");
    }

#if 0
    int rw;
    SDL_GetRendererOutputSize(screen.renderer, &rw, NULL);
    float x_scale = (float)rw / (float)w.w;

    if ( x_scale != 1.0f ) {
        SDL_RenderSetScale(screen.renderer, x_scale, x_scale);
    }
#endif
    
    for ( int i = 0; i < DOS_NUM_PAGES; i++ ) {
        screen.pages[i] = DOS_CreateConsole
        (   screen.renderer,
            console_w,
            console_h,
            mode );
        
        if ( screen.pages[i] == NULL ) {
            return NewScreenError("could not create console");
        }
        
//        if ( x_scale != 1.0f ) {
//            DOS_CSetScale(screen.pages[i], x_scale);
//        }
    }
                    
    DOS_SetFullscreen(false);
    
    atexit(FreeScreen);
}

void DOS_SwitchPage(int new_page)
{// TODO: test
    if ( new_page < 0 || new_page >= DOS_NUM_PAGES ) {
        return;
    }
    
    screen.active_page = new_page;
}

int DOS_CurrentPage()
{
    return screen.active_page;
}

void DOS_DrawScreen()
{
    DOS_SetColor(screen.renderer, screen.border_color);
    SDL_RenderClear(screen.renderer);
    DOS_RenderConsole(ACTIVE_PAGE, screen.render_x, screen.render_y);
    SDL_RenderPresent(screen.renderer);
}

void DOS_DrawScreenEx(void (* user_function)(void * data), void * user_data)
{
    DOS_SetColor(screen.renderer, screen.border_color);
    SDL_RenderClear(screen.renderer);
    DOS_RenderConsole(ACTIVE_PAGE, screen.render_x, screen.render_y);
    
    if ( user_function ) {
        user_function(user_data);
    }
    
    SDL_RenderPresent(screen.renderer);
}

SDL_Window * DOS_GetWindow()
{
    return screen.window;
}

SDL_Renderer * DOS_GetRenderer()
{
    return screen.renderer;
}

void DOS_PrintChar(uint8_t ch)
{
    DOS_CPrintChar(ACTIVE_PAGE, ch);
}

void DOS_PrintString(const char * format, ...)
{    
    va_list args;
    char buf[1000]; // TODO: calculate len
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    DOS_CPrintString(ACTIVE_PAGE, buf);
}


void DOS_ClearScreen()
{
    DOS_ClearConsole(ACTIVE_PAGE);
}


void DOS_GotoXY(int x, int y)
{
    DOS_CGotoXY(ACTIVE_PAGE, x, y);
}


void DOS_SetBlink(bool blink)
{
    DOS_CSetBlink(ACTIVE_PAGE, blink);
}


void DOS_SetBorderColor(int color)
{
    screen.border_color = color;
}


void DOS_SetForeground(int color)
{
    DOS_CSetForeground(ACTIVE_PAGE, color);
}


void DOS_SetBackground(int color)
{
    DOS_CSetBackground(ACTIVE_PAGE, color);
}


static void UpdateRenderScaleAndConsolePosition()
{
    SDL_Rect window;
    SDL_GetWindowSize(screen.window, &window.w, &window.h);
    
    SDL_Rect console = ConsoleSizeInPixels();
    SDL_Rect minimum_area = console;
 
    int margins = screen.border_size * 2;
    minimum_area.w += margins;
    minimum_area.h += margins;
        
    // calculate scale
    // let's assume the monitor is wider than it is tall...
    int scale = 1;
    while ( minimum_area.h * (scale + 1) <= window.h ) {
        ++scale;
    };
        
    // let the draw scale be updateth
    SDL_RenderSetScale(screen.renderer, scale, scale);
    
    // let the render position be updateth so in the middle it be put
    screen.render_x = (window.w/scale - console.w) / 2;
    screen.render_y = (window.h/scale - console.h) / 2;
}


void DOS_SetFullscreen(bool fullscreen)
{
    if ( fullscreen ) {
        SDL_SetWindowFullscreen(screen.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(screen.window, 0);
    }
    
    UpdateRenderScaleAndConsolePosition();
}


void DOS_ToggleFullscreen()
{
    screen.fullscreen = !screen.fullscreen;
    DOS_SetFullscreen(screen.fullscreen);
}


void DOS_SetScreenScale(int scale)
{
    if ( screen.fullscreen ) {
        return;
    }
    
    if ( scale < 1 ) {
        return;
    }
    
    screen.window_scale = scale;
    
    SDL_Rect base_size = UnscaledWindowRect();
    int center = SDL_WINDOWPOS_CENTERED;
    
    SDL_SetWindowSize(screen.window, base_size.w * scale, base_size.h * scale);
    SDL_SetWindowPosition(screen.window, center, center);
    
    UpdateRenderScaleAndConsolePosition();
}


void DOS_IncreaseScreenScale()
{
    DOS_SetScreenScale(screen.window_scale + 1);
}


void DOS_DecreaseScreenScale()
{
    DOS_SetScreenScale(screen.window_scale - 1);
}


void DOS_SetCursorType(DOS_CursorType type)
{
    DOS_CSetCursorType(ACTIVE_PAGE, type);
}


float DOS_LimitFrameRate(int fps)
{
    int interval = 1000 / fps;
    static int last = 0;
    int now;
    int elapsed_ms;
    float dt;
        
    do {
        now = SDL_GetTicks();
        elapsed_ms = now - last;
        
        if ( elapsed_ms > interval ) {
            break;
        }
        
        SDL_Delay(1);
    } while ( elapsed_ms < interval );

    dt = (now - last) / 1000.0f;
    last = now;
    
    return dt;
}


void DOS_ClearBackground(void)
{
    DOS_CClearBackground(ACTIVE_PAGE);
}


int DOS_GetX(void)
{
    return DOS_CGetX(ACTIVE_PAGE);
}


int DOS_GetY(void)
{
    return DOS_CGetY(ACTIVE_PAGE);
}


void DOS_SetMargin(int margin)
{
    DOS_CSetMargin(ACTIVE_PAGE, margin);
}

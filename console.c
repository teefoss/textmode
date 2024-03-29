#include "textmode.h"

struct DOS_Console
{
    int             mode;       // 8 or 16
    int             width;      // screen height in characters
    int             height;     // screen width in characters;
    int             cursor_x;
    int             cursor_y;
    int             fg_color;   // current foreground color
    int             bg_color;   // current background color
    int             tab_size;
    int             margin;     // \n's go here
    bool            blink;      // whether newly printed chars blink
    int             scale;
    SDL_Surface *   surface;
    DOS_CharInfo *  buffer;
    DOS_CursorType  cursor_type;
};

DOS_Console * _current_page;

// -----------------------------------------------------------------------------

static DOS_CharInfo * GetCell(DOS_Console * console, int x, int y)
{
    return console->buffer + y * console->width + x;
}

static void NewLine(DOS_Console * console)
{
    if ( console->cursor_y < console->height - 1 ) {
        console->cursor_x = console->margin;
        ++console->cursor_y;
    }
}

static void AdvanceCursor(DOS_Console * console, int amount)
{
    console->cursor_x += amount;
    
    if ( console->cursor_x >= console->width ) {
        console->cursor_x = console->width - 1;
        NewLine(console);
    }
}

static bool ValidCoord(DOS_Console * c, int x, int y)
{
    return x >= 0 && x < c->width && y >= 0 && y < c->height;
}

static DOS_Console * NewConsoleError(DOS_Console * c, const char * message)
{
    fprintf(stderr, "DOS_CreateConsole: %s\n", message);
    DOS_FreeConsole(c);
    
    return NULL;
}

DOS_Console * DOS_CreateConsole(int w, int h, DOS_Mode mode)
{
    DOS_Console * console = malloc( sizeof(*console) );
    
    if ( console == NULL )
        return NewConsoleError(NULL, "could not allocate memory for console");
    
    _current_page = console;
    console->mode           = mode;
    console->width          = w;
    console->height         = h;
    console->buffer         = NULL;
    console->blink          = false;
    console->tab_size       = 4;
    console->cursor_type    = DOS_CURSOR_NORMAL;
    console->margin         = 0;
    console->scale          = 1;
    
    console->buffer = calloc(w * h, sizeof(*console->buffer));
    
    if ( console->buffer == NULL ) {
        return NewConsoleError(console, "could not allocate buffer");
    }
    
    Uint32 rmask, gmask, bmask, amask;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif
    console->surface = SDL_CreateRGBSurface(0,
                                            w * DOS_CHAR_WIDTH,
                                            h * mode,
                                            32,
                                            rmask, gmask, bmask, amask);
    
    if ( console->surface == NULL ) {
        return NewConsoleError(console, "failed to create console surface");
    }
    
    DOS_ClearScreen();
    
    return console;
}

void DOS_FreeConsole(DOS_Console * console)
{
    if ( console ) {
        if ( console->buffer ) {
            free(console->buffer);
        }
        if ( console->surface ) {
            SDL_FreeSurface(console->surface);
        }
        free(console);
    }
}

void DOS_SetActiveConsole(DOS_Console * console)
{
    _current_page = console;
}

void DOS_ClearScreen()
{
    size_t size = sizeof(DOS_CharInfo) * _current_page->width * _current_page->height;
    memset(_current_page->buffer, 0, size);
    
    SDL_FillRect(_current_page->surface, NULL, 0);
    
    _current_page->cursor_x = 0;
    _current_page->cursor_y = 0;
    _current_page->fg_color = DOS_WHITE;
    _current_page->bg_color = DOS_BLACK;
}

void DOS_ClearBackground()
{
    for ( int y = 0; y < _current_page->height; y++ ) {
        for ( int x = 0; x < _current_page->width; x++ ) {
            DOS_CharInfo * cell = GetCell(_current_page, x, y);
            cell->attributes.bg_color = _current_page->bg_color;
        }
    }
}

void DOS_SetForeground(int color)
{
    _current_page->fg_color = color;
}

void DOS_SetBackground(int color)
{// TODO: test
    _current_page->bg_color = color;
}

void DOS_SetTransparentBackground()
{
    for ( int y = 0; y < _current_page->height; y++ ) {
        for ( int x = 0; x < _current_page->width; x++ ) {
            DOS_CharInfo * cell = GetCell(_current_page, x, y);
            cell->attributes.transparent = 1;
        }
    }
}

//static const SDL_Color transparent = { 0, 0, 0, 0 };

void SetPixel(int x, int y, const SDL_Color * c)
{
    Uint8 * pixels = (Uint8 *)_current_page->surface->pixels;
    int bpp = _current_page->surface->format->BytesPerPixel;
    Uint32 * target_pixel = (Uint32 *)(pixels + y * _current_page->surface->pitch + x * bpp);
    *target_pixel = SDL_MapRGBA(_current_page->surface->format, c->r, c->g, c->b, c->a);
}

void DOS_PrintChar(uint8_t ch)
{
    DOS_CharInfo * cell = GetCell(_current_page, _current_page->cursor_x, _current_page->cursor_y);
    cell->character = ch;
    cell->attributes.fg_color = _current_page->fg_color;
    cell->attributes.bg_color = _current_page->bg_color;
    cell->attributes.blink = _current_page->blink;
    
    const uint8_t * data;
    const uint8_t * DOS_Data8(uint8_t ch);
    const uint8_t * DOS_Data16(uint8_t ch);
    
    if ( _current_page->mode == DOS_MODE40 ) {
        data = DOS_Data8(ch);
    } else {
        data = DOS_Data16(ch);
    }
    
    SDL_LockSurface(_current_page->surface);

    int pitch = _current_page->surface->pitch;
    int bpp = _current_page->surface->format->BytesPerPixel;
    int x = _current_page->cursor_x;
    int y = _current_page->cursor_y;
    Uint8 * pixel = (Uint8 *)_current_page->surface->pixels;
    pixel += y * pitch * _current_page->mode + x * DOS_CHAR_WIDTH * bpp;
    
    for ( int y1 = 0; y1 < (int)_current_page->mode; y1++, data++ ) {
        for ( int x1 = DOS_CHAR_WIDTH - 1; x1 >= 0; x1-- ) {
            if ( *data & (1 << x1) ) {
                const SDL_Color * c;
                if ( cell->attributes.blink && SDL_GetTicks() % 600 < 300) {
                    c = &dos_palette[cell->attributes.bg_color];
                } else {
                    c = &dos_palette[cell->attributes.fg_color];
                }
                *(Uint32 *)pixel = SDL_MapRGBA(_current_page->surface->format, c->r, c->g, c->b, c->a);
            } else {
                if ( !cell->attributes.transparent ) {
                    const SDL_Color * c = &dos_palette[cell->attributes.bg_color];
                    *(Uint32 *)pixel = SDL_MapRGBA(_current_page->surface->format, c->r, c->g, c->b, c->a);
                } else {
                    *(Uint32 *)pixel = SDL_MapRGBA(_current_page->surface->format, 0, 0, 0, 0);
                }
            }
            pixel += bpp;
        }
        pixel -= DOS_CHAR_WIDTH * bpp;
        pixel += pitch;
    }

    SDL_UnlockSurface(_current_page->surface);
    
    AdvanceCursor(_current_page, 1);
}

void DOS_PrintString(const char * format, ...)
{
    int     len;
    char *  buffer;
    char *  ch;
    
    va_list args[2];
    va_start(args[0], format);
    va_copy(args[1], args[0]);
    
    len = vsnprintf(NULL, 0, format, args[0]);
    buffer = calloc(len + 1, sizeof(char));
    vsnprintf(buffer, len + 1, format, args[1]);
    
    va_end(args[0]);
    va_end(args[1]);
    
    ch = buffer;
    while ( *ch ) {
        switch ( *ch ) {
            case '\n':
                NewLine(_current_page);
                break;
            case '\t':
                do {
                    AdvanceCursor(_current_page, 1);
                } while ( _current_page->cursor_x % _current_page->tab_size != 0 );
                break;
            default:
                DOS_PrintChar(*ch);
                break;
        }
        ch++;
    }
    
    free(buffer);
}

static void RenderCursor(SDL_Renderer * renderer, int x_offset, int y_offset)
{
    SDL_Rect cursor;
    cursor.x = _current_page->cursor_x * DOS_CHAR_WIDTH + x_offset;
    cursor.y = _current_page->cursor_y * _current_page->mode + y_offset;
    cursor.w = DOS_CHAR_WIDTH;
    
    switch ( _current_page->cursor_type ) {
        case DOS_CURSOR_NORMAL:
            cursor.h = _current_page->mode / 5;
            cursor.y += _current_page->mode - cursor.h;
            break;
        case DOS_CURSOR_FULL:
            cursor.h = _current_page->mode;
            break;
        default:
            return;
    }
 
    if ( SDL_GetTicks() % 300 < 150 ) {
        return;
    }
        
    uint8_t r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    
    DOS_SetColor(renderer, _current_page->fg_color);
    SDL_RenderFillRect(renderer, &cursor);
    
    SDL_SetRenderDrawColor(renderer, r, g, b, a); // restore
}

void DOS_RenderConsole(SDL_Renderer * renderer, DOS_Console * console, int x, int y)
{
    SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, console->surface);
    
    SDL_Rect dst;
    dst.x = x,
    dst.y = y,
    dst.w = console->width * DOS_CHAR_WIDTH * console->scale;
    dst.h = console->height * console->mode * console->scale;
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    
    SDL_DestroyTexture(texture);
    
    RenderCursor(renderer, x, y);
}

void DOS_GotoXY(int x, int y)
{// TODO: test
    if ( ValidCoord(_current_page, x, y) ) {
        _current_page->cursor_x = x;
        _current_page->cursor_y = y;
    }
}

int DOS_GetX()
{
    return _current_page->cursor_x;
}

int DOS_GetY()
{
    return _current_page->cursor_y;
}

DOS_CharInfo DOS_GetChar()
{// TODO: test
    return *(GetCell(_current_page, _current_page->cursor_x, _current_page->cursor_y));
}

void DOS_SetChar(DOS_CharInfo * char_info)
{// TODO: test
    DOS_CharInfo * cell = GetCell(_current_page, _current_page->cursor_x, _current_page->cursor_y);
    *cell = *char_info;
}

void DOS_SetBlink(bool blink)
{
    _current_page->blink = blink;
}

void DOS_SetTabSize(int tab_size)
{// TODO: test
    _current_page->tab_size = tab_size;
}

void DOS_SetCursorType(DOS_CursorType type)
{
    _current_page->cursor_type = type;
}

void DOS_SetScale(int scale)
{
    _current_page->scale = scale;
}

void DOS_SetMargin(int margin)
{
    _current_page->margin = margin;
}

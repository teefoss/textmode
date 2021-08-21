#include "textmode.h"
#include "internal.h"

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
    bool            blink;      // whether newly printed chars blink
    DOS_Text *      text;
    DOS_CharInfo *  buffer;
    DOS_CursorType  cursor_type;
};

// -----------------------------------------------------------------------------

static DOS_CharInfo * GetCell(DOS_Console * console, int x, int y)
{
    return console->buffer + y * console->width + x;
}

static void NewLine(DOS_Console * console)
{
    if ( console->cursor_y < console->height - 1 ) {
        console->cursor_x = 0;
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
    fprintf(stderr, "DOS_NewConsole: %s\n", message);
    DOS_FreeConsole(c);
    
    return NULL;
}

// -----------------------------------------------------------------------------

DOS_Console *
DOS_NewConsole
(   SDL_Renderer * renderer,
    int w,
    int h,
    DOS_Mode text_style )
{
    DOS_Console * console = malloc( sizeof(*console) );
    
    if ( console == NULL )
        return NewConsoleError(NULL, "could not allocate memory for console");
    
    console->mode           = text_style;
    console->width          = w;
    console->height         = h;
    console->buffer         = NULL;
    console->text           = NULL;
    console->blink          = false;
    console->tab_size       = 4;
    console->cursor_type    = DOS_CURSOR_NORMAL;
    
    console->buffer = calloc(w * h, sizeof(*console->buffer));
    
    if ( console->buffer == NULL ) {
        return NewConsoleError(console, "could not allocate buffer");
    }
    
    console->text = DOS_MakeText(renderer, text_style, dos_palette, 16);
    
    if ( console->text == NULL ) {
        return NewConsoleError(console, "could not create console text");
    }
    
    DOS_ClearConsole(console);
        
    return console;
}


void DOS_FreeConsole(DOS_Console * console)
{
    if ( console ) {
        if ( console->buffer ) {
            free(console->buffer);
        }
        DOS_DestroyText(console->text);
        free(console);
    }
}


void DOS_ClearConsole(DOS_Console * console)
{
    size_t size = sizeof(DOS_CharInfo) * console->width * console->height;
    memset(console->buffer, 0, size);
    
    console->cursor_x = 0;
    console->cursor_y = 0;
    console->fg_color = DOS_WHITE;
    console->bg_color = DOS_BLACK;
}


void DOS_ClearBackground(DOS_Console * console)
{
    for ( int y = 0; y < console->height; y++ ) {
        for ( int x = 0; x < console->width; x++ ) {
            DOS_CharInfo * cell = GetCell(console, x, y);
            cell->attributes.bg_color = console->bg_color;
        }
    }
}


void DOS_CSetForeground(DOS_Console * console, int color)
{
    console->fg_color = color;
}


void DOS_CSetBackground(DOS_Console * console, int color)
{// TODO: test
    console->bg_color = color;
}


void DOS_CPrintChar(DOS_Console * con, uint8_t ch)
{
    DOS_CharInfo * cell = GetCell(con, con->cursor_x, con->cursor_y);
    cell->character = ch;
    cell->attributes.fg_color = con->fg_color;
    cell->attributes.bg_color = con->bg_color;
    cell->attributes.blink = con->blink;
    
    AdvanceCursor(con, 1);
}


void DOS_CPrintString(DOS_Console * console, const char * format, ...)
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
            case '\n': // TODO: test
                NewLine(console);
                break;
            case '\t': // TODO: test
                do {
                    AdvanceCursor(console, 1);
                } while ( console->cursor_x % console->tab_size != 0 );
                break;
            default:
                DOS_CPrintChar(console, *ch);
                break;
        }
        ch++;
    }
    
    free(buffer);
}


static void RenderCursor(DOS_Console * console, int x_offset, int y_offset)
{
    SDL_Rect cursor;
    cursor.x = console->cursor_x * DOS_CHAR_WIDTH + x_offset;
    cursor.y = console->cursor_y * console->mode + y_offset;
    cursor.w = DOS_CHAR_WIDTH;
    
    switch ( console->cursor_type ) {
        case DOS_CURSOR_NORMAL:
            cursor.h = console->mode / 5;
            cursor.x += console->mode - cursor.h;
            break;
        case DOS_CURSOR_FULL:
            cursor.h = console->mode;
            break;
        default:
            return;
    }
 
    if ( SDL_GetTicks() % 300 < 150 ) {
        return;
    }
    
    SDL_Renderer * renderer = DOS_GetTextRenderer(console->text);
        
    uint8_t r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    
    DOS_SetColor(renderer, console->fg_color);
    SDL_RenderFillRect(renderer, &cursor);
    
    SDL_SetRenderDrawColor(renderer, r, g, b, a); // restore
}


void DOS_RenderConsole(DOS_Console * console, int x, int y)
{
    DOS_CharInfo * cell = GetCell(console, 0, 0);

    SDL_Rect cell_rect;
    cell_rect.w = DOS_CHAR_WIDTH;
    cell_rect.h = console->mode;
    
    for ( int y1 = 0; y1 < console->height; y1++ ) {
        for ( int x1 = 0; x1 < console->width; x1++, cell++ ) {

            cell_rect.x = x1 * DOS_CHAR_WIDTH + x;
            cell_rect.y = y1 * console->mode + y;
            
            if (!cell->attributes.blink
                || (cell->attributes.blink && SDL_GetTicks() % 600 < 300) ) {
                DOS_TRenderChar(console->text, cell_rect.x, cell_rect.y, cell);
            }
        }
    }
    
    RenderCursor(console, x, y);
}


void DOS_CGotoXY(DOS_Console * console, int x, int y)
{// TODO: test
    if ( ValidCoord(console, x, y) ) {
        console->cursor_x = x;
        console->cursor_y = y;
    }
}


int DOS_CGetX(DOS_Console * console)
{
    return console->cursor_x;
}


int DOS_CGetY(DOS_Console * console)
{
    return console->cursor_y;
}


int  DOS_CGetWidth(DOS_Console * console)
{
    return console->width;
}


int  DOS_CGetHeight(DOS_Console * console)
{
    return console->height;
}


DOS_CharInfo DOS_CGetChar(DOS_Console * console)
{// TODO: test
    return *(GetCell(console, console->cursor_x, console->cursor_y));
}


void DOS_CSetChar(DOS_Console * console, DOS_CharInfo * char_info)
{// TODO: test
    DOS_CharInfo * cell = GetCell(console, console->cursor_x, console->cursor_y);
    *cell = *char_info;
}


void DOS_CSetBlink(DOS_Console * console, bool blink)
{
    console->blink = blink;
}


void DOS_CSetTabSize(DOS_Console * console, int tab_size)
{// TODO: test
    console->tab_size = tab_size;
}


DOS_Mode DOS_CGetMode(DOS_Console * console)
{
    return console->mode;
}


void DOS_CSetCursorType(DOS_Console * console, DOS_CursorType type)
{
    console->cursor_type = type;
}

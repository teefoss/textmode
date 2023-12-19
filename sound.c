#include "textmode.h"

static SDL_AudioSpec spec;
static SDL_AudioDeviceID device;
static uint8_t volume = 5;

static bool is_muted = false;


static double NoteNumberToFrequency(int note_num)
{
    static const int frequencies[] = { // in octave 6
        4186, // C
        4435, // C#
        4699, // D
        4978, // D#
        5274, // E
        5588, // F
        5920, // F#
        6272, // G
        6645, // G#
        7040, // A
        7459, // A#
        7902, // B
    };

    int octave = (note_num - 1) / 12;
    int note = (note_num - 1) % 12;
    int freq = frequencies[note];

    int octaves_down = 6 - octave;
    while ( octaves_down-- )
        freq /= 2;

    return (double)freq * 2.0; // basic notes sound 1 octave higher
}

static void DOS_QuitSound(void) {
    DOS_StopSound();
    SDL_PauseAudioDevice(device, SDL_TRUE);
    SDL_CloseAudioDevice(device);
}

void DOS_Mute(bool muted)
{
    is_muted = muted;
}

bool DOS_SoundIsPlaying(void)
{
    return SDL_GetQueuedAudioSize(device) > 0;
}

void DOS_AddSound(unsigned frequency, unsigned milliseconds)
{
    if ( is_muted ) {
        return;
    }
    
    float period = (float)spec.freq / (float)frequency;
    int len = (float)spec.freq * ((float)milliseconds / 1000.0f);

    for ( int i = 0; i < len; i++ ) {
        int8_t sample;

        if ( frequency == 0 ) {
            sample = spec.silence;
        } else {
            sample = (int)((float)i / period) % 2 ? volume : -volume;
        }

        SDL_QueueAudio(device, &sample, sizeof(sample));
    }
}

void DOS_InitSound(void)
{
    if ( SDL_WasInit(SDL_INIT_AUDIO) == 0 ) {
        int result = SDL_InitSubSystem(SDL_INIT_AUDIO);
        if ( result < 0 )
            fprintf(stderr, "error: failed to init SDL audio subsystem: %s", SDL_GetError());
    }

    SDL_AudioSpec want = {
        .freq = 44100,
        .format = AUDIO_S8,
        .channels = 1,
        .samples = 4096,
    };

    device = SDL_OpenAudioDevice(NULL, 0, &want, &spec, 0);
    if ( device == 0 ) {
        fprintf(stderr, "error: failed to open audio: %s\n", SDL_GetError());
    }

    SDL_PauseAudioDevice(device, 0);
    atexit(DOS_QuitSound);
}


void DOS_SetVolume(unsigned value)
{
    if ( value > 15 || value <= 0 ) {
        fprintf(stderr, "bad volume, expected value in range 1-15\n");
        return;
    }

    volume = value;
}

void DOS_Sound(unsigned frequency, unsigned milliseconds)
{
    SDL_ClearQueuedAudio(device);
    DOS_AddSound(frequency, milliseconds);
}

void DOS_StopSound(void)
{
    SDL_ClearQueuedAudio(device);
}

void DOS_Beep(void)
{
    DOS_Sound(800, 200);
}


// Play

// L[1,2,4,8,16,32,64] default: 4
// O[0...6] default: 4
// T[32...255] default: 120

// [A...G]([+,#,-][1,2,4,8,16,32,64][.])
// N[0...84](.)
// P[v]

static void PlayError(const char * msg, int line_position)
{
    printf("Play syntax error: %s (position %d)\n.", msg, line_position);
}

#define PLAY_STRING_MAX 255

void DOS_Play(const char * string, ...)
{
    if ( strlen(string) > PLAY_STRING_MAX ) {
        printf("Play error: string too long (max %d)\n", PLAY_STRING_MAX);
        return;
    }

    va_list args;
    va_start(args, string);

    char buffer[PLAY_STRING_MAX + 1] = { 0 };
    vsnprintf(buffer, PLAY_STRING_MAX, string, args);
    va_end(args);

    // default settings
    int bmp = 120;
    int oct = 4;
    int len = 4;

    // A-G
    static const int note_offsets[7] = { 9, 11, 0, 2, 4, 5, 7 };

    enum {
        mode_staccato = 6,  // 6/8
        mode_normal = 7,    // 7/8
        mode_legato = 8     // 8/8
    } mode = mode_normal;

    SDL_ClearQueuedAudio(device);

    // queue up whatever's in the string:

    // TODO: could use a big clean up.
    // TODO: handle tuplet handling
    // TODO: nice if more articulate choice beyond s, n, and l - specify fraction?
    const char * str = buffer;
    while ( *str != '\0') {
        char c = toupper(*str++);
        switch ( c ) {
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'N': case 'P':
            {
                // get note:
                int note = 0;
                switch ( c ) {
                    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                    case 'G':
                        note = 1 + (oct) * 12 + note_offsets[c - 'A'];
                        break;
                    case 'P':
                        note = 0;
                        break;
                    case 'N': {
                        int number = (int)strtol(str, (char **)&str, 10);
                        if ( number < 0 || number > 84 )
                            return PlayError("bad note number", (int)(str - string));
                        if ( number > 0 )
                            note = number;
                        break;
                    }
                    default:
                        break;
                }

                // adjust note per accidental:
                if ( c >= 'A' && c <= 'G' ) {
                    if ( *str == '+' || *str == '#' ) {
                        if ( note < 84 )
                            note++;
                        str++;
                    } else if ( *str == '-' ) {
                        if ( note > 1 )
                            note--;
                        str++;
                    }
                }

                int d = len;

                // check if there's a note length following a note A-G, set d
                if ( c != 'N' ) {
                    int number = (int)strtol(str, (char **)&str, 10);
                    
                    // strtol got a number, but it was a bad one
                    if ( number < 0 || number > 64 )
                        return PlayError("bad note value", (int)(str - string));
                    
                    // strtol found a value number
                    if ( number > 0 )
                        d = number;
                }

                // TODO: this should only happen when after a note length
                // count dots:
                int dot_count = 0;
                while ( *str == '.' ) {
                    dot_count++;
                    str++;
                }

                // adjust duration if there are dots:
                float total_ms = (60.0f / (float)bmp) * 1000.0f * (4.0f / (float)d);
                float prolongation = total_ms / 2.0f;
                while ( dot_count-- ) {
                    total_ms += prolongation;
                    prolongation /= 2;
                }

                // calculate articulation silence:
                int note_ms = total_ms * ((float)mode / 8.0f);
                int silence_ms = total_ms * ((8.0f - (float)mode) / 8.0f);

                // and finally, queue it
                DOS_AddSound(NoteNumberToFrequency(note_num), note_ms);
                DOS_AddSound(0, silence_ms);
                break;
            } // A-G, N, and P

            case 'T':
                bmp = (int)strtol(str, (char **)&str, 10);
                if ( bmp == 0 )
                    return PlayError("bad tempo", (int)(str - string));
                break;

            case 'O':
                if ( *str < '0' || *str > '6' )
                    return PlayError("bad octave", (int)(str - string));
                oct = (int)strtol(str, (char **)&str, 10);
                break;

                // TODO: dots not handled
            case 'L':
                len = (int)strtol(str, (char **)&str, 10);
                if ( len < 1 || len > 64 )
                    return PlayError("bad length", (int)(str - string));
                break;

            case '>':
                if ( oct < 6 )
                    oct++;
                break;

            case '<':
                if ( oct > 0 )
                    oct--;
                break;

            case 'M': {
                char option = toupper(*str++);
                switch ( option ) {
                    case 'L': mode = mode_legato; break;
                    case 'N': mode = mode_normal; break;
                    case 'S': mode = mode_staccato; break;
                    default:
                        return PlayError("bad music option", (int)(str - string));
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
}

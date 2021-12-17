#include "textmode.h"

static SDL_AudioSpec        spec;
static SDL_AudioDeviceID    device;

static double CalculateFrequency(int note, int octave)
{
    double frequency = 32.70325;
        
    for ( int i = 1; i < octave; i++ ) {
        frequency *= 2.0;
    }

    for ( int i = 0; i < note; i++ ) {
        frequency *= 1.05946309438;
    }
    
    return frequency;
}

// queue and play a note
#if 0
static void PlayNote(int note, int octave, int milliseconds)
{
    double frequency = CalculateFrequency(note, octave);
    DOS_Sound(frequency, milliseconds);
}
#endif

static int NoteValueToMS(int nv, int tempo)
{
    float ms_per_beat = (60.0f / (float)tempo) * 1000.0f;
    float ratio = 4.0f / (float)nv;
    float result = ms_per_beat * ratio;
        
    return result;
}

// just queue a note
static void QueueNote(int note, int octave, int milliseconds)
{
    double frequency = CalculateFrequency(note, octave);
    DOS_QueueSound(frequency, milliseconds);
}

// play whatever is in the queue.
static int PlayQueuedSound(void * ptr)
{
    (void)ptr; // present so signature matches type SDL_ThreadFunction
    
    SDL_PauseAudioDevice(device, SDL_FALSE);
    
    while ( SDL_GetQueuedAudioSize(device) )
        ;
    
    SDL_PauseAudioDevice(device, SDL_TRUE);
    
    return 0;
}

static int ValidNoteValue(int value)
{
    int values[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    int count = sizeof(values) / sizeof(values[0]);
    
    for ( int i = 0; i < count; i++ ) {
        if ( value == values[i] ) {
            return 1;
        }
    }
    
    return 0;
}
static int SetOctave(int new_oct)
{
    if ( new_oct > 6 ) {
        new_oct = 6;
    } else if ( new_oct < 1 ) {
        new_oct = 1;
    }

    return new_oct;
}

// -----------------------------------------------------------------------------

void DOS_InitSound(void)
{
    memset(&spec, 0, sizeof(spec));
    
    spec.freq = 44100;
    spec.format = AUDIO_S8;
    spec.channels = 1;
    spec.samples = 4096;
    spec.callback = NULL;

    device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}

// TODO: use QueueSound freq = 0 instead
#if 0
static void QueueSilence(unsigned milliseconds)
{
    int len = (float)spec.freq * ((float)milliseconds / 1000.0f);
    for ( int i = 0; i < len; i++ ) {
        int8_t sample = 0;
        SDL_QueueAudio(device, &sample, sizeof(sample));
    }
}
#endif

// just queue a sound
void DOS_QueueSound(unsigned frequency, unsigned milliseconds)
{
    int period = (float)spec.freq / (float)frequency;
    int len = (float)spec.freq * ((float)milliseconds / 1000.0f);
    int volume = 5;
    
    for ( int i = 0; i < len; i++ ) {
        int8_t sample = (i / period) % 2 ? volume : -volume;
        SDL_QueueAudio(device, &sample, sizeof(sample));
    }
}


// queue a sound and play it
void DOS_Sound(unsigned frequency, unsigned milliseconds)
{
    SDL_ClearQueuedAudio(device);
    DOS_QueueSound(frequency, milliseconds);
    PlayQueuedSound(NULL);
}


void DOS_StopSound()
{
    SDL_ClearQueuedAudio(device);
    SDL_PauseAudioDevice(device, SDL_TRUE);
}


void DOS_SoundAsync(unsigned frequency, unsigned milliseconds)
{
    DOS_QueueSound(frequency, milliseconds);
    DOS_PlayQueuedSound();
}


// play whatever is in the buffer on another thread
void DOS_PlayQueuedSound(void)
{
    SDL_Thread * thread;
    thread = SDL_CreateThread(PlayQueuedSound, "play_thread", NULL);
    if ( thread == NULL ) {
        fprintf(stderr, "could not create play thread: %s\n", SDL_GetError());
    }
}

void DOS_Beep()
{
    DOS_Sound(800, 200);
}

#define PLAY_DEBUG 0

// FIXME: note length bug (l16 +)
void DOS_Play(const char * string)
{
    // default settings
    int bmp = 120;
    int oct = 4;
    int len = 4;
    
    SDL_ClearQueuedAudio(device);
    
    // queue up whatever's in the string:

    const char * c = string;
    while ( *c != '\0') {
        char ch = toupper(*c); // case-insensitive
        int offs = 0; // half-step + or - offset to note
        
        // check the following character for a '+' or '-'
        if ( *(c + 1) != '\0' ) {
            if ( *(c + 1) == '+' ) {
                offs = +1;
            } else if ( *(c + 1) == '-' ) {
                offs = -1;
            }
        }
        
        int ms = NoteValueToMS(len, bmp);
        
        switch ( ch ) {
            case '+': // handled above
            case '-':
                break;
                
            case 'C': QueueNote( 0 + offs, oct, ms); break;
            case 'D': QueueNote( 2 + offs, oct, ms); break;
            case 'E': QueueNote( 4 + offs, oct, ms); break;
            case 'F': QueueNote( 5 + offs, oct, ms); break;
            case 'G': QueueNote( 7 + offs, oct, ms); break;
            case 'A': QueueNote( 9 + offs, oct, ms); break;
            case 'B': QueueNote(11 + offs, oct, ms); break;
                
            case 'T': {
                bmp = strtol(c + 1, NULL, 10);
                if ( bmp == 0 ) {
                    printf("DOS_Play syntax error at position %d\n.",
                           (int)(c - string));
                    return;
                }
                #if PLAY_DEBUG
                printf("set tempo to %d\n", bmp);
                #endif
                break;
            }
            case 'O':
                oct = strtol(c + 1, NULL, 10);
                if ( oct == 0 ) {
                    printf("DOS_Play syntax error at position %d\n.",
                           (int)(c - string));
                    return;
                }
                #if PLAY_DEBUG
                printf("set octave to %d\n", oct);
                #endif
                break;
            case 'L':
                len = strtol(c + 1, NULL, 10);
                if ( len == 0 || !ValidNoteValue(len) ) {
                    printf("TXT_PlayNotes: syntax error at position %d\n.",
                           (int)(c - string));
                    return;
                }
                #if PLAY_DEBUG
                printf("set length to %d\n", len);
                #endif
                break;
            case '>':
                oct = SetOctave(oct + 1);
                #if PLAY_DEBUG
                printf("increase octave\n");
                #endif
                break;
            case '<':
                oct = SetOctave(oct - 1);
                break;
                #if PLAY_DEBUG
                printf("decrease octave\n");
                #endif
            default:
                break;
        }
        
        c++;
    }

    DOS_PlayQueuedSound();
}

#undef PLAY_DEBUG

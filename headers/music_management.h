#include "raylib.h"
#include "song_information.h"

typedef struct
{
    float pan;
    float pitch;
    float volume;
    float percent_played;
} MusicSettings;

// loads an mp3 file specified in path
void change_music_stream(Music* music, const char* songpath);
void set_current_song(const char* playlist_path, SongInformation* song_information, Music* music, MusicSettings settings);
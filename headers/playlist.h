#include "list.h"
#define NPLAYLIST 10
typedef struct
{
    char* path;
    list song_paths;
}playlist;

char* format_as_display(char* file_path);
char* linux_formatted_filename(const char* file_path);
void load_playlists_from_folder(list* playlists, const char* music_folder);
list load_songs_from_playlist(const char* playlist_path);
void extract_song_cover(const char* playlist_path, char* song_path, const char* cover_location);

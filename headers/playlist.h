#include "c_string.h"
#include "list.h"
typedef struct
{
    char path[MAX_LEN];
    list song_paths;
}playlist;

void format_as_display(char* result, const char* file_path);
void linux_formatted_filename(char* dst, const char* file_path);
void get_full_song_path(char* dst, const char* playlist_path, char* song_path);
void get_cover_path(char* cover_path, const char* song_path, const char* cover_location);
void load_playlists_from_folder(list* playlists, const char* music_folder);
void load_songs_from_playlist(list* songs, const char* playlist_path);
void extract_song_cover(const char* playlist_path, const char* song_path, const char* cover_location);

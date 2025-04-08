#include "stdlib.h"
#include "unistd.h"
#include "string.h"

#include "headers/list.h"
#include "headers/raylib.h"
#include "headers/c_string.h"
#include "headers/playlist.h"
#include <stdio.h>

const int N_PLAYLIST = 10;
const char* PLAYLIST_DIR = "/home/bubbq/Music"; // path to folder containing all your playlists
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; // where covers of music will be stored

int main()
{
    list library = create_list(PLAYLIST);
    int current_playlist, current_song;
    current_playlist = current_song = 0;

    load_playlists_from_folder(&library, PLAYLIST_DIR);
    for(int i = 0; i < library.size; i++)
    {
        playlist* pl = &((playlist*)library.elements)[i];
        pl->song_paths = load_songs_from_playlist(pl->path); 
    }

    for(int i = 0; i < library.size; i++)
    {
        playlist* pl = &((playlist*)library.elements)[i];
        for(int j = 0; j < pl->song_paths.size; j++)
        {
            string* s = (string*)pl->song_paths.elements + j;
            printf("%s\n", (s->value));
            extract_song_cover(pl->path, s->value, MP3_COVER_PATH);
        }
    }

    free_list(&library);
}
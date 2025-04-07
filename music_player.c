#include "stdlib.h"
#include "unistd.h"
#include "string.h"

#include "headers/list.h"
#include "headers/raylib.h"
#include "headers/c_string.h"
#include "headers/playlist.h"

const char* PLAYLIST_DIR = "/home/bubbq/Music"; // path to folder containing all your playlists
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; // where covers of music will be stored

int main()
{
    playlist playlist;
    playlist.song_paths = create_list(STRING);
    
    load_playlists_from_folder(&playlist, PLAYLIST_DIR);
    load_songs_from_playlist(&playlist.song_paths, playlist.path);
    extract_song_cover(playlist.path, ((string*)playlist.song_paths.elements)[0].value, MP3_COVER_PATH);

    free_list(&playlist.song_paths);
    free(playlist.path);
}
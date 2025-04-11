#include "headers/raylib.h"
#include "headers/c_string.h"
#include "headers/playlist.h"

const char* PLAYLIST_DIR = "/home/bubbq/Music"; // path to folder containing all your playlists
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; // where covers of music will be stored

void init()
{
    SetTargetFPS(60);
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitAudioDevice();
    InitWindow(1000, 750, "music player");
}

void update_song(char* full_song_path, playlist* pl, string* songs, int current_song, int current_playlist, Music* music)
{
    get_full_song_path(full_song_path, (pl + current_playlist)->path, (songs + current_song)->value);
    UnloadMusicStream(*music);
    *music = LoadMusicStream(full_song_path);
    PlayMusicStream(*music);
}

int main()
{
    init();    
    // initialize library
    list library = create_list(PLAYLIST);
    int current_playlist = 0;
    int current_song = 0;
    bool play_music = false;
    
    load_playlists_from_folder(&library, PLAYLIST_DIR);
    for(int i = 0; i < library.size; i++)
    {
        playlist* pl = (playlist*)library.elements + i;
        pl->song_paths = create_list(STRING);
        load_songs_from_playlist(&pl->song_paths, pl->path);
    }

    playlist* pl = (playlist*)library.elements;
    string* songs = (string*)pl->song_paths.elements;
    
    char full_song_path[MAX_LEN];
    get_full_song_path(full_song_path, (pl + current_playlist)->path, (songs + current_song)->value);
    
    Music music = LoadMusicStream(full_song_path);
    music.looping = false;
    while(!WindowShouldClose())
    {
        UpdateMusicStream(music);
        // play pause
        if(IsKeyPressed(KEY_SPACE))
        {
            if((play_music = !play_music))
                PlayMusicStream(music);
            else
                PauseMusicStream(music);
        }
        // moving to next song either skipping or playing through
        if(IsKeyPressed(KEY_D) || (!IsMusicStreamPlaying(music) && play_music))
        {
            if(++current_song >= pl->song_paths.size)
                current_song = 0;
            update_song(full_song_path, pl, songs, current_song, current_playlist, &music);
        }
        if(IsKeyPressed(KEY_A))
        {
            if(--current_song < 0)
                current_song = pl->song_paths.size - 1;
            update_song(full_song_path, pl, songs, current_song, current_playlist, &music);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
        EndDrawing();
    }

    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    free_list(&library);
}
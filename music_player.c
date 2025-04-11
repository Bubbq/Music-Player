#include "headers/raylib.h"
#include "headers/c_string.h"
#include "headers/playlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* MUSIC_SAVE = "saved_music.txt"; 
const char* PLAYLIST_DIR = "/home/bubbq/Music"; // path to folder containing all your playlists
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; // where covers of music will be stored

void init()
{
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitAudioDevice();
    InitWindow(1000, 750, "music player");
}

void update_song(char* full_song_path, playlist* current_playlist, string* current_song, Music* music)
{
    get_full_song_path(full_song_path, current_playlist->path, current_song->value);
    UnloadMusicStream(*music);
    *music = LoadMusicStream(full_song_path);
    PlayMusicStream(*music);
}

void save_music(char* playlist_path, char* song_path, float pitch)
{
    FILE* ptr = fopen(MUSIC_SAVE, "w");
    if(ptr == NULL)
        return;
    
    fprintf(ptr, "%s\n%s\n%f", playlist_path, song_path, pitch);
    fclose(ptr);
}

int main()
{
    init();    
    list library = create_list(PLAYLIST);
    int playlist_index, song_index;
    playlist_index = song_index = 0;
    bool play_music = false;
    float pitch = 1.0f;
    
    load_playlists_from_folder(&library, PLAYLIST_DIR);
    for(int i = 0; i < library.size; i++)
    {
        playlist* pl = (playlist*)library.elements + i;
        pl->song_paths = create_list(STRING);
        load_songs_from_playlist(&pl->song_paths, pl->path);
    }

    playlist* playlists = (playlist*)library.elements;
    playlist* current_playlist = playlists + playlist_index;
    string* songs = (string*)playlists->song_paths.elements;
    string* current_song = songs + song_index;
    
    char full_song_path[MAX_LEN];
    get_full_song_path(full_song_path, current_playlist->path, current_song->value);
    
    extract_song_cover(current_playlist->path, current_song->value, MP3_COVER_PATH);
    char cover_path[MAX_LEN];
    get_cover_path(cover_path, current_song->value, MP3_COVER_PATH);
    Image img = LoadImage(cover_path);
    ImageResize(&img, 250, 250);
    Texture2D song_cover = LoadTextureFromImage(img);

    Music music = LoadMusicStream(full_song_path);
    music.looping = false;
    while(!WindowShouldClose())
    {
        SetMusicPitch(music, pitch);
        UpdateMusicStream(music);
        // play pause
        if(IsKeyPressed(KEY_SPACE))
        {
            play_music = !play_music;
            if(play_music)
                PlayMusicStream(music);
            else
                PauseMusicStream(music);
        }
        // moving to next song either skipping or playing through
        if(IsKeyPressed(KEY_D) || (!IsMusicStreamPlaying(music) && play_music))
        {
            song_index++;
            if(song_index >= playlists->song_paths.size)
                song_index = 0;
            current_song = songs + song_index;
            update_song(full_song_path, current_playlist, current_song, &music);
            extract_song_cover(current_playlist->path, current_song->value, MP3_COVER_PATH);
            get_cover_path(cover_path, current_song->value, MP3_COVER_PATH);
            if(img.data)
                UnloadImage(img);
            if(song_cover.id)
                UnloadTexture(song_cover);
            img = LoadImage(cover_path);
            ImageResize(&img, 250, 250);
            song_cover = LoadTextureFromImage(img);
        }
        if(IsKeyPressed(KEY_A))
        {
            song_index--;
            if(song_index < 0)
                song_index = playlists->song_paths.size - 1;
            current_song = songs + song_index;
            update_song(full_song_path, current_playlist, current_song, &music);
            extract_song_cover(current_playlist->path, current_song->value, MP3_COVER_PATH);
            get_cover_path(cover_path, current_song->value, MP3_COVER_PATH);
             if(img.data)
                UnloadImage(img);
            if(song_cover.id)
                UnloadTexture(song_cover);
            img = LoadImage(cover_path);
            ImageResize(&img, 250, 250);
            song_cover = LoadTextureFromImage(img);
        }
        if(IsKeyPressed(KEY_W))
            pitch += 0.05;
        if(IsKeyPressed(KEY_S))
            pitch -= 0.05;
        // show music par progression
        // make buttons to pause, play, skip, and rewind
        BeginDrawing();
            if(song_cover.id)
                DrawTexture(song_cover, 0, 0, WHITE);
            ClearBackground(RAYWHITE);
        EndDrawing();
    }
    save_music(current_playlist->path, current_song->value, pitch);
    UnloadMusicStream(music);
    if(song_cover.id)
        UnloadTexture(song_cover);
    if(img.data)
        UnloadImage(img);
    CloseAudioDevice();
    CloseWindow();
    free_list(&library);
}
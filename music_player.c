#include "headers/raylib.h"
#include "headers/list.h"
#include "headers/c_string.h"
#include "headers/playlist.h"
#include <stdbool.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

const char* MUSIC_SAVE = "saved_music.txt"; 
const char* PLAYLIST_DIR = "/home/bubbq/Music"; // path to folder containing all your playlists
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; // where covers of music will be stored

void init()
{
    SetTargetFPS(60);
    // SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitAudioDevice();
    InitWindow(1000, 1000, "music player");
}

void update_song(char* full_song_path, const char* current_playlist_path, char* current_song_path, Music* music)
{
    if(IsMusicReady(*music))
        UnloadMusicStream(*music);
    get_full_song_path(full_song_path, current_playlist_path, current_song_path);
    *music = LoadMusicStream(full_song_path);
    music->looping = false;
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

void update_song_cover(const char* playlist_path, const char* song_path, char* song_cover_path, Image* image, Texture2D* song_cover)
{
    extract_song_cover(playlist_path,song_path, MP3_COVER_PATH);
    get_cover_path(song_cover_path, song_path, MP3_COVER_PATH);
    
    if(image->data)
        UnloadImage(*image);
    if(song_cover->id)
        UnloadTexture(*song_cover);

    *image = LoadImage(song_cover_path); 
    *song_cover = LoadTextureFromImage(*image);
}

void progress_bar(Music* music)
{
    const Rectangle bar = {250, 500, 500, 10};
    float percent_complete = GetMusicTimePlayed(*music) / GetMusicTimeLength(*music);

    Vector2 mouse_position = GetMousePosition();
    if(CheckCollisionPointRec(mouse_position, bar) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        percent_complete = (mouse_position.x - bar.x) / bar.width;
        SeekMusicStream(*music, GetMusicTimeLength(*music) * percent_complete);
    }

    DrawRectangleRec(bar, RED);
    DrawRectangle(bar.x,bar.y,(bar.width * percent_complete), bar.height, GREEN);
}

void pause_button(Music* music, bool* play_music)
{
    const Rectangle button_border = {300, 300, 75, 30};
    if(GuiButton(button_border, (*play_music) ? "PAUSE" : "PLAY") || IsKeyPressed(KEY_SPACE))
    {
        (*play_music) = !(*play_music);
        if(*play_music)
            PlayMusicStream(*music);
        else
            PauseMusicStream(*music);
    }
}

int adjust_index(int index, int size)
{
    return (index + size) % size;
}   

int main()
{
    init();    
    list library = create_list(PLAYLIST);
    int playlist_index, song_index;
    playlist_index = 1;
    song_index = 0;
    bool play_music = false;
    bool update_cover = false;
    float pitch = 1.00;
    load_playlists_from_folder(&library, PLAYLIST_DIR);
    for(int i = 0; i < library.size; i++)
    {
        playlist* pl = (playlist*)library.elements + i;
        pl->song_paths = create_list(STRING);
        char linux_format_playlist[MAX_LEN];
        linux_formatted_filename(linux_format_playlist, pl->path);
        load_songs_from_playlist(&pl->song_paths,linux_format_playlist);
    }

    playlist* playlists = (playlist*)library.elements;
    playlist* current_playlist = playlists + playlist_index;
    string* songs = (string*)current_playlist->song_paths.elements;
    string* current_song = songs + song_index;
    char full_song_path[MAX_LEN];
    get_full_song_path(full_song_path, current_playlist->path, current_song->value);
    
    extract_song_cover(current_playlist->path, current_song->value, MP3_COVER_PATH);
    char cover_path[MAX_LEN];
    get_cover_path(cover_path, current_song->value, MP3_COVER_PATH);
    Image image = LoadImage(cover_path); 
    Texture2D song_cover = LoadTextureFromImage(image);

    Music music = LoadMusicStream(full_song_path);
    music.looping = false;
    while(!WindowShouldClose())
    {
        UpdateMusicStream(music);
        SetMusicPitch(music, pitch);
        if(IsKeyPressed(KEY_W))
            pitch += 0.05;
        else if(IsKeyPressed(KEY_S))
            pitch -= 0.05;
        BeginDrawing();
        DrawFPS(0, 0);
            if(update_cover)
            {
                update_song_cover(current_playlist->path, current_song->value, cover_path, &image, &song_cover);
                update_cover = !update_cover;
            }
            if(song_cover.id != 0)
                DrawTexture(song_cover, (GetScreenWidth() / 2.00) - 125, 0, RAYWHITE);

            const Rectangle next_button_border = {400, 300, 75, 30};
            if(GuiButton(next_button_border, "SKIP") || IsKeyPressed(KEY_D))
            {
                song_index = adjust_index((song_index + 1), current_playlist->song_paths.size);
                current_song = songs + song_index;
                update_song(full_song_path, current_playlist->path, current_song->value, &music);
                update_cover = true;
            }

            const Rectangle previous_button_border = {200, 300, 75, 30};
            if(GuiButton(previous_button_border, "PREV") || IsKeyPressed(KEY_A))
            {
                if(GetMusicTimePlayed(music) >= 3.00)
                    SeekMusicStream(music, 0.50);
                else
                {
                    song_index = adjust_index((song_index - 1), current_playlist->song_paths.size);
                    current_song = songs + song_index;
                    update_song(full_song_path, current_playlist->path, current_song->value, &music);
                    update_cover = true;
                }
            }
            
            pause_button(&music, &play_music);
            progress_bar(&music);
            ClearBackground(RAYWHITE);
        EndDrawing();
    }
    save_music(current_playlist->path, current_song->value, pitch);
    if(IsMusicReady(music))
        UnloadMusicStream(music);
    if(song_cover.id)
        UnloadTexture(song_cover);
    if(image.data)
        UnloadImage(image);
    free_list(&library);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
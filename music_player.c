#include "stdio.h"
#include "headers/raylib.h"
#include <stdlib.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#define MAX_LEN 128
#define NSONGS 100
#define NPLAYLIST 100

const char* MUSIC_SAVE = "saved_music.txt"; 
const char* PLAYLIST_DIR = "/home/bubbq/Music"; 
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; 

void remove_escape_sequence(char* str)
{
    char* dst = str;
    char* src = str;
    while(*src != '\0')
    {
        if(*src != '\\')
        {
            *dst = *src;
            dst++;
        }
        src++;
    }    

    *dst = '\0';
}

void linux_formatted_filename(char* destination, const char* original_path)
{
    const char* INVALID = " ()&'`\\!";  
    const char* replacement_characters[] = {
        "\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\\", "\\!" 
    };

    int length = 0;
    for(int i = 0; i < strlen(original_path); i++)
    {
        char character = original_path[i];
        // find next invalid character
        char* found = strchr(INVALID, character);

        // replace char with linux escape sequence
        if(found != NULL)
        {
            int replacement_index = found - INVALID;
            const char* replacement = replacement_characters[replacement_index];
            int characters_written = snprintf(destination + length, MAX_LEN - length, "%s", replacement);
            
            if(characters_written >= MAX_LEN - length)
                characters_written = MAX_LEN - length - 1;
            length += characters_written;
        }

        else
            destination[length++] = character;
    }

    destination[length] = '\0';
}

int get_files_from_folder(int nchars, int nfiles, char files[][nchars], const char* folder_path)
{
    char command[nchars];
    sprintf(command, "cd %s; ls -1", folder_path);
    FILE* output_stream = popen(command, "r");
   
    int files_read = 0;
    char buffer[nchars];
    while((fgets(buffer, nchars, output_stream) != NULL) && (files_read < nfiles))
    {
        int newline_position = strlen(buffer) - 1;
        buffer[newline_position] = '\0';
        linux_formatted_filename(files[files_read++], buffer);
    }
    
    pclose(output_stream);
    return files_read;
}

void print_list(int nchars, int nfiles, char files[][nchars])
{
    for(int i = 0; i < nfiles; i++)
        printf("%d) %s\n", (i + 1), files[i]);
}

bool is_mp3(const char* file_path)
{
    const char* MP3 = ".mp3";
    char* file_extension = strrchr(file_path, '.');
    return (strcmp(file_extension, MP3) == 0);
}

int adjust_index(int index, int size)
{
    return (index + size) % size;
} 

void remove_extension(char* str)
{
    char* found = strrchr(str, '.');
    if(found != NULL)
        str[found - str] = '\0';
}

void create_cover_directory(const char* cover_location)
{
    char command[MAX_LEN];
    sprintf(command, "mkdir %s", cover_location);
    system(command);
}

void update_cover_path(char* dst, const char* cover_root, const char* song_path)
{
    snprintf(dst, MAX_LEN * 2, "%s/%s", cover_root, song_path);
    remove_extension(dst);
    sprintf(dst + strlen(dst), ".jpg");
}

void update_display_name(char* dst, const char* str)
{
    strcpy(dst, str);
    remove_escape_sequence(dst);
    remove_extension(dst);
}

void extract_song_cover(const char* cover_path, const char* music_root, const char* playlist_path, const char* song_path)
{
    char command[MAX_LEN * 4];
    snprintf(command, MAX_LEN * 4, "ffmpeg -i %s/%s/%s -vf scale=250:-1 \"%s\" -loglevel error", music_root, playlist_path, song_path, cover_path);
    system(command);
}

void update_cover_art(Texture2D* background, Texture2D* song_cover, const char* current_playlist, const char* current_song)
{
    char cover_path[MAX_LEN * 2];
    update_cover_path(cover_path, MP3_COVER_PATH, current_song);
   
    FILE* cover = fopen(cover_path, "r");
    if(cover == NULL)
        extract_song_cover(cover_path, PLAYLIST_DIR, current_playlist, current_song);
    else
        fclose(cover);

    if(song_cover->id)
        UnloadTexture(*song_cover);
    if(background->id)
        UnloadTexture(*background);

    Image song_cover_image = LoadImage(cover_path);
    *song_cover = LoadTextureFromImage(song_cover_image);

    // Image background_image = LoadImage(cover_path);
    // ImageResize(&background_image, GetScreenWidth(), GetScreenHeight());
    // ImageBlurGaussian(&background_image, 10);
    // *background = LoadTextureFromImage(background_image);

    if(song_cover_image.data)
        UnloadImage(song_cover_image);
    // if(background_image.data)
    //     UnloadImage(background_image); 
}

void save_music(char* playlist_path, char* song_path, float pitch)
{
    FILE* ptr = fopen(MUSIC_SAVE, "w");
    if(ptr == NULL)
        return;
    
    fprintf(ptr, "%s\n%s\n%f", playlist_path, song_path, pitch);
    fclose(ptr);
}

void rewind_button(Music* music, bool* rewind_song, bool* restart_song)
{
    const char* button_text = "PREV";
    const Rectangle button_border = {125, 500, 75, 30};

    if(GuiButton(button_border,button_text) || IsKeyPressed(KEY_A))
    {
        if(GetMusicTimePlayed(*music) > 3.00)
            *restart_song = true;
        else
            *rewind_song = true;
    }

    else
        *rewind_song = false;
}

void pause_button(Music* music, bool* play_music)
{
    const char* button_text = (*play_music) ? "PAUSE" : "PLAY";
    const Rectangle button_border = {200, 500, 75, 30};
    
    if(GuiButton(button_border, button_text) || IsKeyPressed(KEY_SPACE))
        *play_music = !(*play_music);
}

void skip_button(Music* music, bool* skip_song, bool play_music)
{
    const char* button_text = "SKIP";
    const Rectangle button_border = {275, 500, 75, 30};
    
    if(GuiButton(button_border, button_text) || IsKeyPressed(KEY_D) || (!IsMusicStreamPlaying(*music) && play_music))
        *skip_song = true;
    else 
        *skip_song = false;
}

void progress_bar(float* percentage_played, bool* update_position)
{
    const Rectangle bar = {200, 400, 200, 10};
    DrawRectangleRec(bar, RED);
    
    Vector2 mouse_position = GetMousePosition();
    if(CheckCollisionPointRec(mouse_position, bar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        *percentage_played = (mouse_position.x - bar.x) / bar.width;
        *update_position = true;
    }
    DrawRectangle(200, 400, (200 * (*percentage_played)), 10, GREEN);
}

void set_pitch(Music* music, float* pitch)
{
    const float delta = 0.025;
    if(IsKeyPressed(KEY_W))
    {
        *pitch += delta;
        SetMusicPitch(*music, *pitch);
    }
    if(IsKeyPressed(KEY_S))
    {
        *pitch -= delta;
        SetMusicPitch(*music, *pitch);
    }
}

void update_music_stream(Music* music, const char* full_song_path, float pitch)
{
    if(IsMusicReady(*music))
        UnloadMusicStream(*music);
    *music = LoadMusicStream(full_song_path);
    music->looping = false;
    SetMusicPitch(*music, pitch);
    PlayMusicStream(*music);
}

// TODO: 
// add shuffle
// add default images
// handle edge cases (file not mp3 etc)
// fix low frames on initialization

int main()
{
    bool play_music = false;
    bool shuffle = false;
    bool skip_song = false;
    bool rewind_song = false;
    bool restart_song = false;
    bool update_position = false;

    float pitch = 1.00;
    float percentage_played = 0.00;
    
    int song_index, playlist_index;
    playlist_index = song_index = 0;

    char playlists[NPLAYLIST][MAX_LEN];
    char current_playlist[MAX_LEN];
    
    char current_song[MAX_LEN];
    char songs[NSONGS][MAX_LEN];
    char display_name[MAX_LEN];

    Music music;
    Texture2D song_cover;
    Texture2D background;

    // init playlist and songs
    int playlists_read = get_files_from_folder(MAX_LEN, NPLAYLIST, playlists, PLAYLIST_DIR);
    char playlist_path[MAX_LEN * 2];
    snprintf(playlist_path, MAX_LEN * 2, "%s/%s", PLAYLIST_DIR, playlists[playlist_index]);
    int songs_read = get_files_from_folder(MAX_LEN, NSONGS, songs, playlist_path);

    sprintf(current_playlist, "%s", playlists[playlist_index]);

    sprintf(current_song, "%s", songs[song_index]);

    char full_song_path[MAX_LEN * 3];
    snprintf(full_song_path, (MAX_LEN * 3), "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
    remove_escape_sequence(full_song_path);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(750, 750, "music player");
    InitAudioDevice();
    SetTargetFPS(60);
    
    FILE* cover_directory = fopen(MP3_COVER_PATH, "r");
    if(cover_directory == NULL)
        create_cover_directory(MP3_COVER_PATH);
    else
        fclose(cover_directory);
    
    if(is_mp3(full_song_path))
    {
        // music = LoadMusicStream(full_song_path);
        // music.looping = false;
        // PlayMusicStream(music);
        update_music_stream(&music, full_song_path, pitch);
        update_cover_art(&background, &song_cover, current_playlist, current_song);
        update_display_name(display_name, current_song);
    }
    
    while(!WindowShouldClose())
    {
        set_pitch(&music, &pitch);
        
        if(IsKeyPressed(KEY_X))
            shuffle = !shuffle;

        if(restart_song)
        {
            restart_song = false;
            StopMusicStream(music);
            PlayMusicStream(music);
        }

        if(update_position)
        {
            update_position = false;
            SeekMusicStream(music, GetMusicTimeLength(music) * percentage_played);
        }

        // update enviornment
        if(skip_song || rewind_song)
        {
            if(skip_song)
            {
                skip_song = false;
                // if(shuffle)
                // {
                //     int rand;
                //     while((rand = GetRandomValue(0, songs_read - 1)) == song_index)
                //         ;
                //     song_index = rand;
                // }
                // else
                    song_index = adjust_index((song_index + 1), songs_read);
            }

            else if(rewind_song)
            {
                rewind_song = false;
                song_index = adjust_index((song_index - 1), songs_read);
            }

            strcpy(current_song, songs[song_index]);
            
            if(is_mp3(current_song))
            {
                update_display_name(display_name, current_song);
                // update full song path
                snprintf(full_song_path, (MAX_LEN * 3), "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
                remove_escape_sequence(full_song_path);
                
                // update music stream
                // if(IsMusicReady(music))
                //     UnloadMusicStream(music);
                // music = LoadMusicStream(full_song_path);
                // music.looping = false;
                // PlayMusicStream(music);
                // play_music = true;
                // SetMusicPitch(music, pitch);
                update_music_stream(&music, full_song_path, pitch);
                // change cover art
                update_cover_art(&background, &song_cover, current_playlist, current_song);
                play_music = true;
            }

            else
            {
                printf("%s is not a mp3\n", current_song);
                return 1;
            }
        }

        if(play_music)
            UpdateMusicStream(music);
        
        percentage_played = GetMusicTimePlayed(music) / GetMusicTimeLength(music);

        BeginDrawing();
            ClearBackground(RAYWHITE);

            if(background.id)
                DrawTexture(background, 0, 0, RAYWHITE);
            if(song_cover.id)
                DrawTexture(song_cover, 0, 0, RAYWHITE);
            if(shuffle)
                DrawText("SHUFFLE", 650, 700, 20, BLACK);
            skip_button(&music, &skip_song, play_music);
            pause_button(&music, &play_music);
            rewind_button(&music, &rewind_song, &restart_song);
            progress_bar(&percentage_played, &update_position);
            DrawText(display_name, 300, 300, 20, BLACK);
            DrawFPS(500,500);
        EndDrawing();
    }

    save_music(current_playlist, current_song, pitch);
    if(song_cover.id)
        UnloadTexture(song_cover);
    if(background.id)
        UnloadTexture(background);
    if(IsMusicReady(music))
        UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
#include "stdio.h"
#include "headers/raylib.h"
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 128
#define NSONGS 100
#define NPLAYLIST 100

const char* MUSIC_SAVE = "saved_music.txt"; 
const char* PLAYLIST_DIR = "/home/bubbq/Music"; 
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; 

// replaces linux escape sequences to english format
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

// converts the original path passed into linux format
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

// returns number of files in folder, loads 
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
    return !strcmp(file_extension, MP3);
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

void get_cover_path(char* dst, const char* cover_root, const char* song_path)
{
    snprintf(dst, MAX_LEN * 2, "%s/%s", cover_root, song_path);
    remove_extension(dst);
    snprintf(dst + strlen(dst), MAX_LEN, ".jpg");
}

void extract_song_cover(const char* cover_path, const char* music_root, const char* playlist_path, const char* song_path)
{
    char command[MAX_LEN * 4];
    snprintf(command, MAX_LEN * 4, "ffmpeg -i %s/%s/%s -vf scale=250:-1 \"%s\" -loglevel error", music_root, playlist_path, song_path, cover_path);
    system(command);
}

void update_cover_art(Image* image, Texture2D* song_cover, const char* current_playlist, const char* current_song)
{
    char cover_path[MAX_LEN * 2];
    get_cover_path(cover_path, MP3_COVER_PATH, current_song);
   
    FILE* cover = fopen(cover_path, "r");
    if(cover == NULL)
        extract_song_cover(cover_path, PLAYLIST_DIR, current_playlist, current_song);
    else
        fclose(cover);

    if(image->data)
        UnloadImage(*image);
    if(song_cover->id)
        UnloadTexture(*song_cover);

    *image = LoadImage(cover_path);
    *song_cover = LoadTextureFromImage(*image);
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
    float pitch = 1.00;
    bool update_song = false;
    bool play_music = false;
    int song_index = 0;
    int playlist_index = 1;
    char songs[NSONGS][MAX_LEN];
    char playlists[NPLAYLIST][MAX_LEN];
    char display_name[MAX_LEN];
    bool update_position = false;
    float percentage_played = 0.00;

    Music music;
    Image image;
    Texture2D song_cover;

    // init playlist and songs
    int playlists_read = get_files_from_folder(MAX_LEN, NPLAYLIST, playlists, PLAYLIST_DIR);
    char playlist_path[MAX_LEN * 2];
    snprintf(playlist_path, MAX_LEN * 2, "%s/%s", PLAYLIST_DIR, playlists[playlist_index]);
    int songs_read = get_files_from_folder(MAX_LEN, NSONGS, songs, playlist_path);

    char current_playlist[MAX_LEN];
    sprintf(current_playlist, "%s", playlists[playlist_index]);

    char current_song[MAX_LEN];
    sprintf(current_song, "%s", songs[song_index]);

    char full_song_path[MAX_LEN * 3];
    snprintf(full_song_path, (MAX_LEN * 3), "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
    remove_escape_sequence(full_song_path);

    InitWindow(750, 750, "music player");
    // SetTraceLogLevel(LOG_ERROR);
    InitAudioDevice();
    SetTargetFPS(60);
    FILE* cover_directory = fopen(MP3_COVER_PATH, "r");
    if(cover_directory == NULL)
        create_cover_directory(MP3_COVER_PATH);
    else
        fclose(cover_directory);
    
    if(is_mp3(full_song_path))
    {
        music = LoadMusicStream(full_song_path);
        music.looping = false;
        update_cover_art(&image, &song_cover, current_playlist, current_song);

        strcpy(display_name, current_song);
        remove_escape_sequence(display_name);
        remove_extension(display_name);
    }
    
    while(!WindowShouldClose())
    {
        // pitch set
        if(IsKeyPressed(KEY_W))
        {
            pitch += 0.025;
            SetMusicPitch(music, pitch);
        }
        if(IsKeyPressed(KEY_S))
        {
            pitch -= 0.025;
            SetMusicPitch(music, pitch);
        }

        if(update_position)
        {
            update_position = false;
            SeekMusicStream(music, GetMusicTimeLength(music) * percentage_played);
        }

        if(update_song)
        {
            update_song = false;
            
            if(is_mp3(full_song_path))
            {
                strcpy(current_song, songs[song_index]);

                strcpy(display_name, current_song);
                remove_escape_sequence(display_name);
                remove_extension(display_name);

                snprintf(full_song_path, (MAX_LEN * 3), "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
                remove_escape_sequence(full_song_path);
                
                if(IsMusicReady(music))
                    UnloadMusicStream(music);
                music = LoadMusicStream(full_song_path);
                music.looping = false;
                PlayMusicStream(music);
                SetMusicPitch(music, pitch);
                
                update_cover_art(&image, &song_cover, current_playlist, current_song);
            }
            else
            {
                printf("%s is not a mp3\n", current_song);
                return 1;
            }
        }
        percentage_played = GetMusicTimePlayed(music) / GetMusicTimeLength(music);

        // play pause
        if(IsKeyPressed(KEY_SPACE))
        {
            play_music = !play_music;
            if(play_music)
                PlayMusicStream(music);
            else
                PauseMusicStream(music);
        }

        // skip 
        if(IsKeyPressed(KEY_D) || (!IsMusicStreamPlaying(music) && play_music))
        {
            song_index = adjust_index((song_index + 1), songs_read);
            update_song = true;
        }

        // rewind 
        if(IsKeyPressed(KEY_A))
        {
            if(GetMusicTimePlayed(music) > 3)
                SeekMusicStream(music, 0.25);
            else
            {
                song_index = adjust_index((song_index - 1), songs_read);
                update_song = true;
            }
        }
        UpdateMusicStream(music);
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawFPS(500,500);
            if(song_cover.id)
                DrawTexture(song_cover, 0, 0, RAYWHITE);

            // play pause button
            Rectangle bar = {200, 400, 200, 10};
            DrawRectangleRec(bar, RED);
            Vector2 mouse_position = GetMousePosition();
            if(CheckCollisionPointRec(mouse_position, bar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                percentage_played = (mouse_position.x - bar.x) / bar.width;
                update_position = true;
            }
            DrawRectangle(200, 400, 200 * percentage_played, 10, GREEN);
            DrawText(display_name, 300, 300, 20, BLACK);
        EndDrawing();
    }

    save_music(current_playlist, current_song, pitch);
    if(image.data)
        UnloadImage(image);
    if(song_cover.id)
        UnloadTexture(song_cover);
    if(IsMusicReady(music))
        UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
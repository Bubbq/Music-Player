#include "headers/raylib.h"
#include <stddef.h>
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
    snprintf(dst, (MAX_LEN * 2), "%s/%s", cover_root, song_path);
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
    // snprintf(command, MAX_LEN * 4, "ffmpeg -i %s/%s/%s -vf scale=250:-1 \"%s\" -loglevel error", music_root, playlist_path, song_path, cover_path);
    snprintf(command, MAX_LEN * 4, "ffmpeg -i %s/%s/%s \"%s\" -loglevel error", music_root, playlist_path, song_path, cover_path);
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
    
    else
    {
        fprintf(ptr, "%s\n%s\n%f", playlist_path, song_path, pitch);
        fclose(ptr);
    }
}

void rewind_button(float time_played, bool* rewind_song, bool* restart_song, bool holding_progress_bar)
{
    const char* button_text = "PREV";
    const Rectangle button_border = {125, 500, 75, 30};

    if((GuiButton(button_border,button_text) && !holding_progress_bar)|| IsKeyPressed(KEY_A))
    {
        if(time_played > 3.00)
            *restart_song = true;
        else
            *rewind_song = true;
    }
}

void pause_button(bool* play_music, bool holding_progress_bar)
{
    const char* button_text = (*play_music) ? "PAUSE" : "PLAY";
    const Rectangle button_border = {200, 500, 75, 30};
    
    if((GuiButton(button_border, button_text) && !holding_progress_bar)|| IsKeyPressed(KEY_SPACE))
        *play_music = !(*play_music);
}

void skip_button(bool* skip_song, bool song_over, bool holding_progress_bar)
{
    const char* button_text = "SKIP";
    const Rectangle button_border = {275, 500, 75, 30};

    if((GuiButton(button_border, button_text) && !holding_progress_bar) || IsKeyPressed(KEY_D) || song_over)
        *skip_song = true;
}

void progress_bar(float* percentage_played, bool* update_position, bool* holding_progress_bar)
{
    const Rectangle BAR = {200, 400, 200, 10};
    
    Vector2 mouse_position = GetMousePosition();
    if(CheckCollisionPointRec(mouse_position, BAR) || (*holding_progress_bar == true))
    {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            *holding_progress_bar = true;

            *percentage_played = (mouse_position.x - BAR.x) / BAR.width;
            if(*percentage_played < 0.00)
                *percentage_played = 0.00;
            else if(*percentage_played > 1.00)
                *percentage_played = 1.00;
        }

        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            *update_position = true;
            *holding_progress_bar = false;

            *percentage_played = (mouse_position.x - BAR.x) / BAR.width;
            if(*percentage_played < 0.00)
                *percentage_played = 0.00;
            else if(*percentage_played > 1.00)
                *percentage_played = 1.00;
        }
    }

    DrawRectangleRec(BAR, RED);
    DrawRectangle(BAR.x, BAR.y, (BAR.width * (*percentage_played)), BAR.height, GREEN);
}

bool set_pitch(float* pitch)
{
    const float delta = 0.025;

    if(IsKeyPressed(KEY_W))
    {
        *pitch += delta;
        return true;
    }

    else if(IsKeyPressed(KEY_S))
    {
        *pitch -= delta;
        return true;
    }

    else
        return false;
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

void update_time_display(char* time_played, int music_played, char* time_left, int music_left)
{
    int minutes_played = music_played / 60;
    int seconds_played = music_played % 60;
    int minutes_left = music_left / 60;
    int seconds_left = music_left % 60;

    sprintf(time_played, "%2d:%02d", minutes_played, seconds_played);
    sprintf(time_left, "%2d:%02d", minutes_left, seconds_left);
}

void resize_stack(int** stack, size_t* size)
{
    (*size) *= 2;
    *stack = realloc(*stack, *size);
}

void add_stack_element(int** stack, int* ptr, size_t* size, int element)
{
    if(((*ptr) + 1) * sizeof(int) == *size)   
        resize_stack(stack, size);
    else
        (*stack)[++(*ptr)] = element;
}

int pop_stack(int* stack, int* ptr)
{
    if((*ptr) >= 0)
        return stack[(*ptr)--];
    else
        return -1;
}

// TODO: 
// add shuffle
// add default images
// handle edge cases (file not mp3 etc)

int main()
{
    bool disable_skip_button = false;
    bool holding_progress_bar = false;
    bool play_music = false;
    bool shuffle = false;
    bool skip_song = false;
    bool rewind_song = false;
    bool restart_song = false;
    bool update_position = false;

    float pitch = 1.00;
    float percentage_played = 0.00;
    
    int song_index = 0;
    int playlist_index = 0;

    char playlists[NPLAYLIST][MAX_LEN];
    char current_playlist[MAX_LEN];
    
    char current_song[MAX_LEN];
    char songs[NSONGS][MAX_LEN];
    char display_name[MAX_LEN];

    char time_played[MAX_LEN];
    char time_left[MAX_LEN];

    Music music = {0};
    Texture2D song_cover = {0};
    Texture2D background = {0};

    int playlists_read = get_files_from_folder(MAX_LEN, NPLAYLIST, playlists, PLAYLIST_DIR);
    char playlist_path[MAX_LEN * 2];
    snprintf(playlist_path, MAX_LEN * 2, "%s/%s", PLAYLIST_DIR, playlists[playlist_index]);
    int songs_read = get_files_from_folder(MAX_LEN, NSONGS, songs, playlist_path);
    
    int ptr = -1;
    size_t stack_size = sizeof(int) * songs_read;
    int* song_history = malloc(stack_size);

    strcpy(current_playlist, playlists[playlist_index]);
    strcpy(current_song, songs[song_index]);

    char full_song_path[MAX_LEN * 3];
    snprintf(full_song_path, (MAX_LEN * 3), "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
    remove_escape_sequence(full_song_path);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(750, 750, "music player");
    SetTraceLogLevel(LOG_ERROR);
    InitAudioDevice();
    SetTargetFPS(60);
    
    FILE* cover_directory = fopen(MP3_COVER_PATH, "r");
    if(cover_directory == NULL)
        create_cover_directory(MP3_COVER_PATH);
    else
        fclose(cover_directory);
    
    if(is_mp3(full_song_path))
    {
        update_music_stream(&music, full_song_path, pitch);
        update_cover_art(&background, &song_cover, current_playlist, current_song);
        update_display_name(display_name, current_song);
    }
    
    while(!WindowShouldClose())
    {
        int music_played = holding_progress_bar ? (GetMusicTimeLength(music) * percentage_played) : GetMusicTimePlayed(music);
        int music_left = GetMusicTimeLength(music) - music_played;
        update_time_display(time_played, music_played, time_left, music_left);

        // disable_skip_button = (shuffle && ptr < 0 && music_played < 3);  

        if(set_pitch(&pitch))
            SetMusicPitch(music, pitch);
        
        if(IsKeyPressed(KEY_X))
        {
            shuffle = !shuffle;
            if(shuffle)
                add_stack_element(&song_history, &ptr, &stack_size, song_index);
            // clearing shuffle history
            else
                ptr = -1;
        }

        if(update_position)
        {
            if(!percentage_played)
                restart_song = true;
            else if(percentage_played == 1)
                skip_song = true;
            else
                SeekMusicStream(music, GetMusicTimeLength(music) * percentage_played);
            
            update_position = false;
        }

        if(restart_song)
        {
            restart_song = false;
            StopMusicStream(music);
            PlayMusicStream(music);
        }

        // update enviornment
        if(skip_song || rewind_song)
        {
            if(skip_song)
            {
                skip_song = false;
                if(shuffle)
                {
                    add_stack_element(&song_history, &ptr, &stack_size, song_index);
                    int rand;
                    while((rand = GetRandomValue(0, songs_read - 1)) == song_index)
                        ;
                    song_index = rand;
                }
                else
                    song_index = adjust_index((song_index + 1), songs_read);
            }

            else if(rewind_song)
            {
                rewind_song = false;
                if(shuffle)
                {
                    int last_song = pop_stack(song_history, &ptr);
                    if(last_song >= 0)
                        song_index = last_song;
                }
                else
                    song_index = adjust_index((song_index - 1), songs_read);
            }

            strcpy(current_song, songs[song_index]);
            
            if(is_mp3(current_song))
            {
                update_display_name(display_name, current_song);
                snprintf(full_song_path, (MAX_LEN * 3), "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
                remove_escape_sequence(full_song_path);
                update_music_stream(&music, full_song_path, pitch);
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

            // if(background.id)
            //     DrawTexture(background, 0, 0, RAYWHITE);
            if(song_cover.id)
                DrawTexture(song_cover, 0, 0, RAYWHITE);
            if(shuffle)
                DrawText("SHUFFLE", 650, 700, 20, BLACK);
            skip_button(&skip_song, (!IsMusicStreamPlaying(music) && play_music), holding_progress_bar);
            pause_button( &play_music, holding_progress_bar);
           
            if(disable_skip_button)
                GuiSetState(STATE_DISABLED);
            rewind_button(GetMusicTimePlayed(music), &rewind_song, &restart_song, holding_progress_bar);
            GuiSetState(STATE_NORMAL);
           
            DrawText(time_played, 150, 400, 17, BLACK);
            progress_bar(&percentage_played, &update_position, &holding_progress_bar);
            DrawText(time_left, 400, 400, 17, BLACK);
            DrawText(display_name, 300, 300, 20, BLACK);
            DrawFPS(500,500);
        EndDrawing();
    }

    free(song_history);
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
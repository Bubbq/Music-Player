#include "headers/raylib.h"
#include "time.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>  
#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

#define MAX_LEN 128
#define NSONGS 100
#define NPLAYLIST 100

const char* MUSIC_SAVE = "saved_music.txt"; 
const char* PLAYLIST_DIR = "/home/bubbq/Music"; 
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers"; 

void remove_character_from_string(char* string, const char target)
{
    int k = 0;

    for(int i = 0; i < strlen(string); i++)
        if(string[i] != target)
            string[k++] = string[i];

    string[k] = '\0';
}

void linux_formatted_filename(int max_string_length, char* destination, const char* original_path)
{
    // characters in linux that need escape sequences and their corresponding replacement
    const char* INVALID = " ()&'`\\!";  
    const char* replacement_characters[] = { "\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\\", "\\!" };

    int length = 0;
    int n_characters = strlen(original_path);

    for(int i = 0; i < n_characters; i++)
    {
        // find next invalid character
        const char* found = strchr(INVALID, original_path[i]);

        // replace invalid character with linux escape sequence
        if(found != NULL)
        {
            int replacement_index = found - INVALID;
            const char* replacement = replacement_characters[replacement_index];
            int replacement_length = strlen(replacement);

            if(length + replacement_length < max_string_length)
            {
                strcpy(destination + length, replacement);
                length += replacement_length;
            }

            else
                break;
        }

        else
            destination[length++] = original_path[i];
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

    for(; (fgets(buffer, nchars, output_stream) != NULL) && (files_read < nfiles); files_read++)
    {
        buffer[strlen(buffer) - 1] = '\0';
        linux_formatted_filename(nchars, files[files_read], buffer);
    }

    pclose(output_stream);

    return files_read;
}

int is_extension(const char* file, const char* extension)
{
    char* file_extension = strrchr(file, '.');

    return (strcmp(file_extension, extension) == 0);
}

void remove_extension(char* file_path)
{
    char* extension = strrchr(file_path, '.');
    if(file_path != NULL)
        file_path[extension - file_path] = '\0';
    else
        printf("\"%s\" was not an extension thus it couldn't be removed\n", file_path);
}

int adjust_index(int index, int size)
{
    return (index + size) % size;
} 

void make_directory(const char* directory_path)
{
    if((mkdir(directory_path, 0755)) == -1)
        printf("there was an error making the directory: \"%s\"\n", directory_path);
}

void update_cover_path(char* cover_path, const char* cover_root, const char* song_path)
{
    sprintf(cover_path, "%s/%s", cover_root, song_path);
    remove_extension(cover_path);

    int length = strlen(cover_path);
    const char* extension = ".jpg";

    sprintf(cover_path + length, "%s", extension);
    length += strlen(extension);
    
    cover_path[length] = '\0';
}

void extract_song_cover(const char* cover_path, const char* full_song_path)
{
    char command[MAX_LEN * 2];
    sprintf(command, "ffmpeg -i %s -vf scale=250:-1 \"%s\" -loglevel error", full_song_path, cover_path);

    system(command);
}

void update_cover_art(Texture2D* background, Texture2D* song_cover, const char* cover_path, const char* full_song_path)
{
    FILE* cover = fopen(cover_path, "r");

    if(cover == NULL)
        extract_song_cover(cover_path,full_song_path);
    else
        fclose(cover);

    if(song_cover->id)
        UnloadTexture(*song_cover);

    if(background->id)
        UnloadTexture(*background);

    int default_used = 0;
    *song_cover = LoadTexture(cover_path);

    if(!song_cover->id)
    {
        default_used = 1;
        *song_cover = LoadTexture("default.png");
    }

    song_cover->width = song_cover->height = 250;
}

void save_music(char* playlist_path, char* song_path, float pitch)
{
    FILE* ptr = fopen(MUSIC_SAVE, "w");
    if(ptr != NULL)
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

void update_music_stream(Music* music, char* full_song_path, float pitch)
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

void add_stack_element(int element, int nelements, int stack[nelements], int* ptr)
{
    if((*ptr) < nelements)
        stack[++(*ptr)] = element;
}   

int pop_stack(int nelements, int stack[nelements], int* ptr)
{
    if(*ptr >= 0)
        return stack[(*ptr)--];
    else
        return -1;
}

void shuffle_array(int size, int array[])
{
    for (int i = size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);

        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void create_song_queue(int nsongs, int current_song, int queue[nsongs])
{
    // create an array of elements not including the current song
    for(int i = 0; i < nsongs; i++)
        if(i < current_song)
            queue[i] = i;
        else
            queue[i] = i + 1;

    // shuffle the array's elements to create a song queue
    shuffle_array(nsongs, queue);
}

void init_app()
{
    srand(time(NULL));
    InitWindow(700,700, "music player");
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitAudioDevice();
    SetTargetFPS(60);
}

int extract_metadata(const char *file_path, const char *parameter, char *output, int output_size)
{
    AVFormatContext *format_context = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret;

    if ((ret = avformat_open_input(&format_context, file_path, NULL, NULL)) < 0) {
        fprintf(stderr, "Error opening file %s\n", file_path);
        return ret;
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Error finding stream info\n");
        avformat_close_input(&format_context);
        return -1;
    }

    tag = av_dict_get(format_context->metadata, parameter, NULL, AV_DICT_IGNORE_SUFFIX);
    if (tag) {
        snprintf(output, output_size, "%s", tag->value);
    } else {
        fprintf(stderr, "Parameter '%s' not found in metadata\n", parameter);
        avformat_close_input(&format_context);
        return -1;
    }

    avformat_close_input(&format_context);
    return 0;
}

// TODO: 
    // change progress bar
    // 

int main()
{
    // music control flags
    bool update_display = false;
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

    char time_left[MAX_LEN];
    char time_played[MAX_LEN];

    char playlists[NPLAYLIST][MAX_LEN];
    int playlists_read = get_files_from_folder(MAX_LEN, NPLAYLIST, playlists, PLAYLIST_DIR);
    
    char playlist_path[MAX_LEN * 2];
    sprintf(playlist_path, "%s/%s", PLAYLIST_DIR, playlists[playlist_index]);
    
    char songs[NSONGS][MAX_LEN];
    int songs_read = get_files_from_folder(MAX_LEN, NSONGS, songs, playlist_path);
    
    int stack_ptr = -1;
    int song_history[songs_read];

    int queue_ptr = 0;
    int song_queue[songs_read - 1];

    char current_playlist[MAX_LEN];
    strcpy(current_playlist, playlists[playlist_index]);
    
    char current_song[MAX_LEN];
    strcpy(current_song, songs[song_index]);

    char full_song_path[MAX_LEN * 3];
    char linux_full_song_path[MAX_LEN * 3];
    sprintf(full_song_path, "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
    remove_character_from_string(full_song_path, '\\');
    strcpy(linux_full_song_path, full_song_path);
    
    
    char display_name[MAX_LEN];
    extract_metadata(full_song_path, "title", display_name, MAX_LEN);

    char artist[MAX_LEN];
    extract_metadata(full_song_path, "artist", artist, MAX_LEN);
    
    init_app();
    
    FILE* cover_directory = fopen(MP3_COVER_PATH, "r");
    if(cover_directory == NULL)
        make_directory(MP3_COVER_PATH);
    else
        fclose(cover_directory);
    
    Music music = {0};
    update_music_stream(&music, full_song_path, pitch);
    
    Texture2D song_cover = {0};
    Texture2D background = {0};
    char cover_path[MAX_LEN * 2];
    update_cover_path(cover_path, MP3_COVER_PATH, current_song);
    update_cover_art(&background, &song_cover, cover_path,linux_full_song_path);
    
    while(!WindowShouldClose())
    {
        int music_played = holding_progress_bar ? (GetMusicTimeLength(music) * percentage_played) : GetMusicTimePlayed(music);
        int music_left = GetMusicTimeLength(music) - music_played;
        update_time_display(time_played, music_played, time_left, music_left);

        if(set_pitch(&pitch))
            SetMusicPitch(music, pitch);
        
        if(IsKeyPressed(KEY_X))
        {
            shuffle = !shuffle;
            if(shuffle)
            {
                queue_ptr = 0;
                stack_ptr = -1;
                create_song_queue((songs_read - 1), song_index, song_queue);
                add_stack_element(song_index, songs_read, song_history, &stack_ptr);
            }
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

        if(skip_song || rewind_song)
        {
            if(skip_song)
            {
                skip_song = false;

                if(shuffle)
                {
                    if(queue_ptr >= songs_read - 1)
                    {
                        queue_ptr = 0;
                        stack_ptr = -1;
                        create_song_queue((songs_read - 1),song_index, song_queue);
                    }

                    add_stack_element(song_index, songs_read, song_history, &stack_ptr);
                    song_index = song_queue[queue_ptr++];
                }

                else
                    song_index = adjust_index((song_index + 1), songs_read);
            }

            else if(rewind_song)
            {
                rewind_song = false;
                if(shuffle)
                {
                    if(queue_ptr > 0)
                        queue_ptr--;
                    
                    int last_song = pop_stack(songs_read, song_history, &stack_ptr);
                    if(last_song >= 0)
                        song_index = last_song;
                }

                else
                    song_index = adjust_index((song_index - 1), songs_read);
            }

            strcpy(current_song, songs[song_index]);
            if(is_extension(current_song, ".mp3"))
            {
                play_music = true;

                sprintf(full_song_path,  "%s/%s/%s", PLAYLIST_DIR, current_playlist, current_song);
                strcpy(linux_full_song_path, full_song_path);
                remove_character_from_string(full_song_path, '\\');

                update_music_stream(&music, full_song_path, pitch);
                update_cover_path(cover_path, MP3_COVER_PATH, current_song);
                update_cover_art(&background, &song_cover, cover_path,linux_full_song_path);
                extract_metadata(full_song_path, "title", display_name, MAX_LEN);
                extract_metadata(full_song_path, "artist", artist, MAX_LEN);
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
                DrawText("SHUFFLE", 600, 600, 20, BLACK);

            skip_button(&skip_song, (!IsMusicStreamPlaying(music) && play_music), holding_progress_bar);
            pause_button( &play_music, holding_progress_bar);
            rewind_button(GetMusicTimePlayed(music), &rewind_song, &restart_song, holding_progress_bar);
            progress_bar(&percentage_played, &update_position, &holding_progress_bar);
           
            DrawText(time_played, 150, 400, 17, BLACK);
            DrawText(time_left, 400, 400, 17, BLACK);
            DrawText(display_name, 300, 300, 20, BLACK);
            DrawText(artist, 300, 325, 20, BLACK);
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
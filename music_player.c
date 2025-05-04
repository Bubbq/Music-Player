#include "headers/raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <libavformat/avformat.h>
#include <sys/stat.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#define LEN 512
#define NSONGS 100
#define NPLAYLIST 100
#define IMG_SIZE 68

const char* MUSIC_SAVE = "saved_music.txt"; 
const char* PLAYLIST_DIR = "/home/bubbq/Music"; 
const char* MP3_COVER_PATH = "/home/bubbq/.cache/mp3_covers";
const char* DEFAULT_IMG = "default.png";

typedef struct
{
    bool holding_progess_bar_slider;
    bool update_song_position;
    bool shuffle_play;
    bool change_song;
    bool skip_song;
    bool rewind_song;
    bool restart_song;
    bool play_music;
} Flags;

// removes all instances of 'target' from 'string'
void remove_character(int len, char string[], const char target)
{
    int k = 0;
    int length = strlen(string);

    for(int i = 0; i < length; i++) {
        if(string[i] != target) {
            if(k != i)
                string[k] = string[i];
            k++;
        }
    }

    string[k] = '\0';
}

// converts a filepath 'src' into a linux formatted file stored into 'dst', retruns dst size
int linux_format(int maxlen, char dst[], const char* src)
{
    // characters in linux that need escape sequences 
    const char BAD_CHARS [] = " ()&'`\\!\"$*?#{}[]|;<>~";

    // their corresponding replacement
    const char* REPLACE[] = {
        "\\ ",  // space
        "\\(",  // (
        "\\)",  // )
        "\\&",  // &
        "\\'",  // '
        "\\`",  // `
        "\\\\", // backslash
        "\\!",  // !
        "\\\"", // "
        "\\$",  // $
        "\\*",  // *
        "\\?",  // ?
        "\\#",  // #
        "\\{",  // {
        "\\}",  // }
        "\\[",  // [
        "\\]",  // ]
        "\\|",  // |
        "\\;",  // ;
        "\\<",  // <
        "\\>",  // >
        "\\~"   // ~
    };

    int n_characters = strlen(src);
    int length = 0; // current index of dst
    char* next_invalid = NULL; // a ptr to an invalid char in BAD_CHARS

    for(int i = 0; i < n_characters; i++, (next_invalid = strchr(BAD_CHARS, src[i]))) {
        
        // if the ith char is a bad one, overwrite with the corresp replacement string
        if(next_invalid) {

            int replacement_index = next_invalid - BAD_CHARS;
            const char* replacement_string = REPLACE[replacement_index];
            int replacement_length = strlen(replacement_string);

            if(length + replacement_length < maxlen - 1) {
                strcpy(dst + length, replacement_string);
                length += replacement_length;
            }
            
            else
                return 0;
        }

        else if(length < maxlen - 1)
            dst[length++] = src[i];

        else
            return 0;
    }

    dst[length] = '\0';
    return length;
}

// comparator needed for qsort
int cmp(const void* a, const void* b)
{
    return strcmp((const char*)a, (const char*)b);
}

// returns the number of files in 'folder_path' of type 'file_type' stored in 'dst'
int get_files_from_folder(int maxlen, int nfiles, char dst[nfiles][maxlen], const char* folder_path, const char file_type)
{
    DIR* pDIR = NULL;
    struct dirent* pDirent = NULL;

    if((pDIR = opendir(folder_path)) == NULL) {
        printf("%s was not able to be opened\n", folder_path);
        return 0;
    }

    char buffer[maxlen];
    int files_read = 0;

    while((pDirent = readdir(pDIR)) != NULL) {
        strcpy(buffer, pDirent->d_name);
        
        if((buffer[0] == '.') || (pDirent->d_type != file_type))
            continue;
        
        if(linux_format(maxlen, dst[files_read], buffer) > 0) 
            files_read++;

        // ignore strings longer than maxlen
        else
            dst[files_read][0] = '\0';
    }

    closedir(pDIR);
    qsort(dst, files_read, sizeof(dst[0]), cmp); // sort the files alphabetically
    return files_read;
}

// returns 1 if file_path is an 'extension'
bool is_extension(const char* file_path, const char* extension)
{
    char* file_extenison = strrchr(file_path, '.');

    if(file_extenison)
        return (strcmp(file_extenison, extension) == 0);
    else
        return false;
}

// removes all non files that are not of an 'extension'
int filter_library(int maxlen, int nsongs, char library[nsongs][maxlen], const char* extension)
{
    int k = 0;

    for(int i = 0; i < nsongs; i++) {
        if(is_extension(library[i], extension)) {
            if(k != i)
                strcpy(library[k], library[i]);
            k++;
        }
    }

    return k;
}

// removes the extension of a mutuable string
void remove_extension(char file[])
{
    if(file) {
    char* file_extension = strrchr(file, '.');

    if(file_extension)
        file[file_extension - file] = '\0';
    }
}

// stores the path of the cover image of the current song in 'song_cover_path'
void update_cover_path(int maxlen, char song_cover_path[], const char* song_path)
{
    sprintf(song_cover_path, "%s/%s", MP3_COVER_PATH, song_path);
    remove_extension(song_cover_path);

    int length = strlen(song_cover_path);

    const char* jpeg_extension = ".jpg";
    int jpeg_length = strlen(jpeg_extension);

    if(length + jpeg_length >= maxlen) {
        printf("the cover image was too large, using default image\n");
        strcpy(song_cover_path, DEFAULT_IMG);
        length = strlen(DEFAULT_IMG);
    }

    else {
        strcpy(song_cover_path + length, jpeg_extension);
        length += strlen(jpeg_extension);
    }
    
    song_cover_path[length] = '\0';
}

// gets the 'parameter' from the audio/video file's metadata into dst
int extract_metadata(int maxlen, char dst[], const char *file_path, const char *parameter)
{
    AVFormatContext *format_context = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret;

    if ((ret = avformat_open_input(&format_context, file_path, NULL, NULL)) < 0) {
        printf("error opening the file \"%s\"\n", file_path);
        return ret;
    }

    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
        printf("error finding stream info\n");
        avformat_close_input(&format_context);
        return ret;
    }

    // the info of the parameter passed
    tag = av_dict_get(format_context->metadata, parameter, NULL, AV_DICT_IGNORE_SUFFIX);

    if(tag && (strlen(tag->value) < maxlen))
        strcpy(dst, tag->value);

    else {
        printf("the parameter \"%s\" was not found in %s\n", parameter, file_path);
        avformat_close_input(&format_context);
        return -1;
    }

    avformat_close_input(&format_context);
    return 0;
}

// Writes the cover image of file path to dst
int extract_cover_image(const char *file_path, const char *dst)
{
    AVFormatContext *format_context = NULL;
    int ret;

    if ((ret = avformat_open_input(&format_context, file_path, NULL, NULL)) < 0) {
        printf("error opening the file: \"%s\"\n", file_path);
        return ret;
    }

    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
        printf("error finding stream info\n");
        avformat_close_input(&format_context);
        return ret;
    }

    // the attached picture occurs when the stream type is of 'AV_DISPOSITION_ATTACHED_PIC'
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        AVStream *stream = format_context->streams[i];
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket pkt = stream->attached_pic;

            FILE *f = fopen(dst, "wb");

            if (!f) {
                avformat_close_input(&format_context);
                return -1;
            }

            fwrite(pkt.data, 1, pkt.size, f);
            fclose(f);

            avformat_close_input(&format_context);
            return 0;  
        }
    }

    // no picture extracted case
    avformat_close_input(&format_context);
    return -1;
}

// for debugging
void print_list(int nelements, int nchars, char library[nelements][nchars])
{
    for(int i = 0; i < nelements; i++)
        printf("%d) %s\n", (i + 1), library[i]);
}

// returns 1 if the directory exists, 0 otherwise
int directory_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0) && (S_ISDIR(st.st_mode));
}

// makes the directory specified in 'directory_path'
void make_directory(const char* directory_path)
{
    if((mkdir(directory_path, 0755)) == -1)
        printf("there was an error making the directory \"%s\"\n", directory_path);
}

// shuffles the elements of an array
void shuffle_array(int size, int array[])
{
    for (int i = size - 1; i > 0; i--) {
        int temp = array[i];
        int j = rand() % (i + 1);

        array[i] = array[j];
        array[j] = temp;
    }
}

// a song queue for shuffle play
void create_song_queue(int nsongs, int current_song_index, int queue[])
{
    queue[0] = current_song_index;

    for(int i = 1; i < nsongs; i++)
        if(i <= current_song_index)
            queue[i] = i - 1;
        else
            queue[i] = i;

    shuffle_array(nsongs - 1, queue + 1);
}

// TODO
    // make list of songs in window like spotify
    // clean bottom bar

// initalizes the application
void init_app()
{
    srand(time(NULL));
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(750,750, "music player");
    InitAudioDevice();
    SetTargetFPS(60);
}

// loads mp3 file specified in full_song_path
void load_new_song(Music* music, char* full_song_path, float pitch)
{
    if(music->ctxData)
        UnloadMusicStream(*music);

    *music = LoadMusicStream(full_song_path);
    music->looping = false;
    SetMusicPitch(*music, pitch);
}

// finds the array's equivalent of index 
int adjust_index(int index, int size)
{
    return (index + size) % size;
} 

// sets the display image to the new song loaded
void update_display_image(Texture2D* display_image, const char* image_path, const char* full_song_path, float image_size)
{
    if(display_image->id)
        UnloadTexture(*display_image);

    Image image = LoadImage(image_path);

    if(!image.data) {
        if(extract_cover_image(full_song_path, image_path) < 0)
            image = LoadImage(DEFAULT_IMG);
        else
            image = LoadImage(image_path);
    }    

    ImageResize(&image, image_size, image_size);
    *display_image = LoadTextureFromImage(image);

    if(image.data)
        UnloadImage(image);
}

// converts n seconds into a time string of minutes and seconds 
void seconds_to_string(int seconds, char string[])
{
    int minutes = (seconds / 60) % 60;
    seconds %= 60;
    sprintf(string, "%2d:%02d", minutes, seconds);
}

// music control buttons
void playback_buttons(Rectangle container, Music music, Flags* flags)
{
    const int button_size = 20;
    const float button_y_level = container.y + ((container.height - button_size) / 2.00);

    // rewind button
    const Rectangle rewind_button = (Rectangle){(GetScreenWidth() / 2.00) - (button_size * 2.00), button_y_level, button_size, button_size};
    if((GuiButton(rewind_button, "<") || IsKeyPressed(KEY_A)) && !flags->holding_progess_bar_slider) {
        if(GetMusicTimePlayed(music) > 3.00)
            flags->restart_song = true;
        else 
            flags->change_song = flags->rewind_song = true;
    }

    // pause/play button
    const Rectangle pause_button = (Rectangle){(GetScreenWidth() / 2.00) - (button_size / 2.00), button_y_level, button_size, button_size};
    if(GuiButton(pause_button, flags->play_music ? "||" : "|>") || IsKeyPressed(KEY_SPACE)) 
        flags->play_music = !flags->play_music;

    // skip button
    bool song_over = !IsMusicStreamPlaying(music) && flags->play_music;
    const Rectangle skip_button_bounds = (Rectangle){(GetScreenWidth() / 2.00) + button_size, button_y_level, button_size, button_size};
    if((GuiButton(skip_button_bounds, ">") || IsKeyPressed(KEY_D) || song_over) && !flags->holding_progess_bar_slider)
        flags->change_song = flags->skip_song = true;
}

void progress_bar(Rectangle container, Flags* flags, float* percentage_played, int time_played, int song_length)
{
    // update the slider strings when dragging the bar around
    if(flags->holding_progess_bar_slider)
        time_played = song_length * (*percentage_played);

    // converting current and total song time into strings in 00:00 format
    char time_played_text[LEN];
    char song_length_text[LEN];
    seconds_to_string(time_played, time_played_text);
    seconds_to_string(song_length, song_length_text);

    float progress_bar_width = GetScreenWidth() * 0.40;
    const Rectangle progress_bar_bounds = {(GetScreenWidth() / 2.00) - (progress_bar_width / 2.00), container.y + (container.height * 0.75), progress_bar_width, 10};
    
    if(GuiSliderBar(progress_bar_bounds, time_played_text, song_length_text, percentage_played, 0.00, 1.00))
        flags->holding_progess_bar_slider = true;
    
    // when the mouse is released after updating the slider is when the stream position is adjusted
    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && flags->holding_progess_bar_slider) {
        flags->holding_progess_bar_slider = false;
        
        if(!(*percentage_played))
            flags->restart_song = true;
        else if(*percentage_played == 1)
            flags->change_song = flags->skip_song = true;
        else
            flags->update_song_position = true;
    }
}

int main()
{   
    // pointers to current playlist and song
    int song_index, playlist_index;
    playlist_index = song_index = 0;

    // the size of both playlist and current song libraries
    int playlists_read, songs_read;
    playlists_read = songs_read = 0;

    // all playlists in PLAYLIST_DIR
    char playlists[NPLAYLIST][LEN];

    // all songs in the current playlist
    char songs[NSONGS][LEN];

    // the relative path of the current playlist and song
    char current_playlist[LEN];
    char current_song[LEN];

    // path to song cover (jpg)
    char song_cover_path[LEN * 2];

    // full path to the current playlist
    char current_playlist_path[LEN * 2];

    // the full path of the current song
    char full_song_path[LEN * 3];

    // the actual song and artist (stored in the metadata of the mp3 file, see 'ffprobe') 
    char song_title[LEN];
    char artist_name[LEN];

    // the list of shuffled songs to be played
    int shuffled_song_queue[NSONGS];
    int song_history = 0;

    // creating the cover path
    if(!directory_exists(MP3_COVER_PATH))
        make_directory(MP3_COVER_PATH);

    playlists_read = get_files_from_folder(LEN, NPLAYLIST, playlists, PLAYLIST_DIR, DT_DIR);
    
    if(!playlists_read) {
        printf("there are no folders to represent playlists in \"%s\", please add some\n", PLAYLIST_DIR);
        return 1;
    }

    // updating current playlist
    strcpy(current_playlist, playlists[playlist_index]);
    sprintf(current_playlist_path, "%s/%s", PLAYLIST_DIR, current_playlist);

    // getting all the song paths from the current playlist and filtering out any non mp3s
    int nfiles = get_files_from_folder(LEN, NSONGS, songs, current_playlist_path, DT_REG);
    songs_read = filter_library(LEN, nfiles, songs, ".mp3");

    if(!songs_read) {
        printf("there are no mp3s in the playlist \"%s\", please add some\n", current_playlist_path);
        return 1;
    }

    // updating current song
    strcpy(current_song, songs[song_index]);
    
    // updating full song path
    sprintf(full_song_path, "%s/%s", current_playlist_path, current_song);
    remove_character(strlen(full_song_path), full_song_path, '\\');

    // updating display text, use filenames if paramters are not avaible
    if(extract_metadata(sizeof(full_song_path), song_title, full_song_path, "title") < 0) {
        strcpy(song_title, current_song);
        remove_character(strlen(song_title), song_title, '\\');
    }

    if(extract_metadata(sizeof(full_song_path), artist_name, full_song_path, "artist") < 0) 
        artist_name[0] = '\0';

    init_app();

    // music control flags
    Flags flags = {false}; 

    // the speed of the music being played
    float pitch = 1.00;
    float percentage_played = 0.00;

    Music music = {0};
    load_new_song(&music, full_song_path, pitch);
    PlayMusicStream(music);

    Texture2D song_image = {0};
    update_cover_path(sizeof(song_cover_path), song_cover_path, current_song);
    update_display_image(&song_image, song_cover_path, full_song_path, IMG_SIZE);
    
    while(!WindowShouldClose()) {
        // toggling shuffle mode
        if(IsKeyPressed(KEY_X)) {
            flags.shuffle_play = !flags.shuffle_play;

            if(flags.shuffle_play) {
                song_history = 0;
                create_song_queue(songs_read, song_index, shuffled_song_queue);
            }
        }

        if(flags.update_song_position) {
            flags.update_song_position = false;
            SeekMusicStream(music, GetMusicTimeLength(music) * percentage_played);
        }

        // restarting current music stream
        if(flags.restart_song) {
            flags.restart_song = false;

            StopMusicStream(music);
            PlayMusicStream(music);
        }

        if(flags.change_song) { 
            flags.change_song = false;
            
            if(flags.skip_song) {
                flags.skip_song = false;

                if(flags.shuffle_play) {
                    if(song_history >= songs_read - 1) { 
                        int first_song;
                        while((first_song = GetRandomValue(0, (songs_read - 1))) == song_index)
                            ;
                        create_song_queue(songs_read, first_song, shuffled_song_queue);
                        song_history = -1;
                    }

                    song_index = shuffled_song_queue[++song_history];
                }

                else
                    song_index++;
            }

            else if(flags.rewind_song) {
                flags.rewind_song = false;

                if(flags.shuffle_play) {
                    if(song_history)
                        song_history--;
                    
                    song_index = shuffled_song_queue[song_history];
                }

                else
                    song_index--;
            }
            
            flags.play_music = true;

            // updating index and strings
            song_index = adjust_index(song_index, songs_read);
            strcpy(current_song, songs[song_index]);
            sprintf(full_song_path, "%s/%s", current_playlist_path, current_song);
            remove_character(sizeof(full_song_path), full_song_path, '\\');
            
            // updating display image
            update_cover_path(sizeof(song_cover_path), song_cover_path, current_song);
            update_display_image(&song_image, song_cover_path, full_song_path, IMG_SIZE);

            // updating display text
            if(extract_metadata(sizeof(song_title), song_title, full_song_path, "title") < 0) {
                strcpy(song_title, current_song);
                remove_character(strlen(song_title), song_title, '\\');
            }

            if(extract_metadata(sizeof(artist_name), artist_name, full_song_path, "artist") < 0) 
                artist_name[0] = '\0';

            load_new_song(&music, full_song_path, pitch);
            PlayMusicStream(music);
        }

        // playing music
        if(flags.play_music)
            UpdateMusicStream(music);

        if(!flags.holding_progess_bar_slider)
            percentage_played = (GetMusicTimePlayed(music) / GetMusicTimeLength(music));

        BeginDrawing();
            ClearBackground(RAYWHITE);
            // adjusting the pitch
            if(GuiSliderBar((Rectangle){35, 500, 100, 15}, "speed", "", &pitch, 0.875, 1.125))
                SetMusicPitch(music, pitch);

            // BOTTOM BAR
            {
                const float bar_height = 75.00; 
                const float image_padding = (bar_height - IMG_SIZE) / 2.00;
                const Rectangle bottom_bar_bounds = (Rectangle) {0, GetScreenHeight() - bar_height, GetScreenWidth(), bar_height};
                
                DrawRectangleRec(bottom_bar_bounds, LIGHTGRAY);
                
                // song cover image
                DrawTexture(song_image, image_padding, (bottom_bar_bounds.y + image_padding), RAYWHITE);

                // displaying song information
                DrawText(song_title, (bottom_bar_bounds.x + (image_padding * 2) + IMG_SIZE), bottom_bar_bounds.y + (bar_height / 4.00), 15, BLACK);
                DrawText(artist_name, (bottom_bar_bounds.x + (image_padding * 2) + IMG_SIZE), bottom_bar_bounds.y + (bar_height / 2.00), 15, BLACK);
                
                // pause, rewind, and skip buttons
                playback_buttons(bottom_bar_bounds, music, &flags);

                // progress bar
                progress_bar(bottom_bar_bounds, &flags, &percentage_played, GetMusicTimePlayed(music), GetMusicTimeLength(music));
            }

            if(flags.shuffle_play)
                DrawText("SHUFFLE", 100, 400, 15, BLACK);
            DrawFPS(0, 0);
        EndDrawing();
    }

    if(song_image.id)
        UnloadTexture(song_image);
    if(music.ctxData)
        UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// fix code :updated by: ...
// fill in effort estimation
// 
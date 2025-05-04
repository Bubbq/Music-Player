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
int is_extension(const char* file_path, const char* extension)
{
    char* file_extenison = strrchr(file_path, '.');

    if(file_extenison)
        return (strcmp(file_extenison, extension) == 0);
    else
        return 0;
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

void play_pause_button(Rectangle container, bool* is_music_playing)
{
    const float button_size = 20;
    const float button_y = container.y + ((container.height - button_size) / 2.00);
    const char* pause_button_text = *is_music_playing ? "||" : "|>";
    Rectangle pause_button_bounds = (Rectangle){(GetScreenWidth() / 2.00) - (button_size / 2.00), button_y, button_size, button_size};

    if(GuiButton(pause_button_bounds, pause_button_text) || IsKeyPressed(KEY_SPACE)) 
        *is_music_playing = !(*is_music_playing);
}

void rewind_button(Rectangle container, int time_played, bool* restart_song, bool* rewind, bool* update_song)
{
    const float button_size = 20;
    const char* rewind_button_text = "<";
    const float button_y = container.y + ((container.height - button_size) / 2.00);
    Rectangle rewind_button_bounds = (Rectangle){(GetScreenWidth() / 2.00) - (button_size * 2.00), button_y, button_size, button_size};
    
    if(GuiButton(rewind_button_bounds, rewind_button_text) || IsKeyPressed(KEY_A)) {
        if(time_played > 3.00)
            *restart_song = true;
        else 
            *update_song = *rewind = true;
    }
}

void skip_button(Rectangle container, bool* update_song, bool* skip_song, bool is_song_over)
{
    const float button_size = 20;
    const float button_y = container.y + ((container.height - button_size) / 2.00);
    const char* skip_button_text = ">";
    Rectangle skip_button_bounds = (Rectangle){(GetScreenWidth() / 2.00) + button_size, button_y, button_size, button_size};

    if(GuiButton(skip_button_bounds, skip_button_text) || IsKeyPressed(KEY_D) || is_song_over)
        *update_song = *skip_song = true;
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
    char linux_full_song_path[LEN * 3];

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
    songs_read = get_files_from_folder(LEN, NSONGS, songs, current_playlist_path, DT_REG);
    songs_read = filter_library(LEN, songs_read, songs, ".mp3");

    if(!songs_read) {
        printf("there are no mp3s in the playlist \"%s\", please add some\n", current_playlist_path);
        return 1;
    }

    // updating current song
    strcpy(current_song, songs[song_index]);
    
    // updating full song path
    sprintf(full_song_path, "%s/%s", current_playlist_path, current_song);
    strcpy(linux_full_song_path, full_song_path);
    remove_character(strlen(full_song_path), full_song_path, '\\');

    // updating display text, use filenames if paramters are not avaible
    if(extract_metadata(LEN, song_title, full_song_path, "title") < 0) {
        strcpy(song_title, current_song);
        remove_character(strlen(song_title), song_title, '\\');
    }

    if(extract_metadata(LEN, artist_name, full_song_path, "artist") < 0) 
        artist_name[0] = '\0';

    init_app();

    // music control flags
    bool holding_progess_bar_slider = false;
    bool update_song_position = false;
    bool shuffle_play = false;
    bool update_song = false;
    bool skip_song = false;
    bool rewind_song = false;
    bool restart_song = false;
    bool play_music = false;

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
            shuffle_play = !shuffle_play;

            if(shuffle_play) {
                song_history = 0;
                create_song_queue(songs_read, song_index, shuffled_song_queue);
            }
        }

        if(update_song_position) {
            update_song_position = false;
            SeekMusicStream(music, GetMusicTimeLength(music) * percentage_played);
        }

        // restarting current music stream
        if(restart_song) {
            restart_song = false;
            StopMusicStream(music);
            PlayMusicStream(music);
        }

        if(update_song) { 
            update_song = false;
            
            if(skip_song) {
                skip_song = false;

                if(shuffle_play) {
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

            else if(rewind_song) {
                rewind_song = false;

                if(shuffle_play) {
                    if(song_history)
                        song_history--;
                    
                    song_index = shuffled_song_queue[song_history];
                }

                else
                    song_index--;
            }
            
            play_music = true;

            // updating index and strings
            song_index = adjust_index(song_index, songs_read);
            strcpy(current_song, songs[song_index]);
            sprintf(full_song_path, "%s/%s", current_playlist_path, current_song);
            strcpy(linux_full_song_path, full_song_path);
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
        if(play_music)
            UpdateMusicStream(music);

        if(!holding_progess_bar_slider)
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
                Rectangle bottom_bar_border = (Rectangle) {0, GetScreenHeight() - bar_height, GetScreenWidth(), bar_height};
                
                // current song image
                DrawTexture(song_image, image_padding, (bottom_bar_border.y + image_padding), RAYWHITE);
                // displaying current song information
                DrawText(song_title, (bottom_bar_border.x + (image_padding * 2) + IMG_SIZE), bottom_bar_border.y + (bar_height / 4.00), 15, BLACK);
                DrawText(artist_name, (bottom_bar_border.x + (image_padding * 2) + IMG_SIZE), bottom_bar_border.y + (bar_height / 2.00), 15, BLACK);
                
                // buttons
                rewind_button(bottom_bar_border, GetMusicTimePlayed(music), &restart_song, &rewind_song, &update_song);
                play_pause_button(bottom_bar_border, &play_music);
                skip_button(bottom_bar_border, &update_song, &skip_song, (!IsMusicStreamPlaying(music) && play_music));

                // progress bar
                int time_elapsed = holding_progess_bar_slider ? (GetMusicTimeLength(music) * percentage_played) : GetMusicTimePlayed(music);
                
                char time_played[LEN];
                seconds_to_string(time_elapsed, time_played);
                
                char duration[LEN];
                seconds_to_string(GetMusicTimeLength(music), duration);

                float progress_bar_width = GetScreenWidth() * 0.40;
                const Rectangle progress_bar_bounds = {(GetScreenWidth() / 2.00) - (progress_bar_width / 2.00), bottom_bar_border.y + (bar_height * 0.75), progress_bar_width, 10};
                
                if(GuiSliderBar(progress_bar_bounds, time_played, duration, &percentage_played, 0.00, 1.00))
                    holding_progess_bar_slider = true;
                
                if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && holding_progess_bar_slider) {
                    holding_progess_bar_slider = false;
                    
                    if(!percentage_played)
                        restart_song = true;
                    else if(percentage_played == 1)
                        update_song = skip_song = true;
                    else
                        update_song_position = true;
                }
               
            }

            if(shuffle_play)
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
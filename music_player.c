#include "headers/raylib.h"
#include <dirent.h>
#include <libavformat/avformat.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#define LEN 256
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

// comparator function needed for qsort
int cmp(const void* a, const void* b)
{
    return strcmp((const char*)a, (const char*)b);
}

// returns the number of files in 'folder_path' of type 'file_type' stored in 'dst'
int get_files_from_folder(int maxlen, int maxfiles, char dst[maxfiles][maxlen], const char* folder_path, const char file_type)
{
    DIR* pDIR = NULL;
    struct dirent* pDirent = NULL;

    if((pDIR = opendir(folder_path)) == NULL) {
        printf("%s was not able to be opened\n", folder_path);
        return 0;
    }

    // read all non hidden files that are of a matching file_type
    int files_read = 0;
    while((pDirent = readdir(pDIR)) != NULL) {
        if(pDirent->d_name[0] != '.' && pDirent->d_type == file_type)
            strcpy(dst[files_read++], pDirent->d_name);
    }

    closedir(pDIR);
    return files_read;
}

// returns true if the path is of the passed extension
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
    char* file_extension = strrchr(file, '.');

    if(file_extension)
        file[file_extension - file] = '\0';
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

// initalizes the application
void init_app()
{
    srand(time(NULL));
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(750,750, "music player");
    InitAudioDevice();
    SetTargetFPS(120);
}

// loads mp3 file specified in full_song_path
void load_new_song(Music* music, char* full_song_path, float pitch, float volume, float pan)
{
    if(music->ctxData)
        UnloadMusicStream(*music);

    *music = LoadMusicStream(full_song_path);
    music->looping = false;
    SetMusicPitch(*music, pitch);
    SetMusicVolume(*music, volume);
    SetMusicPan(*music, pan);
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

bool pause_button(Rectangle container, bool play_music)
{
    const int BUTTON_SIZE = 20;
    const float Y_LEVEL = container.y + ((container.height - BUTTON_SIZE) / 2.00);
    const Rectangle PAUSE = (Rectangle){(GetScreenWidth() / 2.00) - (BUTTON_SIZE / 2.00), Y_LEVEL, BUTTON_SIZE, BUTTON_SIZE};
    
    // returns the opposite of the current music play state if button or space key is pressed 
    return (GuiButton(PAUSE, play_music ? "||" : "|>") || IsKeyPressed(KEY_SPACE)) ? !play_music : play_music;
}

bool skip_button(Rectangle container, bool song_over) 
{
    const int BUTTON_SIZE = 20;
    const float Y_LEVEL = container.y + ((container.height - BUTTON_SIZE) / 2.00);
    const Rectangle SKIP = (Rectangle){(GetScreenWidth() / 2.00) + BUTTON_SIZE, Y_LEVEL, BUTTON_SIZE, BUTTON_SIZE};

    // skip to next song when the song is over or when user presses 'D' 
    return (song_over) || (GuiButton(SKIP, ">") || IsKeyPressed(KEY_D));
}

void rewind_button(Rectangle container, float time_played, bool* change_song, bool* rewind_song, bool* restart_song)
{
    // the amount of time that needs to be played to restart rather than rewind
    const float THRESHOLD = 3.00;
    const int BUTTON_SIZE = 20;
    const float Y_LEVEL = container.y + ((container.height - BUTTON_SIZE) / 2.00);
    const Rectangle REWIND = (Rectangle){(GetScreenWidth() / 2.00) - (BUTTON_SIZE * 2.00), Y_LEVEL, BUTTON_SIZE, BUTTON_SIZE};

    if((GuiButton(REWIND, "<") || IsKeyPressed(KEY_A))) {
        if(time_played > THRESHOLD)
            *restart_song = true;
        else 
            *rewind_song = true;
    }
}

// slider to change position in song
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
    const Rectangle BOUNDS = {(GetScreenWidth() / 2.00) - (progress_bar_width / 2.00), container.y + (container.height * 0.75), progress_bar_width, 10};
    
    if(GuiSliderBar(BOUNDS, time_played_text, song_length_text, percentage_played, 0.00, 1.00))
        flags->holding_progess_bar_slider = true;
    
    // when the mouse is released after updating the slider is when the stream position is adjusted
    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && flags->holding_progess_bar_slider) {
        flags->holding_progess_bar_slider = false;
        
        if((*percentage_played) == 0.00)
            flags->restart_song = true;
        else if(*percentage_played == 1.00)
            flags->change_song = flags->skip_song = true;
        else
            flags->update_song_position = true;
    }
}

// sliders that adjust the music pitch, volume and pan of audio
void music_settings(Rectangle container, Music* music, float* music_pitch, float* music_volume, float* audio_pan)
{
    const float SLIDER_WIDTH = container.width / 10;;

    const Rectangle PITCH_BOUNDS = {container.width - (SLIDER_WIDTH * 1.50), container.y + 15, SLIDER_WIDTH, 10};
    const Rectangle VOLUME_BOUNDS = {container.width - (SLIDER_WIDTH * 1.50), container.y + 35, SLIDER_WIDTH, 10};
    const Rectangle PAN_BOUNDS = {container.width - (SLIDER_WIDTH * 1.50), container.y + 55, SLIDER_WIDTH, 10};

    // adusting music parameters
    if(GuiSliderBar(PITCH_BOUNDS, "PITCH", "", music_pitch, 0.875, 1.125))
        SetMusicPitch(*music, *music_pitch);

    if(GuiSliderBar(VOLUME_BOUNDS, "VOLUME", "", music_volume, 0, 1))
        SetMusicVolume(*music, *music_volume);

    if(GuiSliderBar(PAN_BOUNDS, "PAN", "", audio_pan, 0, 1))
        SetMusicPan(*music, *audio_pan);
}

// reverses the content of a string of length 'len'
void reverse_string(int len, char string[])
{
    for (int i = 0; i < len / 2; i++) {
        char temp = string[i];
        string[i] = string[len - i - 1];
        string[len - i - 1] = temp;
    }
}

// returns the string equivalent of an integer
char* itoa(int value, int maxlen, char string[])
{
    bool neagtive_number = value < 0;
    value = abs(value);

    int i = 0;
    if(!value)
        string[i++] = 0;

    else {
        for(; value && i < maxlen - 1 - neagtive_number; i++, value /= 10)
            string[i] = (value % 10) + '0';
    }

    if(neagtive_number)
        string[i++] = '-';

    string[i] = '\0';
    reverse_string(i, string);
    return string;
}

int main()
{   
    // pointers to current playlist and song
    int song_index, playlist_index;
    playlist_index = song_index = 3;

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

    if((playlists_read = get_files_from_folder(LEN, NPLAYLIST, playlists, PLAYLIST_DIR, DT_DIR)) == 0) {
        printf("there are no folders to represent playlists in \"%s\", please add some\n", PLAYLIST_DIR);
        return 1;
    }

    // updating current playlist
    strcpy(current_playlist, playlists[playlist_index]);
    sprintf(current_playlist_path, "%s/%s", PLAYLIST_DIR, current_playlist);

    // getting all mp3s from the current playlist
    int potential_songs = get_files_from_folder(LEN, NSONGS, songs, current_playlist_path, DT_REG);

    if((songs_read = filter_library(LEN, potential_songs, songs, ".mp3")) == 0) {
        printf("there are no mp3s in the playlist \"%s\", please add some\n", current_playlist_path);
        return 1;
    }

    // updating current song
    strcpy(current_song, songs[song_index]);
    
    // updating full song path
    sprintf(full_song_path, "%s/%s", current_playlist_path, current_song);
    
    if(extract_metadata(LEN, artist_name, full_song_path, "artist") < 0)    
        artist_name[0] = '\0';

    if(extract_metadata(LEN, song_title, full_song_path, "title") < 0) {
        strcpy(song_title, current_song);
        remove_extension(song_title);
    }

    init_app();

    // music control flags
    Flags flags = {false}; 

    // the speed of the music being played
    float percentage_played = 0.00;
    float pitch = 1.00;
    float volume = 0.50;
    float pan = 0.50;

    Music music = {0};
    load_new_song(&music, full_song_path, pitch, volume, pan);
    PlayMusicStream(music);

    Texture2D song_image = {0};
    update_cover_path((LEN * 2), song_cover_path, current_song);
    update_display_image(&song_image, song_cover_path, full_song_path, IMG_SIZE);

    // the bounds that each song will be stored in
    Rectangle content_rectangles[NSONGS];

    // setting the inital positions
    for(int i = 0, y_level = 0; i < songs_read; i++, y_level += 30) 
        content_rectangles[i] = (Rectangle){0, y_level, GetScreenWidth(), 30};
    
    // bounds of scroll panel window and content
    Rectangle panelRec;
    Rectangle panelContentRec = (Rectangle){0, 0, GetScreenWidth(), (content_rectangles->height * songs_read)};

    Rectangle panelView = {0, 0};
    Vector2 panelScroll = {99, -20};

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

        bool update_song = flags.skip_song || flags.rewind_song || flags.change_song;

        if(update_song) { 
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
            
            if(extract_metadata(LEN, artist_name, full_song_path, "artist") < 0)    
                artist_name[0] = '\0';

            if(extract_metadata(LEN, song_title, full_song_path, "title") < 0) {
                strcpy(song_title, current_song);
                remove_extension(song_title);
            }

            // updating display image
            update_cover_path((LEN * 3), song_cover_path, current_song);
            update_display_image(&song_image, song_cover_path, full_song_path, IMG_SIZE);

            // playing new song
            load_new_song(&music, full_song_path, pitch, volume, pan);
            PlayMusicStream(music);
        }

        // playing music
        if(flags.play_music)
            UpdateMusicStream(music);

        if(!flags.holding_progess_bar_slider)
            percentage_played = (GetMusicTimePlayed(music) / GetMusicTimeLength(music));

        BeginDrawing();
            ClearBackground(RAYWHITE);

            const float BAR_HEIGHT = 80.00; 
            const Rectangle BOTTOM_BAR_BOUNDS = (Rectangle) {0, GetScreenHeight() - BAR_HEIGHT, GetScreenWidth(), BAR_HEIGHT};

            // BODY
            {
                const float SCROLL_BAR_WIDTH = 12;

                panelRec = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight()};
                GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

                bool scrollbar_visible = panelContentRec.height > GetScreenHeight();
                
                char buffer[LEN];
                for(int i = 0; i < songs_read; i++) {
                    // the updated bounds of the rectangle from scrolling 
                    Rectangle adjusted_content_rectangle = content_rectangles[i];
                    
                    // only y and width are adjusted each frame
                    adjusted_content_rectangle.y += panelScroll.y;
                    adjusted_content_rectangle.width = GetScreenWidth();
                    if(scrollbar_visible)
                        adjusted_content_rectangle.width -= SCROLL_BAR_WIDTH;  

                    Color color = (i == song_index) ? GREEN : BLACK;

                    if(!CheckCollisionRecs(adjusted_content_rectangle, BOTTOM_BAR_BOUNDS)) {
                        if(CheckCollisionPointRec(GetMousePosition(), adjusted_content_rectangle) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            if(i == song_index)
                                flags.restart_song = true;
                            
                            else {
                                if(flags.shuffle_play) {
                                    song_history = 0;
                                    create_song_queue(songs_read, i, shuffled_song_queue);
                                }

                                flags.change_song = true;
                                song_index = i;
                            }
                       }

                        DrawText(itoa(i + 1, LEN, buffer), 15, adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                        DrawText(songs[i], 60, adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                    }
                }
            }  

            // BOTTOM BAR
            {
                const float IMG_PADDING = (BAR_HEIGHT - IMG_SIZE) / 2.00;
            
                DrawRectanglePro(BOTTOM_BAR_BOUNDS, (Vector2){0,0}, 0.0f, LIGHTGRAY);
                
                // drawing the song image
                DrawTextureEx(song_image, (Vector2){ IMG_PADDING, (BOTTOM_BAR_BOUNDS.y + IMG_PADDING) }, 0.0f, 1.0f, RAYWHITE);
                
                // song information
                float text_starting_point = BOTTOM_BAR_BOUNDS.x + (IMG_PADDING * 2.00) + IMG_SIZE;
                DrawText(current_playlist, text_starting_point, BOTTOM_BAR_BOUNDS.y + 10, 20, BLACK);
                DrawText(song_title, text_starting_point, BOTTOM_BAR_BOUNDS.y + 35, 13, BLACK);
                DrawText(artist_name, text_starting_point, BOTTOM_BAR_BOUNDS.y + 55, 8, BLACK);

                // playback buttons 
                flags.play_music = pause_button(BOTTOM_BAR_BOUNDS, flags.play_music);
                bool song_over = (!IsMusicStreamPlaying(music) && flags.play_music);
                flags.skip_song = skip_button(BOTTOM_BAR_BOUNDS, song_over);
                rewind_button(BOTTOM_BAR_BOUNDS, GetMusicTimePlayed(music), &flags.change_song, &flags.rewind_song, &flags.restart_song);

                // progress bar
                progress_bar(BOTTOM_BAR_BOUNDS, &flags, &percentage_played, GetMusicTimePlayed(music), GetMusicTimeLength(music));

                // music control sliders
                music_settings(BOTTOM_BAR_BOUNDS, &music, &pitch, &volume, &pan);
            }

            if(flags.shuffle_play)
                DrawText("SHUFFLE", 100, 400, 15, BLACK);
            DrawFPS(0, GetScreenHeight()/2);
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

// TODO
// FRONT END
    // change displaynames to actual song titles and display artist
    // add arrow instead of number when hovering over current song
    // have button for shuffle
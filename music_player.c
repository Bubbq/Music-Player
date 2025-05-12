#include "headers/raylib.h"
#include <dirent.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#define LEN 256
#define NSONGS 100
#define NPLAYLIST 100
#define IMG_SIZE 70

const char* DEFAULT_IMG_PATH = "default.png";

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

typedef struct
{
    Texture2D cover;
    char title[LEN];
    char artist[LEN];
    char duration[LEN];
    char album[LEN];
    bool information_ready;
} SongInformation;

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

// for debugging
void print_list(int nelements, int nchars, char library[nelements][nchars])
{
    for(int i = 0; i < nelements; i++)
        printf("%d) %s\n", (i + 1), library[i]);
}

// returns true if the specified directory exists
bool directory_exists(const char *path)
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
void create_song_queue(int nsongs, int current_song_index, int queue[nsongs])
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
    // SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(750,750, "music player");
    InitAudioDevice();
    SetTargetFPS(120);
}

// loads mp3 file specified in full_song_path
void load_new_song(Music* music, char* full_song_path, float pitch, float volume, float pan)
{
    if(IsMusicReady(*music) && IsMusicStreamPlaying(*music)) {
        StopMusicStream(*music);
        UnloadMusicStream(*music);
    }

    *music = LoadMusicStream(full_song_path);

    // updating to current settings
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

// Writes the cover image of file path to dst
int extract_cover_image(AVFormatContext* format_context, const char* output_jpeg_path)
{
    // the attached picture occurs when the stream type is of 'AV_DISPOSITION_ATTACHED_PIC'
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        AVStream *stream = format_context->streams[i];
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket pkt = stream->attached_pic;
            FILE *f = fopen(output_jpeg_path, "wb");
            if (!f) 
                return -1;
            fwrite(pkt.data, 1, pkt.size, f);
            fclose(f);

            return 0;  
        }
    }

    // no picture extracted case
    printf("no image extracted from \"%s\"\n", format_context->url);
    return -1;
}

// given a songname, write the appropraite cover path to dst
int get_cover_path(int maxlen, const char* song_title, const char* cover_directory_path, char dst[maxlen])
{
    const char* JPEG = ".jpg";
    const int JPEG_LEN = strlen(JPEG); 

    sprintf(dst, "%s/%s", cover_directory_path, song_title);
    int len = strlen(dst);

    if(len + JPEG_LEN >= maxlen) {
        printf("the cover path \"%s.jpg\" is longer than the passed maxlen\n", dst);
        dst[0] = '\0';
        return -1;
    }

    else {
        strcpy((dst + len), JPEG);
        return 0;
    }
}

Texture2D get_song_cover(AVFormatContext* song_context, const char* song_title, const char* cover_directory_path, int image_size)
{
    // get the cover path and write the song cover to it
    char cover_path[LEN * 2];
    int cover_path_status = get_cover_path((LEN * 2), song_title, cover_directory_path, cover_path);
    int extract_cover_status = extract_cover_image(song_context, cover_path);

    Image image = { 0 };
    int error = (cover_path_status < 0) || (extract_cover_status < 0);

    if(error)
        image = LoadImage(DEFAULT_IMG_PATH);
    else
        image = LoadImage(cover_path);

    ImageResize(&image, image_size, image_size);
    
    Texture2D ret =  LoadTextureFromImage(image);

    if(IsImageReady(image))
        UnloadImage(image);

    return ret;
}

// write a parameter of a stream's metadata to dst
int get_metadata_parameter(AVDictionary* stream_metadata, const char* parameter, int maxlen, char dst[maxlen])
{
    AVDictionaryEntry* tag = av_dict_get(stream_metadata, parameter, NULL, AV_DICT_IGNORE_SUFFIX);

    if(tag) {
        if(strlen(tag->value) < maxlen) {
            strcpy(dst, tag->value);
            return 0;
        }

        else {
            printf("\"%s\" from the parameter \"%s\" was too large to fit in destination string\n", tag->value, parameter);
            return -1;
        }
    }

    else {
        printf("the parameter \"%s\" was not found in the stream's metadata\n", parameter);
        return -1;
    }
}

// returns the context in an audio or video file
AVFormatContext* get_format_context(const char *path)
{
    AVFormatContext *format_context = NULL;

    if (avformat_open_input(&format_context, path, NULL, NULL) < 0) {
        printf("error opening the file \"%s\"\n", path);
        return NULL;
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        printf("error finding stream info for the file \"%s\"\n", path);
        avformat_close_input(&format_context);
        return NULL;
    }

    return format_context;
}

// get all the information for a song
void get_song_metadata(SongInformation* song_information, const char* full_song_path, const char* cover_directory_path)
{
    // get the metadata from the audio/video path
    AVFormatContext* format_context = get_format_context(full_song_path);
    AVDictionary* metadata = format_context->metadata;
    
    const char* DEFAULT = "---";

    // getting artist name
    if(get_metadata_parameter(metadata, "artist", LEN, song_information->artist) < 0)
        strcpy(song_information->artist, DEFAULT);

    // getting song title
    if(get_metadata_parameter(metadata, "title", LEN, song_information->title) < 0)
        strcpy(song_information->title, DEFAULT);

    if(get_metadata_parameter(metadata, "album", LEN, song_information->album) < 0)
        strcpy(song_information->album, DEFAULT);

    // song cover
    song_information->cover = get_song_cover(format_context, song_information->title, cover_directory_path, IMG_SIZE);

    // song duration
    const float MICRO_TO_SECONDS = 0.000001;
    seconds_to_string((format_context->duration * MICRO_TO_SECONDS), song_information->duration);
    
    song_information->information_ready = true;

    avformat_close_input(&format_context);
}

int main()
{   
    SongInformation song_information[NSONGS] = { 0 };

    // pointers to current playlist and song
    int current_song_index, current_playlist_index;
    current_playlist_index = current_song_index = 1;

    // the size of both playlist and current song libraries
    int playlists_read, songs_read;
    playlists_read = songs_read = 0;

    // all playlists in PLAYLIST_DIR
    char playlists[NPLAYLIST][LEN];

    // all songs in the current playlist
    char songs[NSONGS][LEN];

    // the list of shuffled songs to be played
    int shuffled_song_queue[NSONGS];
    int song_history = 0;

    const char* HOME_DIRECTORY = getenv("HOME");

    // path to folder where covers are stored
    char cover_directory_path[LEN];
    sprintf(cover_directory_path, "%s/.cache/mp3_covers", HOME_DIRECTORY);

    // path to folder where playlists (directorys holding mp3s) are held
    char playlist_directory[LEN];
    sprintf(playlist_directory, "%s/Music", HOME_DIRECTORY);
    
    if(!directory_exists(cover_directory_path))
        make_directory(cover_directory_path);

    if((playlists_read = get_files_from_folder(LEN, NPLAYLIST, playlists, playlist_directory, DT_DIR)) == 0) {
        printf("there are no folders to represent playlists in \"%s\", please add some\n", playlist_directory);
        return 1;
    }

    qsort(playlists, playlists_read, LEN, cmp);

    // full path to the current playlist 
    char current_playlist_path[LEN * 2];
    sprintf(current_playlist_path, "%s/%s", playlist_directory, playlists[current_playlist_index]);

    // getting all mp3s from the current playlist
    int potential_songs = get_files_from_folder(LEN, NSONGS, songs, current_playlist_path, DT_REG);

    if((songs_read = filter_library(LEN, potential_songs, songs, ".mp3")) == 0) {
        printf("there are no mp3s in the playlist \"%s\", please add some\n", current_playlist_path);
        return 1;
    }

    qsort(songs, songs_read, LEN, cmp);

    // the full path of the current song
    char full_song_path[LEN * 3];
    sprintf(full_song_path, "%s/%s", current_playlist_path, songs[current_song_index]);

    init_app();

    get_song_metadata(&song_information[current_song_index], full_song_path, cover_directory_path);

    // for(int i  = 0; i < songs_read; i++) {
    //     sprintf(full_song_path, "%s/%s", current_playlist_path, songs[i]);
    //     get_song_metadata((song_information + i), full_song_path, cover_directory_path);
    // }    

    // music control flags
    Flags flags = { false }; 

    // the speed of the music being played
    float percentage_played = 0.00;
    float pitch = 1.00;
    float volume = 0.50;
    float pan = 0.50;

    Music music = { 0 };
    load_new_song(&music, full_song_path, pitch, volume, pan);
    PlayMusicStream(music);

    // the bounds that each song will be stored in
    Rectangle content_rectangles[NSONGS];

    // setting the inital positions
    for(int i = 0, y_level = 0; i < songs_read; i++, y_level += 30) 
        content_rectangles[i] = (Rectangle){ 0, y_level, GetScreenWidth(), 30 };
    
    // bounds of content
    Rectangle panelContentRec = (Rectangle){ 0, 0, GetScreenWidth(), (content_rectangles->height * songs_read) };

    Rectangle panelView = { 0, 0 };
    Vector2 panelScroll = { 99, -20 };

    while(!WindowShouldClose()) {
        // toggling shuffle mode
        if(IsKeyPressed(KEY_X)) {
            flags.shuffle_play = !flags.shuffle_play;

            if(flags.shuffle_play) {
                song_history = 0;
                create_song_queue(songs_read, current_song_index, shuffled_song_queue);
            }
        }

        if(flags.update_song_position) {
            flags.update_song_position = false;
            if(IsMusicReady(music))
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
                        while((first_song = GetRandomValue(0, (songs_read - 1))) == current_song_index)
                            ;
                        create_song_queue(songs_read, first_song, shuffled_song_queue);
                        song_history = -1;
                    }

                    current_song_index = shuffled_song_queue[++song_history];
                }

                else
                    current_song_index++;
            }

            else if(flags.rewind_song) {
                flags.rewind_song = false;

                if(flags.shuffle_play) {
                    if(song_history)
                        song_history--;
                    
                    current_song_index = shuffled_song_queue[song_history];
                }

                else
                    current_song_index--;
            }
            
            flags.play_music = true;

            // updating index and full song path
            current_song_index = adjust_index(current_song_index, songs_read);
            sprintf(full_song_path, "%s/%s", current_playlist_path, songs[current_song_index]);

            if(song_information[current_song_index].information_ready == false)
                get_song_metadata(&song_information[current_song_index], full_song_path, cover_directory_path);
            
            // playing new song
            load_new_song(&music, full_song_path, pitch, volume, pan);
            PlayMusicStream(music);
        }

        // playing music
        if(flags.play_music && IsMusicReady(music))
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
                
                bool scrollbar_visible = panelContentRec.height > GetScreenHeight();
                
                Rectangle panelRec = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight() - BAR_HEIGHT };

                GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);
                
                for(int i = 0; i < songs_read; i++) {
                    // the updated bounds of the rectangle from scrolling 
                    Rectangle adjusted_content_rectangle = content_rectangles[i];
                    
                    // only y and width are adjusted each frame
                    adjusted_content_rectangle.y += panelScroll.y;
                    adjusted_content_rectangle.width = GetScreenWidth();
                    if(scrollbar_visible)
                        adjusted_content_rectangle.width -= SCROLL_BAR_WIDTH;  

                    Color color = (i == current_song_index) ? GREEN : BLACK;

                    if(!CheckCollisionRecs(adjusted_content_rectangle, BOTTOM_BAR_BOUNDS)) {
                        if(CheckCollisionPointRec(GetMousePosition(), adjusted_content_rectangle) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            if(i == current_song_index)
                                flags.restart_song = true;
                            
                            else {
                                if(flags.shuffle_play) {
                                    song_history = 0;
                                    create_song_queue(songs_read, i, shuffled_song_queue);
                                }

                                flags.change_song = true;
                                current_song_index = i;
                            }
                       }

                        DrawText(songs[i], 15, adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                        // DrawText(song_information[i].title, (GetScreenWidth() * 0.25) - (MeasureText(song_information[i].title, 10) * 0.50), adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                        // DrawText(song_information[i].artist, (GetScreenWidth() * 0.50) - (MeasureText(song_information[i].artist, 10) * 0.50), adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                        // DrawText(song_information[i].album, (GetScreenWidth() * 0.75) - (MeasureText(song_information[i].album, 10) * 0.50), adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                        // DrawText(song_information[i].duration, adjusted_content_rectangle.width - MeasureText(song_information[i].duration, 10) - 15, adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, color);
                    }
                }
            }  

            // BOTTOM BAR
            {
                DrawRectanglePro(BOTTOM_BAR_BOUNDS, (Vector2){0,0}, 0.0f, LIGHTGRAY);
            
                // drawing song cover
                const float IMG_PADDING = (BAR_HEIGHT - IMG_SIZE) / 2.00;
                const Vector2 IMG_LOCATION = { IMG_PADDING, BOTTOM_BAR_BOUNDS.y + IMG_PADDING };
                if(IsTextureReady(song_information[current_song_index].cover))
                    DrawTextureEx(song_information[current_song_index].cover, IMG_LOCATION, 0.0f, 1.0f, RAYWHITE);
                
                // song information
                float text_starting_point = BOTTOM_BAR_BOUNDS.x + (IMG_PADDING * 2.00) + IMG_SIZE;
                DrawText(playlists[current_playlist_index], text_starting_point, BOTTOM_BAR_BOUNDS.y + 10, 20, BLACK);
                DrawText(song_information[current_song_index].title, text_starting_point, BOTTOM_BAR_BOUNDS.y + 35, 13, BLACK);
                DrawText(song_information[current_song_index].artist, text_starting_point, BOTTOM_BAR_BOUNDS.y + 55, 8, BLACK);

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

    for(int i = 0; i < NSONGS; i++)
        if((song_information[i].information_ready) && IsTextureReady(song_information[i].cover))
            UnloadTexture(song_information[i].cover);

    if(IsMusicReady(music) && IsMusicStreamPlaying(music)) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// TODO
// fix last song not showing up, only when scrollbar is there
// save music and load it
// clean new song loading process
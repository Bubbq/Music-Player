#include "headers/raylib.h"
#include "headers/file_management.h"
#include "headers/stream_management.h"
#include <dirent.h>
#include <time.h>
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
#define NSONGS 200
#define NPLAYLIST 100
#define IMG_SIZE 90
#define SCREEN_SIZE 750

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
    bool pitch_change;
    bool pan_change;
    bool volume_change;
} Flags;

typedef struct
{
    Texture2D cover;
    char relative_path[LEN];
    char title[LEN];
    char artist[LEN];
    char album[LEN];
    char duration[LEN];
    long int mod_time;
    bool information_ready;
} SongInfo;

typedef struct
{
    float pan;
    float pitch;
    float volume;
    float percent_played;
} MusicSettings;

// comparator function needed for qsort
int alphabetical_compar(const void* a, const void* b)
{
    return strcmp((const char*)a, (const char*)b);
}

int mod_time_compar(const void* a, const void* b)
{
    SongInfo* info_a = (SongInfo*)a;
    SongInfo* info_b = (SongInfo*)b;

    return info_a->mod_time < info_b->mod_time;
}

// shuffles the elements of an array
void shuffle_array(int size, int array[size])
{
    for (int i = size - 1; i >= 0; i--) {
        int temp = array[i];
        int j = rand() % (i + 1);
        
        array[i] = array[j];
        array[j] = temp;
    }
}

// a song queue for shuffle play
void create_song_queue(int nsongs, int current_song_index, int queue[nsongs])
{
    // the first song in the queue is the last song from the old one
    queue[0] = current_song_index;

    for(int i = 1; i < nsongs; i++)
        if(i <= current_song_index)
            queue[i] = i - 1;
        else
            queue[i] = i;

    shuffle_array(nsongs, queue + 1);
}

// initalizes the application
void init_app()
{
    srand(time(NULL));
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_SIZE,SCREEN_SIZE, "music player");
    InitAudioDevice();
    SetTargetFPS(60);
}

// loads mp3 file specified in full_song_path
void load_new_song(Music* music, const char* song_path)
{
    if(IsMusicReady(*music)) {
        if(IsMusicStreamPlaying(*music))
            StopMusicStream(*music);
        UnloadMusicStream(*music);
    }

    (*music) = LoadMusicStream(song_path);
}

// converts n seconds into a time string of minutes and seconds 
void seconds_to_string(int seconds, int size, char string[size])
{
    // 2 characters for min and seconds, and 1 for ':'
    int min_length = 5;

    if(size < min_length) {
        printf("the string is not long enough\n");
        return;
    }

    int minutes = seconds / 60;
    seconds %= 60;

    sprintf(string, "%2d:%02d", minutes, seconds);
}

void pause_button(Rectangle container, bool* play_music)
{
    const int SIZE = 20;
    const char* TEXT = (*play_music) ? "||" : "|>";
    const float Y_LEVEL = container.y + ((container.height - SIZE) / 2.0f);
    const Rectangle BOUNDS = (Rectangle){ (GetScreenWidth() / 2.0f) - (SIZE / 2.0f), Y_LEVEL, SIZE, SIZE };
    
    if((GuiButton(BOUNDS, TEXT)) || (IsKeyPressed(KEY_SPACE)))
        (*play_music) = !(*play_music); 
}

void skip_button(Rectangle container, bool* skip_song, bool song_over) 
{
    const int SIZE = 20;
    const char* TEXT = ">";
    const float Y_LEVEL = container.y + ((container.height - SIZE) / 2.00);
    const Rectangle BOUNDS = (Rectangle){ (GetScreenWidth() / 2.00) + SIZE, Y_LEVEL, SIZE, SIZE };

    if((song_over) || (GuiButton(BOUNDS, TEXT) || IsKeyPressed(KEY_D)))
        (*skip_song) = true;
}

void rewind_button(Rectangle container, float time_played, bool* rewind_song, bool* restart_song)
{
    // the amount of time that needs to be played to restart rather than rewind
    const float THRESHOLD = 3.0f;

    const int SIZE = 20;
    const char* TEXT = "<";
    const float Y_LEVEL = container.y + ((container.height - SIZE) / 2.00);
    const Rectangle BOUNDS = (Rectangle){ (GetScreenWidth() / 2.00) - (SIZE * 2.00), Y_LEVEL, SIZE, SIZE };

    if((GuiButton(BOUNDS, TEXT)) || (IsKeyPressed(KEY_A))) {
        if(time_played >= THRESHOLD)
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
    seconds_to_string(time_played, LEN, time_played_text);
    
    char song_length_text[LEN];
    seconds_to_string(song_length, LEN, song_length_text);

    const float WIDTH = GetScreenWidth() * 0.40f;
    const Rectangle BOUNDS = { (GetScreenWidth() / 2.0f) - (WIDTH / 2.0f), container.y + (container.height * 0.75f), WIDTH, 10 };
    
    if(GuiSliderBar(BOUNDS, time_played_text, song_length_text, percentage_played, 0.0f, 1.0f))
        flags->holding_progess_bar_slider = true;
    
    if((IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) && (flags->holding_progess_bar_slider)) {
        flags->holding_progess_bar_slider = false;
        
        if((*percentage_played) == 0.00)
            flags->restart_song = true;

        else if(*percentage_played == 1.00)
            flags->skip_song = true;

        else
            flags->update_song_position = true;
    }
}

bool pitch_slider(Rectangle container, float* pitch)
{
    const char* TEXT = "PITCH";
    const Vector2 RANGE = { 0.75f, 1.25f };
    const float WIDTH = container.width / 10;
    const Rectangle BOUNDS = { container.width - (WIDTH * 1.25f), (container.y + 15), WIDTH, 10 };

    return GuiSliderBar(BOUNDS, TEXT, "", pitch, RANGE.x, RANGE.y);
}

bool volume_slider(Rectangle container, float* volume)
{
    const char* TEXT = "VOLUME";
    const Vector2 RANGE = { 0.0f, 1.0f };
    const float WIDTH = container.width / 10;
    const Rectangle BOUNDS = { container.width - (WIDTH * 1.25f), (container.y + 35), WIDTH, 10 };

    return GuiSliderBar(BOUNDS, TEXT, "", volume, RANGE.x,RANGE.y);
}

bool pan_slider(Rectangle container, float* pan)
{
    const char* TEXT = "PAN";
    const Vector2 RANGE = { 0.0f, 1.0f };
    const float WIDTH = container.width / 10;
    const Rectangle BOUNDS = { container.width - (WIDTH * 1.25), (container.y + 55), WIDTH, 10 };

    return GuiSliderBar(BOUNDS, TEXT, "", pan, RANGE.x, RANGE.y);
}

// given a songname, write the appropraite cover path to dst
int get_cover_path(int maxlen, char dst[maxlen], const char* song_title, const char* cover_directory_path)
{
    // invalid dest passed
    if(dst == NULL) {
        printf("destination string is null\n");
        return -1;
    }

    int len = strlen(cover_directory_path) + strlen(song_title) + strlen(".jpg");
    if(len >= maxlen) {
        printf("the created coverpath is longer than the passed maxlen\n");
        dst[0] = '\0';
        return -1;
    } 

    else {
        sprintf(dst, "%s/%s.jpg", cover_directory_path, song_title);
        return 0;
    }
}

Texture2D get_song_cover(const char* song_path, const char* cover_directory_path, int image_size)
{
    const char* DEFAULT_IMG_PATH = "default.png";
    Image image = { 0 };

    // getting the cover path
    char cover_path[LEN * 2];
    if(get_cover_path((LEN * 2), cover_path, GetFileNameWithoutExt(song_path), cover_directory_path) < 0)
        image = LoadImage(DEFAULT_IMG_PATH); 

    // extract the cover image if the file doesnt already exist
    if(!FileExists(cover_path)) {
        if(extract_cover_image(song_path, cover_path) < 0)
            image = LoadImage(DEFAULT_IMG_PATH);
        else
            image = LoadImage(cover_path);
    }
    else
        image = LoadImage(cover_path);

    // if at any point the cover_path is invalid, load the default image
    if(!IsImageReady(image))
        image = LoadImage(DEFAULT_IMG_PATH);

    ImageResize(&image, image_size, image_size);
    
    Texture2D ret = LoadTextureFromImage(image);
    
    if(IsImageReady(image))
        UnloadImage(image);
    return ret;
}

// returns the time in seconds since last modification of a file
time_t modification_time(const char* path)
{
    struct stat st;
    if(stat(path, &st) == 0)
        return st.st_mtim.tv_sec;
    else
        return -1;
}

// get all the information for a song
void get_information(SongInfo* information, const char* full_song_path)
{
    // get the metadata from the audio/video path
    AVFormatContext* format_context;
    if((format_context = get_format_context(full_song_path)) == NULL) {
        printf("was not able to get song information for \"%s\"\n", full_song_path);
        return;
    }

    AVDictionary* metadata = format_context->metadata;

    // basic song information
    const char* DEFAULT = "---";
    if(get_metadata_parameter(metadata, "artist", LEN, information->artist) < 0)  strcpy(information->artist, DEFAULT);
    if(get_metadata_parameter(metadata, "title", LEN, information->title) < 0) strcpy(information->title, DEFAULT);
    if(get_metadata_parameter(metadata, "album", LEN, information->album) < 0) strcpy(information->album, DEFAULT);
    
    // song duration
    const float MICRO_TO_SECONDS = 0.000001;
    seconds_to_string((format_context->duration * MICRO_TO_SECONDS), LEN, information->duration);
    
    // modification time
    information->mod_time = modification_time(full_song_path);

    information->information_ready = true;
    avformat_close_input(&format_context);
}

void save_last_song(int last_playlist_index, int last_song_index, const char* save_path)
{
    FILE* pFILE = fopen(save_path, "w");
    if(!pFILE)
        printf("error trying to save song\n");
    else {
        fprintf(pFILE, "%d,%d\n", last_playlist_index, last_song_index);
        fclose(pFILE);
    } 
}

void load_last_song(const char* load_path, int* playlist_index, int* song_index)
{
    FILE* pFILE = fopen(load_path, "r");
    if(pFILE == NULL) {
        printf("error trying to load last song");
        return;
    }

    char* line = NULL;
    size_t len = 0;
    if(getline(&line, &len, pFILE) == 0) {
        printf("no characters read\n");
        return;
    }

    const char* DELIM = ",";
    (*playlist_index) = atoi(strtok(line, DELIM));
    (*song_index) = atoi(strtok(NULL, DELIM));
}

int load_playlists(int nelements, int len, char playlists[nelements][len], const char* playlist_directory)
{
    int nplaylists;
    if((nplaylists = get_files_from_folder(LEN, NPLAYLIST, playlists, playlist_directory, DT_DIR)) == 0) 
        printf("there are no folders to represent playlists in \"%s\", please add some\n", playlist_directory);

    qsort(playlists, nplaylists, LEN, alphabetical_compar);
    return nplaylists;
}

// getting all mp3s from the current playlist
int load_songs(int nelements, int len, char songs[nelements][len], const char* playlist_path)
{
    int potential_songs = get_files_from_folder(LEN, NSONGS, songs, playlist_path, DT_REG);

    int nsongs;
    if((nsongs = filter_library(LEN, potential_songs, songs, ".mp3")) == 0) 
        printf("there are no mp3s in the initial playlist \"%s\", please add some\n", playlist_path);

    return nsongs;
}

int main()
{   
    const char* HOME_DIRECTORY = getenv("HOME");
    
    // path to folder where covers are stored
    char cover_directory_path[LEN]; 

    // path to folder where playlists (directorys holding mp3s) are held
    char playlist_directory[LEN]; 

    sprintf(playlist_directory, "%s/Music", HOME_DIRECTORY);
    sprintf(cover_directory_path, "%s/.cache/mp3_covers", HOME_DIRECTORY);

    // a list of all playlists paths relative to 'playlist_directory'
    char playlists[NPLAYLIST][LEN];

    // relavent information of every song in the current playlist
    SongInfo song_information[NSONGS] = { 0 };
    
    // pointers to current playlist and song
    int current_song_index, current_playlist_index;
    current_playlist_index = current_song_index = 0;

    // the current size of these lists
    int nplaylists, nsongs;
    nplaylists = nsongs = 0;

    // the list of shuffled songs to be played
    int shuffled_song_queue[NSONGS];
    int song_history = 0;

    // create the 'cover_directory' if it does not exist
    if(!DirectoryExists(cover_directory_path))
        make_directory(cover_directory_path);

    // loading last song from previous session
    const char* SAVE_PATH = "last_song.txt";
    if(FileExists(SAVE_PATH)) 
        load_last_song(SAVE_PATH, &current_playlist_index, &current_song_index);

    // loading all playlists
    if((nplaylists = load_playlists(NPLAYLIST, LEN, playlists, playlist_directory)) == 0)
        return 1;

    // full path to the current playlist 
    char playlist_path[LEN * 2];
    sprintf(playlist_path, "%s/%s", playlist_directory, playlists[current_playlist_index]);

    // getting the relative path of every song in the current playlist
    char relative_paths[NSONGS][LEN];
    if((nsongs = load_songs(NSONGS, LEN, relative_paths, playlist_path)) == 0)
        return 1;

    // load all song information for the current playlist
    for(int i  = 0; i < nsongs; i++) {
        char buffer[LEN * 3];
        sprintf(buffer, "%s/%s", playlist_path, relative_paths[i]);
        strcpy(song_information[i].relative_path, relative_paths[i]);
        get_information((song_information + i), buffer);
    }    

    // sort the songs s.t. the newest added songs are first and oldest last
    qsort(song_information, nsongs, sizeof(SongInfo), mod_time_compar);

    // the full path of the current song
    char full_song_path[LEN * 3];
    sprintf(full_song_path, "%s/%s/%s", playlist_directory, playlists[current_playlist_index], song_information[current_song_index].relative_path);

    init_app();

    song_information[current_song_index].cover = get_song_cover(full_song_path, cover_directory_path, IMG_SIZE);

    // music control flags
    Flags update_flags = { false }; 

    // music settings: 
    MusicSettings settings;

    settings.pan = 0.50f;
    settings.pitch = 1.0f;
    settings.volume = 0.50f;
    settings.percent_played = 0.0f;

    Music music = { 0 };

    load_new_song(&music, full_song_path);
    music.looping = false;

    PlayMusicStream(music);

    // the bounds that each song will be stored in
    Rectangle content_rectangles[NSONGS];

    // setting content positions
    const float CONTENT_HEIGHT = 30;
    for(int i = 0, y_level = CONTENT_HEIGHT; i < nsongs; i++, y_level += CONTENT_HEIGHT) 
        content_rectangles[i] = (Rectangle){ 0, y_level, GetScreenWidth(), CONTENT_HEIGHT };
    
    // bounds of content as a whole
    Rectangle panelContentRec = (Rectangle){ 0, CONTENT_HEIGHT, GetScreenWidth(), (content_rectangles->height * (nsongs + 1)) };

    Rectangle panelView = { 0, 0 };
    Vector2 panelScroll = { 99, -100 };

    while(!WindowShouldClose()) {

        // toggling shuffle mode
        if(IsKeyPressed(KEY_X)) {
            update_flags.shuffle_play = !update_flags.shuffle_play;

            if(update_flags.shuffle_play) {
                song_history = 0;
                create_song_queue(nsongs, current_song_index, shuffled_song_queue);
            }
        }

        if(IsMusicReady(music)) {
            // playing music
            if(update_flags.play_music)
                UpdateMusicStream(music);

            if(update_flags.pitch_change) {
                update_flags.pitch_change = false;
                SetMusicPitch(music, settings.pitch);
            }

            else if(update_flags.volume_change) {
                update_flags.volume_change = false;
                SetMusicVolume(music, settings.volume);
            }
            
            else if(update_flags.pan_change) {
                update_flags.pan_change = false;
                SetMusicPan(music, settings.pan);
            }

            // reset music settings
            if(IsKeyPressed(KEY_R)) {
                settings.pan = 0.50f;
                settings.pitch = 1.0f;
                settings.volume = 0.50f;

                SetMusicPitch(music, settings.pitch);
                SetMusicVolume(music, settings.volume);
                SetMusicPan(music, settings.pan);
            }

            // changing position in song
            if(update_flags.update_song_position) {
                update_flags.update_song_position = false;
                SeekMusicStream(music, GetMusicTimeLength(music) * settings.percent_played);
            }

            // restarting current music stream
            if(update_flags.restart_song) {
                update_flags.restart_song = false;
                StopMusicStream(music);
                PlayMusicStream(music);
            }

            bool update_current_song = (update_flags.skip_song || update_flags.rewind_song || update_flags.change_song);

            if(update_current_song) { 
                
                if(update_flags.skip_song) {
                    update_flags.skip_song = false;

                    if(update_flags.shuffle_play) {
                        if(song_history >= nsongs - 1) { 
                            // create a new queue s.t. the first song is not the last song from the previous 
                            int first_song;
                            while((first_song = GetRandomValue(0, (nsongs - 1))) == current_song_index)
                                ;
                            create_song_queue(nsongs, first_song, shuffled_song_queue);
                            song_history = -1;
                        }

                        current_song_index = shuffled_song_queue[++song_history];
                    }

                    else
                        current_song_index++;
                }

                else if(update_flags.rewind_song) {
                    update_flags.rewind_song = false;

                    // load the previous song in the queue
                    if(update_flags.shuffle_play) {
                        if(song_history)
                            song_history--;
                        
                        current_song_index = shuffled_song_queue[song_history];
                    }

                    else
                        current_song_index--;
                }
                
                else if(update_flags.change_song)
                    update_flags.change_song = false;

                update_flags.play_music = true;

                // updating index and full song path
                current_song_index = (current_song_index + nsongs) % nsongs;
                sprintf(full_song_path, "%s/%s/%s", playlist_directory, playlists[current_playlist_index], song_information[current_song_index].relative_path);

                // load display image if not loaded before
                if(!IsTextureReady(song_information[current_song_index].cover))
                    song_information[current_song_index].cover = get_song_cover(full_song_path, cover_directory_path, IMG_SIZE);

                // loading new music stream
                load_new_song(&music, full_song_path);

                // set the settings
                music.looping = false;
                SetMusicPan(music, settings.pan);
                SetMusicPitch(music, settings.pitch);
                SetMusicVolume(music, settings.volume);

                PlayMusicStream(music);
            }
        }

        if(!update_flags.holding_progess_bar_slider)
            settings.percent_played = (GetMusicTimePlayed(music) / GetMusicTimeLength(music));
        
        BeginDrawing(); 
            ClearBackground(RAYWHITE);
            const float BOTTOM_BAR_HEIGHT = 100.00; 
            
            const float SCROLL_BAR_WIDTH = 13;
            bool vertical_scrollbar_visible = panelContentRec.height > GetScreenHeight() - BOTTOM_BAR_HEIGHT;
            bool horizontal_scrollbar_visible = (GetScreenWidth() <= SCREEN_SIZE + SCROLL_BAR_WIDTH);
            
            const Rectangle BOTTOM_BAR_BOUNDS = (Rectangle) {0, GetScreenHeight() - BOTTOM_BAR_HEIGHT, GetScreenWidth(), BOTTOM_BAR_HEIGHT};
            const Rectangle TOP_BAR_BOUNDS = { 0, 0, GetScreenWidth() - (vertical_scrollbar_visible ? SCROLL_BAR_WIDTH : 0), CONTENT_HEIGHT };
            
            // BODY
            {
                const int FONT_SIZE = 12;
                const Rectangle panelRec = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight() - BOTTOM_BAR_HEIGHT + (horizontal_scrollbar_visible ? SCROLL_BAR_WIDTH : 0) };

                GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);
                
                for(int i = 0; i < nsongs; i++) {
                    // the updated bounds of the rectangle from scrolling 
                    const Rectangle adjusted_content_rectangle = { content_rectangles[i].x, (content_rectangles[i].y + panelScroll.y), 
                                                GetScreenWidth() - (vertical_scrollbar_visible ? SCROLL_BAR_WIDTH : 0), content_rectangles[i].height };
                    Color text_color = BLACK;
                    
                    bool top_bar_collision = CheckCollisionRecs(TOP_BAR_BOUNDS, adjusted_content_rectangle);
                    bool bottom_bar_collision = CheckCollisionRecs(BOTTOM_BAR_BOUNDS, adjusted_content_rectangle);

                    // dont draw songs that collide with other portions of the screen
                    if(!top_bar_collision && !bottom_bar_collision) {
                       
                        bool mouse_hovering = CheckCollisionPointRec(GetMousePosition(), adjusted_content_rectangle);
                        if(mouse_hovering) {
                            text_color = RED;

                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                // restart the song if you select the current one
                                if(i == current_song_index)
                                    update_flags.restart_song = true;
                                
                                else {
                                    // create a new queue every time you seek a song
                                    if(update_flags.shuffle_play) {
                                        song_history = 0;
                                        create_song_queue(nsongs, i, shuffled_song_queue);
                                    }

                                    // set the current index to index sought
                                    update_flags.change_song = true;
                                    current_song_index = i;
                                }
                            }
                        }

                        bool active_song = (i == current_song_index);
                        text_color = active_song ? GREEN : text_color;
                        const float Y_LEVEL = adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00);
                        
                        // displaying song information
                        if(song_information[i].information_ready) {
                            char buff[LEN];
                            sprintf(buff, "%3d", (i + 1));
                            DrawText(buff, 5, adjusted_content_rectangle.y + (adjusted_content_rectangle.height / 2.00), 10, text_color);
                            
                            DrawText(song_information[i].title, 30, Y_LEVEL, FONT_SIZE, text_color);        
                            DrawText(song_information[i].artist, (adjusted_content_rectangle.width * 0.30), Y_LEVEL, FONT_SIZE, text_color);
                            DrawText(song_information[i].album, (adjusted_content_rectangle.width * 0.50), Y_LEVEL, FONT_SIZE, text_color);

                            sprintf(buff, "%ld", song_information[i].mod_time);
                            DrawText(buff, (adjusted_content_rectangle.width * 0.70), Y_LEVEL, FONT_SIZE, text_color);

                            DrawText(song_information[i].duration, (adjusted_content_rectangle.width * 0.90), Y_LEVEL, FONT_SIZE, text_color);
                        }
                    }
                }
            }  

            // TOP BAR
            {
                const int FONT_SIZE = 15;
                const Color TEXT_COLOR = BLACK;
                const float Y_LEVEL = TOP_BAR_BOUNDS.y + (TOP_BAR_BOUNDS.height / 2.00);

                DrawText("#", 13, Y_LEVEL, FONT_SIZE, TEXT_COLOR);
                DrawText("TITLE", 30, Y_LEVEL, FONT_SIZE, TEXT_COLOR);
                DrawText("ARTIST", (TOP_BAR_BOUNDS.width * 0.30), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
                DrawText("ALBUM", (TOP_BAR_BOUNDS.width * 0.50), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
                DrawText("ADDED", (TOP_BAR_BOUNDS.width * 0.70), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
                DrawText("DURATION", (TOP_BAR_BOUNDS.width * 0.90), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
            }

            // BOTTOM BAR
            {
                DrawRectanglePro(BOTTOM_BAR_BOUNDS, (Vector2){0,0}, 0.0f, RAYWHITE);
            
                // drawing song cover
                const float IMG_PADDING = (BOTTOM_BAR_HEIGHT - IMG_SIZE) / 2.00;
                const Vector2 IMG_LOCATION = { IMG_PADDING, BOTTOM_BAR_BOUNDS.y + IMG_PADDING };
                const Texture2D SONG_COVER = song_information[current_song_index].cover;

                if(IsTextureReady(SONG_COVER))
                    DrawTextureEx(SONG_COVER, IMG_LOCATION, 0.0f, 1.0f, RAYWHITE);
                
                // song information
                float text_starting_point = BOTTOM_BAR_BOUNDS.x + (IMG_PADDING * 2.00) + IMG_SIZE;
                DrawText(playlists[current_playlist_index], text_starting_point, BOTTOM_BAR_BOUNDS.y + 10, 20, BLACK);
                DrawText(song_information[current_song_index].title, text_starting_point, BOTTOM_BAR_BOUNDS.y + 35, 13, BLACK);
                DrawText(song_information[current_song_index].artist, text_starting_point, BOTTOM_BAR_BOUNDS.y + 55, 8, BLACK);

                // playback buttons 
                pause_button(BOTTOM_BAR_BOUNDS, &update_flags.play_music);
                skip_button(BOTTOM_BAR_BOUNDS, &update_flags.skip_song, (!IsMusicStreamPlaying(music) && update_flags.play_music));
                rewind_button(BOTTOM_BAR_BOUNDS, GetMusicTimePlayed(music),&update_flags.rewind_song, &update_flags.restart_song);

                // bar to change music time position
                progress_bar(BOTTOM_BAR_BOUNDS, &update_flags, &settings.percent_played, GetMusicTimePlayed(music), GetMusicTimeLength(music));

                // sliders for music settings
                update_flags.pitch_change = pitch_slider(BOTTOM_BAR_BOUNDS, &settings.pitch);
                update_flags.volume_change = volume_slider(BOTTOM_BAR_BOUNDS, &settings.volume);
                update_flags.pan_change = pan_slider(BOTTOM_BAR_BOUNDS, &settings.pan);
            }

            if(update_flags.shuffle_play)
                DrawText("SHUFFLE", 100, 400, 15, BLACK);
            DrawFPS(0, GetScreenHeight()/2);
        EndDrawing();
    }

    save_last_song(current_playlist_index, current_song_index, SAVE_PATH);

    for(int i = 0; i < nsongs; i++)
        if((song_information[i].information_ready) && IsTextureReady(song_information[i].cover))
            UnloadTexture(song_information[i].cover);

    if(IsMusicReady(music)) {
        if(IsMusicStreamPlaying(music))
            StopMusicStream(music);
        UnloadMusicStream(music);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// date added column
// denote when in shuffle mode
// sort songs by every column (alpha, modtime, duration, etc.)
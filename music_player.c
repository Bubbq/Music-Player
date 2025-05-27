#include "headers/raylib.h"
#include "headers/file_watch.h"
#include "headers/file_management.h"
#include "headers/stream_management.h"

#include <sys/inotify.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <libavutil/dict.h>
#include <libavformat/avformat.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#define LEN 256
#define NSONGS 100
#define NPLAYLIST 100
#define IMG_SIZE 90
#define SCREEN_SIZE 750

typedef struct
{
    bool play_music;
    bool shuffle_play;
    bool skip_song;
    bool change_song;
    bool rewind_song;
    bool restart_song;
    bool update_song_position;
} Flags;

typedef struct
{
    Texture2D cover;
    char title[LEN]; 
    char album[LEN]; 
    char artist[LEN]; 
    char relative_path[LEN]; 
    float duration;
    float modification_time;
    bool information_ready; 
} SongInfo;

typedef struct
{
    float pan;
    float pitch;
    float volume;
    float percent_played;
} MusicSettings;


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
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_SIZE,SCREEN_SIZE, "music player");
    InitAudioDevice();
    SetTargetFPS(60);
}

// loads mp3 file specified in full_song_path
void load_new_music(Music* music, const char* song_path)
{
    if(IsMusicReady(*music)) {
        if(IsMusicStreamPlaying(*music))
            StopMusicStream(*music);
        UnloadMusicStream(*music);
    }

    (*music) = LoadMusicStream(song_path);
}

// converts n seconds into a time string of minutes and seconds 
void seconds_to_timestamp(int seconds, int size, char string[size])
{
    // 2 characters for min and seconds, and 1 for ':'
    int min_length = 5;

    if(size < min_length) {
        printf("the string is not long enough\n");
        return;
    }

    int minutes = seconds / 60;
    seconds %= 60;

    snprintf(string, size, "%2d:%02d", minutes, seconds);
}

void playback_buttons(Rectangle container, Flags* flags, int seconds_played, int song_duration)
{
    const int PLAYBACK_BUTTON_SIZE = 20;
    const float Y_LEVEL = container.y + ((container.height - PLAYBACK_BUTTON_SIZE) / 2.00);
    
    // rewind button
    const float THRESHOLD = 3.0f; // the amount of time that needs to be played to restart rather than rewind
    const Rectangle REWIND = (Rectangle){ (GetScreenWidth() / 2.00) - (PLAYBACK_BUTTON_SIZE * 2.00), Y_LEVEL, PLAYBACK_BUTTON_SIZE, PLAYBACK_BUTTON_SIZE };
    if((GuiButton(REWIND, "<")) || (IsKeyPressed(KEY_A))) {
        if(seconds_played >= THRESHOLD)
            flags->restart_song = true;
        else 
            flags->rewind_song = true;
    }

    // pause button
    const char* PAUSE_TEXT = flags->play_music ? "||" : "|>";
    const Rectangle PAUSE = (Rectangle){ (GetScreenWidth() / 2.0f) - (PLAYBACK_BUTTON_SIZE / 2.0f), Y_LEVEL, PLAYBACK_BUTTON_SIZE, PLAYBACK_BUTTON_SIZE };
    if((GuiButton(PAUSE, PAUSE_TEXT)) || (IsKeyPressed(KEY_SPACE)))
        flags->play_music = !flags->play_music;

    // skip button
    const Rectangle SKIP = (Rectangle){ (GetScreenWidth() / 2.00) + PLAYBACK_BUTTON_SIZE, Y_LEVEL, PLAYBACK_BUTTON_SIZE, PLAYBACK_BUTTON_SIZE };
    const bool song_over = (seconds_played == song_duration);
    if((song_over) || (GuiButton(SKIP, ">") || IsKeyPressed(KEY_D)))
        flags->skip_song = true;

}

// slider to change position in song
void progress_bar(Rectangle container, Flags* flags, bool* progress_bar_active, float* percentage_played, int time_played, int song_length)
{
    // update the slider strings when dragging the bar around
    if((*progress_bar_active))
        time_played = song_length * (*percentage_played);

    // converting current and total song time into strings in 00:00 format
    char time_played_text[LEN];
    seconds_to_timestamp(time_played, LEN, time_played_text);
    
    char song_length_text[LEN];
    seconds_to_timestamp(song_length, LEN, song_length_text);

    const float WIDTH = GetScreenWidth() * 0.40f;
    const Rectangle BOUNDS = { (GetScreenWidth() / 2.0f) - (WIDTH / 2.0f), container.y + (container.height * 0.75f), WIDTH, 10 };
    
    if(GuiSliderBar(BOUNDS, time_played_text, song_length_text, percentage_played, 0.0f, 1.0f))
        (*progress_bar_active) = true;
    
    if((IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) && ((*progress_bar_active))) {
        (*progress_bar_active) = false;
        
        if((*percentage_played) == 0.0f)
            flags->restart_song = true;

        else if(*percentage_played == 1.0f)
            flags->skip_song = true;

        else
            flags->update_song_position = true;
    }
}

void music_settings(Rectangle container, MusicSettings* settings, Music* music)
{
    const float SLIDER_WIDTH = container.width / 10.0f;

    // pitch 
    const Vector2 PITCH_RANGE = { 0.75f, 1.25f };
    const Rectangle PITCH = { container.width - (SLIDER_WIDTH * 1.25f), (container.y + 15), SLIDER_WIDTH, 10 };
    if(GuiSliderBar(PITCH, "PITCH", "", &settings->pitch, PITCH_RANGE.x, PITCH_RANGE.y))
        SetMusicPitch((*music), settings->pitch);

    // volume
    const Vector2 VOLUME_RANGE = { 0.0f, 1.0f };
    const Rectangle VOLUME = { container.width - (SLIDER_WIDTH * 1.25f), (container.y + 40), SLIDER_WIDTH, 10 };
    if(GuiSliderBar(VOLUME, "VOLUME", "", &settings->volume, VOLUME_RANGE.x, VOLUME_RANGE.y))
        SetMusicVolume((*music), settings->volume);

    // pan
    const Vector2 PAN_RANGE = { 0.0f, 1.0f };
    const Rectangle PAN = { container.width - (SLIDER_WIDTH * 1.25), (container.y + 65), SLIDER_WIDTH, 10 };
    if(GuiSlider(PAN, "PAN", "", &settings->pan, PAN_RANGE.x, PAN_RANGE.y))
        SetMusicPan((*music), settings->pan);
}

// given a songname, write the appropraite cover path to dst
int get_cover_path(int size, char dst[size], const char* song_title, const char* cover_directory_path)
{
    int len = strlen(cover_directory_path) + strlen(song_title) + strlen(".jpg");
    if(len >= size) {
        printf("the created coverpath is longer than the passed size\n");
        dst[0] = '\0';
        return -1;
    } 

    else {
        snprintf(dst, size, "%s/%s.jpg", cover_directory_path, song_title);
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
float file_modification_time(const char* file)
{
    struct stat st;
    if(stat(file, &st) == 0) 
        return difftime(time(NULL), st.st_mtim.tv_sec);
    else
        return -1;
}

// convert time in seconds since file has been altered to string form
void seconds_to_last_modified(long int seconds, int maxlen, char dst[maxlen])
{
    int days = seconds / 86400;
    seconds -= (days * 86400);
    
    int years = days / 365;
    int months = days / 30;
    int weeks = days / 7;

    int hours = seconds / 3600;
    seconds -= (hours * 3600);
    
    int minutes = seconds / 60;
    seconds -= (minutes * 60);

    if(years)
        snprintf(dst, maxlen, "%d year%s ago", years, (years > 1 ? "s" : ""));
    else if(months)
        snprintf(dst, maxlen, "%d month%s ago", months, (months > 1 ? "s" : ""));
    else if(weeks)
        snprintf(dst, maxlen, "%d week%s ago", weeks, (weeks > 1 ? "s" : ""));
    else if(days)
        snprintf(dst, maxlen, "%d day%s ago", days, (days > 1 ? "s" : ""));
    else if(hours)
        snprintf(dst, maxlen, "%d hour%s ago", hours, (hours > 1 ? "s" : ""));
    else if(minutes)
        snprintf(dst, maxlen, "%d minute%s ago", minutes, (minutes > 1 ? "s" : ""));
    else
        snprintf(dst, maxlen, "%ld seconds ago", seconds);
}

float microseconds_to_seconds(float microseconds)
{
    return microseconds * 0.000001;
}

// get all the information for a song
void get_information(SongInfo* information, const char* full_song_path)
{
    // get the metadata from the audio/video path
    AVFormatContext* format_context;
    if((format_context = get_format_context(full_song_path)) == NULL) 
        return;

    AVDictionary* metadata = format_context->metadata;

    // basic song information
    const char* DEFAULT = "---";
    if(get_metadata_parameter(metadata, "artist", LEN, information->artist) < 0) 
        strcpy(information->artist, DEFAULT);
    if(get_metadata_parameter(metadata, "title", LEN, information->title) < 0) 
        strcpy(information->title, DEFAULT);
    if(get_metadata_parameter(metadata, "album", LEN, information->album) < 0) 
        strcpy(information->album, DEFAULT);
    
    // song duration
    information->duration = microseconds_to_seconds(format_context->duration);

    // modification time
    information->modification_time = file_modification_time(full_song_path);

    information->information_ready = true;

    avformat_close_input(&format_context);
}

// comparator for qsort
int by_modification_time(const void* a, const void* b)
{
    SongInfo* info_a = (SongInfo*)a;
    SongInfo* info_b = (SongInfo*)b;
    return info_a->modification_time > info_b->modification_time;
}

void draw_top_bar(Rectangle container) 
{
    const int FONT_SIZE = 15;
    const Color TEXT_COLOR = BLACK;
    const float Y_LEVEL = container.y + (container.height / 2.0f);

    DrawText("#", 13, Y_LEVEL, FONT_SIZE, TEXT_COLOR);
    DrawText("TITLE", 30, Y_LEVEL, FONT_SIZE, TEXT_COLOR);
    DrawText("ARTIST", (container.width * 0.30f), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
    DrawText("ALBUM", (container.width * 0.50f), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
    DrawText("ADDED", (container.width * 0.70f), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
    DrawText("DURATION", (container.width * 0.90f), Y_LEVEL, FONT_SIZE, TEXT_COLOR);
}

void set_current_song(const char* playlist_path, const char* cover_directory_path, SongInfo* song_information, Music* music, MusicSettings settings)
{
    char full_song_path[LEN * 3];
    snprintf(full_song_path, (LEN * 3), "%s/%s", playlist_path, song_information->relative_path);

    // load display image if not loaded before
    if(!IsTextureReady(song_information->cover))
        song_information->cover = get_song_cover(full_song_path, cover_directory_path, IMG_SIZE);

    // loading new music stream 
    load_new_music(music, full_song_path);

    // music defaults to looping 
    music->looping = false;

    // apply the current settings
    SetMusicPan((*music), settings.pan);
    SetMusicPitch((*music), settings.pitch);
    SetMusicVolume((*music), settings.volume);
}

int load_songs_from_playlist(SongInfo song_information[NSONGS], const char* playlist_path)
{
    // all files in the playlist path
    char relative_paths[NSONGS][LEN];
    int potential_songs = get_files_from_folder(LEN, NSONGS, relative_paths, playlist_path, DT_REG);

    // filter out any non mp3s
    int nsongs = filter_library(LEN, potential_songs, relative_paths, ".mp3");

    // get information of every song
    for(int i = 0; i < nsongs; i++) {
        char buffer[LEN * 3];
        snprintf(buffer, (LEN * 3), "%s/%s", playlist_path, relative_paths[i]);

        strcpy(song_information[i].relative_path, relative_paths[i]);
        get_information((song_information + i), buffer);
    }    

    // sort songs s.t. the newest one is first 
    qsort(song_information, nsongs, sizeof(SongInfo), by_modification_time);

    return nsongs;
}

void delete_song_information(SongInfo* song_information)
{
    if(IsTextureReady(song_information->cover))
        UnloadTexture(song_information->cover);
    (*song_information) = (SongInfo){ 0 };
}

void unload_song_information(int nsongs, SongInfo song_information[nsongs])
{
    for(int i = 0; i < nsongs; i++)
        delete_song_information(song_information + i);
}

int main()
{   
    const char* HOME_DIRECTORY = getenv("HOME");
    
    // path to folder where covers are held
    char cover_directory_path[LEN]; 
    snprintf(cover_directory_path, LEN, "%s/.cache/song_covers", HOME_DIRECTORY);

    // path to folder where playlists (directorys holding mp3s) are held
    char playlist_directory[LEN]; 
    snprintf(playlist_directory, LEN, "%s/Music", HOME_DIRECTORY);

    // a list of relative playlists paths
    char playlists[NPLAYLIST][LEN];

    // pointers to current playlist and song
    int current_song_index, current_playlist_index;
    current_playlist_index = current_song_index = 0;
    
    // full path to the current playlist 
    char playlist_path[LEN * 2];
    snprintf(playlist_path, (LEN * 2), "%s/%s", playlist_directory, playlists[current_playlist_index]);

    // the list of shuffled songs to be played
    int shuffled_song_queue[NSONGS];
    int song_history = 0; // user's position in this queue

    // relavent information of every song in the current playlist
    SongInfo song_information[NSONGS] = { 0 };
    
    // application flags
    bool show_playlist_window = false;
    bool change_playlist = false;
    bool progress_bar_active = false;
    bool no_playlists_error = false;
    bool no_songs_in_playlist_error = false;
    
    // flags that control the flow of music
    Flags update_flags = { false }; 

    // settings
    MusicSettings settings;
    settings.pan = 0.50f;
    settings.pitch = 1.0f;
    settings.volume = 0.50f;
    settings.percent_played = 0.0f;

    Music music = { 0 };

    // creating cover directory
    if(!DirectoryExists(cover_directory_path))
        make_directory(cover_directory_path);

    init_app();

    // loading all playlists
    int nplaylists;
    nplaylists = get_files_from_folder(LEN, NPLAYLIST, playlists, playlist_directory, DT_DIR);
    no_playlists_error = (nplaylists == 0);

    // updating playlist path
    snprintf(playlist_path, (LEN * 2), "%s/%s", playlist_directory, playlists[current_playlist_index]);
    
    FileWatch file_watch;
    const uint32_t SIGNAL  = (IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    init_file_watch(&file_watch, playlist_path, SIGNAL);

    // loading all songs in current playlist
    int nsongs;
    nsongs = load_songs_from_playlist(song_information, playlist_path);
    no_songs_in_playlist_error = (nsongs == 0);

    // load the cover and music stream of the current song
    if(nsongs > 0) {
        set_current_song(playlist_path, cover_directory_path, (song_information + current_song_index), &music, settings);
        PlayMusicStream(music);
    }

    // inital position of scrollbar
    Vector2 panelScroll = { 99, 0 };
    Rectangle panelView = { 10, 10 };

    while(!WindowShouldClose()) {
        // when the current playlist is modified
        bool updating = file_watch.updating;
        file_watch.updating = file_event(&file_watch);
        const bool done_reading = (updating && !file_watch.updating);
        
        if(done_reading) {
            for(int e = 0; e < file_watch.nevents; e++) {
                FileEvent file_event = file_watch.events[e];
                
                printf("%d)", (e + 1));
                print_inotify_event((&file_event.event), file_event.file_name);
                
                // external song deletion
                if(file_event.event.mask == 64) {
                    int del_index = -1;
                    for(int i = 0; i < nsongs; i++) {
                        if(strcmp(file_event.file_name, song_information[i].relative_path) == 0) {
                            del_index = i;
                            break;
                        }
                    }

                    if(del_index >= 0) {
                        // clear content of deleted song
                        if(song_information[del_index].information_ready) 
                            delete_song_information((song_information + del_index));

                        // shift all elements accordingly
                        for(int i = del_index; i < nsongs; i++) 
                            song_information[i] = song_information[i + 1];

                        nsongs--;

                        if(nsongs > 0) {
                            // index adj
                            if(del_index < current_song_index)
                                current_song_index--;

                            else if(del_index == current_song_index) {
                                current_song_index = 0;
                                update_flags.change_song = true;
                            }
                        }
                        else {
                            update_flags = (Flags) { false };
                            no_songs_in_playlist_error = true;
                            break;
                        }
                    }
                }
            }

            file_watch.nevents = 0;
        }
        
        // toggle shuffle mode
        if(IsKeyPressed(KEY_S) && (nsongs > 1)) {
            update_flags.shuffle_play = !update_flags.shuffle_play;

            if(update_flags.shuffle_play) {
                song_history = 0;
                create_song_queue(nsongs, current_song_index, shuffled_song_queue);
            }
        }

        else if(IsKeyPressed(KEY_P))
            show_playlist_window = !show_playlist_window;

        if(change_playlist) {
            change_playlist = false;

            // free information of previous playlist
            unload_song_information(nsongs, song_information);

            // new playlist path
            snprintf(playlist_path, (LEN * 2), "%s/%s", playlist_directory, playlists[current_playlist_index]);

            // songs of new playlist
            nsongs = load_songs_from_playlist(song_information, playlist_path);
            no_songs_in_playlist_error = (nsongs == 0);
            
            if(nsongs > 0) {
                // start from the first song
                current_song_index = 0;
                set_current_song(playlist_path, cover_directory_path, (song_information + current_song_index), &music, settings);
                PlayMusicStream(music);

                if(update_flags.shuffle_play) {
                    song_history = 0;
                    create_song_queue(nsongs, current_song_index, shuffled_song_queue);
                }

                // reset the file watch to observe the new playlist
                deinit_file_watch(&file_watch);
                init_file_watch(&file_watch, playlist_path, SIGNAL);
            }

            update_flags.play_music = false;
        }

        // updating music stream
        if(IsMusicReady(music)) {
            // reset music settings to default
            if(IsKeyPressed(KEY_R)) {
                settings.pan = 0.50f;
                SetMusicPan(music, settings.pan);

                settings.pitch = 1.0f;
                SetMusicPitch(music, settings.pitch);

                settings.volume = 0.50f;
                SetMusicVolume(music, settings.volume);
            }

            // changing position in song
            if(update_flags.update_song_position) {
                update_flags.update_song_position = false;
                SeekMusicStream(music, (GetMusicTimeLength(music) * settings.percent_played));
            }

            // restarting current music stream
            if(update_flags.restart_song) {
                update_flags.restart_song = false;
                update_flags.play_music = true;
                StopMusicStream(music);
                PlayMusicStream(music);
            }

            bool update_current_song = (update_flags.skip_song || update_flags.rewind_song || update_flags.change_song);
            if(update_current_song) { 

                if(update_flags.skip_song) {
                    update_flags.skip_song = false;

                    if(update_flags.shuffle_play) {
                        if(song_history == (nsongs - 1)) { 
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

                // confine index to bounds (when rewinding first song and skipping last)
                current_song_index = (current_song_index + nsongs) % nsongs;

                set_current_song(playlist_path, cover_directory_path, (song_information + current_song_index), &music, settings);
                PlayMusicStream(music);
            }
            
            // playing music, 2nd 'ismusicready' is checking the case when new song is loaded
            if(update_flags.play_music && IsMusicReady(music))
                UpdateMusicStream(music);
        }

        if(!progress_bar_active)
            settings.percent_played = (GetMusicTimePlayed(music) / GetMusicTimeLength(music));

        BeginDrawing(); 
            ClearBackground(RAYWHITE);
            const int FONT_SIZE = 12;
            const float CONTENT_HEIGHT = 30;
            const float SCROLL_BAR_WIDTH = 13;
            const float BOTTOM_BAR_HEIGHT = 100.00; 
            
            if(show_playlist_window) {
                // the bounds of the scrollpanel on the screen
                const Rectangle panelRec = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight() + SCROLL_BAR_WIDTH };
                const Rectangle panelContentRec = (Rectangle){ 0, 0, GetScreenWidth(), (nplaylists * CONTENT_HEIGHT) };
                const bool vertical_scrollbar_visible = panelContentRec.height > (GetScreenHeight() - BOTTOM_BAR_HEIGHT);

                GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

                for(int p = 0, y_level = 0; p < nplaylists; p++, y_level += CONTENT_HEIGHT) {
                    const Rectangle content_rect = { 0, (y_level + panelScroll.y), GetScreenWidth() - (vertical_scrollbar_visible ? SCROLL_BAR_WIDTH : 0), CONTENT_HEIGHT };
                    
                    const bool in_bounds = (content_rect.y + (content_rect.height * 0.75f)) < GetScreenHeight();
                    if(in_bounds) {
                        // selecting a playlist
                        if(CheckCollisionPointRec(GetMousePosition(), content_rect)) {
                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                show_playlist_window = false;
                                // no need to update if the selected playlist is the same as before
                                change_playlist = (p != current_playlist_index);
                                current_playlist_index = p;
                            }
                        }
                        
                        const Color text_color = (p == current_playlist_index) ? GREEN : BLACK;
                        const float Y_LEVEL = content_rect.y + (content_rect.height / 2.0f);

                        // display playlist index
                        char index[4];
                        snprintf(index, 4, "%3d", (p + 1));
                        DrawText(index, 5, Y_LEVEL, FONT_SIZE, text_color);

                        // display playlist name
                        DrawText(playlists[p], 25, Y_LEVEL, FONT_SIZE, text_color);
                    }
                }
            }

            else if(no_songs_in_playlist_error) {
               const char* TEXT = "THERE ARE NO SONGS IN THE PLAYLIST!";
               DrawText(TEXT, (GetScreenWidth() / 2.0f) - (MeasureText(TEXT, 30) / 2.0f), (GetScreenHeight() / 2.0f), 30, BLACK); 
            }

            else {
                const Rectangle BOTTOM_BAR_BOUNDS = (Rectangle) {0, GetScreenHeight() - BOTTOM_BAR_HEIGHT, GetScreenWidth(), 100};

                // the bounds of the scrollpanel on the screen
                const Rectangle panelRec = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight() - BOTTOM_BAR_HEIGHT + SCROLL_BAR_WIDTH };
                
                const Rectangle panelContentRec = (Rectangle){ 0, CONTENT_HEIGHT, GetScreenWidth(), ((nsongs + 1) * CONTENT_HEIGHT)};
                const bool vertical_scrollbar_visible = panelContentRec.height > (GetScreenHeight() - BOTTOM_BAR_HEIGHT);
                const Rectangle TOP_BAR_BOUNDS = { 0, 0, GetScreenWidth() - (vertical_scrollbar_visible ? SCROLL_BAR_WIDTH : 0), CONTENT_HEIGHT };

                GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

                for(int s = 0, y_level = CONTENT_HEIGHT; s < nsongs; s++, y_level += CONTENT_HEIGHT) {
                    // the updated bounds of the rectangle from scrolling 
                    const Rectangle content_rect = { 0, (y_level + panelScroll.y), GetScreenWidth() - (vertical_scrollbar_visible ? SCROLL_BAR_WIDTH : 0), CONTENT_HEIGHT };

                    const bool under_top_bar = content_rect.y + (content_rect.height / 2.0f) >= (TOP_BAR_BOUNDS.y + TOP_BAR_BOUNDS.height);
                    const bool over_bottom_bar = (content_rect.y + (content_rect.height * 0.75f)) < BOTTOM_BAR_BOUNDS.y;
                    const bool valid_position = (under_top_bar && over_bottom_bar);

                    if(valid_position) {
                        bool mouse_hovering = CheckCollisionPointRec(GetMousePosition(), content_rect);
                        if(mouse_hovering) {
                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (song_information[current_song_index].information_ready)) {
                                // restart the song if you select the current one
                                if(s == current_song_index)
                                    update_flags.restart_song = true;
                                else {
                                    // create a new queue every time you seek a song
                                    if(update_flags.shuffle_play) {
                                        song_history = 0;
                                        create_song_queue(nsongs, s, shuffled_song_queue);
                                    }

                                    // set the current index to index sought
                                    update_flags.change_song = true;
                                    current_song_index = s;
                                }
                            }
                        }
                        
                        const Color text_color = (s == current_song_index) ? GREEN : BLACK;
                        const float Y_LEVEL = content_rect.y + (content_rect.height / 2.0f);
                        
                        // displaying song information
                        if(song_information[s].information_ready) {
                            char index[4];
                            snprintf(index, 4, "%3d", (s + 1));
                            DrawText(index, 5, Y_LEVEL, FONT_SIZE, text_color);
                            
                            DrawText(song_information[s].title, 30, Y_LEVEL, FONT_SIZE, text_color);        
                            DrawText(song_information[s].artist, (content_rect.width * 0.30f), Y_LEVEL, FONT_SIZE, text_color);
                            DrawText(song_information[s].album, (content_rect.width * 0.50f), Y_LEVEL, FONT_SIZE, text_color);

                            char last_accessed[LEN];
                            seconds_to_last_modified(song_information[s].modification_time, LEN, last_accessed);
                            DrawText(last_accessed, (content_rect.width * 0.70f), Y_LEVEL, FONT_SIZE, text_color);

                            char duration[LEN];
                            seconds_to_timestamp(song_information[s].duration, LEN, duration);
                            DrawText(duration, (content_rect.width * 0.90f), Y_LEVEL, FONT_SIZE, text_color);
                        }
                    }
                    
                    draw_top_bar(TOP_BAR_BOUNDS);
                }  

                // BOTTOM BAR
                {
                    // background color
                    DrawRectangleRec(BOTTOM_BAR_BOUNDS, RAYWHITE);
                
                    // song cover
                    const float IMG_PADDING = (BOTTOM_BAR_BOUNDS.height - IMG_SIZE) / 2.00;
                    const Vector2 IMG_LOCATION = { IMG_PADDING, BOTTOM_BAR_BOUNDS.y + IMG_PADDING };

                    if(IsTextureReady(song_information[current_song_index].cover))
                        DrawTextureEx(song_information[current_song_index].cover, IMG_LOCATION, 0.0f, 1.0f, RAYWHITE);
                                
                    float text_starting_point = BOTTOM_BAR_BOUNDS.x + IMG_SIZE + 10;

                    // displaying current playlist
                    char playlist[LEN * 2];
                    snprintf(playlist, (LEN * 2), "%s %s", playlists[current_playlist_index], (update_flags.shuffle_play ? "(SHUFFLED)" : ""));
                    DrawText(playlist, text_starting_point, (BOTTOM_BAR_BOUNDS.y + 10), 20, BLACK);

                    // display song information
                    DrawText(song_information[current_song_index].title, text_starting_point, (BOTTOM_BAR_BOUNDS.y + 35), 13, BLACK);
                    DrawText(song_information[current_song_index].artist, text_starting_point, (BOTTOM_BAR_BOUNDS.y + 55), 10, BLACK);

                    // rewind, pause, and skip buttons
                    playback_buttons(BOTTOM_BAR_BOUNDS, &update_flags, GetMusicTimePlayed(music), GetMusicTimeLength(music));

                    // bar to change music time position
                    progress_bar(BOTTOM_BAR_BOUNDS, &update_flags, &progress_bar_active, &settings.percent_played, GetMusicTimePlayed(music), GetMusicTimeLength(music));

                    // sliders to change music settings
                    music_settings(BOTTOM_BAR_BOUNDS, &settings, &music);
                }
            }
            DrawFPS(0, GetScreenHeight() / 2);
        EndDrawing();
    }

    unload_song_information(nsongs, song_information);
    deinit_file_watch(&file_watch);

    if(IsMusicReady(music)) {
        if(IsMusicStreamPlaying(music))
            StopMusicStream(music);
        UnloadMusicStream(music);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// split texttures form song information
// change cover on next frame when selecting song directly
// song covers are blank when clearing playlist and loading new playlist
// fix shuffle issue, some index repeat!
// error handle no playlists
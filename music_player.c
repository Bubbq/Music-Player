#include "headers/raylib.h"
#include "headers/file_watch.h"
#include "headers/file_management.h"
#include "headers/stream_management.h"

#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <libavutil/dict.h>
#include <libavformat/avformat.h>

#define RAYGUI_IMPLEMENTATION
#include "headers/raygui.h"

#define LEN 256
#define NSONGS MAX_EVENTS
#define NPLAYLIST MAX_EVENTS
#define IMG_SIZE 90

typedef enum
{
    NO_PLAYLIST = 0,
    NO_SONGS_IN_PLAYLIST = 1,
    SONG_WINDOW = 2,
    PLAYLIST_WINDOW = 3,
}ApplicationState;

typedef struct
{
    bool play_music;
    bool shuffle_play;
    bool skip_song;
    bool change_song;
    bool rewind_song;
    bool restart_song;
    bool update_song_position;
} MusicFlags;

typedef struct
{
    Texture2D cover;
    char title[LEN]; 
    char album[LEN]; 
    char artist[LEN]; 
    char relative_path[LEN]; 
    float track;
    float duration;
    float access_time;
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

// shift array elements of size nbytes left by 'shift_amount' positions, returns the index of the first redundant element
int shift_array(int nelements, void* array, unsigned int start_index, unsigned int shift_amount, size_t nbytes)
{
    int i = start_index;
    for( ; (i + shift_amount) < nelements; i++) 
        memcpy((array + (i * nbytes)), (array + ((i + shift_amount) * nbytes)), nbytes);
    return i;
}

// swaps the values of two elements
void swap(void* a, void* b, size_t nbytes)
{
    void* temp = malloc(nbytes);
    memcpy(temp, a, nbytes);
    memcpy(a, b, nbytes);
    memcpy(b, temp, nbytes);
    free(temp);
}

// a song queue for shuffle play
void create_song_queue(int nsongs, int current_song_index, int queue[nsongs])
{
    if((current_song_index > (nsongs - 1)) || (current_song_index < 0)) {
        printf("create_song_queue, invalid 'current_song_index' size");
        return;
    }
    for(int i = 0; i < nsongs; i++)
        queue[i] = i;
    // set the first song of the queue to the current one
    swap(&queue[0], &queue[current_song_index], sizeof(int));
    shuffle_array((nsongs - 1), (queue + 1));
}

void init_app()
{
    SetTargetFPS(60);
    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(900,750, "Dotify");
    InitAudioDevice();
}

// loads an mp3 file specified in path
void change_music_stream(Music* music, const char* songpath)
{
    // clear music stream
    if(IsMusicReady(*music)) {
        if(IsMusicStreamPlaying(*music))
            StopMusicStream(*music);
        UnloadMusicStream(*music);
    }
    // load the new music
    if(!FileExists(songpath)) {
        printf("change_music_stream, %s does not exist in device\n", songpath);
        return;
    }
    else if(!IsFileExtension(songpath, ".mp3")) {
        printf("change_music_stream, %s is not a valid audio file\n", songpath);
        return;
    }
    else
        (*music) = LoadMusicStream(songpath);
}

// converts n seconds into a H:M:S format 
void seconds_to_timestamp(int s, int nchars, char timestamp[nchars])
{
    int hours, minutes, seconds;
    hours = s / 3600;
    minutes = (s % 3600) / 60;
    seconds = s % 60;

    if(hours) 
        snprintf(timestamp, nchars, "%02d:%02d:%02d", hours, minutes, seconds);
    else 
        snprintf(timestamp, nchars, "%2d:%02d", minutes, seconds);
}

// draws three buttons (rewind, pause, skip) equidistant from one another within a container
void playback_buttons(Rectangle container, MusicFlags* flags, int seconds_played)
{
    const int size = 20;
    const float y = container.y + ((container.height - size) / 2.0f);
    
    // rewind button
    const float rewind_threshold = 3.0f; // the amount of time that needs to be played to restart rather than rewind
    const Rectangle rewind_bounds = (Rectangle){ (GetScreenWidth() / 2.00) - (size * 2.0f), y, size, size };
    if((GuiButton(rewind_bounds, "<")) || (IsKeyPressed(KEY_A))) {
        if(seconds_played >= rewind_threshold)
            flags->restart_song = true;
        else 
            flags->play_music = flags->rewind_song = true;
    }

    // pause button
    const char* pause_text = flags->play_music ? "||" : "|>";
    const Rectangle pause_bounds = (Rectangle){ (GetScreenWidth() / 2.0f) - (size / 2.0f), y, size, size };
    if((GuiButton(pause_bounds, pause_text)) || (IsKeyPressed(KEY_SPACE))) 
        flags->play_music = !flags->play_music;

    // skip button
    const Rectangle skip_bounds = (Rectangle){ (GetScreenWidth() / 2.0f) + size, y, size, size };
    if((GuiButton(skip_bounds, ">") || IsKeyPressed(KEY_D)))
        flags->play_music = flags->skip_song = true;
}

// slider to change position in song
void progress_bar(Rectangle container, MusicFlags* flags, bool* progress_bar_active, float* percentage_played, float time_played, float song_length)
{
    // update the time played when engaging with the progress bar
    if((*progress_bar_active))
        time_played = song_length * (*percentage_played);

    // converting current and total song time into strings in 00:00 format
    char time_played_text[LEN];
    seconds_to_timestamp(time_played, LEN, time_played_text);
    char song_length_text[LEN];
    seconds_to_timestamp(song_length, LEN, song_length_text);

    const float slider_width = GetScreenWidth() * 0.40f;
    const Rectangle progress_bar_bounds = { (GetScreenWidth() / 2.0f) - (slider_width / 2.0f), container.y + (container.height * 0.75f), slider_width, 10 };
    if(GuiSliderBar(progress_bar_bounds, time_played_text, song_length_text, percentage_played, 0.0f, 1.0f))
        (*progress_bar_active) = true;
    else
        *percentage_played = (time_played / song_length);
    
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

    // volume
    const Vector2 VOLUME_RANGE = { 0.0f, 1.0f };
    const Rectangle VOLUME = { container.width - (SLIDER_WIDTH * 1.25f), (container.y + 15), SLIDER_WIDTH, 10 };
    if(GuiSliderBar(VOLUME, "VOLUME", "", &settings->volume, VOLUME_RANGE.x, VOLUME_RANGE.y))
        SetMusicVolume((*music), settings->volume);
    // pitch 
    const Vector2 PITCH_RANGE = { 0.75f, 1.25f };
    const Rectangle PITCH = { container.width - (SLIDER_WIDTH * 1.25f), (container.y + 40), SLIDER_WIDTH, 10 };
    if(GuiSliderBar(PITCH, "PITCH", "", &settings->pitch, PITCH_RANGE.x, PITCH_RANGE.y))
        SetMusicPitch((*music), settings->pitch);
    // pan
    const Vector2 PAN_RANGE = { 0.0f, 1.0f };
    const Rectangle PAN = { container.width - (SLIDER_WIDTH * 1.25), (container.y + 65), SLIDER_WIDTH, 10 };
    if(GuiSlider(PAN, "PAN", "", &settings->pan, PAN_RANGE.x, PAN_RANGE.y)) 
        SetMusicPan((*music), 1 - settings->pan);
}

// given a songname, write the appropraite cover path to dst
int get_cover_path(int nchars, char dst[nchars], const char* song_title, const char* cover_directory_path)
{
    const int min_len = strlen(cover_directory_path) + strlen(song_title) + strlen(".jpg");
    if(nchars < min_len) {
        printf("get_cover_path, nchars of dst (%d) is smaller than the min length (%d)\n", nchars, min_len);
        memset(dst, 0, nchars);
        return -1;
    } 
    else {
        snprintf(dst, nchars, "%s/%s.jpg", cover_directory_path, song_title);
        return 0;
    } 
}

Texture2D get_song_cover(const char* song_path, const char* cover_directory_path, int image_size)
{
    // getting the cover path
    char cover_path[LEN * 2];
    if(get_cover_path((LEN * 2), cover_path, GetFileNameWithoutExt(song_path), cover_directory_path) < 0)
        return (Texture2D){ 0 };
    // extract the cover image if the file doesnt already exist
    else if(!FileExists(cover_path) && (extract_cover_image(song_path, cover_path) < 0))
        return (Texture2D){ 0 };
    else {
        Image image = LoadImage(cover_path);
        if(!IsImageReady(image))
            return (Texture2D){ 0 };
        else {
            ImageResize(&image, image_size, image_size);
            Texture2D ret = LoadTextureFromImage(image);
            UnloadImage(image);
            return ret;
        }
    }
}

Texture2D get_default_song_cover()
{
    // default song cover texture used when not availible
    const char* DEFAULT_IMG_PATH = "default.png";
    Texture2D defualt_texture = { 0 };
    Image default_image = LoadImage(DEFAULT_IMG_PATH);
    
    if(IsImageReady(default_image)) {
        ImageResize(&default_image, IMG_SIZE, IMG_SIZE);
        defualt_texture = LoadTextureFromImage(default_image);
    }
    return defualt_texture;
}

// returns the time in seconds since last access of a file
float file_access_time(const char* file)
{
    const time_t right_now = time(NULL);
    struct stat st;
    if(stat(file, &st) == 0) 
        return difftime(right_now, st.st_atim.tv_sec);
    else
        return -1;
}

// convert time in seconds since file has been altered to string form
void seconds_to_last_accessed(long int s, int nchars, char dst[nchars])
{
    int days = s / 86400;
    int years = days / 365;
    int months = days / 30;
    int weeks = days / 7;
    int hours = (s % 86400) / 3600;
    int minutes = ((s % 86400) % 3600) / 60;
    int seconds = s % 60;

    if(years)
        snprintf(dst, nchars, "%d %s ago", years, "year(s)");
    else if(months)
        snprintf(dst, nchars, "%d %s ago", months, "month(s)");
    else if(weeks)
        snprintf(dst, nchars, "%d %s ago", weeks, "week(s)");
    else if(days)
        snprintf(dst, nchars, "%d %s ago", days, "day(s)");
    else if(hours)
        snprintf(dst, nchars, "%d %s ago", hours, "hour(s)");
    else if(minutes)
        snprintf(dst, nchars, "%d %s ago", minutes, "minute(s)");
    else if(seconds)
        snprintf(dst, nchars, "%d %s ago", seconds, "second(s)");
    else
        snprintf(dst, nchars, "now");
}

float microseconds_to_seconds(float microseconds)
{
    return (microseconds * 0.000001);
}

// get all the information for a song
void get_information(SongInfo* information, const char* full_song_path)
{
    // get the metadata from the audio/video path
    AVFormatContext* format_context;
    if((format_context = get_format_context(full_song_path)) == NULL) 
        return;

    AVDictionary* metadata = format_context->metadata;

    const char* DEFAULT = "...";

    strcpy(information->relative_path, GetFileName(full_song_path));

    // basic song information
    if(get_metadata_parameter(metadata, "artist", LEN, information->artist) < 0) strcpy(information->artist, DEFAULT);
    if(get_metadata_parameter(metadata, "title", LEN, information->title) < 0) strcpy(information->title, information->relative_path);
    if(get_metadata_parameter(metadata, "album", LEN, information->album) < 0) strcpy(information->album, DEFAULT);
    
    char track[LEN];
    if(get_metadata_parameter(metadata, "track", LEN, track) <  0) 
        information->track = 999;
    else
        information->track = atof(track);

    information->duration = microseconds_to_seconds(format_context->duration);
    information->access_time = file_access_time(full_song_path);
    information->information_ready = true;

    avformat_close_input(&format_context);
}

// comparators for qsort
int by_playlist(const void* a, const void* b)
{
    return strcmp((char*)a, (char*)b);
}

int by_title(const void* a, const void* b)
{
    SongInfo* info_a = (SongInfo*)a;
    SongInfo* info_b = (SongInfo*)b;
    return strcmp(info_a->title, info_b->title);
}

int by_track(const void* a, const void* b)
{
    SongInfo* info_a = (SongInfo*)a;
    SongInfo* info_b = (SongInfo*)b;
    return info_a->track > info_b->track;
}

void set_current_song(const char* playlist_path, const char* cover_directory_path, SongInfo* song_information, Music* music, MusicSettings settings)
{
    char full_song_path[LEN * 3];
    snprintf(full_song_path, (LEN * 3), "%s/%s", playlist_path, song_information->relative_path);
    if(FileExists(full_song_path)) {
        // maintain previous loop state
        const bool loop = music->looping;

        // get new cover
        song_information->cover = get_song_cover(full_song_path, cover_directory_path, IMG_SIZE);
        
        // load new music stream
        change_music_stream(music, full_song_path);

        music->looping = loop;

        // apply the current settings
        SetMusicPan((*music), settings.pan);
        SetMusicPitch((*music), settings.pitch);
        SetMusicVolume((*music), settings.volume);
    }
}

// a playlist is an album whenever there are no differences in albums
bool is_album(int nsongs, SongInfo song_information[nsongs])
{
    char* album = song_information[0].album;
    for(int i = 0; i < nsongs; i++)
        if(strcmp(album, song_information[i].album) != 0 && (strlen(song_information[i].album) > 0))
            return false;
    return true;
}

int load_playlist_information(int max_nsongs, SongInfo song_information[max_nsongs], const char* playlist_path)
{
    // store all files in the playlist path
    char relative_paths[NSONGS][LEN] = { 0 };
    int potential_songs = get_files_from_folder(LEN, NSONGS, relative_paths, playlist_path, DT_REG);

    // filter any non mp3s
    int nsongs = filter_library(LEN, potential_songs, relative_paths, ".mp3");

    // get information of every song
    char buffer[LEN * 3];
    for(int i = 0; i < nsongs; i++) {
        snprintf(buffer, (LEN * 3), "%s/%s", playlist_path, relative_paths[i]);
        get_information(&song_information[i], buffer);
    }    

    // sort songs in alphabetical order
    const bool album = is_album(nsongs, song_information);
    qsort(song_information, nsongs, sizeof(SongInfo), (album ? by_track : by_title));

    return nsongs;
}

void delete_song_information(SongInfo* song_information)
{
    if(IsTextureReady(song_information->cover))
        UnloadTexture(song_information->cover);
    memset(song_information, 0, sizeof((*song_information)));
}

// returns the index of the song with a matching relative path
int find_song_index(int nsongs, SongInfo song_information[nsongs], const char* relative_path)
{
    for(int s = 0; s < nsongs; s++) 
        if(strcmp(song_information[s].relative_path, relative_path) == 0)
            return s;
    printf("find_index, %s was not found\n", relative_path);
    return -1;
}

int find_playlist_index(int nplaylists, int nchars, char playlists[nplaylists][nchars], const char* target)
{
    for(int p = 0; p < nplaylists; p++)
        if(strcmp(playlists[p], target) == 0)
            return p;
    printf("find_playlist_index, %s was not found\n", target);
    return -1;
}

void toggle_playlist_view(int nsongs, int nplaylists, int current_playlist_index, ApplicationState* state)
{
    if(IsKeyPressed(KEY_P)) {
        if((*state) == PLAYLIST_WINDOW) {
            if(current_playlist_index >= 0) 
                (*state) = (nsongs > 0) ? SONG_WINDOW : NO_SONGS_IN_PLAYLIST;
        }
        else 
            (*state) = (nplaylists > 0) ? PLAYLIST_WINDOW : NO_PLAYLIST;
    }
}

int main()
{   
    const int frame_read_rewind_threshold = 10;
    const char* HOME_DIRECTORY = getenv("HOME");
    
    // path to folder where covers are held, jpgs from the mp3s are extracted and written to this folder
    char cover_directory_path[LEN]; 
    snprintf(cover_directory_path, LEN, "%s/.cache/song_covers", HOME_DIRECTORY);
    
    // creating the directory
    if(!DirectoryExists(cover_directory_path))
        make_directory(cover_directory_path);
    
    // path to folder where playlists (directorys holding mp3s) are held
    char playlist_directory[LEN]; 
    snprintf(playlist_directory, LEN, "%s/Music", HOME_DIRECTORY);

    // creating the directory
    if(!DirectoryExists(playlist_directory))
        make_directory(playlist_directory);

    // a list of relative playlists paths
    char playlists[NPLAYLIST][LEN] = { 0 };

    // pointers to current playlist and song
    int current_song_index, current_playlist_index;
    current_playlist_index = current_song_index = -1;
    
    // full path to the current playlist 
    char playlist_path[LEN * 2];
    snprintf(playlist_path, (LEN * 2), "%s/%s", playlist_directory, playlists[current_playlist_index]);

    // the list of shuffled songs to be played
    int shuffled_song_queue[NSONGS];

    // current position in this queue
    int song_history = 0; 

    // relavent information of every song in the current playlist
    SongInfo song_information[NSONGS] = { 0 };

    // the current number of songs and playlists in the application
    int nsongs;
    int nplaylists;
    
    // application flags
    bool switch_playlist = false;
    bool progress_bar_active = false;
    
    // flags that control the flow of music
    MusicFlags music_flags = { false }; 

    // settings
    MusicSettings settings;
    settings.pan = 0.50f;
    settings.pitch = 1.0f;
    settings.volume = 0.50f;
    settings.percent_played = 0.0f;

    Music music = { 0 };

    ApplicationState application_state;

    // the signals that trigger external events: any addition or deletion to a file or folder
    const uint32_t SIGNALS  = (IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

    // objects that detect external events to the user's playlist and songs in the current playlist
    // an 'external event' can be described as manipulating files and folders in the file explorer while the application is running
    // a simplified example of this in action is seen in 'file_watch_example.c'
    FileWatch external_song_watch;
    FileWatch external_playlist_watch;
    
    // write the relative path of all folders in the playlist directory to 'playlists'
    if((nplaylists = get_files_from_folder(LEN, NPLAYLIST, playlists, playlist_directory, DT_DIR)) > 0) {
        application_state = PLAYLIST_WINDOW;
        // sort playlists alphabetically
        qsort(playlists, nplaylists, LEN, by_playlist);
    }
    else
        application_state = NO_PLAYLIST;

    // start watching for external events regarding playlists
    init_file_watch(&external_playlist_watch, playlist_directory, SIGNALS);

    // initalizing the application
    init_app();

    // inital position of scrollbar
    Vector2 panelScroll = { 99, 0 };

    // no idea what this is tbh, raylib example isnt very specfic
    Rectangle panelView = { 10, 10 };

    // default song cover used when mp3s metadata does not have an image embedded
    const Texture2D defualt_texture = get_default_song_cover();
    
    while(!WindowShouldClose()) {
        toggle_playlist_view(nsongs, nplaylists, current_playlist_index, &application_state);

        // switching from one playlist to another
        if(switch_playlist) {
            switch_playlist = false;

            // free information of the previous playlist
            for(int i = 0; i < nsongs; i++)
                delete_song_information(&song_information[i]);

            // modify the playlist path to the new playlist
            snprintf(playlist_path, (LEN * 2), "%s/%s", playlist_directory, playlists[current_playlist_index]);

            // load information of the new playlist
            if((nsongs = load_playlist_information(NSONGS, song_information, playlist_path)) > 0) {
                application_state = SONG_WINDOW;
                
                // retain previous loop status
                const bool loop = music.looping;
                
                // start music stream from the first song 
                current_song_index = 0;
                set_current_song(playlist_path, cover_directory_path, &song_information[current_song_index], &music, settings);
                PlayMusicStream(music);
                
                // start the song paused
                music_flags.play_music = false;
                music.looping = loop;

                // create song queue if last playlist was in shuffle mode
                if(music_flags.shuffle_play) {
                    song_history = 0;
                    create_song_queue(nsongs, current_song_index, shuffled_song_queue);
                }
            }
            else {
                application_state = NO_SONGS_IN_PLAYLIST;
                music_flags = (MusicFlags){ false }; // halt any potential updates
            }

            // reset the file watch to observe the new playlist
            if(external_song_watch.wd)
                deinit_file_watch(&external_song_watch);

            init_file_watch(&external_song_watch, playlist_path, SIGNALS);
        }

        // updating music stream
        if(IsMusicReady(music)) {
            if(music_flags.play_music) 
                UpdateMusicStream(music);
            
            // changing position in song
            if(music_flags.update_song_position) {
                music_flags.update_song_position = false;
                SeekMusicStream(music, (GetMusicTimeLength(music) * settings.percent_played));
            }

            // restarting current music stream
            if(music_flags.restart_song) {
                music_flags.restart_song = false;
                StopMusicStream(music);
                PlayMusicStream(music);
            }

            // handling any instance of song change
            if(music_flags.skip_song || music_flags.rewind_song || music_flags.change_song) { 
                if(music_flags.skip_song) {
                    music_flags.skip_song = false;

                    if(music_flags.shuffle_play) {
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

                else if(music_flags.rewind_song) {
                    music_flags.rewind_song = false;

                    // load the previous song in the queue
                    if(music_flags.shuffle_play) {
                        if(song_history)
                            song_history--;
                        current_song_index = shuffled_song_queue[song_history];
                    }
                    else
                        current_song_index--;
                }
                
                else if(music_flags.change_song)
                    music_flags.change_song = false;
                
                // confine index to array bounds (ie. rewinding first song and skipping last)
                current_song_index = (current_song_index + nsongs) % nsongs;

                // load the content of the new song
                set_current_song(playlist_path, cover_directory_path, &song_information[current_song_index], &music, settings);
                PlayMusicStream(music);
            }

            // when music stream is exhausted
            if(!IsMusicStreamPlaying(music) && music_flags.play_music) {
                if(music.looping)
                    music_flags.restart_song = true;
                else
                    music_flags.skip_song = true;
            }
        }

        // handling external song events (deleting, adding, and/or renaming songs in your file explorer while the application is running)
        if((application_state == SONG_WINDOW) || (application_state == NO_SONGS_IN_PLAYLIST)) {
            // starting position is not explicity 0 due to the fact that when reading a large amount of events, not all are read in one frame
            // this is highly dependent on the FPS of the application where more FPS -> higher chance of n events splitting
            const int events_read = watch_events(NSONGS, external_song_watch.nevents, external_song_watch.events, external_song_watch.fd);

            // append/init the number of events read
            external_song_watch.nevents += events_read;

            // when reading a new event
            if((external_song_watch.reading_events == false) && (events_read > 0)) {
                external_song_watch.nframes_reading = 0;
                external_song_watch.reading_events = true;
            }

            // continue to read until rewind_threshold is met
            else if(external_song_watch.reading_events)
                external_song_watch.nframes_reading++; 
                
            // after reading events for n frames, now process file events
            if((external_song_watch.reading_events) && (external_song_watch.nframes_reading >= frame_read_rewind_threshold)) {
                int nsongs_added = 0;
                int nsongs_deleted = 0;
                bool current_song_deleted = false;
                const int old_size = nsongs;
                char old_song[LEN];
                strcpy(old_song, song_information[current_song_index].relative_path);

                for(int e = 0; e < external_song_watch.nevents; e++) {
                    const FileEvent file_event = external_song_watch.events[e];
                    const struct inotify_event inotify_event = file_event.event;
                    const char* filename = file_event.file_name;

                    // external song deletion
                    if(inotify_event.mask == 64) {
                        // get the index of the deleted song from its filename
                        const int deleted_index = find_song_index(nsongs, song_information, filename);
                        
                        // clear information
                        if(deleted_index >= 0) {
                            delete_song_information(&song_information[deleted_index]);
                            nsongs_deleted++;

                            if(deleted_index == current_song_index) 
                                current_song_deleted = true;
                        }
                    }
                    
                    // external song addition
                    else if((inotify_event.mask == 128) && (IsFileExtension(filename, ".mp3")) && (nsongs < NSONGS)) {
                        // the path of the added song 
                        char full_song_path[LEN * 3];
                        snprintf(full_song_path, (LEN * 3), "%s/%s", playlist_path, filename);

                        // get new song's information
                        get_information(&song_information[nsongs], full_song_path);
                        
                        // update song count
                        nsongs_added++;
                        nsongs++;
                    }
                }

                // list is now sorted s.t. any deleted elements are at the front
                const bool album = is_album(nsongs, song_information);
                qsort(song_information, nsongs, sizeof(SongInfo), (album ? by_track : by_title));

                if(nsongs_deleted > 0) {
                    // shift remaining elements into null locations
                    if(nsongs_deleted < nsongs)
                        shift_array(nsongs, song_information, 0, nsongs_deleted, sizeof(SongInfo));
                    
                    // adjust size
                    nsongs -= nsongs_deleted;
                }

                if(nsongs > 0) {
                    // start from the first song in the playlist when the current song playing is deleted
                    // else keep the same song information displayed by finding the new index of the current song 
                    current_song_index = (current_song_deleted || old_size == 0) ? 0 : find_song_index(nsongs, song_information, old_song);
                    set_current_song(playlist_path, cover_directory_path, &song_information[current_song_index], &music, settings);
                    PlayMusicStream(music);
                    // start song paused
                    music_flags.play_music = false;

                    // reset song queue if in shuffle mode
                    if(music_flags.shuffle_play) {
                        song_history = 0;
                        create_song_queue(nsongs, current_song_index, shuffled_song_queue);
                    }

                    application_state = SONG_WINDOW;
                }
                else {
                    // halt potential updates
                    current_song_index = -1;
                    music_flags = (MusicFlags){ false };
                    application_state = NO_SONGS_IN_PLAYLIST;
                }

                // reset reading params in FileEvent object
                external_song_watch.nevents = 0;
                external_song_watch.reading_events = false;
            }
        }
        
        // handling external playlist events (deleting, adding, and/or renaming playlists in your file explorer while the application is running)
        {
            // read n events into FileWatch object
            const int events_read = watch_events(NPLAYLIST, external_playlist_watch.nevents, external_playlist_watch.events, external_playlist_watch.fd);
            
            // append/init the number of events read
            external_playlist_watch.nevents += events_read;

            // when reading a new event
            if((external_playlist_watch.reading_events == false) && (events_read > 0)) {
                external_playlist_watch.nframes_reading = 0;
                external_playlist_watch.reading_events = true;
            }
            
            // continue to read until rewind_threshold is met
            else if(external_playlist_watch.reading_events)
                external_playlist_watch.nframes_reading++; 

            // process external events after reading for 'frame_read_rewind_threshold' frames
            if(external_playlist_watch.reading_events && (external_playlist_watch.nframes_reading >= frame_read_rewind_threshold)) {
                int nplaylists_deleted = 0;
                int nplaylists_added = 0;
                bool current_playlist_deleted = false;
                
                char old_playlist[LEN];
                strcpy(old_playlist, playlists[current_playlist_index]);

                for(int e = 0; e < external_playlist_watch.nevents; e++) {
                    const FileEvent file_event = external_playlist_watch.events[e];
                    const struct inotify_event inotify_event = file_event.event; 
                    const char* filename = file_event.file_name; 
                    // print_inotify_event(&inotify_event,  filename);

                    // deletion
                    const bool external_playlist_deletion = (inotify_event.mask == 1073741888);
                    if(external_playlist_deletion) {
                        const int deleted_playlist_index = find_playlist_index(nplaylists, LEN, playlists, filename);
                        if(deleted_playlist_index >= 0) {
                            // remove relative path of the deleted playlist 
                            memset(playlists[deleted_playlist_index], 0, sizeof(playlists[deleted_playlist_index]));
                            nplaylists_deleted++;

                            if(current_playlist_index == deleted_playlist_index) {
                                current_playlist_deleted = true;
                                // clear all song information of the current playlist
                                music_flags = (MusicFlags){ false };
                                for(int i = 0; i < nsongs; i++)
                                    delete_song_information(&song_information[i]);
                            }
                        }
                    }
                    // addtion
                    const bool external_playlist_addtion = ((inotify_event.mask == 1073741952) || (inotify_event.mask == 1073742080)) && (nplaylists < NPLAYLIST);
                    if(external_playlist_addtion) {
                        // only directorys are to update the 'playlists' object
                        char potential_directory[LEN * 2];
                        snprintf(potential_directory, (LEN * 2), "%s/%s", playlist_directory, filename);
                        
                        // add the new relative path
                        if(DirectoryExists(potential_directory) && (inotify_event.len < LEN)) {
                            nplaylists_added++;
                            strcpy(playlists[nplaylists++], filename);
                        }
                    }
                }

                // sort elements alphabetically
                // this means that any deleted elements are at the front since "" is first in ASCII
                qsort(playlists, nplaylists, LEN, by_playlist);
                
                if(nplaylists_deleted > 0) {
                    // shift real elements into empty space
                    shift_array(nplaylists, playlists, 0, nplaylists_deleted, LEN);
                    
                    // update the count
                    nplaylists -= nplaylists_deleted;
                }

                const bool go_to_playlist_view = ((application_state == PLAYLIST_WINDOW)|| (application_state == NO_PLAYLIST) || current_playlist_deleted);
                if(go_to_playlist_view) 
                    application_state = (nplaylists == 0) ? NO_PLAYLIST : PLAYLIST_WINDOW;

                // retain playlist index, or -1 null if old playlist index is deleted
                current_playlist_index = find_playlist_index(nplaylists, LEN, playlists, old_playlist);
                
                // reset reading params in FileEvent object
                external_playlist_watch.nevents = 0;
                external_playlist_watch.reading_events = false;
            } 
        }
       
        BeginDrawing(); 
            ClearBackground(RAYWHITE);
            const float CONTENT_HEIGHT = 30;
            const float SCROLL_BAR_WIDTH = 14;

            if(application_state == PLAYLIST_WINDOW) {
                // the bounds of the scrollpanel on the screen
                const Rectangle playlistPanel = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight() + SCROLL_BAR_WIDTH };
                // the area that displaying the information takes
                const Rectangle playlistContent = (Rectangle){ 0, 0, GetScreenWidth(), (nplaylists * CONTENT_HEIGHT) };
                const bool vertical_scrollbar_visible = (playlistContent.height > GetScreenHeight());

                GuiScrollPanel(playlistPanel, NULL, playlistContent, &panelScroll, &panelView);
                
                for(int p = 0, y_level = 0; p < nplaylists; p++, y_level += CONTENT_HEIGHT) {
                    // the area that the pth playlist is contained in
                    const Rectangle content_rect = { 0, (y_level + panelScroll.y), GetScreenWidth() - (vertical_scrollbar_visible ? SCROLL_BAR_WIDTH : 0), CONTENT_HEIGHT };
                   
                    // only display the pth playlist if its within the screen border
                    const bool in_bounds = (content_rect.y + (content_rect.height * 0.75f)) < GetScreenHeight();
                    if(in_bounds) {
                        // selecting a playlist
                        const bool playlist_selected = (CheckCollisionPointRec(GetMousePosition(), content_rect)) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
                        if(playlist_selected) {
                            // only change playlists when playlist selected is diffrent from the current one
                            if(p != current_playlist_index) {
                                switch_playlist = true;
                                current_playlist_index = p;
                            }
                            else
                                application_state = (nsongs > 0) ? SONG_WINDOW : NO_SONGS_IN_PLAYLIST;
                        }
                        // drawing pth playlist to screen
                        const int font_size = 12;
                        const Color display_color = (p == current_playlist_index) ? GREEN : BLACK;
                        const float y = content_rect.y + (content_rect.height / 2.0f);
                        char index[4];
                        snprintf(index, 4, "%3d", (p + 1));
                        DrawText(index, 5, y, font_size, display_color);
                        DrawText(playlists[p], 25, y, font_size, display_color);
                    }
                }
            }
            else if(application_state == SONG_WINDOW) {
                // toggling shuffle mode
                if(IsKeyPressed(KEY_S) && (nsongs > 1)) {
                    music_flags.shuffle_play = !music_flags.shuffle_play;
                    
                    if(music_flags.shuffle_play) {
                        song_history = 0;
                        create_song_queue(nsongs, current_song_index, shuffled_song_queue);
                    }
                }
                // toggling looping
                if(IsKeyPressed(KEY_L)) 
                    music.looping = !music.looping;
                // restart song 
                if(IsKeyPressed(KEY_R))
                    music_flags.restart_song = true;

                const Rectangle BOTTOM_BAR_BOUNDS = (Rectangle) { 0, GetScreenHeight() - 100, GetScreenWidth(), 100 };
                
                // the bounds of the scrollpanel on the screen
                const Rectangle songPanel = (Rectangle){ 0, -1, GetScreenWidth(), GetScreenHeight() - BOTTOM_BAR_BOUNDS.height + SCROLL_BAR_WIDTH };
                // the area that displaying the information takes
                const Rectangle songContent = (Rectangle){ 0, CONTENT_HEIGHT, GetScreenWidth(), ((nsongs + 1) * CONTENT_HEIGHT) };
                const bool vertical_scroll_visible = songContent.height > (GetScreenHeight() - BOTTOM_BAR_BOUNDS.height);
                
                GuiScrollPanel(songPanel, NULL, songContent, &panelScroll, &panelView);
                
                // the top portion of the screen  
                const Rectangle TOP_BAR_BOUNDS = { 0, 0, GetScreenWidth() - (vertical_scroll_visible ? SCROLL_BAR_WIDTH : 0), CONTENT_HEIGHT };
                {
                    const int font_size = 15;
                    const Color display_color = BLACK;
                    const float y = TOP_BAR_BOUNDS.y + (TOP_BAR_BOUNDS.height / 2.0f);
                    DrawText("#", 13, y, font_size, display_color);
                    DrawText("Title", 30, y, font_size, display_color);
                    DrawText("Artist", (TOP_BAR_BOUNDS.width * 0.30f), y, font_size, display_color);
                    DrawText("Album", (TOP_BAR_BOUNDS.width * 0.50f), y, font_size, display_color);
                    DrawText("Date Added", (TOP_BAR_BOUNDS.width * 0.70f), y, font_size, display_color);
                    DrawText("Duration", (TOP_BAR_BOUNDS.width * 0.90f), y, font_size, display_color);
                } 
                // contains the buttons and sliders that change the flow and characteristics of the song played
                {
                    // background color
                    DrawRectangleRec(BOTTOM_BAR_BOUNDS, RAYWHITE);

                    // song cover
                    const float IMG_PADDING = (BOTTOM_BAR_BOUNDS.height - IMG_SIZE) / 2.0f;
                    const Vector2 IMG_LOCATION = { IMG_PADDING, BOTTOM_BAR_BOUNDS.y + IMG_PADDING };
                    const Texture2D song_cover = IsTextureReady(song_information[current_song_index].cover) ? song_information[current_song_index].cover : defualt_texture;
                    DrawTextureEx(song_cover, IMG_LOCATION, 0.0f, 1.0f, RAYWHITE);

                    // x pos of display text          
                    const float x = BOTTOM_BAR_BOUNDS.x + (IMG_PADDING * 2.0f) + IMG_SIZE;

                    // display current song information
                    DrawText(playlists[current_playlist_index], x, (BOTTOM_BAR_BOUNDS.y + 10), 20, BLACK);
                    if(music_flags.shuffle_play)
                        DrawText("(SHUFFLED)", (x + MeasureText(playlists[current_playlist_index], 20)) + 5, (BOTTOM_BAR_BOUNDS.y + 10), 20, BLACK);
                    
                    DrawText(song_information[current_song_index].title, x, (BOTTOM_BAR_BOUNDS.y + 35), 13, BLACK);
                    if(music.looping)
                        DrawText("(LOOPED)", (x + MeasureText(song_information[current_song_index].title, 13) + 5), (BOTTOM_BAR_BOUNDS.y + 35), 13, BLACK);
                    
                    DrawText(song_information[current_song_index].artist, x, (BOTTOM_BAR_BOUNDS.y + 55), 10, BLACK);
                    
                    // rewind, pause, and skip buttons
                    playback_buttons(BOTTOM_BAR_BOUNDS, &music_flags, GetMusicTimePlayed(music));
                    
                    // bar to change music time position
                    progress_bar(BOTTOM_BAR_BOUNDS, &music_flags, &progress_bar_active, &settings.percent_played, GetMusicTimePlayed(music), GetMusicTimeLength(music));
                    
                    // sliders to change music settings
                    music_settings(BOTTOM_BAR_BOUNDS, &settings, &music);
                }
                // displays all songs in the current playlist s.t. when pressed the selected song is played
                {
                    for(int s = 0, y_level = CONTENT_HEIGHT; s < nsongs; s++, y_level += CONTENT_HEIGHT) {  
                        // the area that the pth playlist is contained in
                        const Rectangle content_rect = { 0, (y_level + panelScroll.y), (GetScreenWidth() - (vertical_scroll_visible ? SCROLL_BAR_WIDTH : 0)), CONTENT_HEIGHT };

                        // only display songs that are within the screen
                        const bool under_draw_top_bar = content_rect.y + (content_rect.height / 2.0f) > (TOP_BAR_BOUNDS.y + TOP_BAR_BOUNDS.height);
                        const bool over_bottom_bar = (content_rect.y + (content_rect.height * 0.75f)) < (BOTTOM_BAR_BOUNDS.y);
                        const bool valid_position = (under_draw_top_bar && over_bottom_bar);
                        if(valid_position) {
                            // selecting a song
                            const bool song_selected = (CheckCollisionPointRec(GetMousePosition(), content_rect)) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
                            if(song_selected && song_information[s].information_ready) {
                                // resart song if same song is selected
                                if(s == current_song_index) 
                                    music_flags.play_music = music_flags.restart_song = true;
                                else {
                                    // create a new queue every time you seek a song
                                    if(music_flags.shuffle_play) {
                                        song_history = 0;
                                        create_song_queue(nsongs, s, shuffled_song_queue);
                                    }
                                    // set the current index to index sought
                                    current_song_index = s;
                                    music_flags.play_music = music_flags.change_song = true;
                                }
                            }
                            // displaying song information
                            const Color display_color = (s == current_song_index) ? GREEN : BLACK;
                            const float y = content_rect.y + (content_rect.height / 2.0f);
                            const int font_size = 12;
                            if(song_information[s].information_ready) {
                                char index[4];
                                char duration[LEN];
                                char last_accessed[LEN];
                                snprintf(index, 4, "%3d", (s + 1));
                                seconds_to_last_accessed(song_information[s].access_time, LEN, last_accessed);
                                seconds_to_timestamp(song_information[s].duration, LEN, duration);
                                DrawText(index, 5, y, font_size, display_color);
                                DrawText(song_information[s].title, 30, y, font_size, display_color);        
                                DrawText(song_information[s].artist, (content_rect.width * 0.30f), y, font_size, display_color);
                                DrawText(song_information[s].album, (content_rect.width * 0.50f), y, font_size, display_color);
                                DrawText(last_accessed, (content_rect.width * 0.70f), y, font_size, display_color);
                                DrawText(duration, (content_rect.width * 0.90f), y, font_size, display_color);
                            }
                        }
                    }
                }
            }
            else if(application_state == NO_SONGS_IN_PLAYLIST) {
                char no_song_msg[LEN * 3];
                snprintf(no_song_msg, (LEN * 3), "\"%s\" has no mp3s, add some", playlist_path);
                DrawText(no_song_msg, (GetScreenWidth() / 2.0f) - (MeasureText(no_song_msg, 20) / 2.0f), (GetScreenHeight() / 2.0f), 20, BLACK); 
            }
            else if(application_state == NO_PLAYLIST) {
                char no_playlist_msg[LEN * 2];
                snprintf(no_playlist_msg, (LEN * 2), "\"%s\" has no playlists, create folders of mp3s to act as playlists here", playlist_directory);
                DrawText(no_playlist_msg, (GetScreenWidth() / 2.0f) - (MeasureText(no_playlist_msg, 20) / 2.0f), (GetScreenHeight() / 2.0f), 20, BLACK);
            }
            DrawFPS(0, GetScreenHeight() / 2);
        EndDrawing();
    }
    // dealloc
    if(IsTextureReady(defualt_texture))
        UnloadTexture(defualt_texture);
    for(int i = 0; i < nsongs; i++)
        delete_song_information(&song_information[i]);
    if(external_song_watch.wd)
        deinit_file_watch(&external_song_watch);
    if(external_playlist_watch.wd)
        deinit_file_watch(&external_playlist_watch);
    if(IsMusicReady(music)) {
        if(IsMusicStreamPlaying(music))
            StopMusicStream(music);
        UnloadMusicStream(music);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
#include "headers/stream_management.h"
#include "headers/song_information.h"
#include "headers/file_management.h"
#include "headers/raylib.h"
#include <libavutil/dict.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

float microseconds_to_seconds(float microseconds)
{
    return (microseconds * 0.000001);
}

// given a songname, write the appropraite cover path to dst
int get_cover_path(int nchars, char dst[nchars], const char* song_title, const char* cover_directory_path)
{
    // ensure that the space to write the path is enough, as filenames are case-sensitive
    const int min_len = strlen(cover_directory_path) + strlen(song_title) + strlen(".jpg");
    if(nchars < min_len) {
        printf("get_cover_path, nchars of dst (%d) is smaller than the min length (%d)\n", nchars, min_len);
        memset(dst, 0, nchars);
        return -1;
    } 
    else {
        sprintf(dst, "%s/%s.jpg", cover_directory_path, song_title);
        return 0;
    } 
}

void get_information(SongInformation* song_information, const char* cover_directory, const char* mp3_path)
{
    // get the metadata from the audio path
    AVFormatContext* audio_format_context;
    if((audio_format_context = get_format_context(mp3_path)) == NULL) {
        printf("there was an error getting the format context of the file: %s\n", mp3_path);
        return;
    }

    // the pointer to the mp3's metadata
    AVDictionary* audio_metadata = audio_format_context->metadata;

    const char* parameter_not_availible = "N/A";

    // the path is relative to the playlist the mp3 is found in 
    strcpy(song_information->relative_path, GetFileName(mp3_path));
    get_cover_path(LEN, song_information->cover_path, GetFileNameWithoutExt(song_information->relative_path), cover_directory);

    // song title
    if (get_metadata_parameter(audio_metadata, "title", LEN, song_information->title) < 0) 
        strcpy(song_information->title, GetFileNameWithoutExt(mp3_path));

    // artist
    if (get_metadata_parameter(audio_metadata, "artist", LEN, song_information->artist) < 0) 
        strcpy(song_information->artist, parameter_not_availible);

    // album
    if (get_metadata_parameter(audio_metadata, "album", LEN, song_information->album) < 0) 
        strcpy(song_information->album, parameter_not_availible);

    // track
    char track[LEN];
    if (get_metadata_parameter(audio_metadata, "track", LEN, track) < 0) 
        song_information->track = 999;
    else 
        song_information->track = atoi(track);

    // the duration param of the format context object is in microseconds
    song_information->duration = microseconds_to_seconds(audio_format_context->duration);
    song_information->access_time = file_access_time(mp3_path);
    
    // textures are loaded when needed to save computation
    song_information->cover = (Texture2D){ 0 };

    // update flag
    song_information->information_ready = true;
    avformat_close_input(&audio_format_context);
}

void delete_song_information(SongInformation* song_information)
{
    if(IsTextureReady(song_information->cover))
        UnloadTexture(song_information->cover);
    memset(song_information, 0, sizeof((*song_information)));
}
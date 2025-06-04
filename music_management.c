#include "headers/stream_management.h"
#include "headers/music_management.h"
#include "headers/raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("change_music_stream, \"%s\" does not exist in device\n", songpath);
        return;
    }
    else if(!IsFileExtension(songpath, ".mp3")) {
        printf("change_music_stream, \"%s\" is not a valid audio file\n", songpath);
        return;
    }
    else
        (*music) = LoadMusicStream(songpath);
}

// returns the texture of a song either by finding the jpg in the computer or extracting the image itself  
Texture2D get_song_cover(const char* song_path, const char* cover_path, int image_size)
{
    // issue with extracting jpg from mp3
    if(!FileExists(cover_path) && (extract_cover_image(song_path, cover_path) < 0))
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

void apply_music_settings(Music* music, MusicSettings settings)
{
    SetMusicPan((*music), settings.pan);
    SetMusicPitch((*music), settings.pitch);
    SetMusicVolume((*music), settings.volume);
}

void set_current_song(const char* playlist_path, SongInformation* song_information, Music* music, MusicSettings settings)
{
    char full_song_path[LEN * 3];
    snprintf(full_song_path, (LEN * 3), "%s/%s", playlist_path, song_information->relative_path);
    if(FileExists(full_song_path)) {
        // maintain previous loop state
        const bool loop = music->looping;

        // new cover
        song_information->cover = get_song_cover(full_song_path, song_information->cover_path, song_information->cover_size);
        
        // load new music stream
        change_music_stream(music, full_song_path);
        apply_music_settings(music, settings);
        music->looping = loop;
    }
}
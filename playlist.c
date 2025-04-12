#include "headers/playlist.h"
#include "headers/c_string.h"
#include "headers/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_full_song_path(char* dst, const char* playlist_path, char* song_path)
{
    sprintf(dst, "%s/%s", playlist_path, song_path);
}

void get_cover_path(char* cover_path, const char* song_path, const char* cover_location)
{
    char cover_name[MAX_LEN];
    format_as_display(cover_name, song_path);
    sprintf(cover_path, "%s/%s.jpg", cover_location, cover_name);
} 

void format_as_display(char* dst, const char* file_path)
{
    const char* src = file_path;
    char* result = dst;

    while (*src && (*src != '.'))
    {
        if (*src != '\\')
        {
            *result = *src;
            result++;
        }
        src++;
    }

    *result = '\0';
}

void linux_formatted_filename(char* dst, const char* file_path)
{
    const char* DELIM = " ()&'`\\!";  
    const char* replacement_characters[] = {
        "\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\\", "\\!" 
    };
    
    int length = 0;
    for (int i = 0; file_path[i] != '\0'; i++)
    {
        char* found = strchr(DELIM, file_path[i]);
        if (found != NULL)
        {
            int index = found - DELIM;
            const char* replacement = replacement_characters[index];
            int characters_written = snprintf(dst + length, MAX_LEN - length, "%s", replacement); 
            length += characters_written;
        }
        else
        {
            dst[length++] = file_path[i];
        }
    }
    dst[length] = '\0';
}

void load_playlists_from_folder(list* playlists, const char* music_folder)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "ls -1d %s/*", music_folder);
    char buffer[MAX_LEN];
    char linux_formatted_playlist_path[MAX_LEN];
    FILE* file_ptr = popen(command, "r");
    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL))
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        playlist* pl = malloc(sizeof(playlist));
        // linux_formatted_filename(linux_formatted_playlist_path, buffer);
        // strcpy(pl->path, linux_formatted_playlist_path);
        strcpy(pl->path, buffer);
        add_element(playlists, pl);
        free(pl);
    }
    pclose(file_ptr);
}

void load_songs_from_playlist(list* songs, const char* playlist_path)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "cd %s; ls -1", playlist_path);

    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");
    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL))
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        string temp = create_string(buffer);
        add_element(songs, &temp);
    }
    pclose(file_ptr);
}

void extract_song_cover(const char* playlist_path, const char* song_path, const char* cover_location)
{
    char cmd[MAX_LEN];
    char cover_path[MAX_LEN];

    char linux_formatted_song_path [MAX_LEN];
    char linux_formatted_playlist_path [MAX_LEN];
    
    FILE* cover_directory_ptr = fopen(cover_location, "r");
    if(cover_directory_ptr == NULL)
    {
        snprintf(cmd, sizeof(cmd), "mkdir %s", cover_location);
        system(cmd);
    }
    else
        fclose(cover_directory_ptr);
    
    get_cover_path(cover_path, song_path, cover_location);
    FILE* ptr = fopen(cover_path, "r");
    if(ptr == NULL)
    {
        linux_formatted_filename(linux_formatted_playlist_path, playlist_path);
        linux_formatted_filename(linux_formatted_song_path, song_path);
        snprintf(cmd, sizeof(cmd), "ffmpeg -i %s/%s -vf scale=250:-1 \"%s\" -loglevel error", linux_formatted_playlist_path, linux_formatted_song_path, cover_path);
        system(cmd);
    }
    else
        fclose(ptr);
}
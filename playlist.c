#include "headers/playlist.h"
#include "headers/c_string.h"
#include "headers/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_full_song_path(char* dst, char* playlist_path, char* song_path)
{
    char* playlist = format_as_display(playlist_path);
    sprintf(dst, "%s/%s", playlist, song_path);
    free(playlist);
}

void get_cover_path(char cover_path[MAX_LEN], char* song_path, const char* cover_location)
{
    char* cover_name = format_as_display(song_path);
    sprintf(cover_path, "%s/%s.png", cover_location, cover_name);
    free(cover_name);
} 

char* format_as_display(char* file_path)
{
    char* result = malloc(sizeof(char) * strlen(file_path) + 1);
    char* src = file_path;
    char* dst = result;

    while (*src && (*src != '.'))
    {
        if (*src != '\\')
        {
            *dst = *src;
            dst++;
        }
        src++;
    }

    *dst = '\0';
    result = realloc(result, sizeof(char) * (strlen(result) + 1));
    return result;
}

char* linux_formatted_filename(const char* file_path)
{
    const char* DELIM = " ()&'`\\!";
    char* replacement_characters[] = {"\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\"", "\\!"};
    
    int length = 0;
    char* linux_file_path = malloc(sizeof(char) * MAX_LEN);
    for(int i = 0; i < strlen(file_path); i++)
    {
        char* found = strchr(DELIM, file_path[i]);
        if(found != NULL)
        {
            int index = found - DELIM;
            char* dest = linux_file_path + length;
            char* replacement = replacement_characters[index];
            int characters_written = snprintf(dest, (dest - linux_file_path), "%s", replacement); 
            length += (characters_written < MAX_LEN - length) ? characters_written : (MAX_LEN - length - 1);
        }

        else
            linux_file_path[length++] = file_path[i];
    }
    linux_file_path[length] = '\0';
    linux_file_path = realloc(linux_file_path, sizeof(char) * (length + 1));

    return linux_file_path;
}

void load_playlists_from_folder(list* playlists, const char* music_folder)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "ls -1d %s/*", music_folder);
    
    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");
    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL))
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        playlist playlist;
        playlist.path = linux_formatted_filename(buffer);
        add_element(playlists, &playlist);
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

void extract_song_cover(const char* playlist_path, char* song_path, const char* cover_location)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir %s", cover_location);
    system(cmd);
    
    char cover_path[MAX_LEN];
    get_cover_path(cover_path, song_path, cover_location);
    FILE* ptr = fopen(cover_path, "r");
    if(ptr == NULL)
    {
        char* linux_formatted_song_path = linux_formatted_filename(song_path);
        snprintf(cmd, sizeof(cmd), "ffmpeg -i %s/%s -an -vcodec copy \"%s\"", playlist_path, linux_formatted_song_path, cover_path);
        system(cmd);
        free(linux_formatted_song_path);
    }
    else
        fclose(ptr);
}
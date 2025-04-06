#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"

#include "headers/raylib.h"
#include "headers/c_string.h"
#include "headers/list.h"
#include <stdio.h>

const char* PLAYLIST_DIR = "$HOME/Music";
const char* MP3_COVER_PATH = "$HOME/.cache/mp3_covers/";

typedef struct
{
    char path[MAX_LEN];
    list song_paths;
}playlist;

char* english_format(char* file_path)
{
    static char result[MAX_LEN];
    memset(result, 0, sizeof(result));

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
    return result;
}

char* linux_format(const char* file_path)
{
    const char* DELIM = " ()&'`\\!";
    char* replacement_characters[] = {"\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\"", "\\!"};
    
    int length = 0;
    static char linux_file_path[MAX_LEN];
    memset(linux_file_path, 0, sizeof(linux_file_path));
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
        {
            linux_file_path[length++] = file_path[i];
        }
    }

    linux_file_path[length] = '\0';
    return linux_file_path;
}

void get_playlist_paths(playlist* playlists)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "ls -1d %s/*", PLAYLIST_DIR);

    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");

    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL))
    {
        int stopping_point = strcspn(buffer, "\n");
        buffer[stopping_point] = '\0';
        sprintf(playlists->path, "%s", linux_format(buffer));
    }
    pclose(file_ptr);
}

void get_songs_from_playlist(list* song_paths, const char* playlist_path)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "cd %s; ls -1", playlist_path);

    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");
    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL))
    {
        int stopping_point = strcspn(buffer, "\n");
        buffer[stopping_point] = '\0';
        string temp = create_string();
        update_string(&temp, linux_format(buffer));
        add_element(song_paths, &temp);
    }
    pclose(file_ptr);
}

void get_song_cover(const char* playlist_path, char* song_path)
{
    char cmd[MAX_LEN];
    // todo: only make directory when its not here
    snprintf(cmd, sizeof(cmd), "mkdir %s", MP3_COVER_PATH);
    system(cmd);

    char* english = english_format(song_path);
    char image_path[MAX_LEN];
    sprintf(image_path, "%s/.cache/mp3_covers/%s.jpg", getenv("HOME"), english);
    
    FILE* ptr = fopen(image_path, "r");
    if(ptr == NULL)
    {
        snprintf(cmd, sizeof(cmd), "ffmpeg -i %s/%s -an -vcodec copy \"%s%s.jpg\" -loglevel error", playlist_path, song_path, MP3_COVER_PATH, english);
        system(cmd);
    }
    else
    {
        fclose(ptr);
    }
}

int main()
{
    // TODO, make list of a playlist type
    // create library struct
    // mayb clean the reading command loop repetitivness

    playlist playlist;
    playlist.song_paths = create_list(STRING);
    
    get_playlist_paths(&playlist);
    // get_songs_from_playlist(&playlist.song_paths, playlist.path);
    // get_song_cover(playlist.path, ((string*)playlist.song_paths.elements)[0].value);
    free_list(&playlist.song_paths);
}
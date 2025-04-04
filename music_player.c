#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"

#include "headers/raylib.h"
#include "headers/c_string.h"
#include "headers/list.h"

#define N_PLAYLIST 10
const char* MUSIC_PATH = "$HOME/Music";

typedef struct
{
    char path[MAX_LEN];
    list song_paths;
}playlist;

typedef struct
{
    playlist* playlists;
}library;

char* linux_format(char* file_path)
{
    const char* DELIM = " ()&'`\\";
    char* replacement_characters[] = {"\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\""};

    list valid_path_chunks = create_list(STRING);
    list replacements = create_list(INT);

    // get a list of replacement chars to be appended with the valid path parts
    for(int i = 0; i < strlen(file_path); i++)
    {
        char* found_position = strchr(DELIM, file_path[i]);
        if(found_position != NULL)
        {
            int replacement_index = found_position - DELIM;
            add_element(&replacements, &replacement_index);
        }
    }

    // partion the valid characters of the filepath
    char* buffer = strtok(file_path, DELIM);   
    while(buffer != NULL)
    {
        string str = create_string();
        update_string(&str, buffer);
        add_element(&valid_path_chunks, &str);
        buffer = strtok(NULL, DELIM);
    }

    // combine the chunks and the replacement parts into a string
    char linux_file_path[MAX_LEN];
    int length = 0;
    int a = 0; int b = 0;
    while((a < valid_path_chunks.size) || (b < replacements.size))
    {
        if(a < valid_path_chunks.size)
        {
            char* chunk = ((string*)valid_path_chunks.elements)[a++].value;
            // snprint returns the length of chars appended
            length += snprintf((linux_file_path + length), sizeof(linux_file_path) - length, "%s", chunk);
        }

        if(b < replacements.size)
        {
            int replacement_index = ((int*)replacements.elements)[b++];
            length += snprintf((linux_file_path + length), sizeof(linux_file_path) - length, "%s", replacement_characters[replacement_index]);
        }
    }
    
    if(strlen(linux_file_path) >= MAX_LEN)
    {
        printf("%s exceeds max length", linux_file_path);
        return "";
    }

    free_list(&valid_path_chunks);
    free_list(&replacements);

    strcpy(file_path, linux_file_path);
    return file_path;
}

void get_playlist_paths(playlist* playlist)
{
    char current_directory[MAX_LEN];
    if(getcwd(current_directory, sizeof(current_directory)) == NULL)
    {
        printf("error retreiving the current directory\n");
        return;
    }

    // print out the filepaths of every playlist
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "ls -1d %s/*", MUSIC_PATH);

    int count = 0;
    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");

    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL) && (count < N_PLAYLIST))
    {
        int stopping_point = strcspn(buffer, "\n");
        buffer[stopping_point] = '\0';
        strcpy(playlist->path, linux_format(buffer));
        count++;
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
        printf("%s\n", ((string*)song_paths->elements)[song_paths->size - 1].value);
    }
    pclose(file_ptr);
}

int main()
{
    playlist playlists;
    playlists.song_paths = create_list(STRING);

    get_playlist_paths(&playlists);
    get_songs_from_playlist(&playlists.song_paths, playlists.path);

    free_list(&playlists.song_paths);
}
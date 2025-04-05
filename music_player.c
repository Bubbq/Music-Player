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
    char* path;
    list song_paths;
}playlist;

char* linux_format(char* file_path)
{
    const char* DELIM = " ()&'`\\";
    char* replacement_characters[] = {"\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\""};
    
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
            length += snprintf(dest, (dest - linux_file_path), "%s", replacement); // snprintf returns the number of characters written
        }

        else
        {
            linux_file_path[length++] = file_path[i];
        }
    }

    return linux_file_path;
}

// print out the filepaths of every playlist
void get_playlist_paths(playlist* playlists)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "ls -1d %s/*", MUSIC_PATH);

    int count = 0;
    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");

    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL) && (count < N_PLAYLIST))
    {
        int stopping_point = strcspn(buffer, "\n");
        buffer[stopping_point] = '\0';
        playlists[count].path = linux_format(buffer);
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
    // TODO, make list of a playlist type
    // create library struct
    // mayb clean the reading command loop repetitivness
    // grab images from mp3

    playlist library[N_PLAYLIST];
    get_playlist_paths(library);
    for(int i = 0; i < 3; i++)
    {
        printf("%s\n", library[i].path);
    }
    // playlists.song_paths = create_list(STRING);

    // get_songs_from_playlist(&playlists.song_paths, playlists.path);

    // free_list(&playlists.song_paths);
}
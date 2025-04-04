#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "headers/raylib.h"
#include <stdio.h>

#define MAX_LEN 256
#define N_PLAYLIST 10
const char* MUSIC_PATH = "$HOME/Music";

typedef struct
{
    char file_path[MAX_LEN];
}Playlist;

// typedef struct
// {
//     Playlist* list;
//     int current_playlist_index;
// }Library;

char* get_linux_format(char file_path[MAX_LEN])
{
    int n = 7;
    char* bad_character[] = {" ", "(", ")", "&", "'", "`", "\\"};
    char* replacement_character[] = {"\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\""};
    char* copy = file_path;
    
    // TODO: convert string into linux format
    
    
    return copy;
}

void get_playlist_paths(Playlist* playlists)
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
     
        strcpy(playlists[count].file_path, buffer);
        printf("%s\n", playlists[count].file_path);
        count++;
    }

    pclose(file_ptr);
}

int main()
{
    Playlist playlist[N_PLAYLIST];
    // SetTraceLogLevel(LOG_ERROR);
    // InitWindow(1000, 1000, "music_player");
    // SetTargetFPS(60);
    get_playlist_paths(playlist);
    // while(!WindowShouldClose())
    // {
    //     BeginDrawing();
    //         DrawFPS(0, 0);
    //         ClearBackground(BLACK);
    //     EndDrawing();
    // }
    // CloseWindow();
    return 0;
}
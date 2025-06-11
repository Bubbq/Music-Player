#include "raylib.h"
#include <stdbool.h>

#define LEN 256

typedef struct
{
    // the extracted image of the mp3
    Texture2D cover;
    int cover_size;
    char cover_path[LEN]; 
    
    // metadata information of the mp3 (see ffprobe)
    int track;
    float duration;
    char title[LEN];
    char album[LEN];
    char artist[LEN]; 
    
    // basic file information
    float modified_time;
    char relative_path[LEN]; 

    // flag detailing wether the information above has been loaded or not
    bool information_ready; 
} SongInformation;

// given a songname, write the appropraite cover path to dst
int get_cover_path(int nchars, char dst[nchars], const char* song_title, const char* cover_directory_path);

// given the path to an mp3, write the SongInformation equivalent to dst
void get_information(SongInformation* dst, const char* cover_directory, const char* mp3_path, int cover_size);

// clears the information of SongInformation obj
void delete_song_information(SongInformation* song_information);
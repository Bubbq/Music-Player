#include "headers/file_management.h"
#include <time.h>
#include <stdio.h>

void make_directory(const char* directory_path)
{
    if((mkdir(directory_path, 0755)) == -1)
        printf("make_directory");
}

// returns the time in seconds since last modification of a file
float file_modification_time(const char* file)
{
    const time_t right_now = time(NULL);
    struct stat st;
    if(stat(file, &st) == 0) 
        return difftime(right_now, st.st_mtim.tv_sec);
    else
        return -1;
}
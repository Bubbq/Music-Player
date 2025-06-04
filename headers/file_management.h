#include <stdbool.h>
#include <sys/stat.h>

// returns the number of files in 'folder_path' of type 'file_type' stored in 'dst'
int get_files_from_folder(int maxlen, int maxfiles, char dst[maxfiles][maxlen], const char* folder_path, const char file_type);

// removes all non files that are not of an 'extension'
int filter_library(int maxlen, int nsongs, char library[nsongs][maxlen], const char* extension);

// makes the directory specified in 'directory_path'
void make_directory(const char* directory_path);

// returns the time in seconds since last access of a file
float file_access_time(const char* file);
#include <sys/stat.h>

// makes the directory specified in 'directory_path'
void make_directory(const char* directory_path);

// returns the time in seconds since last modification of a file
float file_modification_time(const char* file);
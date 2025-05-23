#include <sys/inotify.h>
#include <stdbool.h>

typedef struct 
{
    int fd;
    int wd;
    struct inotify_event* event;
} FileWatch;

void init_file_watch(FileWatch* file_watch, const char* filepath, const uint32_t signal);
void deinit_file_watch(FileWatch* file_watch);
int file_event(FileWatch* file_watch);
void print_inotify_event(struct inotify_event* event);
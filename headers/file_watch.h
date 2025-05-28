#include <sys/inotify.h>
#include <stdbool.h>
#define LEN 256
#define MAX_EVENTS 250
typedef struct
{
    char file_name[LEN];
    struct inotify_event event;
} FileEvent;

typedef struct 
{
    int fd;
    int wd;
    bool reading_events;
    int nevents;
    FileEvent events[MAX_EVENTS];
} FileWatch;

void init_file_watch(FileWatch* file_watch, const char* filepath, const uint32_t signal);
void deinit_file_watch(FileWatch* file_watch);
int file_event(FileWatch* file_watch);
void print_inotify_event(struct inotify_event* event, const char* file_name);
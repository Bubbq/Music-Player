#include <sys/inotify.h>
#include <stdbool.h>
#define LEN 256
#define MAX_EVENTS 100
typedef struct
{
    char file_name[LEN];
    struct inotify_event event;
} FileEvent;

typedef struct 
{
    int fd;
    int wd;
    int nevents;
    bool reading_events;
    long int nframes_reading;
    FileEvent events[MAX_EVENTS];
} FileWatch;

void init_file_watch(FileWatch* file_watch, const char* filepath, const uint32_t signal);
void deinit_file_watch(FileWatch* file_watch);
void file_event(FileWatch* file_watch);
void print_inotify_event(const struct inotify_event* event, const char* file_name);
int watch_events(int maxevents, int starting_point, FileEvent events[maxevents], int fd);
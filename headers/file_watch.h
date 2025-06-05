#include <stdbool.h>
#include <sys/inotify.h>

#define LEN 256
#define MAX_EVENTS 256

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

// writes up to 'maxevents' into FileEvennt array, given an inital position reading from a passed file descriptor
int read_events(int maxevents, int starting_point, FileEvent events[maxevents], int fd);

// create and unloading an instance of this object
void init_file_watch(FileWatch* file_watch, const char* filepath, const uint32_t signal);
void deinit_file_watch(FileWatch* file_watch);

// returns true when its time to process external events
bool process_external_events(const int maxevents, const int frames_to_read, FileWatch* file_watch);

// for debugging
void print_inotify_event(const struct inotify_event* event, const char* file_name);
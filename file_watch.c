#include "headers/file_watch.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>

// the size of the inotify event does not include the 'name' parameter
#define EVENT_SIZE sizeof(struct inotify_event)
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + LEN))

void init_file_watch(FileWatch* file_watch, const char* filepath, const uint32_t signal)
{
    // creates an inotify instance inside the kernel and returns the associated file descriptor
    file_watch->fd = inotify_init1(IN_NONBLOCK);
    if(file_watch->fd < 0) 
        perror("init_file_watch, inotify_init1");
    // returns watch descriptor that specifices to inotify what signals to watch out for in the given filepath 
    file_watch->wd = inotify_add_watch(file_watch->fd, filepath, signal);
    if(file_watch->wd < 0)
        perror("init_file_watch, inotify_add_watch");
    file_watch->nevents = 0;
    file_watch->reading_events = false;
}

void deinit_file_watch(FileWatch* file_watch)
{
    file_watch->nevents = 0;
    inotify_rm_watch(file_watch->fd, file_watch->wd);
    close(file_watch->fd);
}

int watch_events(int maxevents, int starting_point, FileEvent events[maxevents], int fd) 
{
    int events_read = 0;

    // buffer can read up to 'maxevents' events
    //  the 'name' param of inotify event is not included in event size, thus, 'LEN' is added
    char buffer[maxevents * (EVENT_SIZE + LEN)];

    // read events when they become availible
    const int chars_read = read(fd, buffer, sizeof(buffer));
    
    // when nothing is read, read returns -1
    if(chars_read < 0) {
        const bool no_events_read = ((errno == EAGAIN )|| (errno == EWOULDBLOCK));
        if(no_events_read) 
            return 0;
        else {
            perror("watch_events, read");
            return 0;
        }
    } 

    // iter for buffer
    int i = 0;
    // iter for dst array
    int j = starting_point;
    while((i < chars_read) && (j < maxevents)) {
        struct inotify_event* new_event = (struct inotify_event*) &buffer[i];
        memcpy(&events[j].event, new_event, EVENT_SIZE);
        strcpy(events[j].file_name, new_event->name);
        j++;
        events_read++;
        i += (EVENT_SIZE + new_event->len);
    }

    return events_read;
}

void print_inotify_event(const struct inotify_event* event, const char* file_name) 
{
    printf("wd=%d mask=%u cookie=%u len=%u name=%s\n",
        event->wd, event->mask,event->cookie, event->len, file_name);
}
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "headers/file_watch.h"
// the size of the inotify event does not include the 'name' parameter
#define EVENT_SIZE sizeof(struct inotify_event)
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + LEN))

void init_file_watch(FileWatch* file_watch, const char* filepath, const uint32_t signal)
{
    // creates an inotify instance inside the kernel and returns the associated file descriptor
    file_watch->fd = inotify_init1(IN_NONBLOCK);
    if(file_watch->fd < 0) {
        perror("inotify_init1");
        exit(1);
    }
    // returns watch descriptor that specifices to inotify whats signals to watch out for in the given filepath 
    file_watch->wd = inotify_add_watch(file_watch->fd, filepath, signal);
    if(file_watch->wd < 0) {
        perror("inotify_add_watch");
        exit(2);
    }
    file_watch->nevents = 0;
    file_watch->reading_events = false;
}

void deinit_file_watch(FileWatch* file_watch)
{
    file_watch->nevents = 0;
    inotify_rm_watch(file_watch->fd, file_watch->wd);
    close(file_watch->fd);
}

void file_event(FileWatch* file_watch)
{
    int total_read = 0;
    char buffer[BUF_LEN];

    // read events when they become availible
    int len = read(file_watch->fd, buffer, sizeof(buffer));
    if (len < 0) {
        const bool no_events = ((errno == EAGAIN )|| (errno == EWOULDBLOCK));
        if (no_events) {
            // break;
            return;
        }
        else {
            perror("file_event / read");
            return;
        }
    }

    // found an event in the filepath, start the counting sequence
    file_watch->nframes_reading = 0;
    file_watch->reading_events = true;

    int i = 0;
    while ((i < len) && (file_watch->nevents < MAX_EVENTS)) {
        struct inotify_event* event = (struct inotify_event*) &buffer[i];
        file_watch->events[file_watch->nevents].event = (*event);
        strcpy(file_watch->events[file_watch->nevents].file_name, event->name);
        file_watch->nevents++;
        i += EVENT_SIZE + event->len;
        total_read++;
    }
}

void print_inotify_event(struct inotify_event* event, const char* file_name) 
{
    printf("wd=%d mask=%u cookie=%u len=%u name=%s\n",
        event->wd, event->mask,event->cookie, event->len, file_name);
}
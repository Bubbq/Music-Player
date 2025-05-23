#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "headers/file_watch.h"

#define LEN 256
// the size of the inotify event does not include the 'name' parameter
#define EVENT_SIZE sizeof(struct inotify_event)
#define BUF_LEN (EVENT_SIZE + LEN)

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

    file_watch->event = NULL;
}

void deinit_file_watch(FileWatch* file_watch)
{
    if(file_watch->event) {
        free(file_watch->event);
        file_watch->event = NULL;
    }
    inotify_rm_watch(file_watch->fd, file_watch->wd);
    close(file_watch->fd);
}

// writes the newest inotify event to the filewatch
int file_event(FileWatch* file_watch)
{
    // read the the event (if any) to the buffer
    char buffer[BUF_LEN];
    int len = read(file_watch->fd, buffer, BUF_LEN);
    if(len < 0) {
        const bool no_event = (errno == EAGAIN || errno == EWOULDBLOCK);
        if(no_event) 
            return -1;
        else {
            perror("read");
            return -1;
        }
    }

    // copy the new event to the event parameter in filewatch
    struct inotify_event* event = (struct inotify_event*) &buffer;
    file_watch->event = malloc((EVENT_SIZE + event->len));
    if(!file_watch->event) {
        perror("malloc");
        return -1;
    }
    
    memcpy(file_watch->event, event, (EVENT_SIZE + event->len));
    return 0;
}

void print_inotify_event(struct inotify_event* event) 
{
    printf("wd=%d mask=%u cookie=%u len=%u\n",
        event->wd, event->mask,
        event->cookie, event->len);

    if(event->len)
        printf ("name=%s\n", event->name);
}
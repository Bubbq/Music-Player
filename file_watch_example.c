#include "headers/file_watch.h"
#include <stdio.h>
#include <sys/inotify.h>
#include "time.h"

int main()
{
    const char* path = "./";
    const uint32_t signal = (IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

    FileWatch file_watch;
    init_file_watch(&file_watch, path, signal);

    const int threshold = 3;

    while(1) {
        struct timespec ts = {0, 100 * 1000000}; // 100ms each frame
        nanosleep(&ts, NULL);

        const int events_read = watch_events(MAX_EVENTS, file_watch.nevents, file_watch.events, file_watch.fd);
        file_watch.nevents += events_read; 
        
        // reading new event
        if((file_watch.reading_events == false) && (events_read > 0)) {
            file_watch.nframes_reading = 0;
            file_watch.reading_events = true;
        }
        // reading from previous event
        else if(file_watch.reading_events)
            file_watch.nframes_reading++;
        
        // when you stop updating, print everything
        if(file_watch.reading_events && file_watch.nframes_reading >= threshold) {
            printf("What happened in %s:\n", path);
            for(int i = 0; i < file_watch.nevents; i++) {
                printf("%d) ", (i + 1));
                print_inotify_event(&((file_watch.events + i)->event), file_watch.events[i].file_name);
            }
            printf("\n");
            file_watch.nevents = 0;
            file_watch.reading_events = false;
        }
    }


    deinit_file_watch(&file_watch);    
    return 0;
}
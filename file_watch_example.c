#include "headers/file_watch.h"

#include <time.h>
#include <stdio.h>
#include <sys/inotify.h>

/*This program was created to better show the FileWatch object in a much simplier envoirnment*/

int main()
{
    // this example is watching events happening in the folder this file is in 
    const char* path = "./";
    
    // any instance of file creation or deletion in 'path' will be processed
    const uint32_t signal = (IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

    FileWatch file_watch;
    init_file_watch(&file_watch, path, signal);

    // the amount of frames the FileWatch obj will read before processing
    const int threshold = 5;

    while(1) {
        struct timespec ts = {0, 33.33 * 1000000}; // simulating 33.33ms each frame (or 30 FPS)
        nanosleep(&ts, NULL);

        // write up to MAXEVENTS events in the dst array from some stop position
        const int events_read = watch_events(MAX_EVENTS, file_watch.nevents, file_watch.events, file_watch.fd);
        
        // init/append the array size
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
            for(int i = 0; i < file_watch.nevents; i++) 
                print_inotify_event(&file_watch.events[i].event, file_watch.events[i].file_name);
            
            printf("\n");

            // reset the reading params
            file_watch.nevents = 0;
            file_watch.reading_events = false;
        }
    }
    deinit_file_watch(&file_watch);    
    return 0;
}
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

    while(1) {
        struct timespec ts = {0, 100 * 1000000}; 
        nanosleep(&ts, NULL);

        bool update = file_watch.updating;
        file_watch.updating = file_event(&file_watch);

        // when you stop updating, print everything
        if(update && !file_watch.updating) {
            for(int i = 0; i < file_watch.nevents; i++) {
                printf("%d)", i+1);
                print_inotify_event(&((file_watch.events + i)->event), file_watch.events[i].file_name);
            }
            printf("\n\n");
            file_watch.nevents = 0;
        }
    }

    deinit_file_watch(&file_watch);    
    return 0;
}
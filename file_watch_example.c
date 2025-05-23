#include "headers/file_watch.h"
#include <stdio.h>
#include <sys/inotify.h>

int main()
{
    const char* path = "./";
    const uint32_t signal = (IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

    FileWatch file_watch;
    init_file_watch(&file_watch, path, signal);

    while(1) {
        if(file_event(&file_watch) == 0) 
            print_inotify_event(file_watch.event);
    }

    deinit_file_watch(&file_watch);    
    return 0;
}
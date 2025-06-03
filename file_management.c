#include "headers/file_management.h"
#include "headers/raylib.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>

int get_files_from_folder(int nchars, int max_elements, char dst[max_elements][nchars], const char* folder_path, const char file_type)
{
    DIR* pDIR = opendir(folder_path);
    struct dirent* pDirent = NULL;

    if(!pDIR) {
        printf("get_files_from_folder, %s was not able to be open", folder_path);
        return 0;
    }

    // read all non hidden files that are of a matching file_type into dst
    int files_read = 0;
    while(((pDirent = readdir(pDIR)) != NULL) && (files_read < max_elements)) 
        if((pDirent->d_name[0] != '.') && (pDirent->d_type == file_type) && (strlen(pDirent->d_name) < nchars))
            strcpy(dst[files_read++], pDirent->d_name);

    closedir(pDIR);
    return files_read;
}

int filter_library(int nchars, int nelements, char library[nelements][nchars], const char* extension)
{
    int k = 0;
    for(int i = 0; i < nelements; i++) {
        if(IsFileExtension(library[i], extension)) {
            if(k != i)
                strcpy(library[k], library[i]);
            k++;
        }
    }
    return k;
}

void make_directory(const char* directory_path)
{
    if((mkdir(directory_path, 0755)) == -1)
        printf("make_directory");
}
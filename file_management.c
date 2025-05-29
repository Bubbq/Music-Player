#include "headers/file_management.h"
#include "headers/raylib.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>

int get_files_from_folder(int size, int maxfiles, char dst[maxfiles][size], const char* folder_path, const char file_type)
{
    DIR* pDIR = NULL;
    struct dirent* pDirent = NULL;

    if((pDIR = opendir(folder_path)) == NULL) {
        printf("%s was not able to be opened\n", folder_path);
        return 0;
    }

    // read all non hidden files that are of a matching file_type
    int files_read = 0;
    while(((pDirent = readdir(pDIR)) != NULL) && (files_read < maxfiles)) {
        if((pDirent->d_name[0] != '.') && (pDirent->d_type == file_type) && (strlen(pDirent->d_name) < size))
            strcpy(dst[files_read++], pDirent->d_name);
    }

    closedir(pDIR);
    return files_read;
}

int filter_library(int size, int nsongs, char library[nsongs][size], const char* extension)
{
    int k = 0;
    for(int i = 0; i < nsongs; i++) {
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
        printf("there was an error making the directory \"%s\"\n", directory_path);
}

int linux_format(int maxlen, char dst[maxlen], const char* path)
{
    // characters in linux that need escape sequences 
    const char BAD_CHARS [] = " ()&'`\\!\"$*?#{}[]|;<>~";

    // their corresponding replacement
    const char* REPLACE[] = {
        "\\ ",  // space
        "\\(",  // (
        "\\)",  // )
        "\\&",  // &
        "\\'",  // '
        "\\`",  // `
        "\\\\", // backslash
        "\\!",  // !
        "\\\"", // "
        "\\$",  // $
        "\\*",  // *
        "\\?",  // ?
        "\\#",  // #
        "\\{",  // {
        "\\}",  // }
        "\\[",  // [
        "\\]",  // ]
        "\\|",  // |
        "\\;",  // ;
        "\\<",  // <
        "\\>",  // >
        "\\~"   // ~
    };

    int n_characters = strlen(path);
    int length = 0; // current index of dst
    char* next_invalid = NULL; // a ptr to an invalid char in BAD_CHARS

    for(int i = 0; i < n_characters; i++, (next_invalid = strchr(BAD_CHARS, path[i]))) {
        
        // if the ith char is a bad one, overwrite with the corresp replacement string
        if(next_invalid) {

            int replacement_index = next_invalid - BAD_CHARS;
            const char* replacement = REPLACE[replacement_index];
            int replacement_length = strlen(replacement);

            if(length + replacement_length < maxlen - 1) {
                strcpy(dst + length, replacement);
                length += replacement_length;
            }
            
            else
                return 0;
        }

        else if(length < maxlen - 1)
            dst[length++] = path[i];

        else
            return 0;
    }

    dst[length] = '\0';
    return length;
}

char* get_relative_path(const char filepath[])
{
    char* final_forward_slash = strrchr(filepath, '/');
    if(final_forward_slash)
        return (final_forward_slash + 1);
    else {
        perror("get_relative_path");
        return NULL;
    }
}
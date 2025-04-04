#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "headers/raylib.h"

#define MAX_LEN 512
#define N_PLAYLIST 10
const char* MUSIC_PATH = "$HOME/Music";

typedef struct
{
    char file_path[MAX_LEN];
}Playlist;

typedef struct
{
    int length;
    char* value;
} string;

string create_string(char* str)
{
    string string;
    string.length = strlen(str);
    string.value = malloc(sizeof(char) * MAX_LEN);
    strcpy(string.value, str);
    
    return string;
}

void update_string(string* str, char* new_str)
{
    if(new_str == NULL)
        return;
    else if(sizeof(new_str) > (sizeof(char) * MAX_LEN))
    {
        perror("new string is larger than the string struct's max size");
        return;
    }
    
    strcpy(str->value, new_str);
    str->length = strlen(str->value);
}

int get_string_length(string* str)
{
    str->length = strlen(str->value);
    return str->length;
}

typedef struct
{
    int size;
    void* elements;
    size_t mem_capacity;
} list;

size_t get_size(char type)
{
    switch (type)
    {
        case 's': 
            return sizeof(string); 
            break;
        case 'i':
            return sizeof(int);
        default: 
            return 0;
    }
}

list create_list(char type)
{
    list list;
    list.size = 0;

    list.mem_capacity = get_size(type);
    list.elements = malloc(list.mem_capacity);

    return list;
}

void resize_list(list* list)
{
    list->mem_capacity *= 2;
    list->elements = realloc(list->elements, list->mem_capacity);
}

void add_list_element(list* list, void* input, char type)
{
    if((list->size * get_size(type)) == list->mem_capacity)
    {
        resize_list(list);
    }

    void* ptr;
    switch (type)
    {
        case 's': 
            ptr = ((string*)list->elements) + list->size++;
            memcpy(ptr, (string*)input, sizeof(string));
            break;
        case 'i':
            ptr = ((int*)list->elements) + list->size++;
            memcpy(ptr, (int*)input, get_size(type));
        default:
            return;
    }
}

void print_list(list list, char type)
{
    for(int i = 0; i < list.size; i++)
    {
        switch (type)
        {
            case 's': 
                printf("%s\n", ((string*)list.elements)[i].value);
                break;
            case 'i':
                printf("%d\n", ((int*)list.elements)[i]);
                break;
            default:
                return;
        }
    }
}

void free_list(list* list)
{
    free(list->elements);
}

char* get_linux_format(char file_path[MAX_LEN])
{
    const char* DELIM = " ()&'`\\";
    char* replacement_characters[] = {"\\ ", "\\(", "\\)", "\\&", "\\'", "\\`", "\\\""};

    list valid_path_chunks = create_list('s');
    list replacements = create_list('i');
    string str = create_string("");

    // get a list of replacement chars to be appended with the valid path parts
    for(int i = 0; i < strlen(file_path); i++)
    {
        char c = file_path[i];

        for(int j = 0; j < strlen(DELIM); j++)
        {
            if(c == DELIM[j])
            {
                int replacement_index = j;
                add_list_element(&replacements, &replacement_index, 'i');
            }
        }
    }
    // print_list(replacements, 'i');

    // get list of valid chunks of the filepath TODO: fix string value storege, its taking all into one string?
    char* buffer = strtok(file_path, DELIM);   

    while(buffer != NULL)
    {
        update_string(&str, buffer);
        add_list_element(&valid_path_chunks, &str, 's');
        buffer = strtok(NULL, DELIM);
    }
    // printf("%d\n", valid_path_chunks.size);
    // print_list(valid_path_chunks, 's');

    // dealloc
    free_list(&valid_path_chunks);
    free_list(&replacements);
    free(str.value);

    return file_path;
}

void get_playlist_paths(Playlist playlists)
{
    char current_directory[MAX_LEN];
    if(getcwd(current_directory, sizeof(current_directory)) == NULL)
    {
        printf("error retreiving the current directory\n");
        return;
    }

    // print out the filepaths of every playlist
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "ls -1d %s/*", MUSIC_PATH);

    int count = 0;
    char buffer[MAX_LEN];
    FILE* file_ptr = popen(command, "r");

    while((fgets(buffer, sizeof(buffer), file_ptr) != NULL) && (count < N_PLAYLIST))
    {
        int stopping_point = strcspn(buffer, "\n");
        buffer[stopping_point] = '\0';
        strcpy(playlists.file_path, get_linux_format(buffer));
        // printf("%s\n", playlists[count].file_path);
        count++;
    }

    pclose(file_ptr);
}

int main()
{
    Playlist playlists;
    get_playlist_paths(playlists);
}
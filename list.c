#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/list.h"
#include "headers/c_string.h"
#include "headers/playlist.h"
size_t get_size(type type)
{
    switch (type)
    {
        case INT: return sizeof(int);
        case STRING: return sizeof(string); 
        case PLAYLIST: return sizeof(playlist);
        default: 
            return -1;
    }
}

list create_list(type type)
{
    list list;
    list.size = 0;
    list.type = type;
    list.memory_capacity = get_size(list.type);
    list.elements = malloc(list.memory_capacity);

    return list;
}

void resize_list(list* list)
{
    list->memory_capacity *= 2;
    list->elements = realloc(list->elements, list->memory_capacity);
}

void add_element(list* list, void* input)
{
    if((list->size * get_size(list->type)) == list->memory_capacity)
    {
        resize_list(list);
    }

    // move pointer forward size*nelements * 1 bytes 
    void* ptr = (char*)list->elements + (list->size * get_size(list->type)); 
    memcpy(ptr, input, get_size(list->type)); 
    list->size++;
}

void print_list(list*list)
{
    for(int i = 0; i < list->size; i++)
    {
        switch (list->type)
        {
            case INT: printf("%d\n", ((int*)list->elements)[i]); break;
            case STRING: printf("%s\n", ((string*)list->elements)[i].value); break;
            case PLAYLIST:
            default:
                return;
        }
    }
}

void free_list(list *list)
{
    if(list->type == STRING)
    {
        for (int i = 0; i < list->size; i++)
        {
            string* s = (string*)list->elements + i;
            free(s->value);  
        }
    }

    else if(list->type == PLAYLIST)
    {
        for(int i = 0; i < list->size; i++)
        {
            playlist* ptr = (playlist*)list->elements + i;
            free(ptr->path);
            free_list(&ptr->song_paths);
        }
    }    
    
    free(list->elements);
}

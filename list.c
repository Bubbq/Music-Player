#include <stdlib.h>
#include <string.h>
#include "headers/list.h"
#include "headers/c_string.h"

size_t get_size(type type)
{
    switch (type)
    {
        case INT: return sizeof(int);
        case STRING: return sizeof(string); 
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

    void* ptr;
    switch (list->type)
    {
        case INT:
            ptr = ((int*)list->elements) + list->size;
            memcpy(ptr, (int*)input, get_size(list->type));
            break;
            
        case STRING: 
            ptr = ((string*)list->elements) + list->size;
            memcpy(ptr, (string*)input, sizeof(string));
            break;

        default:
            return;
    }
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
            free(((string*)list->elements)[i].value);  
        }
    }
    
    free(list->elements);
}

#define LIST_H
#include "stdio.h"

typedef enum
{
    INT = 0,
    DOUBLE = 1,
    FLOAT = 2,
    STRING = 3,
} type;

typedef struct
{
    int size;
    type type;
    void* elements;
    size_t memory_capacity;
} list;

size_t get_size(type type);
list create_list(type type);
void resize_list(list* list);
void print_list(list* list);
void add_element(list* list, void* input);
void free_list(list* list);
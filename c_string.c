#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/c_string.h"

string create_string()
{
    return (string){0, NULL};
}

void update_string(string* str, char* new_str)
{
    if(strlen(new_str) > MAX_LEN)
    {
        perror("new string is larger than the max size");
        return;
    }

    if(str->value != NULL)
    {
        free(str->value);
    }
    str->value = malloc(sizeof(char) * (strlen(new_str) + 1));
    strcpy(str->value, new_str);
    str->length = strlen(str->value);
}
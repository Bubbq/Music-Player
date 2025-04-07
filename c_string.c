#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/c_string.h"

string create_string(char* str)
{
    string string;
    string.value = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(string.value, str);
    return string;
}

void update_string(string* str, char* new_str)
{
    if((strlen(new_str) + 1) > MAX_LEN)
    {
        printf("new string is larger than the max size");
        return;
    }

    if(str->value != NULL)
    {
        free(str->value);
    }
    str->value = malloc(sizeof(char) * (strlen(new_str) + 1));
    strcpy(str->value, new_str);
}
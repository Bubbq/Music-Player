#define MAX_LEN 256
#ifndef CSTR
#define CSTR
typedef struct
{
    char* value;
} string;

string create_string( char* str);
void update_string(string* str, char* new_str);
#endif
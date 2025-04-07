#define CSTR
#define MAX_LEN 256

typedef struct
{
    char* value;
} string;

string create_string( char* str);
void update_string(string* str, char* new_str);
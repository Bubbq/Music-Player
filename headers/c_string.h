#define CSTR
#define MAX_LEN 256

typedef struct
{
    int length;
    char* value;
} string;

string create_string();
void update_string(string* str, char* new_str);
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define LEN 256

typedef struct SongNode
{
    struct SongNode* next;

    char title[LEN]; 
    char album[LEN]; 
    char artist[LEN]; 
    char relative_path[LEN]; 
    float duration;
    float modification_time;
    bool information_ready; 
} SongNode;

SongNode* create_song_node(SongNode value);

typedef struct 
{
    SongNode* head;
    int size;
} SongList;

SongList create_song_list();
// inserts node based on modification time, returns 0 upon sucessfull addition
int add_song_node(SongList* song_list, SongNode song_node);
// position is 0 indexed, returns 0 upon sucessful deletion 
int delete_song_node(SongList* list, int position);
void print_song_list(SongList list);
void deinit_song_list(SongList* list);
SongNode* get_nth_song_node(SongList* list, int n);
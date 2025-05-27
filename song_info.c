#include "headers/song_info.h"

SongNode* create_song_node(SongNode value)
{
    SongNode* song_node = malloc(sizeof(SongNode));
    if(!song_node) {
        perror("create_song_node / malloc");
        return NULL;
    }

    (*song_node) = value;
    song_node->next = NULL;

    return song_node;
}

SongList create_song_list()
{
    return (SongList){ NULL, 0 };
}

// inserts node based on modification time, returns 0 upon sucessfull addition
int add_song_node(SongList* song_list, SongNode song_node) 
{
    SongNode* new = create_song_node(song_node);
    if(!new) {
        perror("add_song_node / create_song_node");
        return -1;
    }

    if(song_list->size == 0) 
        song_list->head = new;
    else {
        SongNode* prev = song_list->head;
        SongNode* current = song_list->head->next;
                
        // new head case
        if(prev->modification_time > song_node.modification_time) {
            new->next = song_list->head;
            song_list->head = new;
        }
        // the nodes are sorted by modification time (increasing)
        else {
            while(current) {
                // insertion case
                if((prev->modification_time <= new->modification_time) && (new->modification_time <= current->modification_time)) 
                    break;
                else {
                    prev = current;
                    current = current->next;
                }
            }
            prev->next = new;
            new->next = current;
        }        
    }

    song_list->size++;
    return 0;
}

// position is 0 indexed, returns 0 upon sucessful deletion 
int delete_song_node(SongList* list, int position)
{
    if((position < 0) || (position >= list->size)) {
        perror("delete_song_node / invalid deletion index");
        return -1;
    }

    else if(list->size == 0) {
        perror("delete_song_node / empty list");
        return -1;
    } 

    else {
        SongNode* prev = NULL;
        SongNode* current = list->head;

        // find the node to be deleted
        for(int i = 0; (i != position); i++, prev = current, current = current->next)
            ;
        // deleting head
        if(prev == NULL) {
            prev = list->head;
            list->head = list->head->next;
            free(prev);
        }
        else {
            prev->next = current->next;
            free(current);
        }

        list->size--;
        return 0;
    }
}

void print_song_list(SongList list)
{
    SongNode* current = list.head;
    while(current) {
        printf("title: %s, album: %s, artist: %s, relative path: %s, duration: %f, modification time: %f, information ready: %d\n", 
                    current->title, current->album, current->artist, current->relative_path, current->duration, current->modification_time, current->information_ready);
        current = current->next;
    }
}

void deinit_song_list(SongList* list) 
{
    while(list->size) 
        delete_song_node(list, 0);
}

// returns a ptr to the nth node of a song list (n is 0 indexed)
SongNode* get_nth_song_node(SongList* list, int n)
{
    SongNode* current = list->head;
    for(int i = 0; (i != n); current = current->next, i++)
        ;
    return current;
}
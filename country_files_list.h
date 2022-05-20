#ifndef COUNTRY_FILES_LIST_H
#define COUNTRY_FILES_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct cFiles_listNode{
    char* name;
    struct cFiles_listNode* next;

}cFiles_listNode;

typedef struct cFiles_List{
    cFiles_listNode* head;

}cFiles_List;


/*---------------------------------COUNTRY FILES LIST FUNCTIONS--------------------------------*/
cFiles_List* initialize_cFiles_list();
cFiles_listNode* initialize_cFiles_listNode(char *name);
cFiles_listNode* insert_cFiles_list(cFiles_List** cfList, char* name);     
cFiles_listNode* search_cFiles_list(cFiles_List* list, char* key);
void print_cFiles_list(cFiles_List* list);
void destroy_cFiles_listNode(cFiles_listNode* lNode);
void destroy_cFiles_list(cFiles_List* list);











#endif
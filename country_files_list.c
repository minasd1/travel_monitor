#include "country_files_list.h"


//INITIALIZE LIST
cFiles_List* initialize_cFiles_list(){
    cFiles_List* list;
    list = (cFiles_List*)malloc(sizeof(cFiles_List));
    list->head = NULL;

    return list;
}

//INITIALIZE A LIST NODE
cFiles_listNode* initialize_cFiles_listNode(char *name){
    //ALLOCATE PROPER MEMORY
    cFiles_listNode* lNode = (cFiles_listNode*)malloc(sizeof(cFiles_listNode));
    lNode->name = (char*)malloc(strlen(name) + 1);
    strcpy(lNode->name, name);

    return lNode;
}

//INSERT A NODE TO LIST
cFiles_listNode* insert_cFiles_list(cFiles_List** list, char* name){  
    //INITIALIZE THE NODE        
    cFiles_listNode* lNode = initialize_cFiles_listNode(name);

    //INSERT THE NODE TO THE BEGINNING OF THE LIST
    lNode->next = (*list)->head;
    (*list)->head = lNode;

    return lNode;
}

//SEARCH IF GIVEN KEY EXISTS IN LIST
cFiles_listNode* search_cFiles_list(cFiles_List* list, char* key){
    cFiles_listNode* current;
    current = list->head;
    //TRAVERSE THE LIST TO FIND IF GIVEN KEY EXISTS
    while(current != NULL){
        if(strcmp(current->name, key) == 0){            
            return current;
        }
        else{
            current = current->next;
        }
    }
    return current;
}

//PRINT COUNTRY'S FILE LIST - ONLY FOR CHECKING PUPROSES
void print_cFiles_list(cFiles_List* list){
    cFiles_listNode* current;
    current = list->head;
    
    //TRAVERSE THE LIST AND FREE EVERY NODE'S MEMORY
    while(current != NULL){
        printf("%s\n", current->name);
        current = current->next;
    }
}

//FREE A LIST NODE MEMORY
void destroy_cFiles_listNode(cFiles_listNode* lNode){
    free(lNode->name);
    free(lNode);
}

//FREE MEMORY ALLOCATED FROM LIST
void destroy_cFiles_list(cFiles_List* list){
    cFiles_listNode* current;
    current = list->head;
    cFiles_listNode* throwaway;
    //TRAVERSE THE LIST AND FREE EVERY NODE'S MEMORY
    while(current != NULL){
        throwaway = current;
        current = current->next;
        destroy_cFiles_listNode(throwaway);

    }
    
    free(list);
}
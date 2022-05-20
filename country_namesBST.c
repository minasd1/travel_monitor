//#define _GNU_SOURCE
#include "country_namesBST.h"

//INITIALIZE A COUNTRY NAMES BINARY SEARCH TREE NODE
void initialize_cNamesBST_node(cNames_nodeptr* n, char* key){
    *n = malloc(sizeof(struct cNamesNode));
    (*n)->left = (*n)->right = NULL;
    (*n)->name = (char*)malloc(strlen(key)+1);
    strcpy((*n)->name, key);
    (*n)->country_files_list = initialize_cFiles_list();
}

//INSERT A COUNTRY TO COUNTRY NAMES BST
void insert_country_namesBST(cNames_nodeptr* n, char* key){
    //IF NOT ALREADY EXISTS - THEN INITIALIZE IT
    if(*n == NULL){
        initialize_cNamesBST_node(n, key);
    }
    else{
        int result = strcmp(key, (*n)->name);
        if(result < 0){
            insert_country_namesBST(&((*n)->left), key);
        }
        else if(result > 0){
            insert_country_namesBST(&((*n)->right), key);
        }
    }

}

//GET COUNTRY NAMES NODES IN ALPHABETICAL ORDER
cNames_nodeptr get_nodes_inorder(cNames_nodeptr* n, bool* visited){                  
    
    if(((*n)->left) != NULL){
        return get_nodes_inorder(&((*n)->left), visited);        
    }
    
    if(*visited == false){
        *visited=false;
        return *n;
    }
        
    *visited = false;
    if(((*n)->right) != NULL){
        return get_nodes_inorder(&((*n)->right), visited);    
    }
}

//ASSIGN COUNTRIES TO MONITOR PROCESSES EVENLY USING ALPHABETIC ROUND-ROBIN
void assign_alphabetic_RR(cNames_nodeptr* n, int* fdnum, int numofMonitors, int *count){
    if(*n == NULL){
        return;
    }
    assign_alphabetic_RR(&((*n)->left), fdnum, numofMonitors, count);
    //ROUND-ROBIN, START FROM THE FIRST MONITOR PROCESS AGAIN
    if(*count >= numofMonitors){
        *count = 0;
    }
    //SEND THE COUNTRY TO MONITOR PROCESS VIA PIPE
    int num_of_chars = strlen((*n)->name)+1;
    if(write(*(fdnum+(*count)), &num_of_chars, sizeof(int)) == -1){
        exit(1);
    }
    if(write(*(fdnum+(*count)), (*n)->name, num_of_chars*sizeof(char)) == -1){
        exit(1);
    }
    //REMEMBER THE MONITOR THAT HANDLES CURRENT COUNTRY FROM NOW ON
    (*n)->monitor = *count;                      
    (*count)++;
    assign_alphabetic_RR(&((*n)->right), fdnum, numofMonitors,count);
}

cNames_nodeptr search_cNamesBST(cNames_nodeptr n, char* key){
    //IF KEY NOT FOUND, RETURN NULL, ELSE RETURN A POINTER TO THE NODE WITH THAT KEY
    if(n == NULL){
        return NULL;
    }
    int result = strcmp(key,n->name);
    if(result == 0){
        return n;
    }
    else if(result < 0){
        return search_cNamesBST(n->left,key);
    }
    else{
        return search_cNamesBST(n->right, key);
    }
}

//WRITE COUNTRIES NAMES - LOGFILE
void write_countries_names(cNames_nodeptr* n, FILE* file){
    if(*n == NULL){
        return;
    }
    write_countries_names(&((*n)->left), file);
    write_countries_names(&((*n)->right), file);

    fwrite((*n)->name, 1, strlen((*n)->name)-1, file);
    fprintf(file, "\n");
}

//PRINT COUNTRIES NAMES - ONLY FOR CHECKING PURPOSE
void cNames_printdata(cNames_nodeptr* n){
    if(*n == NULL){
        return;
    }
    cNames_printdata(&((*n)->left));
    printf("%s\n", (*n)->name);
    cNames_printdata(&((*n)->right));
}

//FREE MEMORY ALLOCATED BY COUNTRY NAMES BST
void deallocate_country_namesBST(cNames_nodeptr* n){
    if((*n) == NULL){
        return;
    }
    deallocate_country_namesBST(&((*n)->left));
    deallocate_country_namesBST(&((*n)->right));

    free((*n)->name);
    (*n)->name = NULL;
    destroy_cFiles_list((*n)->country_files_list);
    free(*n);
    *n = NULL;
}
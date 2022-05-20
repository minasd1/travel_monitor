#ifndef COUNTRY_NAMESBST_H
#define COUNTRY_NAMESBST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include "country_files_list.h"


typedef struct cNamesNode* cNames_nodeptr;

typedef struct cNamesNode{
    char* name;
    int monitor;                        //THE MONITOR PROCESS THAT HANDLES THE COUNTRY
    cFiles_List* country_files_list;    //A LIST OF ALL THE FILES THAT COUNTRY HAS UNTIL NOW
    cNames_nodeptr left, right;

}cNamesNode;
cNames_nodeptr cNamesBST_root, cNamesBST_tree;

/*------------------------------COUNTRY NAMES BINARY SEARCH TREE FUNCTIONS-------------------------------*/
void initialize_cNamesBST_node(cNames_nodeptr* n, char* key);
void insert_country_namesBST(cNames_nodeptr* n, char* key);
cNames_nodeptr get_nodes_inorder(cNames_nodeptr* n, bool* visited);
void send_numofRecords(cNames_nodeptr* key, int* fdnum, DIR* country_dir, struct dirent* dent, FILE* file, ssize_t read, size_t len, struct stat st, char* line, int numofMonitors, int* numofRecords,int *count);
void assign_alphabetic_RR(cNames_nodeptr* n, int* fdnum, int numofMonitors, int *count);
cNames_nodeptr search_cNamesBST(cNames_nodeptr n, char* key);
void write_countries_names(cNames_nodeptr* n, FILE* file);
void cNames_printdata(cNames_nodeptr *n);
void deallocate_country_namesBST(cNames_nodeptr* n);


#endif
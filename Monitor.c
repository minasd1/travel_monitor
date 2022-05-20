#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <signal.h>
#include "citizenRecord.h"
#include "hash.h"
#include "BST.h"
#include "bitops.h"
#include "list.h"
#include "error_handling.h"
#include "hashTable.h"
#include "skipList.h"
#include "country_namesBST.h"
#include "pipe_handling.h"

#define MSG "SIGUSR1 RECEIVED"

//FLAGS WE ARE USING TO HANDLE SIGNALS SENT TO THE PROCESS
static volatile sig_atomic_t sigint_flag = 0;
static volatile sig_atomic_t sigquit_flag = 0;
static volatile sig_atomic_t sigusr1_flag = 0;
static volatile sig_atomic_t country;

//WE RAISE THOSE FLAGS WHEN A CORRESPONDING SIGNAL ARRIVES
static void handle_sigint(int sig){
    if(sig == SIGINT){
        sigint_flag = 1;
    }
}

static void handle_sigquit(int sig){
    if(sig == SIGQUIT){
        sigquit_flag = 1;
    }
}

static void handle_sigusr1(int sig){
    if(sig == SIGUSR1){
        sigusr1_flag = 1;
    }
}

int main(int argc, char* argv[]){
    //kill(getpid(), SIGKILL);
    
    ssize_t rbytes;
    DIR* country_dir = NULL;
    struct dirent* dent;
    char* country_name;
    FILE* file = NULL;
    char* line = NULL;
    size_t len = 0;
    char* input_dir;                //THE DIRECTORY THAT WE CREATE USING THE BASH SCRIPT
    struct stat st;
    int dir_count = 0;
    size_t bloomSize = 0;
    int bufferSize = 0;
    char* process = "child";
    pid_t process_id;

    char* read_pipe_name = argv[0];
    char* write_pipe_name = argv[1];

    int writefd = 0;
    int readfd = 0;
    int num_of_chars = 0;
    int numofRecords = 0;
    int stop_input = -1;
    bool visited = false;

    //OPEN READ END OF PIPE
    readfd = open_pipe(read_pipe_name, O_RDONLY, process);

    //OPEN WRITE END OF PIPE   
    writefd = open_pipe(write_pipe_name, O_WRONLY, process);

    //READ BLOOMFILTER SIZE
    read_from_pipe(readfd, &bloomSize, sizeof(ssize_t), process);

    //READ BUFFER SIZE
    read_from_pipe(readfd, &bufferSize, sizeof(int), process);

    //READ THE NAME OF THE INPUT DIRECTORY
    read_from_pipe(readfd, &num_of_chars, sizeof(int), process);
    input_dir = (char*)malloc(num_of_chars*sizeof(char));
    read_from_pipe(readfd, input_dir, num_of_chars*sizeof(char), process);

    //AND ACCESS IT'S CONTENTS
    chdir(input_dir);

    //READ COUNTRY NAMES THAT WILL BE ASSIGNED TO THIS CHILD PROCESS
    while(1){
        
        read_from_pipe(readfd, &num_of_chars, sizeof(int), process);
        if(num_of_chars != -1){
            country_name = (char*)malloc(num_of_chars*sizeof(char));
            
            //READ A COUNTRY'S NAME
            read_from_pipe(readfd, country_name, num_of_chars*sizeof(char), process);

            //OPEN IT'S CORRESPONDING DIRECTORY
            country_dir = opendir(country_name);
            if (country_dir == NULL) {
                printf ("Cannot open directory '%s'\n", country_name);
                exit(1);
            }
            chdir(country_name);

            //READ THE DIRECTORY - START READING IT'S FILES 
            while((dent = readdir(country_dir)) != NULL){
                if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
                    continue;
                }

                if(stat(dent->d_name, &st) != 0){                   //FILE DOES NOT EXIST
                    printf("%s", strerror(errno));
                }
                else if(st.st_size <= 1){                           //FILE IS EMPTY
                    //printf("File %s is empty!\n", dent->d_name);
                }
                else{                                               //FILE EXISTS
                    //OPEN FILE TO GET ITS RECORDS
                    file = fopen(dent->d_name, "r");
                    if(file == NULL){
                        printf("Could not open file!\n");
                        exit(1);
                    }
                    //COUNT TOTAL NUMBER OF RECORDS TO INITIALIZE DATA STRUCTURES ACCORDINGLY
                    while((rbytes = getline(&line, &len, file)) != -1){
                        numofRecords++;
                    
                    }
                
                    free(line);
                    line = NULL;
                    
                    fclose(file);
                    
                }
                
            }
            
            //INSERT COUNTRY NAME TO MONITOR'S COUNTRY NAMES BINARY SEARCH TREE
            insert_country_namesBST(&cNamesBST_root, country_name);
            //KEEP TRACK OF THE NUMBER OF COUNTRIES THAT HAVE BEEN ASSIGNED TO THE MONITOR
            dir_count++;
    
            closedir(country_dir);
            chdir("..");
            free(country_name);
    
        }
        else{
            break;
        }
    }

    struct citizenRecord record;
    float fsize = (float)numofRecords*1.2;
    htable.size = (int)fsize;                           //GET CITIZEN'S HASHTABLE SIZE
    size_t skiplistSize = (log(numofRecords)/log(2));   //AND VIRUSES SKIPLIST SIZE ACCORDING TO NUMBER OF
    unsigned long BF_position;                          //RECORDS RECEIVED
    hashTableList* citizen_node;
    len = 0;

    initialize_hashTable();                             //INITIALIZE CITIZENS HASHTABLE

    //START READING RECORDS OF EACH COUNTRY FILE BY FILE 
    for(int i = 0; i < dir_count; i++){
        //GET COUNTRIES BY ALPHABETICAL ORDED
        if(i == 0){
            cNamesBST_tree = get_nodes_inorder(&cNamesBST_root, &visited);
        }
        else{
            cNamesBST_tree = get_nodes_inorder(&cNamesBST_tree, &visited);
        }
        visited = true;
        //OPEN CURRENT COUNTRY'S DIRECTORY
        country_dir = opendir(cNamesBST_tree->name);
        
        if (country_dir == NULL) {
            printf("Cannot open directory %s\n", cNamesBST_tree->name);
            perror("error: ");
            exit(1);
        }
        //AND ACCESS IT'S CONTENTS
        chdir(cNamesBST_tree->name);
        
        while((dent = readdir(country_dir)) != NULL){
            
            if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
                continue;
            }

            if(stat(dent->d_name, &st) != 0){                   //FILE DOES NOT EXIST
                printf("%s\n", strerror(errno));
            }
            else if(st.st_size <= 1){                           //FILE IS EMPTY
                //printf("File %s is empty!\n", dent->d_name);
                insert_cFiles_list(&(cNamesBST_tree->country_files_list), dent->d_name);
            }
            else{                                               //FILE EXISTS
                //WE KEEP TRACK OF FILES THAT EACH COUNTRY DIRECTORY HAS - USE IT IN ADDVACCINATIONRECORDS
                //TO KNOW IF A FILE IS NEW OR NOT
                insert_cFiles_list(&(cNamesBST_tree->country_files_list), dent->d_name);
                //OPEN FILE TO GET ITS RECORDS
                file = fopen(dent->d_name, "r");
                if(file == NULL){
                    printf("Could not open file!\n");
                    exit(1);
                }
                
                while((rbytes = getline(&line, &len, file)) != -1){
                    //TOKENIZE LINE STRINGS AND ASSIGN THEM TO PROPER RECORD MEMBERS
                    record.citizenID = strtok(line, " ");
                    record.firstName = strtok(NULL, " ");
                    record.lastName = strtok(NULL, " ");
                    record.country = strtok(NULL, " ");
                    record.age = atoi(strtok(NULL, " "));
                    record.virusName = strtok(NULL, " ");
                    record.isvaccinated = strtok(NULL, " ");
                    record.dateVaccinated = strtok(NULL, "\n");
                    
                    //ERROR, NOT VACCINATED CITIZEN MUST NOT HAVE A VACCINATION DATE
                    if((strcmp(record.isvaccinated, "NO") == 0) && (record.dateVaccinated != NULL)){
                        //printf("ERROR IN RECORD %s %s %s %s %d %s %s %s\n", record.citizenID ,record.firstName, record.lastName, record.country, record.age, record.virusName, record.isvaccinated, record.dateVaccinated);
                    }
                    else{
                        
                        citizen_node = search_hashList(record.citizenID);
                        if(citizen_node != NULL){
                            //printf("citizen id is %s\n", citizen_node->id);
                        }
                        if ((citizen_node != NULL) && ((strcmp(record.firstName, citizen_node->record->firstName) != 0) || (strcmp(record.lastName, citizen_node->record->lastName) != 0) || (strcmp(record.country, citizen_node->record->country) !=0) || (record.age != citizen_node->record->age))){
                            //INCONSISTENCY IN GIVEN CITIZEN DATA
                            //printf("Inconsistent record!\n");
                        }
                        else{   //INSERT PROPER RECORD MEMBERS TO HASHTABLE  
                           
                            inserthashTable(record.citizenID, record.firstName, record.lastName, record.country, record.age);
                            //INSERT THE VIRUS TO BST OR GET A POINTER TO ITS NODE
                            tree = insertBST(&root, record.virusName, bloomSize, skiplistSize);
                            if(strcmp(record.isvaccinated, "YES") == 0){
                                
                                if(search_skipList(tree->notVaccinated, record.citizenID) == NULL){
                                    //UPDATE BLOOM FILTER
                                    for(int i = 0; i < 16; i++){
                                        BF_position = hash_i(record.citizenID, i) % bloomSize;
                                        setBit(tree->BF, BF_position);
                                    }
                                    //INSERT IN VACCINATED SKIPLIST
                                    insertskipList(&(tree->vaccinated), record.citizenID, record.isvaccinated, record.dateVaccinated, skiplistSize);
                                }
                                else{
                                    //CITIZEN ALREADY IN NON-VACCINATED LIST
                                    //printf("Inconsistent record!\n");
                                }
                            }
                            else{
                                if(search_skipList(tree->vaccinated, record.citizenID) == NULL){
                                    insertskipList(&(tree->notVaccinated), record.citizenID, record.isvaccinated, record.dateVaccinated, skiplistSize);
                                }
                                else{
                                    //CITIZEN ALREADY IN VACCINATED LIST
                                    //printf("Inconsistent record!\n");
                                }
                            }
                        }

                    }
                    
                    free(line);
                    line = NULL;
                }
                
                fclose(file);
                
            }
        }
        closedir(country_dir);
        chdir("..");
       
    }
    chdir("..");

    //SEND VIRUSES BLOOMFILTERS TO PARENT PROCESS
    send_viruses_bloomfilters(&root, &writefd, bloomSize);
    write_to_pipe(writefd, &stop_input, sizeof(int), process);

    char* received_command;
    char* operation;
    char* arguments;
    char* id;                           //ALL POSSIBLE ARGUMENTS TO BE TOKENIZED BY PARENT RECEIVED COMMAND
    char* date;
    char* countryFrom;
    char* countryTo;
    char* virusName;
    char* yes = "YES";                  //INFORM PARENT PROCESS WHETHER GIVEN TRAVELREQUEST IS ACCEPTED OR NOT
    int yes_len = strlen(yes) + 1;
    char* no = "NO";
    int no_len = strlen(no) + 1;
    int command_len;
    bool found;
    int total_requests = 0;             //KEEP TRACK OF THE MONITOR'S REQUEST COUNTERS
    int accepted_requests = 0;
    int rejected_requests = 0;
    char* logfile_init = "log_file.";   //PREFIX OF MONITOR PROCESS LOGFILE NAME
    char* logfile_name;
    FILE* logfile = NULL;
    int processId_digits;
    bool request_accepted;              
    ssize_t readbytes;
    visited = false;

    //SIGACTION THAT HANDLES RECEIVED SIGINT SIGNAL
    static struct sigaction action1 = {0};
    memset(&action1, 0, sizeof(action1));
    action1.sa_handler = handle_sigint;
    action1.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &action1, NULL);

    //SIGACTION THAT HANDLES RECEIVED SIGQUIT SIGNAL
    static struct sigaction action2 = {0};
    memset(&action2, 0, sizeof(action2));
    action2.sa_handler = handle_sigquit;
    action2.sa_flags = SA_SIGINFO;
    sigaction(SIGQUIT, &action2, NULL);

    //SIGACTION THAT HANDLES SIGUSR1 SIGNAL
    static struct sigaction action3 = {0};
    memset(&action3, 0, sizeof(action3));
    action3.sa_handler = handle_sigusr1;
    action3.sa_flags = SA_SIGINFO;                      
    sigaction(SIGUSR1, &action3, NULL);
    
    //RECEIVE COMMANDS FROM PARENT PROCESS
    while(1){

        //READ LENGTH OF COMMAND TO BE RECEIVED - CHECK IF A SIGNAL FLAG HAS BEEN RAISED
        while(((readbytes = read_from_pipe(readfd, &command_len, sizeof(int), process)) >= 0) && (sigint_flag == 0) && (sigquit_flag == 0) && (sigusr1_flag == 0)){

            if(command_len == -1){
                //printf("child exiting from command read!\n");
                break;
            }

            //CHECK IF A SIGNAL IS RECEIVED - WHICH MEANS A SIGNAL FLAG IS RAISED
            if((sigint_flag != 0) || (sigquit_flag != 0) || (sigusr1_flag != 0)){
                break;
            } 

            received_command = (char*)malloc(command_len*sizeof(char));
            
            //READ THE COMMAND
            read_from_pipe(readfd, received_command, command_len*sizeof(char), process);
            
            operation = strtok(received_command, " ");
            arguments = strtok(NULL, "\n");

            //KEEP TRACK OF THE SIGNAL FLAGS - HANLDE THEM IF FLAG RAISED
            if((sigint_flag != 0) || (sigquit_flag != 0) || (sigusr1_flag != 0)){
                free(received_command);
                received_command = NULL;
                break;
            } 

            //IF PARENT PROCESS HAS SENT A TRAVELREQUEST COMMAND RECEIVED BY USER
            if(strcmp(operation, "/travelRequest") == 0){
                id = strtok(arguments, " ");
                hashTableList* htable_node = search_hashList(id);
                //CHECK IF CITIZEN EXISTS IN CITIZEN HASHTABLE
                if(htable_node != NULL){
                    //IF EXISTS - TOKENIZE CITIZEN'S DATA
                    date = strtok(NULL, " ");
                    if(date != NULL){
                        countryFrom = strtok(NULL, " ");
                        if(countryFrom != NULL){
                            countryTo = strtok(NULL, " ");
                            if(countryTo != NULL){
                                virusName = strtok(NULL, "\n");
                                //GET ACCESS TO GIVEN VIRUS'S VIRUS BST NODE
                                nodeptr virus_node = searchBST(root, virusName);
                                //AND SEARCH IN IT'S SKIPLIST WHETHER CITIZEN IS VACCINATED TO IT OR NOT
                                skipListNode* sNode = search_skipList(virus_node->vaccinated, id);
                                if(sNode == NULL){
                                    //NOT VACCINATED - INFORM PARENT PROCESS
                                    write_to_pipe(writefd, &no_len, sizeof(int), process);
                                    write_to_pipe(writefd, no, no_len, process);
                                    rejected_requests++;
                                }
                                else{   
                                    //VACCINATED - INFORM PARENT PROCESS
                                    write_to_pipe(writefd, &yes_len, sizeof(int), process);
                                    write_to_pipe(writefd, yes, yes_len, process);
                                    //AND SEND DATE OF VACCINATION TO PARENT
                                    int date_len = strlen(sNode->record->dateVaccinated) + 1;
                                    write_to_pipe(writefd, &date_len, sizeof(int), process);
                                    write_to_pipe(writefd, sNode->record->dateVaccinated, date_len, process);

                                    //RECEIVE FROM PARENT IF REQUEST IS ACCEPTED OR NOT, DEPENDING ON VACCINATION DATE
                                    read_from_pipe(readfd, &request_accepted, sizeof(bool), process);

                                    //KEEP TRACK OF ACCEPTED, REJECTED AND TOTAL REQUESTS OF MONITOR PROCESS
                                    if(request_accepted == true){
                                        accepted_requests++;
                                    }
                                    else{
                                        rejected_requests++;
                                    }

                                }
                            }
                        }
                    }
                }
                else{
                    //CITIZEN DOES NOT EXIST IN DATABASE - HAS NOT BEEN VACCINATED FOR ANYTHING WE KNOW
                    //INFORM PARENT PROCESS THAT CITIZEN IS NOT VACCINATED (piazza)
                    write_to_pipe(writefd, &no_len, sizeof(int), process);
                    write_to_pipe(writefd, no, no_len, process);
                    rejected_requests++;
                }
                total_requests++;
                
            }
            else if(strcmp(operation, "/searchVaccinationStatus") == 0){
                id = strtok(arguments, "\n");
                hashTableList* citizen_node;
                citizen_node = search_hashList(id);
                //IF GIVEN CITIZEN ID FROM PARENT EXISTS
                if(citizen_node != NULL){
                    //INFORM PARENT PROCESS THAT CITIZEN IS FOUND
                    found = true;
                    write_to_pipe(writefd, &found, sizeof(bool), process);
                    
                    //SEND CITIZEN'S INFORMATION
                    int name_len = strlen(citizen_node->record->firstName) + 1;
                    write_to_pipe(writefd, &name_len, sizeof(int), process);
                    write_to_pipe(writefd, citizen_node->record->firstName, name_len, process);

                    int lastName_len = strlen(citizen_node->record->lastName) + 1;
                    write_to_pipe(writefd, &lastName_len, sizeof(int), process);
                    write_to_pipe(writefd, citizen_node->record->lastName, lastName_len, process);

                    int country_len = strlen(citizen_node->record->country) + 1;
                    write_to_pipe(writefd, &country_len, sizeof(int), process);
                    write_to_pipe(writefd, citizen_node->record->country, country_len, process);

                    write_to_pipe(writefd, &(citizen_node->record->age), sizeof(int), process);

                    //SEND CITIZEN'S VIRUSES STATUS - WE USE A FUNCTION FOR THAT
                    send_citizen_status(&root, &writefd, id, process);
                    write_to_pipe(writefd, &stop_input, sizeof(int), process);
                }
                else{               //CITIZEN NOT FOUND
                    found = false;
                    write_to_pipe(writefd, &found, sizeof(bool), process);
                }
            }

            

            
            free(received_command);
            received_command = NULL;
            break;
        }

        //IF A SIGINT OR A SIGQUIT SIGNAL HAS BEEN RECEIVED
        if((sigint_flag == 1) || (sigquit_flag == 1)){
            //GET PROCESSES ID - WE NEED IT FOR IT'S LOFILE NAME
            process_id = getpid();
            processId_digits = floor(log10(abs(process_id))) + 1;                              
            logfile_name = (char*)malloc(strlen(logfile_init)+processId_digits+1);
            sprintf(logfile_name, "%s%d", logfile_init, process_id);

            //OPEN LOGFILE
            logfile = fopen(logfile_name, "w");

            //WRITE COUNTRIES TO LOGFILE
            write_countries_names(&cNamesBST_root, logfile);

            //WRITE REQUESTS STATUS TO LOGFILE
            fprintf(logfile, "\nTOTAL TRAVEL REQUESTS %d\n", total_requests);
            fprintf(logfile, "ACCEPTED %d\n", accepted_requests);
            fprintf(logfile, "REJECTED %d\n", rejected_requests);

            fclose(logfile);
            free(logfile_name);
                
            sigint_flag = 0;
            sigquit_flag = 0;
        }

        
        //IF A SIGUSR1 SIGNAL HAS BEEN RECEIVED - addVaccinationRecords FUNCTION CALL FROM PARENT
        if(sigusr1_flag == 1){
            visited = false;
            //ACCESS INPUT DIRECTORY'S CONTENTS
            chdir(input_dir);
            for(int i = 0; i < dir_count; i++){
                //SEARCH FOR NEW CITIZEN RECORDS IN ANY OF COUNTRIES
                if(i == 0){
                    cNamesBST_tree = get_nodes_inorder(&cNamesBST_root, &visited);
                }
                else{
                    cNamesBST_tree = get_nodes_inorder(&cNamesBST_tree, &visited);
                }
                visited = true;
                
                country_dir = opendir(cNamesBST_tree->name);
                    
                if (country_dir == NULL) {
                    printf("Cannot open directory %s\n", cNamesBST_tree->name);
                    perror("error: ");
                    exit(1);
                }
                
                chdir(cNamesBST_tree->name);
                    
                while((dent = readdir(country_dir)) != NULL){
                    
                    if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
                        continue;
                    }

                    if(stat(dent->d_name, &st) != 0){                   //FILE DOES NOT EXIST
                        printf("%s\n", strerror(errno));
                    }
                    else if(st.st_size <= 1){                           //FILE IS EMPTY
                        //printf("File %s is empty!\n", dent->d_name);
                        if(search_cFiles_list(cNamesBST_tree->country_files_list, dent->d_name) == NULL){
                            //NEW FILE, BUT EMPTY (CORNER CASE, MAYBE NOT NEEDED)
                            insert_cFiles_list(&(cNamesBST_tree->country_files_list), dent->d_name);
                            //printf("new file is %s\n", dent->d_name);
                        }
                    }
                    else{                                               //FILE EXISTS
                        //printf("file %s exists!\n", dent->d_name);
                        //IF IT IS A NEW FILE
                        if(search_cFiles_list(cNamesBST_tree->country_files_list, dent->d_name) == NULL){
                            //INSERT IT TO COUNTRY FILES LIST
                            insert_cFiles_list(&(cNamesBST_tree->country_files_list), dent->d_name);
                            //AND OPEN THE FILE
                            file = fopen(dent->d_name, "r");
                            if(file == NULL){
                                printf("Could not open file!\n");
                                exit(1);
                            }
                            //READ THE NEW RECORDS AND ASSIGN THEM TO PROPER DATA STRUCTURES
                            while((rbytes = getline(&line, &len, file)) != -1){
                                //TOKENIZE LINE STRINGS AND ASSIGN THEM TO PROPER RECORD MEMBERS
                                record.citizenID = strtok(line, " ");
                                record.firstName = strtok(NULL, " ");
                                record.lastName = strtok(NULL, " ");
                                record.country = strtok(NULL, " ");
                                record.age = atoi(strtok(NULL, " "));
                                record.virusName = strtok(NULL, " ");
                                record.isvaccinated = strtok(NULL, " ");
                                record.dateVaccinated = strtok(NULL, "\n");
                                //printf("%s %s %s %s %d %s %s %s\n", record.citizenID ,record.firstName, record.lastName, record.country, record.age, record.virusName, record.isvaccinated, record.dateVaccinated);
                                //ERROR, NOT VACCINATED CITIZEN MUST NOT HAVE A VACCINATION DATE
                                if((strcmp(record.isvaccinated, "NO") == 0) && (record.dateVaccinated != NULL)){
                                    //printf("ERROR IN RECORD %s %s %s %s %d %s %s %s\n", record.citizenID ,record.firstName, record.lastName, record.country, record.age, record.virusName, record.isvaccinated, record.dateVaccinated);
                                }
                                else{
                                    
                                    citizen_node = search_hashList(record.citizenID);
                                    if(citizen_node != NULL){
                                        //printf("citizen id is %s\n", citizen_node->id);
                                    }
                                    if ((citizen_node != NULL) && ((strcmp(record.firstName, citizen_node->record->firstName) != 0) || (strcmp(record.lastName, citizen_node->record->lastName) != 0) || (strcmp(record.country, citizen_node->record->country) !=0) || (record.age != citizen_node->record->age))){
                                        //INCONSISTENCY IN GIVEN CITIZEN DATA
                                        //printf("Inconsistent record!\n");
                                    }
                                    else{   //INSERT PROPER RECORD MEMBERS TO HASHTABLE  
                                    
                                        inserthashTable(record.citizenID, record.firstName, record.lastName, record.country, record.age);
                                        //INSERT THE VIRUS TO BST OR GET A POINTER TO ITS NODE
                                        tree = insertBST(&root, record.virusName, bloomSize, skiplistSize);
                                        if(strcmp(record.isvaccinated, "YES") == 0){
                                            
                                            if(search_skipList(tree->notVaccinated, record.citizenID) == NULL){
                                                //UPDATE BLOOM FILTER
                                                for(int i = 0; i < 16; i++){
                                                    BF_position = hash_i(record.citizenID, i) % bloomSize;
                                                    setBit(tree->BF, BF_position);
                                                }
                                                //INSERT IN VACCINATED SKIPLIST
                                                insertskipList(&(tree->vaccinated), record.citizenID, record.isvaccinated, record.dateVaccinated, skiplistSize);
                                            }
                                            else{
                                                //CITIZEN ALREADY IN NON-VACCINATED LIST
                                                //printf("Inconsistent record!\n");
                                            }
                                        }
                                        else{
                                            if(search_skipList(tree->vaccinated, record.citizenID) == NULL){
                                                insertskipList(&(tree->notVaccinated), record.citizenID, record.isvaccinated, record.dateVaccinated, skiplistSize);
                                            }
                                            else{
                                                //CITIZEN ALREADY IN VACCINATED LIST
                                                //printf("Inconsistent record!\n");
                                            }
                                        }
                                    }

                                }
                                
                                free(line);
                                line = NULL;
                            }
                                
                            fclose(file);
                        }
                            
                    }
                }
                closedir(country_dir);
                chdir("..");
                
            }

            //SEND UPDATED VIRUSES BLOOMFILTERS TO PARENT PROCESS
            send_viruses_bloomfilters(&root, &writefd, bloomSize);
            write_to_pipe(writefd, &stop_input, sizeof(int), process);

            chdir("..");
            sigusr1_flag = 0;

        }
        
        
    }

    
    free(input_dir);

    deallocate(&root);
    destroy_hashTable();


    return 0;
}
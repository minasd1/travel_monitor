#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "country_namesBST.h"
#include "error_handling.h"
#include "skipList.h"
#include "bitops.h"
#include "hash.h"
#include "list.h"
#include "tMonitor_functions.h"
#include "pipe_handling.h"
#include "parentBST.h"
#include "childProcess.h"

extern int errno;

#define PERMS 0666  //PERMISSIONS OF DIRECTORIES 
#define K 16        //NUMBER OF HASH FUNCTIONS

static volatile sig_atomic_t sigint_flag = 0;
static volatile sig_atomic_t sigquit_flag = 0;
static volatile sig_atomic_t sigchld_flag = 0;

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


int main(int argc, char* argv[]){

    int numMonitors = 0;                    //NUMBER OF CHILD PROCESSES PRODUCED WITH FORK()
    size_t bufferSize = 0;                  //SIZE OF BUFFER IN NAMED PIPES
    size_t sizeOfBloom = 0;                 //BLOOMFILTERS SIZE
    struct dirent *dent;
    DIR *input_dir = NULL;                  //DIRECTORY WITH COUNTRIES SUBDIRECTORIES AND CORRESPONDING FILES 
    size_t len = 0;
    char *input_dir_name = NULL;
    pid_t wait_pid;
    int status = 0;
    int exec_return_value = 0;
    cNamesBST_root = NULL;
    cNamesBST_tree = NULL;
    pVirus_root = NULL;
    pVirus_tree = NULL;
    char* process = "parent";
    pid_t parent_id;

    //READ COMMAND LINE ARGUMENTS
    for(int i=0; i < argc; i++){
        if(strcmp(argv[i], "-m") == 0){
            i++;
            numMonitors = atoi(argv[i]);
        }
        else if(strcmp(argv[i], "-b") == 0){
            i++;
            bufferSize = atoi(argv[i]);
        }
        else if(strcmp(argv[i], "-s") == 0){
            i++;
            sizeOfBloom = atoi(argv[i]);
        }
        else if(strcmp(argv[i], "-i") == 0){
            i++;
            input_dir = opendir(argv[i]);
            if (input_dir == NULL) {
                printf ("Cannot open directory '%s'\n", argv[i]);
                exit(1);
            }
            input_dir_name = argv[i];
        }
    }
    struct childProcess monitor[numMonitors];
    int readfd[numMonitors];                //READ FILE DESCRIPTORS OF NAMED PIPES
    int writefd[numMonitors];               //WRITE FILE DESCRIPTORS
    int pipe_num = 0;
    char *pipe_glob_name = "fifo_";         //ALL THE PIPE NAMES START WITH THIS PREFIX
    int numOfPipes;
    size_t numOfPipes_digits = 0;                           
    char *pipe_name;
    int dir_count = 0;                      //COUNTER OF COUNTRY DIRECTORIES IN INPUT FILE

    

    while((dent = readdir(input_dir)) != NULL){
        struct stat st;
  
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
            continue;
        }
        if (fstatat(dirfd(input_dir), dent->d_name, &st, 0) < 0)
        {
            perror(dent->d_name);
            continue;
        }
    
        if (S_ISDIR(st.st_mode)) dir_count++;
        

    }
    rewinddir(input_dir);

    //IF MONITORS ARE MORE THAN COUNTRIES
    if(numMonitors > dir_count){
        //SET THEM TO BE EQUAL TO COUNTRIES - EACH MONITOR WILL HANDLE ONE COUNTRY
        numMonitors = dir_count;
    }

    numOfPipes = 2*numMonitors;         //2 PIPES FOR EACH MONITOR - ONE FOR READING AND ONE FOR WRITING
    numOfPipes_digits = floor(log10(abs(numOfPipes))) + 1;                              
    pipe_name = (char*)malloc(strlen(pipe_glob_name)+numOfPipes_digits+1);

    //CREATE NAMED PIPES TO COMMUNICATE WITH CHILD PROCESSES
    for(int i=0; i < numOfPipes; i++){
        
        pipe_num++;
        sprintf(pipe_name, "%s%d", pipe_glob_name, pipe_num);
        
        if((mkfifo(pipe_name, PERMS) < 0) && (errno != EEXIST)){
            printf("%s\n", pipe_name);
            perror("Can't create fifo\n");
            exit(1);
        }

    }
    
    //CREATE NUMOFMONITORS MONITORS
    for(int i = 0; i < numMonitors; i++){           
        monitor[i].pid = fork();                                                         
        if(monitor[i].pid < 0){                 //FORK FAILED                                                                                   
            perror("fork failed");
            return 2;
        }
        else if(monitor[i].pid == 0){           //CHILD PROCESS                            
            pipe_num = (2*i)+1;

            //GET THE READ PIPE AND THE WRITE PIPE FOR CURRENT MONITOR
            char* read_pipe_name = (char*)malloc(strlen(pipe_glob_name)+numOfPipes_digits+1);
            strcpy(read_pipe_name, pipe_glob_name);
            char* write_pipe_name = (char*)malloc(strlen(pipe_glob_name)+numOfPipes_digits+1);
            strcpy(write_pipe_name, pipe_glob_name);

            sprintf(read_pipe_name, "%s%d", pipe_glob_name, pipe_num);
            
            sprintf(write_pipe_name, "%s%d", pipe_glob_name, pipe_num+1);

            monitor[i].read_pipe = read_pipe_name;
            monitor[i].write_pipe = write_pipe_name;
            
            //CONTINUE EXECUTION FROM MONITOR PROGRAM
            exec_return_value = execl("Monitor", read_pipe_name, write_pipe_name, NULL);

            if(exec_return_value == -1){
                perror("execl ");
                exit(1);
            }
            break;
        }
        
    }

    //PARENT PROCESS
    int count = 0;
    pipe_num = 0;
    int stop_input = -1;
    dir_count = 0;
    char ch = '/';
    int cap;


    for(int i = 0; i < numMonitors; i++){
        pipe_num++;
        sprintf(pipe_name, "%s%d", pipe_glob_name, pipe_num);

        //OPEN NAMED PIPES TO WRITE TO MONITOR PROCESSES
        writefd[i] = open_pipe(pipe_name, O_WRONLY, process);
        fcntl(writefd[i], F_SETPIPE_SZ, bufferSize);

        cap = fcntl(writefd[i], F_GETPIPE_SZ);

        pipe_num++;
        sprintf(pipe_name, "%s%d", pipe_glob_name, pipe_num);

        //OPEN NAMED PIPES TO READ FROM MONITOR PROCESSES
        readfd[i] = open_pipe(pipe_name, O_RDONLY, process);
        fcntl(readfd[i], F_SETPIPE_SZ, bufferSize);

        cap = fcntl(readfd[i], F_GETPIPE_SZ);
    
    }
    
    //SEND SOME INITIAL NECESSARY DATA TO CHILD PROCESSES
    for(int i = 0; i < numMonitors; i++){
        //SEND BLOOMFILTER SIZE 
        write_to_pipe(writefd[i], &sizeOfBloom, sizeof(size_t), process);

        cap = fcntl(writefd[i], F_GETPIPE_SZ);
        //SEND BUFFER SIZE
        write_to_pipe(writefd[i], &cap, sizeof(int), process);

        int input_dir_name_len = strlen(input_dir_name) + 1;

        write_to_pipe(writefd[i], &input_dir_name_len, sizeof(int), process);

        //SEND NAME OF INPUT DIRECTORY TO ALL CHILD PROCESSES SO THEY CAN OPEN IT
        write_to_pipe(writefd[i], input_dir_name, input_dir_name_len*sizeof(char), process);
        
    }
    

    //READ CONTENTS OF INPUT DIRECTORY - INSERT SUBDIRECTORIES' COUNTRY NAMES TO COUNTRY NAMES BST
    while((dent = readdir(input_dir)) != NULL){
    
        struct stat st;
    
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
            continue;
        }
        if (fstatat(dirfd(input_dir), dent->d_name, &st, 0) < 0)
        {
            perror(dent->d_name);
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) dir_count++;
        
        strncat(dent->d_name, &ch, sizeof(char));
        //INSERTION
        insert_country_namesBST(&cNamesBST_root, dent->d_name);

    }
    rewinddir(input_dir);

    chdir(input_dir_name);
    //ASSIGN COUNTRIES TO MONITOR PROCESSES EVENLY USING ALPHABETIC ROUND ROBIN
    assign_alphabetic_RR(&cNamesBST_root, &writefd[count], numMonitors, &count);
    for(int i = 0; i < numMonitors; i++){

        write_to_pipe(writefd[i], &stop_input, sizeof(int), process);
    }
    chdir("..");


    char* virusName;
    int virusName_len = 0;
    int BF_position = 0;
    int numMonitors_digits = floor(log10(abs(numMonitors))) + 1;

    //READ VIRUSES BLOOMFILTERS DATA FROM MONITOR PROCESSES
    for(int i = 0; i < numMonitors; i++){
        while(1){
            //READ LENGTH OF VIRUS NAME STRING
            read_from_pipe(readfd[i], &virusName_len, sizeof(int), process);
            if(virusName_len == -1){
                break;
            }
            else{
                //READ VIRUSNAME
                virusName = (char*)malloc(virusName_len*sizeof(char) + numMonitors_digits + 1);                  
                read_from_pipe(readfd[i], virusName, virusName_len*sizeof(char), process);

                //INSERT VIRUS TO PARENT'S VIRUS BST - GET A POINTER TO ITS' NODE IF ALREADY INSERTED
                pVirus_tree = insert_pVirusBST(&pVirus_root, virusName, sizeOfBloom);

                //SET BST ACCORDING TO CHILD PROCESSES GIVEN DATA 
                while(1){

                    read_from_pipe(readfd[i], &BF_position, sizeof(int), process);
                    if(BF_position == -1){  //END OF INPUT
                        break;
                    }
                    else{        //SET GIVEN BST POSITION TO 1 IN CORRESPONDING VIRUS'S BLOOMFILTER OF PARENT
                        setBit(pVirus_tree->BF, BF_position);
                    }
                }
                free(virusName);
                virusName = NULL;
            }
        }

    }

    char* command = NULL;   //USER COMMAND TO INTERACT WITH DATABASE
    char* operation;        
    char* arguments;
    ssize_t rbytes;
    char* logfile_init = "log_file.";   //LOGFILE PREFIX
    char* logfile_name;
    int parentId_digits;
    FILE* logfile = NULL;
    int total_requests = 0;             //REQUESTS COUNTERS
    int accepted_requests = 0;
    int rejected_requests = 0;

    //SIGACTION TO CATCH AND HANDLE SIGINT RECEIVED SIGNAL
    static struct sigaction action1 = {0};
    memset(&action1, 0, sizeof(action1));
    action1.sa_handler = handle_sigint;
    action1.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &action1, NULL);

    //SIGACTION TO CATCH AND HANDLE SIGQUIT RECEIVED SIGNAL
    static struct sigaction action2 = {0};
    memset(&action2, 0, sizeof(action2));
    action2.sa_handler = handle_sigquit;
    action2.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGQUIT, &action2, NULL);
    
    //RECEIVE REQUESTS FROM USER
    while(1){

        printf("Please insert command: ");
        rbytes = getline(&command, &len, stdin);
        if(rbytes == -1){
            fprintf(stderr, "can't read line, errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        int command_len = strlen(command) + 1;
        char command_cp[command_len];
        //if getline == -1 || sigflag == kati -> 
        strcpy(command_cp, command);
        operation = strtok(command, " ");
        arguments = strtok(NULL, "\n");

        if(strcmp(operation, "/travelRequest") == 0){
            travelRequest(command_cp, arguments, &writefd[0], &readfd[0], sizeOfBloom, K, process);
        }
        else if(strcmp(operation, "/travelStats") == 0){
            travelStats(command_cp, arguments);
        }
        else if(strcmp(operation, "/addVaccinationRecords") == 0){
            addVaccinationRecords(command_cp, arguments, &readfd[0], &monitor[0], sizeOfBloom, process);
        }
        else if(strcmp(operation, "/searchVaccinationStatus") == 0){
            searchVaccinationStatus(command_cp, arguments, &writefd[0], &readfd[0], numMonitors, process);
        }
        else if(strcmp(operation, "exit\n") != 0){

            printf("Not a proper operation! Operation should be one of the following:\n"
            "/travelRequest\n"
            "/travelStats\n"
            "/addVaccinationRecords\n"
            "/searchVaccinationStatus\n"
            "/exit\n\n");

        }
        //IF A SIGINT OR A SIGQUIT SIGNAL RECEIVED - WE FOLLOW THE SAME PROCEDURE
        //AS IF AN EXIT COMMAND HAS BEEN RECEIVED FROM USER
        if(strcmp(operation, "exit\n") == 0 || sigint_flag == 1 || sigquit_flag == 1){
            
            for(int i = 0; i < numMonitors; i++){
                
                //SENT SIGKILL SIGNAL TO ALL THE MONITOR PROCESSES
                kill(monitor[i].pid, SIGKILL);
                
            }
            //GET PROCESS ID - USE IT FOR LOGFILE NAME
            parent_id = getpid();
            parentId_digits = floor(log10(abs(parent_id))) + 1;                              
            logfile_name = (char*)malloc(strlen(logfile_init)+parentId_digits+1);

            sprintf(logfile_name, "%s%d", logfile_init, parent_id);

            //OPEN LOGFILE
            logfile = fopen(logfile_name, "w");
            //WRITE COUNTRIES TO LOGFILE
            while((dent = readdir(input_dir)) != NULL){
    
                struct stat st;
    
                if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
                    continue;
                }
                if (fstatat(dirfd(input_dir), dent->d_name, &st, 0) < 0)
                {
                    perror(dent->d_name);
                    continue;
                }
                
                fprintf(logfile, "%s\n", dent->d_name); 
            }
            //CALL THIS PARENT BST FUNCTION TO RECEIVE ALL REQUESTS STATUS 
            get_requests_status(&pVirus_root, &total_requests, &accepted_requests, &rejected_requests);
            
            //WRITE REQUESTS STATUS TO LOGFILE
            fprintf(logfile, "TOTAL TRAVEL REQUESTS %d\n", total_requests);
            fprintf(logfile, "ACCEPTED %d\n", accepted_requests);
            fprintf(logfile, "REJECTED %d\n", rejected_requests);

            fclose(logfile);
            free(logfile_name);

            free(command);
            command = NULL;
            
            printf("Exiting...\n");
            break;
        }

        
    }


    
    //CLOSE AND UNLINK ALL NAMED PIPES THAT HAVE BEEN CREATED
    pipe_num = 0;
    for(int i=0; i < numMonitors; i++){

        pipe_num++;
        sprintf(pipe_name, "%s%d", pipe_glob_name, pipe_num);

        close_pipe(writefd[i], process);
        unlink_pipe(pipe_name, process);

        pipe_num++;
        sprintf(pipe_name, "%s%d", pipe_glob_name, pipe_num);
       
        close_pipe(readfd[i], process);
        unlink_pipe(pipe_name, process);
    }


    //WAIT CHILD PROCESSES(MONITORS) TO FINISH
    while ((wait_pid = wait(&status)) > 0);
    
    //CLOSE INPUT DIRECTORY AND FREE ALL THE ALLOCATED MEMORY
    free(pipe_name);

    closedir(input_dir);
    deallocate_country_namesBST(&cNamesBST_root);
    pVirusBST_deallocate(&pVirus_root);

   

    return 0;  
}
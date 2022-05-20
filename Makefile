CC = gcc
tFiles = tMonitor_functions.c hashTable.c parentBST.c skipList.c bloomFilter.c list.c country_namesBST.c country_files_list.c pipe_handling.c error_handling.c hash.c bitops.c
mFiles = hashTable.c BST.c skipList.c bloomFilter.c list.c hash.c bitops.c country_namesBST.c country_files_list.c pipe_handling.c
args = -g -Wall -lm -o

all: 
		$(CC) travelMonitor.c $(tFiles) $(args) travelMonitor
		$(CC) Monitor.c $(mFiles) $(args) Monitor

travelMonitor:
	$(CC) travelMonitor.c $(tFiles) $(args) travelMonitor

Monitor:
	$(CC) Monitor.c $(mFiles) $(args) Monitor

clean:
	if [ -f travelMonitor ]; then rm travelMonitor; fi;
	if [ -f Monitor ]; then rm Monitor; fi;
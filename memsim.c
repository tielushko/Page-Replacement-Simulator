#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#define __SIZE_MAX__ 0xffffffffU

int nframes;
int countReads = 0;
int countWrites = 0;
bool debug;

void rdm(char* filename, int numframes);
void lru(char* filename, int numframes);
void fifo(char* filename, int numframes); 
void vms(char* filename, int numframes); 

struct Entries {
    unsigned address;
    char readwrite;
    int valid;

} entries;

int main (int argc, char* argv[]) {
    //making sure that there is a correct number of arguments in the execution of the problem. 
    if (argc < 5) {
        fprintf(stderr, "Not enough arguments in the execution. Usage: 'memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>");
        return EXIT_FAILURE;
    }

    //copying the name of the tracefile from argv[1] and opening the file.
    char* filename = argv[1];

    //make sure in the page simulator functions we have that we check if file was opened correctly or not. 


    //reading the number of frames from the argument list argv[2].
    nframes = atoi(argv[2]); 
    if (nframes <= 0) {
        fprintf(stderr, "Incorrect number of frames entered, abort.");
        return EXIT_FAILURE;
    }

    //reading the argument for the debug or quiet argv[4]
    char* outputMode = argv[4];
    
    if (strcmp(outputMode, "debug") == 0 || strcmp(outputMode, "DEBUG") == 0) 
        debug = true;
    else if (strcmp(outputMode, "quiet") == 0 || strcmp(outputMode, "QUIET") == 0)
        debug = false; 
    else {
        fprintf(stderr, "Incorrect quiet/debug mode selection, abort"); 
        return EXIT_FAILURE;
    }

    //reading the page replacement simulation function the program will run - argv[3] (rdm|lru|fifo|vms) 
    char* simulationMode = argv[3];
    printf("simulation mode is %s\n", simulationMode);

    if (strcmp(simulationMode, "LRU") || strcmp(simulationMode, "lru")) {
        lru(filename, nframes);
    } else if (strcmp(simulationMode, "FIFO") || strcmp(simulationMode, "fifo")) {
        fifo(filename, nframes);
    } else if (strcmp(simulationMode, "RDM") || strcmp(simulationMode, "rdm")) {
        rdm(filename, nframes);
    } else if (strcmp(simulationMode, "VMS") || strcmp(simulationMode, "vms")) {
        vms(filename, nframes);  
    } else {
        fprintf(stderr, "Incorrect Page Replacement Simulation is selected. Abort.");
        return EXIT_FAILURE;
    }

    //output portion of the code
    printf("Total memory frames: %d\n", nframes);
    //printf("Events in trace %d\n", nEvents);
    printf("Total Disk Reads: %d\n", countReads); 
    printf("Total Disk Writes: %d\n", countWrites);
    
    return 0;
}

void rdm(char* filename, int numframes) {
    struct Entries entries[numframes];
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }

    int i = 0;
    while ( (fscanf(fp, "%x %c", &entries[i].address, &entries[i].readwrite)) != EOF) {
        entries[i].valid = 0; // Not sure which val to initialize with
        printf("Addr: %x  RW: %c  Valid: %d\n", entries[i].address, entries[i].readwrite, entries[i].valid);
        i++;
    }

}
void lru(char* filename, int numframes) {
    struct Entries entries[numframes];
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }

    int i = 0;
    while ( (fscanf(fp, "%x %c", &entries[i].address, &entries[i].readwrite)) != EOF) {
        entries[i].valid = 0; // Not sure which val to initialize with
        printf("Addr: %x  RW: %c  Valid: %d\n", entries[i].address, entries[i].readwrite, entries[i].valid);
        i++;
    }

}
void fifo(char* filename, int numframes) {
    struct Entries entries[numframes];
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }

    int i = 0;
    while ( (fscanf(fp, "%x %c", &entries[i].address, &entries[i].readwrite)) != EOF) {
        entries[i].valid = 0; // Not sure which val to initialize with
        printf("Addr: %x  RW: %c  Valid: %d\n", entries[i].address, entries[i].readwrite, entries[i].valid);
        i++;
    }
}
void vms(char* filename, int numframes) {
    struct Entries entries[numframes];
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }

    int i = 0;
    while ( (fscanf(fp, "%x %c", &entries[i].address, &entries[i].readwrite)) != EOF) {
        entries[i].valid = 0; // Not sure which val to initialize with
        printf("Addr: %x  RW: %c  Valid: %d\n", entries[i].address, entries[i].readwrite, entries[i].valid);
        i++;
    }
}
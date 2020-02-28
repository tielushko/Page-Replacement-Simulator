#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

int nframes;
int countReads = 0;
int countWrites = 0;
bool debug;

void rdm(char* filename);
void lru(char* filename);
void fifo(char* filename); 
void vms(char* filename); 

struct PageEntry {
    unsigned VPN;
    char rw;
    int dirty; //0 - clean 1 - dirty
    unsigned BASE;
    unsigned BOUND;
} PageEntry;

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
    
    if (strcmp(simulationMode, "LRU") == 0 || strcmp(simulationMode, "lru") == 0) {
        lru(filename);
    } else if (strcmp(simulationMode, "FIFO") == 0|| strcmp(simulationMode, "fifo") == 0) {
        fifo(filename);
    } else if (strcmp(simulationMode, "RDM") == 0 || strcmp(simulationMode, "rdm") == 0) {
        rdm(filename);
    } else if (strcmp(simulationMode, "VMS") == 0 || strcmp(simulationMode, "vms") == 0) {
        vms(filename);  
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


void rdm(char* filename) {
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }
      
    char rw; 
    unsigned addr;
    int i = 0;
    char READ = 'R';
    char WRITE = 'W';
    char* temp;
    bool found; 
    struct PageEntry RAM[nframes];
    int k;

    for (k = 0; k < nframes; k++) { //we need to initialize RAM as NULL all the way through 
        RAM[k].VPN = 0;
        RAM[k].rw = 0;
        RAM[k].dirty = 0; //initialize dirty bit to zero
    }

    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        //printf("%x %c\n", addr, rw);
    
        //direct the flow if the R is seen
        int j;
        if(rw == 'R') { //works
            for (j = 0; j < nframes; j++) { //scanning the RAM to find it in memory.
                //if found need to break and set found to true;
                unsigned readVPN = addr/4096;
                if (readVPN == RAM[j].VPN) {
                    printf("Reading from ram. HIT\n");
                    found = true;
                    break;//this is a hit we dont need to do anything //we can break.
                } else {
                    found = false;
                }
            }

            if (!found) { //in case the page wasn't found in the RAM, we need to load it in MEMORY
                int l; 
                for (int l = 0; l < nframes; l++) {
                    if (RAM[l].VPN == 0)  { //load the first empty page in RAM with information from the disk.
                        RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[l].rw = rw; //rw part
                        RAM[l].dirty = 0; //initialize dirty bit to zero
                        RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        countReads++;
                        printf("%x %c %d %x %x\n", RAM[l].VPN, RAM[l].rw, RAM[l].dirty, RAM[l].BASE, RAM[l].BOUND);
                        break;
                    }
                }
            }
        } //end if Read

        if (rw == 'W') {

        }
        /*
        RAM[i].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
        RAM[i].rw = rw; //rw part
        RAM[i].dirty = 0; //initialize dirty bit to zero
        RAM[i].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
        RAM[i].BOUND = RAM[i].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
        */
        //printf("%x %c %d %x %x\n", RAM[i].VPN, RAM[i].rw, RAM[i].dirty, RAM[i].BASE, RAM[i].BOUND); //the test to make sure it works correctly proven itself.
        //i++;
        //where the population starts
        
        //for R if it is not in RAM -> reads the process from disk (readCount++) 
          //writes the process into the RAM and marks it as "dirty"
          
        //for W
          //if (have space in RAM) -> populate it into the RAM and mark the page as dirty. 
          //else -> page replacement alogrithm. if page under replacement is dirty, numberWrites to disk is ++ 
    }
    
    fclose(fp);
}

void lru(char* filename) {
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }
      
    char* rw; 
    unsigned addr;
    
    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        printf("%x %c\n", addr, rw);
    
    }
    fclose(fp);
}
void fifo(char* filename) {
   FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }
      
    char* rw; 
    unsigned addr;
    
    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        printf("%x %c\n", addr, rw);
    
    }
    fclose(fp);
}
void vms(char* filename) {
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }
      
    char* rw; 
    unsigned addr;
    
    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        printf("%x %c\n", addr, rw);
    
    }
    fclose(fp);
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

int nframes;
int countReads = 0;
int countWrites = 0;
bool debug;
int nEvents = 0;

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
    printf("Simulation mode is %s\n", simulationMode);
    
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
    printf("\nTotal memory frames: %d\n", nframes);
    printf("Events in trace: %d\n", nEvents);
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
    //int i = 0;
    char READ = 'R';
    char WRITE = 'W';
    //char* temp;
    bool found;
    bool full = false;
    struct PageEntry RAM[nframes];
    int k;

    for (k = 0; k < nframes; k++) { //we need to initialize RAM as NULL all the way through 
        RAM[k].VPN = 0;
        RAM[k].rw = 0;
        RAM[k].dirty = 0; //initialize dirty bit to zero
    }

    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        nEvents++;
    
        //direct the flow if the R is seen
        int j;
        if(rw == 'R') { //works
            for (j = 0; j < nframes; j++) { //scanning the RAM to find it in memory (if the process is already in memory).
                //if found need to break and set found to true;
                unsigned readVPN = addr/4096;
                if (readVPN == RAM[j].VPN) {
                    //add condition to print only for debug mode 
                    if(debug == true) {                   
                        printf("Reading %x memory reference from page %x in RAM. HIT. Dirty: %d\n", addr, readVPN, RAM[j].dirty);
                    }
                    RAM[j].dirty = 0;
                    found = true;
                     //update the last operation to read -> dirty is zero.
                    break;//this is a hit we dont need to do anything //we can break.
                } else {
                    found = false;
                }
            }

            if (!found) { //in case the page wasn't found in the RAM, we need to load it in MEMORY
                int m;
                int emptypages = 0;

                for (m = 0; m < nframes; m++) { //UPDATE: Processes the empty correctly. 
                    if (RAM[m].VPN == 0)        
                        emptypages++;
                }
                
                if (emptypages == 0)
                    full = true;
                else 
                    full = false;
                
                int l;
                 
                for (l = 0; l < nframes; l++) { //run for loop to check for empty frames.
                    if (RAM[l].VPN == 0)  { //load the first empty page in RAM with information from the disk.
                        RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[l].rw = rw; //rw part
                        RAM[l].dirty = 0; //initialize dirty bit to zero
                        RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        countReads++;
                        //add condition to print only for debug mode
                        if(debug == true)
                            printf("\nReading from disk and placing the following memory reference: %x into an empty space %d in RAM. VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", addr, l, RAM[l].VPN, RAM[l].rw, RAM[l].dirty, RAM[l].BASE, RAM[l].BOUND);
                        break;
                    }
                } 
                
                //otherwise if the entire table is full, we need to use a page replacement algorithm here. 
                //THIS IS THE JUICE OF ALGORITHM
                if (full) {
                    srand(time(0));
                    int randIndex = (rand() % nframes);
                    //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", randIndex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                    }
                    
                    if (RAM[randIndex].dirty == 0) {
                        //your casual replacement.
                            if(debug == true)
                                printf("\nEliminating a clean page %x, no write to the disk required.\n", RAM[randIndex].VPN);
                            RAM[randIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                            RAM[randIndex].rw = rw; //rw part
                            RAM[randIndex].dirty = 0; //initialize dirty bit to zero
                            RAM[randIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                            RAM[randIndex].BOUND = RAM[randIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                            countReads++;
                            //add condition to print only for debug mode
                            if(debug == true) 
                                printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", randIndex, RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                    } else {
                    //if(RAM[randIndex].dirty == 1) {
                        if(debug == true)
                            printf("\nEliminating a dirty page %x, requires a write to the disk.\n", RAM[randIndex].VPN);
                        countWrites++;
                        //think if this should get replaced...
                        RAM[randIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[randIndex].rw = rw; //rw part
                        RAM[randIndex].dirty = 0; //initialize dirty bit to zero
                        RAM[randIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[randIndex].BOUND = RAM[randIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        countReads++;
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", randIndex, RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                        
                    }

                }   
            }
        } //end if Read

        if (rw == 'W') {
            //check if the page already exists in the ram, then it will just be written on top of it. 
            for (j = 0; j < nframes; j++) { //scanning the RAM to find it in memory (if the process is already in memory).
                //if found need to break and set found to true;
                unsigned writeVPN = addr/4096;
                if (writeVPN == RAM[j].VPN) {
                    //add condition to print only for debug mode 
                    if(debug == true) {                   
                        printf("Rewriting the page: %x memory reference onto the page %x in RAM.\n", addr, writeVPN);
                    }
                    RAM[j].dirty = 1;
                    found = true;
                    break;//this is a hit we dont need to do anything //we can break.
                } else {
                    found = false;
                }
            }

            //if the page is not located in the ram, we need to find an empty space for it to fit into the ram
            if (!found) {
                int m;
                int emptypages = 0;

                for (m = 0; m < nframes; m++) { //UPDATE: Processes the empty correctly. 
                    if (RAM[m].VPN == 0)        
                        emptypages++;
                }
                    
                if (emptypages == 0)
                    full = true;
                else 
                    full = false;

                if (!full) {
                    int l;
                    for (l = 0; l < nframes; l++) { //run for loop to check for empty frames.
                        if (RAM[l].VPN == 0)  { //if we find the first empty page, we need to write into it
                            RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                            RAM[l].rw = rw; //rw part
                            RAM[l].dirty = 1; //if the operation is write, dirty bit must be 1 then.
                            RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                            RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                            //add condition to print only for debug mode
                            if(debug == true)
                                printf("\nWriting the following memory reference: %x into an empty space %d in RAM. VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", addr, l, RAM[l].VPN, RAM[l].rw, RAM[l].dirty, RAM[l].BASE, RAM[l].BOUND);
                            break;
                        }
                    }
                } else { //ALGORITHM JUICE
                    srand(time(0));
                    int randIndex = (rand() % nframes);
                    //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", randIndex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                    }
                    
                    if (RAM[randIndex].dirty == 0) {
                        if(debug == true)
                            printf("\nEliminating a clean page %x, no write to the disk required.\n", RAM[randIndex].VPN);
                        //rewriting the page
                        RAM[randIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[randIndex].rw = rw; //rw part
                        RAM[randIndex].dirty = 1; //initialize dirty bit to one as the command was W
                        RAM[randIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[randIndex].BOUND = RAM[randIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", randIndex, RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                    } else {

                    //if(RAM[randIndex].dirty == 1) {
                        if(debug == true)
                            printf("\nEliminating a dirty page %x, requires a write to the disk.\n", RAM[randIndex].VPN);
                        countWrites++;
                        //rewriting the page.
                        RAM[randIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[randIndex].rw = rw; //rw part
                        RAM[randIndex].dirty = 1; //initialize dirty bit to one as the command was W
                        RAM[randIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[randIndex].BOUND = RAM[randIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", randIndex, RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                    }                     
                }
            }
            //if empty space is not found, we need to apply the page replacement algorithm to select which page to evict.
                //if page under eviction is dirty, we need to save it to the disk -> diskWrites++
                //if page under eviction is !dirty, we just replace it. no need to increment anything. 
            
            
        } //end if write
        
        //for R if it is not in RAM -> reads the process from disk (readCount++) 
          
        //for W
          
          //if (have space in RAM) -> populate it into the RAM and mark the page as dirty. 
            //writes the process into the RAM and marks it as "dirty"
          //else -> page replacement alogrithm. if page under replacement is dirty, numberWrites to disk is ++ as we need to save that page to disk.
    }
    if (debug == true) {
        //The final Ram 
        printf("The final RAM contents are the following: \n");
        for (int z = 0; z < nframes; z++) {
            printf("Slot: %d, VPN: %x, Dirty: %d\n", z, RAM[z].VPN, RAM[z].dirty);
        }
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
      
    char rw; 
    unsigned addr;
    char READ = 'R';
    char WRITE = 'W';
    //char* temp;
    bool found;
    bool full = false;
    struct PageEntry RAM[nframes];
    int k;
    int fifoIndex = 0; 

    //The RAM will also serve as our FIFO queue, as we are filling up ram top to bottom, keeping track of proper indeces.
    for (k = 0; k < nframes; k++) { //we need to initialize RAM as NULL all the way through 
        RAM[k].VPN = 0;
        RAM[k].rw = 0;
        RAM[k].dirty = 0; //initialize dirty bit to zero
    }

    
    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        nEvents++;
    
        int j;
        //direct the flow if the R is seen
        if(rw == 'R') { //works
            for (j = 0; j < nframes; j++) { //scanning the RAM to find it in memory (if the process is already in memory).
                //if found need to break and set found to true;
                unsigned readVPN = addr/4096;
                if (readVPN == RAM[j].VPN) {
                    //add condition to print only for debug mode 
                    RAM[j].dirty = 0;     
                    if(debug == true) {                   
                        printf("Reading %x memory reference from page %x in RAM. HIT. Dirty: %d\n", addr, readVPN, RAM[j].dirty);
                    }
                    found = true;
                    break;//this is a hit we dont need to do anything //we can break.
                } else {
                    found = false;
                }
            }

            if (!found) { //in case the page wasn't found in the RAM, we need to load it in MEMORY
                int m;
                int emptypages = 0;

                for (m = 0; m < nframes; m++) { //UPDATE: Processes the empty correctly. 
                    if (RAM[m].VPN == 0)        
                        emptypages++;
                }
                
                if (emptypages == 0)
                    full = true;
                else 
                    full = false;
                
                int l;
                 
                for (l = 0; l < nframes; l++) { //run for loop to check for empty frames.
                    if (RAM[l].VPN == 0)  { //load the first empty page in RAM with information from the disk.
                        RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[l].rw = rw; //rw part
                        RAM[l].dirty = 0; //initialize dirty bit to zero
                        RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        countReads++;
                        //add condition to print only for debug mode
                        if(debug == true)
                            printf("\nReading from disk and placing the following memory reference: %x into an empty space %d in RAM. VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", addr, l, RAM[l].VPN, RAM[l].rw, RAM[l].dirty, RAM[l].BASE, RAM[l].BOUND);
                        break;
                    }
                } 
                
                //otherwise if the entire table is full, we need to use a page replacement algorithm here. 
                //THIS IS THE JUICE OF ALGORITHM
                if (full) {
                    
                    //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", fifoIndex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[fifoIndex].VPN, RAM[fifoIndex].rw, RAM[fifoIndex].dirty, RAM[fifoIndex].BASE, RAM[fifoIndex].BOUND);
                    }

                    if (RAM[fifoIndex].dirty == 0) {
                        //your casual replacement.
                            if(debug == true)
                                printf("\nEliminating a clean page %x, no write to the disk required.\n", RAM[fifoIndex].VPN);
                            RAM[fifoIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                            RAM[fifoIndex].rw = rw; //rw part
                            RAM[fifoIndex].dirty = 0; //initialize dirty bit to zero
                            RAM[fifoIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                            RAM[fifoIndex].BOUND = RAM[fifoIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                            countReads++;
                            //add condition to print only for debug mode
                            if(debug == true) 
                                printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", fifoIndex, RAM[fifoIndex].VPN, RAM[fifoIndex].rw, RAM[fifoIndex].dirty, RAM[fifoIndex].BASE, RAM[fifoIndex].BOUND);
                    } else {
                    //if(RAM[fifoIndex].dirty == 1) {
                        if(debug == true)
                            printf("\nEliminating a dirty page %x, requires a write to the disk.\n", RAM[fifoIndex].VPN);
                        countWrites++;
                        //think if this should get replaced...
                        RAM[fifoIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[fifoIndex].rw = rw; //rw part
                        RAM[fifoIndex].dirty = 0; //initialize dirty bit to zero
                        RAM[fifoIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[fifoIndex].BOUND = RAM[fifoIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        countReads++;
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", fifoIndex, RAM[fifoIndex].VPN, RAM[fifoIndex].rw, RAM[fifoIndex].dirty, RAM[fifoIndex].BASE, RAM[fifoIndex].BOUND);
                        
                    }

                    if (fifoIndex == nframes-1) {
                        fifoIndex = 0;
                    } else {
                        fifoIndex++;
                    }

                }   
            }
        } //end if Read

        if (rw == 'W') {
            //check if the page already exists in the ram, then it will just be written on top of it. 
            for (j = 0; j < nframes; j++) { //scanning the RAM to find it in memory (if the process is already in memory).
                //if found need to break and set found to true;
                unsigned writeVPN = addr/4096;
                if (writeVPN == RAM[j].VPN) {
                    //add condition to print only for debug mode 
                    RAM[j].dirty = 1;
                    if(debug == true) {                   
                        printf("Rewriting the page: %x memory reference onto the page %x in RAM.\n", addr, writeVPN);
                    }   
                    found = true;
                    //add the fifoIndex??? maybe...
                    break;//this is a hit we dont need to do anything //we can break.
                } else {
                    found = false;
                }
            }

            //if the page is not located in the ram, we need to find an empty space for it to fit into the ram
            if (!found) {
                int m;
                int emptypages = 0;

                for (m = 0; m < nframes; m++) { //UPDATE: Processes the empty correctly. 
                    if (RAM[m].VPN == 0)        
                        emptypages++;
                }
                    
                if (emptypages == 0)
                    full = true;
                else 
                    full = false;

                if (!full) {
                    int l;
                    for (l = 0; l < nframes; l++) { //run for loop to check for empty frames.
                        if (RAM[l].VPN == 0)  { //if we find the first empty page, we need to write into it
                            RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                            RAM[l].rw = rw; //rw part
                            RAM[l].dirty = 1; //if the operation is write, dirty bit must be 1 then.
                            RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                            RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                            //add condition to print only for debug mode
                            if(debug == true)
                                printf("\nWriting the following memory reference: %x into an empty space %d in RAM. VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", addr, l, RAM[l].VPN, RAM[l].rw, RAM[l].dirty, RAM[l].BASE, RAM[l].BOUND);
                            break;
                        }
                    }
                } else { //ALGORITHM JUICE
                    
                    //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", fifoIndex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[fifoIndex].VPN, RAM[fifoIndex].rw, RAM[fifoIndex].dirty, RAM[fifoIndex].BASE, RAM[fifoIndex].BOUND);
                    }

                    if (RAM[fifoIndex].dirty == 0) {
                        if(debug == true)
                            printf("\nEliminating a clean page %x, no write to the disk required.\n", RAM[fifoIndex].VPN);
                        //rewriting the page
                        RAM[fifoIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[fifoIndex].rw = rw; //rw part
                        RAM[fifoIndex].dirty = 1; //initialize dirty bit to one as the command was W
                        RAM[fifoIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[fifoIndex].BOUND = RAM[fifoIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", fifoIndex, RAM[fifoIndex].VPN, RAM[fifoIndex].rw, RAM[fifoIndex].dirty, RAM[fifoIndex].BASE, RAM[fifoIndex].BOUND); 
                    } else {
                    //over here it bugs out where if the dirty index was switched from 0 to 1 it would perform two page replacements. 
                    //if(RAM[fifoIndex].dirty == 1) {
                        if(debug == true)
                            printf("\nEliminating a dirty page %x, requires a write to the disk.\n", RAM[fifoIndex].VPN);
                        countWrites++;
                        //rewriting the page.
                        RAM[fifoIndex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[fifoIndex].rw = rw; //rw part
                        RAM[fifoIndex].dirty = 1; //initialize dirty bit to one as the command was W
                        RAM[fifoIndex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[fifoIndex].BOUND = RAM[fifoIndex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", fifoIndex, RAM[fifoIndex].VPN, RAM[fifoIndex].rw, RAM[fifoIndex].dirty, RAM[fifoIndex].BASE, RAM[fifoIndex].BOUND);
                        
                    }       
                        if (fifoIndex == nframes-1) {
                        fifoIndex = 0;
                        } else {
                        fifoIndex++;
                        }
                                 
                }
            }
            //if empty space is not found, we need to apply the page replacement algorithm to select which page to evict.
                //if page under eviction is dirty, we need to save it to the disk -> diskWrites++
                //if page under eviction is !dirty, we just replace it. no need to increment anything. 
            
        }
    }

    if (debug == true) {
        //The final Ram 
        printf("The final RAM contents are the following: \n");
        for (int z = 0; z < nframes; z++) {
            printf("Slot: %d, VPN: %x, Dirty: %d\n", z, RAM[z].VPN, RAM[z].dirty);
        }
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
    char READ = 'R';
    char WRITE = 'W';
    bool found;
    bool full = false;
    struct PageEntry RAM[nframes];
    int k;
    int RSS = nframes/2;
    struct PageEntry FIFOA[RSS]; //for processes !=3 stores page numbers 
    struct PageEntry FIFOB[RSS]; //for processes ==3 stores page numbers. 

    int fifoAinsertIndex = 0;
    int fifoBinsertIndex = 0;

    struct PageEntry Clean[RSS + 1];
    struct PageEntry Dirty[RSS + 1];
   
    bool doNothing = false; 

    for (k = 0; k < nframes; k++) { //we need to initialize RAM as NULL all the way through 
        RAM[k].VPN = 0;
        RAM[k].rw = 0;
        RAM[k].dirty = 0; //initialize dirty bit to zero
    }

    for (k = 0; k < RSS; k++) { //we need to initialize RAM as NULL all the way through 
        FIFOA[k].VPN = 0;
        FIFOA[k].rw = 0;
        FIFOA[k].dirty = 0; //initialize dirty bit to zero
        FIFOB[k].VPN = 0;
        FIFOB[k].rw = 0;
        FIFOB[k].dirty = 0; //initialize dirty bit to zero
    }
    for (k = 0; k < RSS+1; k++) { 
        Clean[k].VPN = 0;
        Clean[k].rw = 0;
        Clean[k].dirty = 0; //initialize dirty bit to zero
        Dirty[k].VPN = 0;
        Dirty[k].rw = 0;
        Dirty[k].dirty = 0; //initialize dirty bit to zero
    }

    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
       /* unsigned readVPN = addr/4096; //new page number.
        unsigned process;
        if ((process = readVPN/0x10000)!=3)
            process = 0; // the division to determine the number of the process. 
        else 
            process = 3;

        printf("ProcessInt: %x\n", process);

        int i;
        if (process = 0) { //if new_page in FIFO (Current process)  -> do nothing.
            for (i = 0; i < RSS; i++) {
                if (readVPN == FIFOA[i]) //if new page is in fifo -> do nothing.
                    doNothing = true;
            }

            if (doNothing == false) {
                
                unsigned pageOut = FIFOA[fifoAinsertIndex];

                FIFOA[fifoAinsertIndex] = readVPN;

                if (fifoAinsertIndex == RSS-1)
                    fifoAinsertIndex = 0;
                else 
                    fifoAinsertIndex++;

                if (pageOut != 0) {
                    //if 
                } else {

                }
            }
        } else {
            for (i = 0; i < RSS; i++) {
                if (readVPN == FIFOB[i]) //if new page is in fifo -> do nothing.
                    doNothing = true;
            }
            //in case process number is 3
        }

        //if (doNothing == false) {
            //maintain FIFO and CLEAN/DIRTY buffers.
        //}
        */
    }
    fclose(fp);
}


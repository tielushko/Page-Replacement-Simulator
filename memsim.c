#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <float.h> //for DBL_MAX
#define INT_MAX 2147483647

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
    time_t time_placed; // Time placed is used for LRU. In our LRU Algorithm it pops the "oldest" element in RAM. 
                        // Oldest is defined as greatest time difference from current time.
} PageEntry;

// struct IndexMap { //Holds information for LRU. address holds the VPN and index holds the index where it is located in RAM array
//     unsigned address;
//     int index;
//    // time_t time_placed;

// } IndexMap; //For LRU, if a replacement is required on a full page table: locate the VPN with the minimum index in RAM, remove and rewrite it
//             //By locating the VPN that has the lowest index, we have identified the VPN that has been used LEAST recently

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
    char* temp;
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
                    srand(time(0));
                    int randIndex = (rand() % nframes);
                    //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", randIndex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[randIndex].VPN, RAM[randIndex].rw, RAM[randIndex].dirty, RAM[randIndex].BASE, RAM[randIndex].BOUND);
                    }
                    
                    if (RAM[randIndex].dirty == 0) {
                        //your casual replacement.
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
                    if(RAM[randIndex].dirty == 1) {
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
                        RAM[j].dirty = 1;
                    }
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
                    }

                    if(RAM[randIndex].dirty == 1) {
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
          
        //for W
          
          //if (have space in RAM) -> populate it into the RAM and mark the page as dirty. 
            //writes the process into the RAM and marks it as "dirty"
          //else -> page replacement alogrithm. if page under replacement is dirty, numberWrites to disk is ++ as we need to save that page to disk.
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
      
    char rw; 
    unsigned addr;
    //int i = 0;
    char READ = 'R';
    char WRITE = 'W';
    char* temp;
    bool found;
    bool full = false;
    time_t current_time;
    double diff_t;
    struct PageEntry RAM[nframes];
    //struct IndexMap indexes[nframes]; // Used to hold the recent occurred index of each page in a map-like struct
    int k;

    for (k = 0; k < nframes; k++) { //we need to initialize RAM as NULL all the way through 
        RAM[k].VPN = 0;
        RAM[k].rw = 0;
        RAM[k].dirty = 0; //initialize dirty bit to zero
    }

    // for (k = 0; k < nframes; k++) { //we need to initialize our indexMap
    //     indexes[k].address = 0;
    //     indexes[k].index = 0;
    //     //indexes[k].time_placed = NULL;
    // }

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
                        // Get the time in which x element is placed into RAM
                        current_time = time(NULL);
                        if (current_time == ((time_t)-1)) {
                            printf("Failed to obtain time.");
                            exit(1);
                        }
                        RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[l].rw = rw; //rw part
                        RAM[l].dirty = 0; //initialize dirty bit to zero
                        RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        RAM[l].time_placed = current_time;
                        //update index map with the address and the index it is stored in RAM, as well as the time it was placed
                        // indexes[l].address = addr/4096;
                        // indexes[l].index = l; 
                        // indexes[l].time_placed = current_time;
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
                    int LRUindex;
                    //int lru = INT_MAX;
                    double max_diff = 0;
                    time_t time_found;
                    current_time = time(NULL); // Get current time BEFORE entering loop
                    if (current_time == ((time_t)-1)) {
                            printf("Failed to obtain time.");
                            exit(1);
                    }
                    for (k = 0; k < nframes; k++) { //Scan over index map to identify the oldest VPN
                        diff_t = difftime(current_time, RAM[k].time_placed); // Evaluate the time passed from current time, trying to find element with greatest time difference (oldest)
                        if (diff_t > max_diff) {
                            max_diff = diff_t;
                            LRUindex = k;
                        }
                    }
                    //LRUindex = lru;
                    // //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", LRUindex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[LRUindex].VPN, RAM[LRUindex].rw, RAM[LRUindex].dirty, RAM[LRUindex].BASE, RAM[LRUindex].BOUND);
                    }
                    
                    if (RAM[LRUindex].dirty == 0) {
                        //your casual replacement.
                            time_found = time(NULL); //update the time of the new element with current time
                            if (time_found == ((time_t)-1)) {
                                printf("Failed to obtain time.");
                                exit(1);
                            }
                            RAM[LRUindex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                            RAM[LRUindex].rw = rw; //rw part
                            RAM[LRUindex].dirty = 0; //initialize dirty bit to zero
                            RAM[LRUindex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                            RAM[LRUindex].BOUND = RAM[LRUindex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                            RAM[LRUindex].time_placed = time_found;
                            // indexes[LRUindex].index = LRUindex; // Update index map at our replacement index
                            // indexes[LRUindex].address = addr/4096;
                            // indexes[LRUindex].time_placed = time_found;
                            countReads++;
                            //add condition to print only for debug mode
                            if(debug == true) 
                                printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", LRUindex, RAM[LRUindex].VPN, RAM[LRUindex].rw, RAM[LRUindex].dirty, RAM[LRUindex].BASE, RAM[LRUindex].BOUND);
                    }
                    if(RAM[LRUindex].dirty == 1) {
                        if(debug == true)
                            printf("\nEliminating a dirty page %x, requires a write to the disk.\n", RAM[LRUindex].VPN);
                        countWrites++;
                        //think if this should get replaced...
                        time_found = time(NULL); //update the time of the new element with current time
                        if (time_found == ((time_t)-1)) {
                            printf("Failed to obtain time.");
                            exit(1);
                        }
                        RAM[LRUindex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[LRUindex].rw = rw; //rw part
                        RAM[LRUindex].dirty = 0; //initialize dirty bit to zero
                        RAM[LRUindex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[LRUindex].BOUND = RAM[LRUindex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        RAM[LRUindex].time_placed = time_found;
                        // indexes[LRUindex].index = LRUindex; // Update index map at our replacement index
                        // indexes[LRUindex].address = addr/4096;
                        // indexes[LRUindex].time_placed = time_found;
                        countReads++;
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", LRUindex, RAM[LRUindex].VPN, RAM[LRUindex].rw, RAM[LRUindex].dirty, RAM[LRUindex].BASE, RAM[LRUindex].BOUND);
                        
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
                        RAM[j].dirty = 1;
                    }
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
                        if (RAM[l].VPN == 0)  { //load the first empty page in RAM with information from the disk.
                            // Get the time in which x element is placed into RAM
                            current_time = time(NULL);
                            if (current_time == ((time_t)-1)) {
                                printf("Failed to obtain time.");
                                exit(1);
                            }
                            RAM[l].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                            RAM[l].rw = rw; //rw part
                            RAM[l].dirty = 0; //initialize dirty bit to zero
                            RAM[l].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                            RAM[l].BOUND = RAM[l].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                            RAM[l].time_placed = current_time;
                            //update index map with the address and the index it is stored in RAM, as well as the time it was placed
                            // indexes[l].address = addr/4096;
                            // indexes[l].index = l; 
                            // indexes[l].time_placed = current_time;
                            countReads++;
                            //add condition to print only for debug mode
                            if(debug == true)
                                printf("\nReading from disk and placing the following memory reference: %x into an empty space %d in RAM. VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", addr, l, RAM[l].VPN, RAM[l].rw, RAM[l].dirty, RAM[l].BASE, RAM[l].BOUND);
                            break;
                        }
                    }
                } else { //ALGORITHM JUICE
                    int LRUindex;
                    //int lru = INT_MAX;
                    double max_diff = 0;
                    time_t time_found;
                    current_time = time(NULL); // Get current time BEFORE entering loop
                    if (current_time == ((time_t)-1)) {
                            printf("Failed to obtain time.");
                            exit(1);
                    }
                    for (k = 0; k < nframes; k++) { //Scan over index map to identify the oldest VPN
                        diff_t = difftime(current_time, RAM[k].time_placed); // Evaluate the time passed from current time, trying to find element with greatest time difference (oldest)
                        if (diff_t > max_diff) {
                            max_diff = diff_t;
                            LRUindex = k;
                        }
                    }
                    //LRUindex = lru;
                    //condition to check which index to be eliminated.
                    if(debug == true) {
                        printf("\nIndex to be eliminated %d\n", LRUindex);
                        printf("Page to be eliminated: VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", RAM[LRUindex].VPN, RAM[LRUindex].rw, RAM[LRUindex].dirty, RAM[LRUindex].BASE, RAM[LRUindex].BOUND);
                    }
                    
                    if (RAM[LRUindex].dirty == 0) {
                        if(debug == true)
                            printf("\nEliminating a clean page %x, no write to the disk required.\n", RAM[LRUindex].VPN);
                        //rewriting the page
                        time_found = time(NULL); //update the time of the new element with current time
                        if (time_found == ((time_t)-1)) {
                            printf("Failed to obtain time.");
                            exit(1);
                        }
                        RAM[LRUindex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[LRUindex].rw = rw; //rw part
                        RAM[LRUindex].dirty = 1; //initialize dirty bit to one as the command was W
                        RAM[LRUindex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[LRUindex].BOUND = RAM[LRUindex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        RAM[LRUindex].time_placed = time_found;
                        // indexes[LRUindex].address = addr/4096; // update index map
                        // indexes[LRUindex].index = LRUindex;
                        // indexes[LRUindex].time_placed = time_found;
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", LRUindex, RAM[LRUindex].VPN, RAM[LRUindex].rw, RAM[LRUindex].dirty, RAM[LRUindex].BASE, RAM[LRUindex].BOUND);
                    }

                    if(RAM[LRUindex].dirty == 1) {
                        if(debug == true)
                            printf("\nEliminating a dirty page %x, requires a write to the disk.\n", RAM[LRUindex].VPN);
                        countWrites++;
                        //rewriting the page.
                        time_found = time(NULL); //update the time of the new element with current time
                        if (time_found == ((time_t)-1)) {
                            printf("Failed to obtain time.");
                            exit(1);
                        }
                        RAM[LRUindex].VPN = addr/4096; //we need to divide by 4096 to eliminate the page offset. (in hex 4096 is equal to 0x1000))
                        RAM[LRUindex].rw = rw; //rw part
                        RAM[LRUindex].dirty = 1; //initialize dirty bit to one as the command was W
                        RAM[LRUindex].BASE = addr/4096 * 0x1000; //getting the BASE of the single page. Many of the accesses will be within the same page
                        RAM[LRUindex].BOUND = RAM[LRUindex].BASE + 0xfff; //getting the BOUND of the page. if you add +1, this will technically be the different page.
                        RAM[LRUindex].time_placed = time_found;
                        // indexes[LRUindex].address = addr/4096; // update index map
                        // indexes[LRUindex].index = LRUindex;
                        // indexes[LRUindex].time_placed = time_found;
                        //add condition to print only for debug mode
                        if(debug == true) 
                            printf("\nPage replacement for space %d in RAM. Current VPN: %x RW: %c Dirty: %d Base: %x Bound: %x\n", LRUindex, RAM[LRUindex].VPN, RAM[LRUindex].rw, RAM[LRUindex].dirty, RAM[LRUindex].BASE, RAM[LRUindex].BOUND);
                    }                     
                }
            }
            
        } 
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
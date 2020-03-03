#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
// #include <float.h> //for DBL_MAX
// #define INT_MAX 2147483647

int nframes;
int countReads = 0;
int countWrites = 0;
bool debug;
int nEvents = 0;

//Policy Algorithms
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

struct frame { // Data structed used to hold individual frames in our single-page page-table
    unsigned address;
    int dirty; 
} frame;

struct page_tableQ { // Data structure that contains all the frames in our single-page page-table
    int size;
    int index;
    struct frame *frame_table;
} page_tableQ;

// Helper functions for queue usage in LRU:
void LRUmove(struct page_tableQ *queue, int init, char rw) { // This function is responsible for deleting the head of the queue, moves data back toward the head
                                                    // variable init allows us to pop elements from anywhere in the queue beginning from init
    // if (debug == true) { // outputs weird stuff, not sure why
    //     printf("Removing frame from RAMQ for LRU Upkeep: Address %c  Request Type %c\n", queue->frame_table[init].address, rw );
    // }
    for (int i = init; i < queue->index - 1; i++) {
        queue->frame_table[i].address = queue->frame_table[i + 1].address; // Standard data copy synstax
        queue->frame_table[i].dirty = queue->frame_table[i + 1].dirty;
    }
    queue->frame_table[queue->index - 1].dirty = 0; // Scrub last index, it is now stored in the preceding frame
    queue->frame_table[queue->index - 1].address = 0;
}

void insert(struct page_tableQ *queue, struct frame *found) { // Store information from node found into the queue, not the same as enqueue
    queue->frame_table[queue->index].dirty = found->dirty;
    queue->frame_table[queue->index].address = found->address;
    queue->index = queue->index + 1; // Incremement index location, we just added an element into the rear of the queue
}

void dequeue(struct page_tableQ *queue) { // Pops the head of the queue, shifts elements toward head of queue
    for (int j = 0; j < queue->index; j++) {
        queue->frame_table[j].address = queue->frame_table[j + 1].address; // Remove element by copying data from the succeeding element
        queue->frame_table[j].dirty = queue->frame_table[j + 1].dirty; // Standard data copy syntax
    }
    // Now we need to clean the data in the last element of our queue which is not empty since we shifted down towards head
    queue->frame_table[queue->index - 1].address = 0;
    queue->frame_table[queue->index - 1].dirty = 0;
    //Update index location
    if (queue->index == 0) {
        printf("Empty Queue");
    }
    else
        queue->index = queue->index - 1; // Since we deleted an element, decrement queue index variable. Now holds accurate location of the rear of queue
}

bool find(struct page_tableQ *queue, struct frame *toFind) {
    for  (int k = 0; k < queue->index; k++) {
        if (queue->frame_table[k].address == toFind->address) { // Loop through the frame table, if you find the hexadecimal VPN then it exists already in frame table
            return true;
        }
    }
    return false;
} 

int findLoc(struct page_tableQ *queue, struct frame *current) {
    for (int L = 0; L < queue->index; L++) {
        if (queue->frame_table[L].address == current->address) { // simple scan and check to find the index of the frame we're looking for
            return L;
        }
    }
    return -1;
}


 

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
    // LRU removes the element that was least recently used, which is the oldest element.
    char rw; 
    unsigned addr;
    char READ = 'R';
    char WRITE = 'W';
    FILE *fp;
    fp = fopen(filename, "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }

    struct page_tableQ *RAMQ;
    RAMQ = (struct page_tableQ *)malloc(sizeof(struct page_tableQ));
    RAMQ->frame_table = (struct frame *)malloc(sizeof(struct frame) * nframes);
    RAMQ->size = nframes;
    RAMQ->index = 0;
    // initialize our frame table
    for (int z = 0; z < nframes; z++) {
        RAMQ->frame_table[z].dirty = 0;
        RAMQ->frame_table[z].address = 0;
    }
    
    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        // Record data of current trace
        struct frame *current;
        current = (struct frame *)malloc(sizeof(struct frame));
        current->address = addr/4096;
        current->dirty = 0; //initialize
        nEvents++; // Increment trace counter

        if (debug == true) {
            printf("Page request for %x, operation requested: %c\n", current->address, rw);
        }

        // SECTION 1: WHEN TRACE IS A 'W' REQUEST
        if (rw == 'W' || rw == 'w') {
            current->dirty == 1; // Update flag to account for W request
            // Need to check if current frame already exists in RAMQ
            if (find(RAMQ, current) == true) {
                LRUmove(RAMQ, findLoc(RAMQ, current), rw); // Update queue to maintain LRU policy. Move elements toward front from current
                RAMQ->index = RAMQ->index - 1;         // if frame table in RAMQ is 1 2 3 4 5 6 (1 is head, 6 is rear, 3 is curr), then LRUmove(RAMQ, findLoc(RAMQ, current)) 
                insert(RAMQ, current);                 // will perform operations that return RAMQ in the form: 1 2 4 5 6 
                                                       // the insert(RAMQ, current) will return RAMQ in the form: 1 2 4 5 6 3 now that 3 is MOST recently used
            }

            else {// Otherwise the frame is not in our frame table and we need to perform LRU to place it
                if (RAMQ->index == RAMQ->size) {// Case 1: RAMQ is full and we need to place a new frame
                    if (RAMQ->frame_table[0].dirty == 1) { // Check if the head of the queue is a dirty frame
                        countWrites++; // Increment write counter to account for the disk operation due to dirty RAMQ frame
                    }
                    LRUmove(RAMQ, 0, rw); // Perform LRU on the head (least recent) of the queue
                    RAMQ->index = RAMQ->index - 1; // Decrememnt our index, we just popped the head of the queue
                }
                countReads++; // Increment reads, have to go to disk to find this frame
                current->dirty = 1; // Since request is a W, change dirty bit to 1 
                insert(RAMQ, current); // Standard insert, if RAMQ is full the LRUmove has already occured in case 1, otherwise RAMQ not full and we can insert regularly.

            }

            if (debug == true) { 
                printf("RAMQ has been appended with trace: Address %x       Request Type: %c\n", current->address, rw);
            }

        }
        // SECTION 2: TRACE IS A 'R' REQUEST
        else {
            // Need to check if current frame already exists in RAMQ
            if (find(RAMQ, current) == true) {
                LRUmove(RAMQ, findLoc(RAMQ, current), rw); // Update queue to maintain LRU policy. Move elements toward front from current
                RAMQ->index = RAMQ->index - 1;         // if frame table in RAMQ is 1 2 3 4 5 6 (1 is head, 6 is rear, 3 is curr), then LRUmove(RAMQ, findLoc(RAMQ, current)) 
                insert(RAMQ, current);                 // will perform operations that return RAMQ in the form: 1 2 4 5 6 
                                                       // the insert(RAMQ, current) will return RAMQ in the form: 1 2 4 5 6 3 now that 3 is MOST recently used
            }

            else { // Otherwise the frame is not in our frame table and we need to perform LRU to place it
                if (RAMQ->index == RAMQ->size) {// Case 1: RAMQ is full and we need to place a new frame
                    if (RAMQ->frame_table[0].dirty == 1) { // Check if the head of the queue is a dirty frame
                        countWrites++; // Increment write counter to account for the disk operation due to dirty RAMQ frame
                    }
                    LRUmove(RAMQ, 0, rw); // Perform LRU on the head (least recent) of the queue
                    RAMQ->index = RAMQ->index - 1; // Decrememnt our index, we just popped the head of the queue
                }
                countReads++; // Increment reads, have to go to disk to find this frame
                //Don't need to update curr->dirty here, request was not a 'W'
                insert(RAMQ, current); // Standard insert, if RAMQ is full the LRUmove has already occured in case 1, otherwise RAMQ not full and we can insert regularly.
                
            }

            if (debug == true) { 
                printf("RAMQ has been appended with trace: Address %x       Request Type: %c\n", current->address, rw);
            }

        }
        if (debug == true) { // print our frame table for debug display 
            printf("PRINTING TABLE FOR DISPLAY\n");
            for (int p = 0; p < nframes; p++){
                printf("Index in Queue: %d    VPN: %x   Dirty: %d\n", p, RAMQ->frame_table[p].address, RAMQ->frame_table[p].dirty);
            }
        }
    } // end of while loop
    
    free(RAMQ->frame_table);
    free(RAMQ);
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
    
    while (fscanf(fp, "%x %c", &addr, &rw) != EOF) {
        printf("%x %c\n", addr, rw);
    
    }
    fclose(fp);
}
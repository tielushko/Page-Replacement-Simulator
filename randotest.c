#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

struct page_tableQ { // Data structure that contains all the frames in our single-page page-table
    int size;
    int index;
    struct frame *frame_table;
} page_tableQ;

struct frame { // Data structed used to hold individual frames in our single-page page-table
    unsigned address;
    int dirty; 
} frame;

void insert(struct page_tableQ *queue, struct frame *found) { // Store information from node found into the queue, not the same as enqueue
    queue->frame_table[queue->index].dirty = found->dirty;
    queue->frame_table[queue->index].address = found->address;
    queue->index = queue->index + 1; // Incremement index location, we just added an element into the queue
}
void dequeue(struct page_tableQ *queue) { // When an element is popped, the order is adjusted to maintain the Page Table Queue.
    for (int j = 0; j < queue->index; j++) {
        queue->frame_table[j].address = queue->frame_table[j + 1].address; // Remove element by copying data from the succeeding element
        queue->frame_table[j].dirty = queue->frame_table[j + 1].dirty; // Standard data copy syntax
    }
    // Now we need to clean the data in the last element of our queue
    queue->frame_table[queue->index - 1].address = 0;
    queue->frame_table[queue->index - 1].dirty = 0;
    //Update index location
    if (queue->index == 0) {
        printf("Empty Queue");
    }
    else
        queue->index = queue->index - 1; // Since we deleted an element, decrement queue index var
}

int main(int argc, char **argv)
{
    int traces = 0;
    char rw; 
    unsigned addr;
    char READ = 'R';
    char WRITE = 'W';
    int nFrames;
    printf("How many frames?\n");
    scanf("%d", &nFrames);
    struct page_tableQ *RAMQ;

    RAMQ = (struct page_tableQ *)malloc(sizeof(struct page_tableQ));
    RAMQ->frame_table = (struct frame *)malloc(sizeof(struct frame) * nFrames);
    RAMQ->size = nFrames;
    RAMQ->index = 0;

    FILE *fp;
    fp = fopen("test.trace", "r"); // Open our file.

    if (fp == NULL) { //Verify it was successful
        printf("Error opening file.\n");
        exit(0);
    }
    
    int k = 0;
    while ( (k < nFrames) && (fscanf(fp, "%x %c", &addr, &rw) != EOF) ) {
        struct frame *current;
        current = (struct frame *)malloc(sizeof(struct frame));
        current->address = addr/4096;
        current->dirty = 0;
        traces++;
        insert(RAMQ, current);
        printf("Index in Queue: %d    VPN: %x   Dirty: %d\n", RAMQ->index, RAMQ->frame_table[k].address, RAMQ->frame_table[k].dirty);
        k++;
        
    }

    dequeue(RAMQ);
    printf("Dequque exectuted\n");
    for (int i = 0; i < nFrames - 1; i++){
      printf("Index in Queue: %d    VPN: %x   Dirty: %d\n", RAMQ->index, RAMQ->frame_table[i].address, RAMQ->frame_table[i].dirty);
    }

    // printf("Printing Q for display and testing\n");
    // for (int i = 0; i < nFrames; i++) {
    //   printf("Index in Queue: %d    VPN: %x   Dirty: %d\n", RAMQ->index, RAMQ->frame_table[i].address, RAMQ->frame_table[i].dirty);
    // }

  return 0;
}
// include other header files as needed
#include "mems.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#define HOLE 0
#define PROCESS 1

#define PAGE_SIZE 4096

// Define a structure for the main chain of the free list
struct MainChainNode {
    struct SubChainNode* subChainHead;
    struct MainChainNode* next;
};


// Define a structure for the sub-chain nodes
struct SubChainNode {
    size_t size;
    int status; 
    void* virtual_address;
    struct SubChainNode* prev;
    struct SubChainNode* next;
};


static struct MainChainNode* freeListHead = NULL;

static void addSubChainNode(struct MainChainNode* mainNode, size_t size, int status) {
    struct SubChainNode* newSubNode = (struct SubChainNode*)malloc(sizeof(struct SubChainNode));
    if (newSubNode == NULL) {
        // Handle memory allocation error
        fprintf(stderr, "Error: Failed to allocate memory for a new SubChainNode.\n");
        return;
    }

    newSubNode->size = size;
    newSubNode->status = status; 
    newSubNode->prev = NULL;     
    newSubNode->next = NULL;

    if (mainNode->subChainHead == NULL) {
        mainNode->subChainHead = newSubNode;
    } else {
        struct SubChainNode* lastNode = mainNode->subChainHead;
        while (lastNode->next != NULL) {
            lastNode = lastNode->next;
        }
        lastNode->next = newSubNode;
        newSubNode->prev = lastNode;
    }
}
void mems_init() {
    // Initialize the MeMS system
    // This includes initializing the free list, creating the initial node, etc.
}

void mems_finish() {
    // Clean up the MeMS system, unmap any remaining memory, and release resources
}

void* mems_malloc(size_t size) {
    // Implement memory allocation logic
    // Check if a segment in the free list can fulfill the request, if not, use mmap
    // Update the free list accordingly and return a MeMS virtual address
}

void mems_free(void* v_ptr) {

    struct MainChainNode* currentNode = freeListHead;
    
while (currentNode != NULL) {
    struct SubChainNode* subChainNode = currentNode->subChainHead;
    while (subChainNode != NULL) {
        if (subChainNode->virtual_address == v_ptr) {
            subChainNode->status = HOLE;

            return;
        }
        subChainNode = subChainNode->next; 
    }
    currentNode = currentNode->next;
}

}

void mems_print_stats() {
    int totalPagesUsed = 0;
    size_t totalUnusedMemory = 0;

    struct MainChainNode* mainNode = freeListHead;
    while (mainNode != NULL) {
        printf("Main Chain Node: (Details about this node)\n");

        struct SubChainNode* subNode = mainNode->subChainHead;
        while (subNode != NULL) {
            printf("Sub-Chain Node: (Details about this segment)\n");

            if (subNode->status == HOLE) {
                totalUnusedMemory += subNode->size;
            }

            subNode = subNode->next;
        }

        mainNode = mainNode->next;
    }

    printf("Total Pages Used: %d\n", totalPagesUsed);
    printf("Total Unused Memory: %zu bytes\n", totalUnusedMemory);
}


void* mems_get(void* v_ptr) {
    // Convert a MeMS virtual address to a MeMS physical address
    // This function allows users to access the allocated memory
}
//--------------------------------------------------------------------//

int main(int argc, char const *argv[])
{
    // initialise the MeMS system 
    mems_init();
    int* ptr[10];

    /*
    This allocates 10 arrays of 250 integers each
    */
    printf("\n------- Allocated virtual addresses [mems_malloc] -------\n");
    for(int i=0;i<10;i++){
        ptr[i] = (int*)mems_malloc(sizeof(int)*250);
        printf("Virtual address: %lu\n", (unsigned long)ptr[i]);
    }

    /*
    In this section we are tring to write value to 1st index of array[0] (here it is 0 based indexing).
    We get get value of both the 0th index and 1st index of array[0] by using function mems_get.
    Then we write value to 1st index using 1st index pointer and try to access it via 0th index pointer.

    This section is show that even if we have allocated an array using mems_malloc but we can 
    retrive MeMS physical address of any of the element from that array using mems_get. 
    */
    printf("\n------ Assigning value to Virtual address [mems_get] -----\n");
    // how to write to the virtual address of the MeMS (this is given to show that the system works on arrays as well)
    int* phy_ptr= (int*) mems_get(&ptr[0][1]); // get the address of index 1
    phy_ptr[0]=200; // put value at index 1
    int* phy_ptr2= (int*) mems_get(&ptr[0][0]); // get the address of index 0
    printf("Virtual address: %lu\tPhysical Address: %lu\n",(unsigned long)ptr[0],(unsigned long)phy_ptr2);
    printf("Value written: %d\n", phy_ptr2[1]); // print the address of index 1 

    /*
    This shows the stats of the MeMS system.  
    */
    printf("\n--------- Printing Stats [mems_print_stats] --------\n");
    mems_print_stats();

    /*
    This section shows the effect of freeing up space on free list and also the effect of 
    reallocating the space that will be fullfilled by the free list.
    */
    printf("\n--------- Freeing up the memory [mems_free] --------\n");
    mems_free(ptr[3]);
    mems_print_stats();
    ptr[3] = (int*)mems_malloc(sizeof(int)*250);
    mems_print_stats();
    return 0;
}

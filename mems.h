/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions 
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#define HOLE 0
#define PROCESS 1


/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this 
macro to make the output of all system same and conduct a fair evaluation. 
*/
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

static void addSubChainNode(struct MainChainNode* mainNode, size_t size, int status, void* v_addr) {
    struct SubChainNode* newSubNode = (struct SubChainNode*)mmap(v_addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (newSubNode == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    newSubNode->size = size;
    newSubNode->status = status; 
    newSubNode->virtual_address = v_addr;
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

/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_init(){
    freeListHead = (struct MainChainNode*)mmap(NULL, sizeof(struct MainChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (freeListHead == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    freeListHead->subChainHead = NULL;
    freeListHead->next = NULL;
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish(){
    struct MainChainNode* currentNode = freeListHead;
    while (currentNode != NULL) {
        struct SubChainNode* subChainNode = currentNode->subChainHead;
        while (subChainNode != NULL) {
            struct SubChainNode* temp = subChainNode;  // Define temp here
            subChainNode = subChainNode->next;
            munmap(temp, temp->size);
        }
        struct MainChainNode* temp = currentNode;
        currentNode = currentNode->next;
        munmap(temp, sizeof(struct MainChainNode));
    }
    freeListHead = NULL;
    
}


/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available. 

Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/ 
void* mems_malloc(size_t size){
    struct MainChainNode* currentNode = freeListHead;

    while (currentNode != NULL) {
        struct SubChainNode* subChainNode = currentNode->subChainHead;
        while (subChainNode != NULL) {
            if (subChainNode->status == 0 && subChainNode->size >= size) {
                // Found a suitable segment in the free list
                if (subChainNode->size > size) {
                    // Split the segment if it's larger than required
                    size_t newSize = subChainNode->size - size;
                    subChainNode->size = size;

                    // Create a new sub-chain node for the remaining space
                    addSubChainNode(currentNode, newSize, 0, subChainNode->virtual_address + size);
                }

                subChainNode->status = 1; // Set the status to PROCESS
                return subChainNode->virtual_address;
            }
            subChainNode = subChainNode->next;
        }
        currentNode = currentNode->next;
    }

    // If no suitable segment is found, use mmap
    void* newMemory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (newMemory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Add the newly mapped memory to the free list
    addSubChainNode(currentNode, size, 1, newMemory);
    return newMemory;
}


/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
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
            } else if (subNode->status == PROCESS) {
                totalPagesUsed++;
            }

            subNode = subNode->next;
        }

        mainNode = mainNode->next;
    }

    printf("Total Pages Used: %d\n", totalPagesUsed);
    printf("Total Unused Memory: %zu bytes\n", totalUnusedMemory);
}



/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void*v_ptr){
    
}


/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
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

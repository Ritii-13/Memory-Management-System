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
#include <stdbool.h>
#include <stdint.h>

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

// Define a structure to represent a memory segment
struct MemorySegment {
    void* start_address;
    size_t size;
    struct MemorySegment* next;
};
static struct MainChainNode* freeListHead = NULL;
void * get_start_virtual_address()
{
    return (void*) 0x7f7f7f7f7f7f;
}
// // Calculate the virtual address for the next allocation
// void* get_virtual_address(void* p_ptr) {
//     // Calculate the MeMS virtual address based on your mapping
//     uintptr_t mems_physical_base = 140535789383680;
//     uintptr_t offset = (uintptr_t)p_ptr - mems_physical_base;
//     uintptr_t mems_virtual_address = 0x7f7f7f7f7f7f; // Replace with the actual virtual address

//     // Return the MeMS virtual address
//     return (void*)mems_virtual_address;
// }
void* get_virtual_address(void* p_ptr) {
    // Calculate the virtual address for the next allocation
    if (freeListHead != NULL) {
        // If there is a main chain, allocate sequentially
        void* next_virtual_address = freeListHead->subChainHead->virtual_address +
                                   freeListHead->subChainHead->size;
        next_virtual_address = (void*)((uintptr_t)next_virtual_address & ~(PAGE_SIZE - 1));

        while (next_virtual_address == freeListHead->subChainHead->virtual_address +
                                     freeListHead->subChainHead->size) {
            // Continue to the next page if the previous one was fully used
            next_virtual_address = (void*)((uintptr_t)next_virtual_address + PAGE_SIZE);
        }

        return next_virtual_address;
    } else {
        // If no suitable HOLE segment is found, and there is no main chain, start from the beginning
        void* start_virtual_address = get_start_virtual_address();

        // Calculate the aligned starting virtual address based on PAGE_SIZE
        uintptr_t aligned_virtual_address = (uintptr_t)start_virtual_address;

        while (aligned_virtual_address % PAGE_SIZE != 0) {
            aligned_virtual_address++;
        }

        return (void*)aligned_virtual_address;
    }
}
// Create a global pointer to the first memory segment
struct MemorySegment* allocated_segments = NULL;

bool is_valid_virtual_address(void* v_ptr) {
    // Iterate through your allocated memory segments and check if v_ptr falls within any of them
    struct MemorySegment* segment = allocated_segments;
    
    while (segment != NULL) {
        void* start_address = segment->start_address;
        void* end_address = start_address + segment->size;
        
        if (v_ptr >= start_address && v_ptr < end_address) {
            // v_ptr is within an allocated segment, so it's a valid virtual address
            return true;
        }
        
        segment = segment->next; // Move to the next segment
    }

    // If no matching segment is found, v_ptr is not a valid virtual address
    return false;
}





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
void mems_init() {
    freeListHead = (struct MainChainNode*)mmap(NULL, sizeof(struct MainChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (freeListHead == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    freeListHead->subChainHead = (struct SubChainNode*)mmap(NULL, sizeof(struct SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (freeListHead->subChainHead == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    freeListHead->next = NULL;
    freeListHead->subChainHead->size = PAGE_SIZE;
    freeListHead->subChainHead->status = HOLE;
    freeListHead->subChainHead->virtual_address = get_start_virtual_address();
    freeListHead->subChainHead->prev = NULL;
    freeListHead->subChainHead->next = NULL;
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish() {
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
void* mems_malloc(size_t size) {
    struct MainChainNode* currentNode = freeListHead;
    struct MainChainNode* mainNode = NULL;

    while (currentNode != NULL) {
        struct SubChainNode* subChainNode = currentNode->subChainHead;
        while (subChainNode != NULL) {
            if (subChainNode->status == HOLE && subChainNode->size >= size) {
                // Found a suitable HOLE segment in the free list
                if (subChainNode->size > size) {
                    // Split the segment if it's larger than required
                    size_t newSize = subChainNode->size - size;
                    subChainNode->size = size;

                    // Create a new sub-chain node for the remaining space
                    addSubChainNode(currentNode, newSize, HOLE, subChainNode->virtual_address + size);
                }

                subChainNode->status = PROCESS; // Set the status to PROCESS
                return subChainNode->virtual_address;
            }
            subChainNode = subChainNode->next;
        }
        // Move to the next main chain node
        mainNode = currentNode;
        currentNode = currentNode->next;
    }

    // Calculate the virtual address for the next allocation
    if (mainNode != NULL) {
        // If there is a main chain, allocate sequentially
        void* next_virtual_address = mainNode->subChainHead->virtual_address +
                                   mainNode->subChainHead->size;
        next_virtual_address = (void*)((uintptr_t)next_virtual_address & ~(PAGE_SIZE - 1));

        while (next_virtual_address == mainNode->subChainHead->virtual_address +
                                     mainNode->subChainHead->size) {
            // Continue to the next page if the previous one was fully used
            next_virtual_address = (void*)((uintptr_t)next_virtual_address + PAGE_SIZE);
        }

        // Use mmap to allocate memory
        void* newMemory = mmap(next_virtual_address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (newMemory == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }

        // Add the newly mapped memory to the free list as a new sub-chain
        struct SubChainNode* newSubChainNode = (struct SubChainNode*)newMemory;
        newSubChainNode->size = size;
        newSubChainNode->status = PROCESS;
        newSubChainNode->virtual_address = newMemory;
        newSubChainNode->prev = NULL;
        newSubChainNode->next = mainNode->subChainHead;

        mainNode->subChainHead->prev = newSubChainNode;
        mainNode->subChainHead = newSubChainNode;

        return newMemory;
    } else {
        // If no suitable HOLE segment is found, and there is no main chain, start from the beginning
        void* start_virtual_address = get_start_virtual_address();

        // Calculate the aligned starting virtual address based on PAGE_SIZE
        uintptr_t aligned_virtual_address = (uintptr_t)start_virtual_address;

        while (aligned_virtual_address % PAGE_SIZE != 0) {
            aligned_virtual_address++;
        }

        // Use mmap to allocate memory
        void* newMemory = mmap((void*)aligned_virtual_address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (newMemory == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }

        // Create a new main chain and sub-chain for the allocated memory
        struct MainChainNode* newMainNode = (struct MainChainNode*)newMemory;
        newMainNode->subChainHead = (struct SubChainNode*)newMemory;
        newMainNode->next = NULL;

        freeListHead = newMainNode;

        newMainNode->subChainHead->size = size;
        newMainNode->subChainHead->status = PROCESS;
        newMainNode->subChainHead->virtual_address = newMemory;
        newMainNode->subChainHead->prev = NULL;
        newMainNode->subChainHead->next = NULL;

        return get_virtual_address(newMemory);
    }
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

    printf("--------- Printing Stats [mems_print_stats] ---------\n");

    struct MainChainNode* mainNode = freeListHead;
    int mainChainLength = 0;

    while (mainNode != NULL) {
        printf("----- MeMS SYSTEM STATS -----\n");
        printf("MAIN[%lu:%lu]-> ", (unsigned long)mainNode->subChainHead->virtual_address, (unsigned long)(mainNode->subChainHead->virtual_address + mainNode->subChainHead->size - 1));

        struct SubChainNode* subNode = mainNode->subChainHead;
        int subChainLength = 0;

        while (subNode != NULL) {
            printf("%s[%lu:%lu] <-> ", (subNode->status == PROCESS) ? "P" : "H", (unsigned long)subNode->virtual_address, (unsigned long)(subNode->virtual_address + subNode->size - 1));

            if (subNode->status == HOLE) {
                totalUnusedMemory += subNode->size;
            } else if (subNode->status == PROCESS) {
                totalPagesUsed++;
            }

            subNode = subNode->next;
            subChainLength++;
        }

        printf("NULL\n");
        mainNode = mainNode->next;
        mainChainLength++;
    }

    printf("Pages used:\t%d\n", totalPagesUsed);
    printf("Space unused:\t%zu\n", totalUnusedMemory);
    printf("Main Chain Length:\t%d\n", mainChainLength);

    // Print sub-chain length array
    int subChainLengths[mainChainLength];
    mainNode = freeListHead;
    int i = 0;

    while (mainNode != NULL) {
        struct SubChainNode* subNode = mainNode->subChainHead;
        subChainLengths[i] = 0;

        while (subNode != NULL) {
            subNode = subNode->next;
            subChainLengths[i]++;
        }

        mainNode = mainNode->next;
        i++;
    }

    printf("Sub-chain Length array: [");
    for (i = 0; i < mainChainLength; i++) {
        printf("%d", subChainLengths[i]);
        if (i < mainChainLength - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}



/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void* v_ptr) {
    if (v_ptr == NULL) {
        fprintf(stderr, "Error: Attempt to access a NULL virtual address.\n");
        return NULL;
    }

    // Calculate the MeMS physical address based on your mapping
    uintptr_t mems_virtual_base = (uintptr_t)get_start_virtual_address();

    // You should have a mechanism to verify if v_ptr is a valid virtual address.
    if (!is_valid_virtual_address(v_ptr)) {
        fprintf(stderr, "Error: Invalid virtual address.\n");
        return NULL;
    }

    uintptr_t offset = (uintptr_t)v_ptr - mems_virtual_base;
    uintptr_t mems_physical_address = 140535789383680; // Replace with the actual physical address

    // Return the MeMS physical address
    return (void*)mems_physical_address;
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
                // Mark the sub-chain node as HOLE
                subChainNode->status = HOLE;

                // Check for consecutive HOLE segments and merge them
                if (subChainNode->prev != NULL && subChainNode->prev->status == HOLE) {
                    // Merge with the previous HOLE segment
                    subChainNode->prev->size += subChainNode->size;
                    subChainNode->prev->next = subChainNode->next;
                    if (subChainNode->next != NULL) {
                        subChainNode->next->prev = subChainNode->prev;
                    }
                    free(subChainNode); // Free the current node
                    subChainNode = subChainNode->prev; // Move to the merged segment
                }

                if (subChainNode->next != NULL && subChainNode->next->status == HOLE) {
                    // Merge with the next HOLE segment
                    subChainNode->size += subChainNode->next->size;
                    subChainNode->next = subChainNode->next->next;
                    if (subChainNode->next != NULL) {
                        subChainNode->next->prev = subChainNode;
                    }
                    free(subChainNode->next); // Free the next node
                }
                
                printf("----- MeMS SYSTEM STATS -----\n");
                mems_print_stats(); // Print the updated stats

                return;
            }
            subChainNode = subChainNode->next;
        }
        currentNode = currentNode->next;
    }
}


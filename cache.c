/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))

/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

// Use this when calling printAction. Do not modify the enumerated type below.
enum actionType { cacheToProcessor, processorToCache, memoryToCache, cacheToMemory, cacheToNowhere };

/* You may add or remove variables from these structs */
typedef struct blockStruct {
    int valid;
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
} blockStruct;

typedef struct cacheStruct {
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

blockStruct* getBlock(int set, int block) {
    return &(cache.blocks[set * cache.blocksPerSet + block]);
}

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet) {
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n", numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n", blocksPerSet, numSets);

    // Your code here
    cache.numSets = numSets;
    cache.blockSize = blockSize;
    cache.blocksPerSet = blocksPerSet;
    for (int i = 0; i < numSets; ++i)
        for (int j = 0; j < blocksPerSet; ++j) {
            blockStruct* theBlock = getBlock(i, j);
            theBlock->dirty = 0;
            theBlock->lruLabel = j;
            theBlock->valid = 0;
            theBlock->tag = 0;
        }
    return;
}

void adjustLru(int set, int updatedBlock) {
    blockStruct* theBlock = getBlock(set, updatedBlock);
    int x = theBlock->lruLabel;  // adjust lru
    theBlock->lruLabel = cache.blocksPerSet - 1;
    for (int jj = 0; jj < cache.blocksPerSet; ++jj) {
        blockStruct* theTempBlock = getBlock(set, jj);
        if (theTempBlock != theBlock && theTempBlock->lruLabel > x)
            theTempBlock->lruLabel--;
    }
    // printf("lru label for set %d:\n", set);
    // for (int j = 0; j < cache.blocksPerSet; ++j) {
    //     printf("%d ", getBlock(set, j)->lruLabel);
    // }
    // printf("\n");
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data) {
    /* The next line is a placeholder to connect the simulator to
    memory with no cache. You will remove this line and implement
    a cache which interfaces between the simulator and memory. */
    // printf("================ %d %d %d\n", addr, write_flag, write_data);
    int targetSet = (addr / cache.blockSize) % cache.numSets;
    int targetTag = addr / (cache.blockSize * cache.blocksPerSet);
    int targetLine = addr % cache.blockSize;
    int addrStart = targetTag * cache.blocksPerSet * cache.blockSize;
    if (write_flag) {  // writes
        for (int j = 0; j < cache.blocksPerSet; ++j) {
            blockStruct* theBlock = getBlock(targetSet, j);
            if (theBlock->valid && targetTag == theBlock->tag) {  // hit
                adjustLru(targetSet, j);
                printAction(addr, 1, processorToCache);
                theBlock->dirty = 1;
                theBlock->data[targetLine] = write_data;
                return 0;
            }
        }
        // miss
        for (int j = 0; j < cache.blocksPerSet; ++j) {
            blockStruct* theBlock = getBlock(targetSet, j);
            if (theBlock->lruLabel == 0) {
                if (theBlock->valid && theBlock->dirty) {  // about to overwrite! write back to mem
                    int oldAddrStart = theBlock->tag * cache.blocksPerSet * cache.blockSize;
                    printAction(oldAddrStart, cache.blockSize, cacheToMemory);
                    for (int k = 0; k < cache.blockSize; ++k) {
                        mem_access(oldAddrStart + k, 1, theBlock->data[k]);
                    }
                } else if (theBlock->valid) {
                    int oldAddrStart = theBlock->tag * cache.blocksPerSet * cache.blockSize;
                    printAction(oldAddrStart, cache.blockSize, cacheToNowhere);
                }

                theBlock->dirty = 1;
                theBlock->valid = 1;
                theBlock->tag = targetTag;
                adjustLru(targetSet, j);
                printAction(addrStart, cache.blockSize, memoryToCache);
                for (int k = 0; k < cache.blockSize; ++k) {
                    theBlock->data[k] = mem_access(addrStart + k, 0, 0);
                }
                printAction(addr, 1, processorToCache);
                theBlock->data[targetLine] = write_data;
                return 0;
            }
        }
    } else {  // reads
        for (int j = 0; j < cache.blocksPerSet; ++j) {
            blockStruct* theBlock = getBlock(targetSet, j);
            if (theBlock->valid && targetTag == theBlock->tag) {  // hit
                adjustLru(targetSet, j);
                printAction(addr, 1, cacheToProcessor);
                return theBlock->data[targetLine];
            }
        }
        // miss
        for (int j = 0; j < cache.blocksPerSet; ++j) {
            blockStruct* theBlock = getBlock(targetSet, j);
            if (theBlock->lruLabel == 0) {                 // read from mem to cache
                if (theBlock->valid && theBlock->dirty) {  // about to overwrite! write back to mem
                    int oldAddrStart = theBlock->tag * cache.blocksPerSet * cache.blockSize;
                    printAction(oldAddrStart, cache.blockSize, cacheToMemory);
                    for (int k = 0; k < cache.blockSize; ++k) {
                        mem_access(oldAddrStart + k, 1, theBlock->data[k]);
                    }
                } else if (theBlock->valid) {
                    int oldAddrStart = theBlock->tag * cache.blocksPerSet * cache.blockSize;
                    printAction(oldAddrStart, cache.blockSize, cacheToNowhere);
                }
                theBlock->tag = targetTag;
                theBlock->dirty = 0;
                theBlock->valid = 1;
                adjustLru(targetSet, j);
                printAction(addrStart, cache.blockSize, memoryToCache);
                for (int k = 0; k < cache.blockSize; ++k) {
                    theBlock->data[k] = mem_access(addrStart + k, 0, 0);
                }
                printAction(addr, 1, cacheToProcessor);
                return theBlock->data[targetLine];
            }
        }
    }
}

/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void) {
    return;
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type) {
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    } else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    } else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    } else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    } else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    } else {
        printf("Error: unrecognized action\n");
        exit(1);
    }
}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void) {
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}

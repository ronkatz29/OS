#include "VirtualMemory.h"
#include "PhysicalMemory.h"


// ----- Helper Functions --------
/**
 * Check if the address that is given to our library are not too big.
 */
bool AddressChecker(uint64_t addressToCheck) {
    return (addressToCheck < VIRTUAL_MEMORY_SIZE);
}

/**
 * A recursive function that travel the tree.
 * @param maxFrame - saves the max frame we encounter.
 *                   saving it for the second options to choose a new frame too.
 * @param originFatherAddress - the node address we are looking to have a child too.
 *                              Saving it because this node cant be the node that we return.
 * @param currDepth - Keep track about the current depth, for knowing if we got to the leaf's.
 * @param currNode - the current node we are at.
 * @param myFather - the current node father.
 *
 * Arguments for the eviction calculation.
 * @param mostDistNode
 * @param mostDistNodeFather
 * @param virtualMemoryAddress - the origin virtual memory address we are locking a space for.
 * @param currPageAddress - Keep track the virtual address we are.
 * @param mostDistNodeVirtualAddress
 * @param mostDist
 * @return
 */
word_t TravelTree(word_t *maxFrame, word_t originFatherAddress, uint64_t currDepth, word_t currNode, uint64_t myFather,
                  word_t *mostDistNode, uint64_t *mostDistNodeFather, uint64_t virtualMemoryAddress,
                  uint64_t currPageAddress,
                  uint64_t *mostDistNodeVirtualAddress, uint64_t *mostDist) {
    //Stopping the recursive call if we got to the leaf's.
    if (currDepth == TABLES_DEPTH) {
        //Calculation for to evict part
        //be chosen is the frame containing a page p such that the cyclical distance:
        // min{NUM_PAGES - |page_swapped_in - p|, |page_swapped_in - p| is maximal
        uint64_t abs = currPageAddress - virtualMemoryAddress;
        if (virtualMemoryAddress > currPageAddress)
            abs = virtualMemoryAddress - currPageAddress;

        uint64_t firstDistance = NUM_PAGES - abs;
        uint64_t finalDistance = firstDistance;
        if (abs < firstDistance)
            finalDistance = abs;

        if (*mostDist < finalDistance) {
            *mostDist = finalDistance;
            *mostDistNodeFather = myFather;
            *mostDistNode = currNode;
            *mostDistNodeVirtualAddress = currPageAddress;
        }
        return 0;
    }

    bool allZero = true;
    for (int i = 0; i < PAGE_SIZE; ++i) { //For each node traveling on all of his children.
        word_t currSon = 0;
        PMread(currNode * PAGE_SIZE + i, &currSon);

        if (*maxFrame < currSon) {//update max depth we reached
            *maxFrame = currSon;
        }

        if (currSon != 0) {
            word_t newFrame = TravelTree(maxFrame, originFatherAddress, currDepth + 1, currSon,
                                         currNode * PAGE_SIZE + i, mostDistNode, mostDistNodeFather,
                                         virtualMemoryAddress, (currPageAddress << OFFSET_WIDTH) + i,
                                         mostDistNodeVirtualAddress, mostDist);
            allZero = false; //One of the children is not empty
            if (newFrame != 0) { //found frame that is not 0
                return newFrame;
            }
        }
    }

    if (allZero && currNode != originFatherAddress) {
        PMwrite(myFather, 0); //change the pointer of the father of the new node we will use to zero
        return currNode;
    }

    return 0;
}

/**
 * Finding the exact frame to put our page in and evicting another page if necessary.
 * We choose the frame by traversing the entire tree of tables in the physical memory.
 */
word_t FindFrame(word_t fatherAddress, uint64_t virtualMemoryAddress) {
    //1. Looking for a frame containing an empty table:
    //where all rows are 0. We don’t have to evict it,
    //but we do have to remove the reference to this table from its parent.
    word_t maxFrame = 0;
    word_t mostDistNode = 0;
    uint64_t mostDistNodeFather = 0;
    uint64_t mostDistNodeVirtualAddress = 0;
    uint64_t mostDist = 0;
    word_t newFrame = TravelTree(&maxFrame, fatherAddress, 0, 0, 0, &mostDistNode, &mostDistNodeFather,
                                 virtualMemoryAddress, 0, &mostDistNodeVirtualAddress, &mostDist);
    if (newFrame != 0) return newFrame;
    //2 - An unused frame
    //When traversing the tree, we are keeping a variable with the maximal frame
    //Index referenced from any table we visit. Since we fill frames in order, all used
    //frames are contiguous in the memory, and if max_frame_index+1 < NUM_FRAMES
    //then we know that the frame in the index (max_frame_index + 1) is unused.
    if (maxFrame + 1 < NUM_FRAMES) return (maxFrame + 1);
    //3. If all frames are already used, then a page must be swapped out from some frame in
    //order to replace it with the relevant page (a table or actual page). The frame that will
    //be chosen is the frame containing a page p such that the cyclical distance:
    // min{NUM_PAGES - |page_swapped_in - p|, |page_swapped_in - p|}
    //is maximal (don’t worry about tie-breaking). This page must be swapped out before
    //we use the frame, and we also have to remove the reference to this page from its
    //parent table.
    //According to this design, VMInitialize() only has to clear frame 0.
    PMevict(mostDistNode, mostDistNodeVirtualAddress); //Evicting the chosen frame to the disk.
    PMwrite(mostDistNodeFather, 0); //Writing to the father of the chosen frame 0.
    return mostDistNode;
}

/**
 * This function gets a frame and sets to 0 all of his values.
 */
void ClearFrame(uint64_t frameToClear) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameToClear * PAGE_SIZE + i, 0);
    }
}

/**
 * The main exercise function.
 * First we are saving the virtual address to all of the inside table address in an address array.
 * Second we are traveling the tree and if needed finding a new frame.
 * @return The physic address.
 */
uint64_t MainAddressCalculator(uint64_t originAddress) {
    //Splitting the address into partial address, and saving them in array.
    uint64_t offSet = originAddress & uint64_t((1 << OFFSET_WIDTH) - 1); //Getting the offset.
    uint64_t allAddress[TABLES_DEPTH]; // An array containing the address for each depth of the tables.
    uint64_t addressSize = PAGE_SIZE - 1; // Light all bits until the bit of page size
    uint64_t remainingAddress = originAddress >> OFFSET_WIDTH; //Getting the address without the offset.
    uint64_t virtualMemoryAddress = remainingAddress;

    for (int i = 0; i < TABLES_DEPTH; ++i) {
        allAddress[TABLES_DEPTH - (i + 1)] = remainingAddress & addressSize; //Setting the array from right to left.
        remainingAddress = remainingAddress >> OFFSET_WIDTH; // offset_width = log(page_size)
    }
    //Find the physic address.
    word_t currBaseAddress = 0;
    for (int i = 0; i < TABLES_DEPTH; ++i) {
        word_t fatherAddress = currBaseAddress;
        PMread(currBaseAddress * PAGE_SIZE + allAddress[i], &currBaseAddress);
        if (currBaseAddress == 0) {
            // Find an unused frame or evict a page from some frame.
            word_t newFrame = FindFrame(fatherAddress, virtualMemoryAddress);
            //Set the new frame.
            if (i != TABLES_DEPTH - 1) {
                ClearFrame(newFrame); //If we are not at the leaf's we clear the table
            } else PMrestore(newFrame, virtualMemoryAddress); //Next layer in Ram.
            PMwrite(fatherAddress * PAGE_SIZE + allAddress[i],
                    newFrame); //Write the new frame to point to the fatherAddress one.
            currBaseAddress = newFrame;
        }
    }
    return currBaseAddress * PAGE_SIZE + offSet;
}


// ----- Library Functions --------

/*
 * Initialize the virtual memory.
 * Sets the value off all the rows of the root frame to zero.
 */
void VMinitialize() {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(i, 0);
    }
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t *value) {
    if (!AddressChecker(virtualAddress)) return 0;
    PMread(MainAddressCalculator(virtualAddress), value);
    return 1;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value) {
    if (!AddressChecker(virtualAddress)) return 0;
    PMwrite(MainAddressCalculator(virtualAddress), value);
    return 1;
}
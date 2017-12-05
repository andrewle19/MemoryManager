// Author: Andrew Le
// email: andrewle19@csu.fullerton.edu
// 12/5/17
//

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>


using namespace std;



const int FRAMES = 256; // total number of the frames
const int FRAMESIZE = 256; // size of the frame
const int PAGESIZE = 256; // size of page table
const int TLBSIZE = 16; // TLB size
const int READBYTESIZE = 256;//  number of bytes to read in

const int ADDRESS_MASK =  0xFFFF;
const int OFFSET_MASK = 0xFF;


int pageTableNumbers[PAGESIZE]; // array to hold page numbers
int pageTableFrames[PAGESIZE]; // array to hold corresponding frames

int TLBPageNumbers[TLBSIZE]; // array to represent the TLB keys or page numbers
int TLBFrameNumbers[TLBSIZE]; // array to hold the corresponding TLB page number

int physicalMemory[FRAMES][FRAMESIZE]; // hold the value of the physical Addresses

int pageFaults = 0; // counter to track page faults
int TLBHits = 0; // counter for TLB Hits
int firstFrameAvaliable = 0; // counter for first frame avaliable
int firstPageTableNumberAvaliable = 0; // counter to track the first avalible page table entry
int TLBcurrentsize = 0; // counter to keep track of the number of current entries in TLB

signed char buffer[READBYTESIZE]; // buffer containing reads from backing store
signed char value; //the value of the byte(signed char in memory)


FILE *backingStore;


// function prototypes
void getPage(int logicalAddress);
void insertTLB(int pageNumber, int frameNumber);
void getFromBackingStore(int logicalAddress);



// function takes in logical address and obtain the physical value and value
// stored at the address
void getPage(int logicalAddress){
    
    bool TLBfound = false;
    bool pageTablefound = false;
    
    
    int pageNumber = ((logicalAddress & ADDRESS_MASK)>>8); // calculate page number
    int offset = (logicalAddress & OFFSET_MASK); // calculate the offset
    
    int frameNumber = -1; // frame number
    
    // look through TLB to check if pagenumber and key match
    for(int i = 0; i < TLBSIZE; i++){
        // TLB index is equal to the page number
        if(TLBPageNumbers[i] == pageNumber){
            frameNumber = TLBFrameNumbers[i]; // then frame number is extracted
            TLBHits++; // increment the TLB hits
            TLBfound = true; // make the tlb found true
            pageTablefound = true; // then dont check the page table
        }
    }
    // if the tlb found is not true
    if(!TLBfound){
        // Search through the page Table
        for(int i = 0; i < firstPageTableNumberAvaliable; i++){
            // if the page is found go to the corresponding array
            if(pageTableNumbers[i] == pageNumber){
                frameNumber = pageTableFrames[i];
                pageTablefound = true; // set page table found
            }
        }
    }
    // if pagenumber is not found in page table
    if(!pageTablefound){
        // read from store and get the frame into physical memory and the page memory
        getFromBackingStore(pageNumber);
        pageFaults++; // increment page fault
        frameNumber = firstFrameAvaliable - 1; // set frame number to current first frame index
    }
    
    // if the TLB was not hit insert into TLB
    if(!TLBfound){
        insertTLB(pageNumber,frameNumber); // insert the page number and frame number into TLB
    }
    value = physicalMemory[frameNumber][offset]; // frame number and offset used to get value in address
    
//    cout << "Frame Number: " << frameNumber << endl;
//    cout << "Page Number: " << pageNumber << endl;
//    cout << "Offset: " << offset << endl;
//
    // output when it was found in TLB
    if(TLBfound) {
        cout << hex;
        cout << "virt: " << logicalAddress << "(pg:0x" << pageNumber << "," << "off:0x" << offset << ")";
        cout << "--->*IN_TLB*   ,----------,-----------,  phys:0x" << ((frameNumber <<8) | offset) << ", val: " << dec << int(value) << "    tlb_Hits: " << TLBHits << endl;
    } else if (pageTablefound && !TLBfound){
        cout << hex;
        cout << "virt: " << logicalAddress << "(pg:0x" << pageNumber << "," << "off:0x" << offset << ")";
        cout << "--->----------,*IN_P_TABLE,-----------,  phys:0x" << ((frameNumber <<8) | offset) << ", val: " << dec <<int(value) << "    tlb_Hits: " << TLBHits << endl;
    } else {
        cout << hex;
        cout << "virt: " << logicalAddress << "(pg:0x" << pageNumber << "," << "off:0x" << offset << ")";
        cout << "--->----------,----------,*IN_BACK_S*,  phys:0x" << ((frameNumber <<8) | offset) << ", val: " << dec << int(value) << "    tlb_Hits: " << TLBHits << endl;
    }
    //cout << "virt: " << logicalAddress << " Physical Address " << ((frameNumber << 8) | offset ) << " Value: " << (int)value << " pageFaults: " << pageFaults << endl;
 
}
// function to insert frame and page number into the TLB with FIFO
void insertTLB(int pageNumber, int frameNumber){
    
    // if the current size is less then max size insert
    if(TLBcurrentsize < TLBSIZE){
        TLBPageNumbers[TLBcurrentsize] = pageNumber;
        TLBFrameNumbers[TLBcurrentsize] = pageNumber;
        TLBcurrentsize++;
    }
    // else we have to replace/shift
    else {
        // shift everything left by one to do FIFO
        for(int i = 0; i < TLBSIZE - 1; i++){
            TLBPageNumbers[i] = TLBPageNumbers[i+1];
            TLBFrameNumbers[i] = TLBFrameNumbers[i+1];
        }
        
        // insert at the back
        TLBPageNumbers[TLBcurrentsize-1] = pageNumber;
        TLBFrameNumbers[TLBcurrentsize-1] = frameNumber;
    }
}

// function to read from backing store and bring the frame into physical memory and page table
void getFromBackingStore(int pageNumber){
    //seek to byte READBYTESIZE in backing store
    // seek set starts from the begingng of the file
    if(fseek(backingStore, pageNumber * READBYTESIZE, SEEK_SET) != 0){
        cout << "Error seeking in backing store " << endl;
    }
    
    // now read READBYTESIZE bytes from the backing store to the buffer
    if(fread(buffer, sizeof(signed char), READBYTESIZE, backingStore) == 0){
        cout << "Error reading from backing store" << endl;
    }
    
    // load the bits into the first avaliable frame in memory
    for(int i = 0; i < READBYTESIZE; i++){
        physicalMemory[firstFrameAvaliable][i] = buffer[i];
    }
    
    // load the frame number into page table in first avalible frame
    pageTableNumbers[firstPageTableNumberAvaliable] = pageNumber;
    pageTableFrames[firstFrameAvaliable] = firstFrameAvaliable;
    
    // increment counters
    firstFrameAvaliable++;
    firstPageTableNumberAvaliable++;
}








int main(int argc, char *argv[]) {
    
    ifstream input;
    int logicalAddress; // from the address file
    
    
    if(argc != 2) {
        cout << "USAAGE: ./a.out <inputfile?\n";
        return -1;
    }
    
    // open the argument file addresses.txt
    input.open(argv[1]);
    
    // check if the file does not open
    if(!input) {
        cout << "Unable to open file addresses.txt" << endl;
        return 1;
    }
    // open the backing store
    backingStore = fopen("BACKING_STORE.bin", "rb");
    
    int translatedAddressTotal = 0; // number of address translated
   
    cout << "Beginning translation of logical addresses to physical addresses..." << endl;

    // translate until end of file
    while(input >> logicalAddress){
       
        getPage(logicalAddress);
        translatedAddressTotal++;
    }
    double pageFaultRate = pageFaults / double(translatedAddressTotal);
    double TLBHitRate = TLBHits / double(translatedAddressTotal);

    // calculate the page hits
    cout << "Total addresses translated: " << translatedAddressTotal << endl;
    cout << "Page faults: " << pageFaults << endl;
    cout << "Page fault rate: " << pageFaultRate << endl;
    cout << "TLB hits: " << TLBHits << endl;
    cout << "TLB hit rate: " << TLBHitRate << endl;
    
    
    cout << "..... done\n";
    
    
    input.close();
    fclose(backingStore);
    return 0;
}

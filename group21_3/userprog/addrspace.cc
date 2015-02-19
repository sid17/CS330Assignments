// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable,char * filename)
{
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;
    int length=0;
    while (*(filename+length)!='\0')
    {
        length++;
    }
    execFile=new char[length+1];

    for (int i=0;i<=length;i++)
    {
        execFile[i]=filename[i];
    }

    executableCode=fileSystem->Open(execFile);
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    		

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) 
    {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = -1;
	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
    pageTable[i].shared=FALSE; 
    pageTable[i].readFromDisk=FALSE;
					// a separate page, we could set its 
					// pages to be read-only
    }

    hardDiskBuffer=new char[size];
    bzero(hardDiskBuffer,size);
    // for (i=0;i<size;i++)
    // {
    //    hardDiskBuffer[i]='\0'; 
    // }
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace (AddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

void AddrSpace::AllocateSpace(AddrSpace *parentSpace,int pid_val)
{
    numPages = parentSpace->GetNumPages();
    unsigned i, size = numPages * PageSize;
    //printf("Fork Started");
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) 
    {
        if (parentPageTable[i].shared==FALSE && parentPageTable[i].valid==TRUE)
        {
        pageTable[i].virtualPage = i;
       // printf("Parent Page table %d %d %d\n",parentPageTable[i].virtualPage,pid_val,parentPageTable[i].physicalPage);
        // fprintf(stderr, "Num pjrwhfuirfureiages %d",numPages);
        pageTable[i].physicalPage = machine->getMainMemoryPage(pid_val,parentPageTable[i].physicalPage);
        pageTable[i].valid = TRUE;
        pageTable[i].use = parentPageTable[i].use;
        pageTable[i].dirty = parentPageTable[i].dirty;
        pageTable[i].readOnly = parentPageTable[i].readOnly;    // if the code segment was entirely on
        pageTable[i].shared=FALSE;  
        pageTable[i].readFromDisk=parentPageTable[i].readFromDisk;                                     // a separate page, we could set its
        




        unsigned AddrParent=parentPageTable[i].physicalPage*PageSize;
        unsigned AddrChild=pageTable[i].physicalPage*PageSize;

        for (int j=0;j<PageSize;j++)
        {
           // printf("In fork writing %d %d \n",j,i);
            machine->mainMemory[AddrChild+j] = machine->mainMemory[AddrParent+j];

        }



       currentThread->SortedInsertInWaitQueue(1000+stats->totalTicks);

        // copy the page from memory

                                                    // pages to be read-only 
        }
        else if (parentPageTable[i].shared==FALSE && parentPageTable[i].valid==FALSE)
        {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = FALSE;
        pageTable[i].use = parentPageTable[i].use;
        pageTable[i].dirty = parentPageTable[i].dirty;
        pageTable[i].readOnly = parentPageTable[i].readOnly;    // if the code segment was entirely on
        pageTable[i].shared=FALSE;  
        pageTable[i].readFromDisk=parentPageTable[i].readFromDisk; 
        }
        else if (parentPageTable[i].shared==TRUE && parentPageTable[i].valid==TRUE)
        {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = parentPageTable[i].physicalPage;
        pageTable[i].valid = parentPageTable[i].valid;
        pageTable[i].use = parentPageTable[i].use;
        pageTable[i].dirty = parentPageTable[i].dirty;
        pageTable[i].readOnly = parentPageTable[i].readOnly;              // if the code segment was entirely on
        pageTable[i].shared=TRUE; 
        pageTable[i].readFromDisk=parentPageTable[i].readFromDisk;                                         // a separate page, we could set its                                     // pages to be read-only 
        }
        
    }
//printf("Hello , how are u !\n");



    
    for (i=0;i<size;i++)
    {
        //printf("Copying\n");
        if(hardDiskBuffer[i]=='\0')
        {
            
            hardDiskBuffer[i]=parentSpace->hardDiskBuffer[i]; 
        }
            
    }
     //printf("Fork Ended");

}



AddrSpace::AddrSpace(AddrSpace *parentSpace,int pid_val)
{
    numPages = parentSpace->GetNumPages();
    unsigned i, size = numPages * PageSize;
    execFile=parentSpace->execFile;
    executableCode=fileSystem->Open(execFile);  
    
    hardDiskBuffer=new char[size];
    bzero(hardDiskBuffer,size);
    
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) 
    {
        
        pageTable[i].virtualPage = i;
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;    // if the code segment was entirely on
        pageTable[i].shared=FALSE;  
        pageTable[i].readFromDisk=FALSE;                                     // a separate page, we could set its
                                                    // pages to be read-only 
    }

    numPages=-1;

}




//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}


void 
AddrSpace::unallocatePhysicalMemory()
{
    int array_phy[NumPhysPages];
    for(int i=0;i<NumPhysPages;i++)
        array_phy[i]=0;
    for (int i = 0; i < numPages; i++) 
    {
    
    if (pageTable[i].valid==TRUE && pageTable[i].shared==FALSE)
    {
        if (pageReplacementAlgo==2)
        {
            //array_phy[pageTable[i].physicalPage]=1;
            deleted(fifoQueue,fifo_arr[pageTable[i].physicalPage]); 
            
        }
        
        machine->freeMainMemory(pageTable[i].physicalPage);
        if (pageReplacementAlgo==3)
        {
          deleted(lru_algo,lru_arr[pageTable[i].physicalPage]);  
        }
        

    }

   }
   
   // if (pageReplacementAlgo==2)
   // {
   //  List *tempQueue = new List;
   //  while (!(fifoQueue->IsEmpty()))
   //  {
   //       int *add1 = (int *)fifoQueue->Remove();
   //       if(array_phy[*add1]!=1)
   //          tempQueue->Append((void *)add1);
   //  }
   //  while (!(tempQueue->IsEmpty()))
   //  {
   //       int *add1 = (int *)tempQueue->Remove();
   //          fifoQueue->Append((void *)add1);
   //  }
   // }
    
}


unsigned
AddrSpace::Update_Shared_Space(int size_pass)
{
    int numPages_shared = divRoundUp(size_pass, PageSize);
    size_pass = numPages_shared * PageSize;
    
    TranslationEntry * updatedPageTable = new TranslationEntry[numPages+numPages_shared];
    for (int i = 0; i < numPages; i++) 
    {
        
        updatedPageTable[i].virtualPage = pageTable[i].virtualPage;
        updatedPageTable[i].physicalPage = pageTable[i].physicalPage;
        updatedPageTable[i].valid = pageTable[i].valid;
        updatedPageTable[i].use = pageTable[i].use;
        updatedPageTable[i].dirty =  pageTable[i].dirty;
        updatedPageTable[i].readOnly = pageTable[i].readOnly;    // if the code segment was entirely on
        updatedPageTable[i].shared=pageTable[i].shared;
        updatedPageTable[i].readFromDisk=pageTable[i].readFromDisk;                                       // a separate page, we could set its
                                                    // pages to be read-only 
    }
     
    for (int i = numPages; i < numPages+numPages_shared; i++) 
    {
        
    updatedPageTable[i].virtualPage = i;
    updatedPageTable[i].physicalPage = machine->getMainMemoryPage(-1,-1);
    updatedPageTable[i].valid = TRUE;
    updatedPageTable[i].use = FALSE;
    updatedPageTable[i].dirty = FALSE;
    updatedPageTable[i].readOnly = FALSE;  // if the code segment was entirely on
    updatedPageTable[i].shared=TRUE; 
    updatedPageTable[i].readFromDisk=pageTable[i].readFromDisk;
    }

    delete pageTable;
    pageTable=updatedPageTable;
    
    unsigned return_val=pageTable[numPages].virtualPage*PageSize;
    //printf("Return value%d\n",return_val);
    numPages=numPages+numPages_shared;
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    return return_val;
}

void AddrSpace::unsetPageTableEntry(int pg,int pid_val)
{
    
    int outer;
    outer=numPages;

     for (int i = 0; i < outer; i++) 
    {
        if (pageTable[i].physicalPage==pg)
        {
           // printf("Replaced virtual page %d of process %d\n",i,pid_val);
            pageTable[i].virtualPage=i;
            pageTable[i].physicalPage=-1;
            pageTable[i].valid = FALSE;
            if (pageTable[i].dirty==TRUE) 
            {
                int j;
                for (j=0;j<PageSize;j++)
                {
                    hardDiskBuffer[i*PageSize+j]=machine->mainMemory[pg*PageSize+j];
                }
                pageTable[i].readFromDisk=TRUE;
                pageTable[i].dirty=FALSE;
            }
                

            
                    
        }
    }
}
//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

unsigned
AddrSpace::GetNumPages()
{
   return numPages;
}

TranslationEntry*
AddrSpace::GetPageTable()
{
   return pageTable;
}





// AddrSpace::AddrSpace(AddrSpace *parentSpace)
// {
//     numPages = parentSpace->GetNumPages();
//     unsigned i, size = numPages * PageSize;

//     ASSERT(numPages+numPagesAllocated <= NumPhysPages);                // check we're not trying
//                                                                                 // to run anything too big --
//                                                                                 // at least until we have
//                                                                                 // virtual memory

//     DEBUG('a', "Initializing address space, num pages %d, size %d\n",
//                                         numPages, size);
//     // first, set up the translation
//     TranslationEntry* parentPageTable = parentSpace->GetPageTable();
//     pageTable = new TranslationEntry[numPages];
//     int shared_offset=0;
//     for (i = 0; i < numPages; i++) {
//         if (parentPageTable[i].shared==FALSE)
//         {
//         pageTable[i].virtualPage = i;
//         pageTable[i].physicalPage = i+numPagesAllocated-shared_offset;
//         pageTable[i].valid = parentPageTable[i].valid;
//         pageTable[i].use = parentPageTable[i].use;
//         pageTable[i].dirty = parentPageTable[i].dirty;
//         pageTable[i].readOnly = parentPageTable[i].readOnly;    // if the code segment was entirely on
//         pageTable[i].shared=FALSE;                                       // a separate page, we could set its
//                                                     // pages to be read-only 
//         }
//         else
//         {
//         pageTable[i].virtualPage = i;
//         pageTable[i].physicalPage = parentPageTable[i].physicalPage;
//         pageTable[i].valid = parentPageTable[i].valid;
//         pageTable[i].use = parentPageTable[i].use;
//         pageTable[i].dirty = parentPageTable[i].dirty;
//         pageTable[i].readOnly = parentPageTable[i].readOnly;    // if the code segment was entirely on
//         pageTable[i].shared=TRUE;                                         // a separate page, we could set its
//         shared_offset=shared_offset+1;                                      // pages to be read-only 
//         }
        
//     }

//     // Copy the contents
//     //unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
//     //unsigned startAddrChild = numPagesAllocated*PageSize;

//     for (i=0;i<numPages;i++)
//     {

//         if (parentPageTable[i].shared==FALSE)
//         {

//             unsigned AddrParent=parentPageTable[i].physicalPage*PageSize;
//             unsigned AddrChild=pageTable[i].physicalPage*PageSize;

//             for (int j=0;j<PageSize;j++)
//             {
//                 machine->mainMemory[AddrChild+j] = machine->mainMemory[AddrParent+j];
//             }
//         }
        
//     }
//     // for (i=0; i<size; i++) 
//     // {
//     //    machine->mainMemory[startAddrChild+i] = machine->mainMemory[startAddrParent+i];
//     // }
//     numPagesAllocated += (numPages-shared_offset);
//     //printf("Num Pages%d\n",numPages);
//     //printf("Num Pages Allocated %d\n",numPagesAllocated);
    
// }



// unsigned
// AddrSpace::Update_Shared_Space(int size_pass)
// {
    
    
//     int numPages_shared = divRoundUp(size_pass, PageSize);
//     ASSERT(numPages_shared+numPagesAllocated <= NumPhysPages); 
//     size_pass = numPages_shared * PageSize;
//     int original_start_point=pageTable[0].physicalPage;
//     //printf("Size passed shared  %d\n",size_pass);
//     //printf("Shared page size is %d\n",numPages_shared);
//     //printf("Current Page table size is %d\n",machine->pageTableSize);
//     //printf("Page offset is %d\n",numPagesAllocated);
//     //printf("Start Point is %d\n",original_start_point);
//     TranslationEntry * updatedPageTable = new TranslationEntry[numPages+numPages_shared];
//     for (int i = 0; i < numPages; i++) 
//     {
        
//         updatedPageTable[i].virtualPage = pageTable[i].virtualPage;
//         updatedPageTable[i].physicalPage = pageTable[i].physicalPage;
//         updatedPageTable[i].valid = pageTable[i].valid;
//         updatedPageTable[i].use = pageTable[i].use;
//         updatedPageTable[i].dirty =  pageTable[i].dirty;
//         updatedPageTable[i].readOnly = pageTable[i].readOnly;    // if the code segment was entirely on
//         updatedPageTable[i].shared=pageTable[i].shared;                                       // a separate page, we could set its
//                                                     // pages to be read-only 
//     }
     
//     for (int i = numPages; i < numPages+numPages_shared; i++) 
//     {
        
//     updatedPageTable[i].virtualPage = i;
//     updatedPageTable[i].physicalPage = i-numPages+numPagesAllocated;
//     updatedPageTable[i].valid = TRUE;
//     updatedPageTable[i].use = FALSE;
//     updatedPageTable[i].dirty = FALSE;
//     updatedPageTable[i].readOnly = FALSE;  // if the code segment was entirely on
//     updatedPageTable[i].shared=TRUE; 
//     }

//     delete pageTable;
//     pageTable=updatedPageTable;

//     //printf("Num Pages%d\n",numPages);
//     //printf("Num Pages Allocated %d\n",numPagesAllocated);

//     unsigned return_val=pageTable[numPages].virtualPage*PageSize;
//     //printf("Return value%d\n",return_val);
//     numPagesAllocated=numPagesAllocated+numPages_shared;
//     numPages=numPages+numPages_shared;
    
//     machine->pageTable = pageTable;
//     machine->pageTableSize = numPages;
//     //printf("Num Pages%d\n",numPages);
//     //printf("Num Pages Allocated %d\n",numPagesAllocated);
//     return return_val;



// }



// AddrSpace::AddrSpace(OpenFile *executable)
// {
//     NoffHeader noffH;
//     unsigned int i, size;
//     unsigned vpn, offset;
//     TranslationEntry *entry;
//     unsigned int pageFrame;


//     //executableCode=executable;


//     executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
//     if ((noffH.noffMagic != NOFFMAGIC) && 
//         (WordToHost(noffH.noffMagic) == NOFFMAGIC))
//         SwapHeader(&noffH);
//     ASSERT(noffH.noffMagic == NOFFMAGIC);

// // how big is address space?
//     size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
//             + UserStackSize;    // we need to increase the size
//                         // to leave room for the stack
//     numPages = divRoundUp(size, PageSize);
//     size = numPages * PageSize;

//     ASSERT(numPages+numPagesAllocated <= NumPhysPages);     // check we're not trying
//                                         // to run anything too big --
//                                         // at least until we have
//                                         // virtual memory

//     DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
//                     numPages, size);
// // first, set up the translation 
//     pageTable = new TranslationEntry[numPages];
//     for (i = 0; i < numPages; i++) {
//     pageTable[i].virtualPage = i;
//     pageTable[i].physicalPage = i+numPagesAllocated;
//     pageTable[i].valid = TRUE;
//     pageTable[i].use = FALSE;
//     pageTable[i].dirty = FALSE;
//     pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
//     pageTable[i].shared=FALSE; 
//                     // a separate page, we could set its 
//                     // pages to be read-only
//     }
// // zero out the entire address space, to zero the unitialized data segment 
// // and the stack segment
//     bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);
 
//     numPagesAllocated += numPages;

// // then, copy in the code and data segments into memory
//     if (noffH.code.size > 0) {
//         DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
//             noffH.code.virtualAddr, noffH.code.size);
//         vpn = noffH.code.virtualAddr/PageSize;
//         offset = noffH.code.virtualAddr%PageSize;
//         entry = &pageTable[vpn];
//         pageFrame = entry->physicalPage;
//         executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
//             noffH.code.size, noffH.code.inFileAddr);
//     }
//     if (noffH.initData.size > 0) {
//         DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
//             noffH.initData.virtualAddr, noffH.initData.size);
//         vpn = noffH.initData.virtualAddr/PageSize;
//         offset = noffH.initData.virtualAddr%PageSize;
//         entry = &pageTable[vpn];
//         pageFrame = entry->physicalPage;
//         executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
//             noffH.initData.size, noffH.initData.inFileAddr);
//     }

// }

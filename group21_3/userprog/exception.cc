// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"
#include "synchop.h"
#include "noff.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
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







static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

extern void StartProcess (char*);

void
ForkStartFunction (int dummy)
{
   currentThread->Startup();
   machine->Run();
}

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;	// Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);
    int exitcode;		// Used in syscall_Exit
    unsigned i;
    char buffer[1024];		// Used in syscall_Exec
    int waitpid;		// Used in syscall_Join
    int whichChild;		// Used in syscall_Join
    Thread *child;		// Used by syscall_Fork
    unsigned sleeptime;		// Used by syscall_Sleep

    if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == syscall_Exit)) {
       exitcode = machine->ReadRegister(4);
     //  printf("[pid %d]: Exit called. Code: %d\n", currentThread->GetPID(), exitcode);

       (currentThread->space)->unallocatePhysicalMemory();
       
        


       // We do not wait for the children to finish.
       // The children will continue to run.
       // We will worry about this when and if we implement signals.
       exitThreadArray[currentThread->GetPID()] = true;

       // Find out if all threads have called exit
       for (i=0; i<thread_index; i++) {
          if (!exitThreadArray[i]) break;
       }
       currentThread->Exit(i==thread_index, exitcode);
    }
    else if ((which == SyscallException) && (type == syscall_Exec)) {
       // Copy the executable name into kernel space
       vaddr = machine->ReadRegister(4);

        while (machine->ReadMem(vaddr, 1, &memval)==FALSE);
       i = 0;
       while ((*(char*)&memval) != '\0') {
          buffer[i] = (*(char*)&memval);
          i++;
          vaddr++;
           while (machine->ReadMem(vaddr, 1, &memval)==FALSE);
       }
       buffer[i] = (*(char*)&memval);
       (currentThread->space)->unallocatePhysicalMemory();
       delete currentThread->space;

       //printf("Unallocated free space in exec call\n");

       StartProcess(buffer);
    }
    else if ((which == SyscallException) && (type == syscall_Join)) {
       waitpid = machine->ReadRegister(4);
       // Check if this is my child. If not, return -1.
       whichChild = currentThread->CheckIfChild (waitpid);
       if (whichChild == -1) {
        //  printf("[pid %d] Cannot join with non-existent child [pid %d].\n", currentThread->GetPID(), waitpid);
          machine->WriteRegister(2, -1);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
       else {
          exitcode = currentThread->JoinWithChild (whichChild);
          machine->WriteRegister(2, exitcode);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
    }
    else if ((which == SyscallException) && (type == syscall_Fork)) {
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       
       // IntStatus oldLevel = interrupt->SetLevel(IntOff);
   
   


       child = new Thread("Forked thread", GET_NICE_FROM_PARENT);
       child->space = new AddrSpace (currentThread->space,child->GetPID());  // Duplicates the address space
       



      
       child->space->AllocateSpace(currentThread->space,child->GetPID());
 // (void) interrupt->SetLevel(oldLevel);

       child->SaveUserState ();		     		      // Duplicate the register set
       child->ResetReturnValue ();			     // Sets the return register to zero
       child->StackAllocate (ForkStartFunction, 0);	// Make it ready for a later context switch
       child->Schedule ();
       machine->WriteRegister(2, child->GetPID());		// Return value for parent
    }
    else if ((which == SyscallException) && (type == syscall_Yield)) {
       currentThread->Yield();
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
             writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
             writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintChar)) {
        writeDone->P() ;        // wait for previous write to finish
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);

       while (machine->ReadMem(vaddr, 1, &memval)==FALSE);
       
       //int returnval=machine->ReadRegister(4);
       while ((*(char*)&memval) != '\0') {
          writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
           while (machine->ReadMem(vaddr, 1, &memval)==FALSE);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetReg)) {
       machine->WriteRegister(2, machine->ReadRegister(machine->ReadRegister(4))); // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPA)) {
       vaddr = machine->ReadRegister(4);
       machine->WriteRegister(2, machine->GetPA(vaddr));  // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPID)) {
       machine->WriteRegister(2, currentThread->GetPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPPID)) {
       machine->WriteRegister(2, currentThread->GetPPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Sleep)) {
       sleeptime = machine->ReadRegister(4);
       if (sleeptime == 0) {
          // emulate a yield
          currentThread->Yield();
       }
       else {
          currentThread->SortedInsertInWaitQueue (sleeptime+stats->totalTicks);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Time)) {
       machine->WriteRegister(2, stats->totalTicks);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_NumInstr)) {
       machine->WriteRegister(2, currentThread->GetInstructionCount());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type ==syscall_ShmAllocate))
    {
      int size = (unsigned)machine->ReadRegister(4);
      //printf("the value of size is %d\n",size);
      unsigned returnval=currentThread->space->Update_Shared_Space(size);

      //printf("the return value is %d",returnval);
       machine->WriteRegister(2, returnval);
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);


    }
    else if ((which == SyscallException) && (type ==syscall_SemGet))
    {
       int key =machine->ReadRegister(4);
       //printf("key is %d",key);
       
       int result=-1;
       for (int i=0;i<MAX_SEMAPHORE_COUNT;i++)
       {
        if (semValid[i] && semKey[i]==key)
        {
          result=i;
          break;
        }
       }
       if (result==-1)
       {
        int i;
        for (i=0;i<MAX_SEMAPHORE_COUNT;i++)
        {
          if (semValid[i]==0)
          {
            result=i;
            semKey[i]=key;
            semValid[i]=1;
            semArray[i] = new Semaphore("semaphore1", 0);
            //make a new semaphore
            break;
          }

       }
       }
       machine->WriteRegister(2, result);

       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }
    else if ((which == SyscallException) && (type ==syscall_SemOp))
    {
       int id= machine->ReadRegister(4);
       int adjust = machine->ReadRegister(5);
       //printf("id is %d, adjustment is %d",id,adjust);
       if (semValid[id])
       {
        if (adjust==1)
       {
        semArray[id]->V();
       }
       else if (adjust==-1)
       {
        semArray[id]->P();
       }
       }
       else
       {
        printf("id is not set");
        ASSERT(FALSE);
       }
      
         //machine->WriteRegister(2, returnval);

       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);


    }
    else if ((which == SyscallException) && (type ==syscall_SemCtl))
    {
      int id = (unsigned)machine->ReadRegister(4);
      int type=machine->ReadRegister(5);
      vaddr = machine->ReadRegister(6);
  //printf("vitual Address %d\n",vaddr);
      //printf("Value is %d\n",value);
      //printf("%d,%d\n",id,type);
      int returnval=-1;
      int success=1;
      if (semValid[id])
       {
              if (type==SYNCH_REMOVE)
              {
                //printf("SYNC_REMOVE");
                semValid[id]=0;
                delete semArray[id];
                semArray[id]=NULL;
                semKey[id]=-1;

              }
              else if (type==SYNCH_GET)
              {
                //printf("SYNC_GET");
                // printf("vitual Address2 %d %d\n",vaddr,currentThread->GetPID());
                ASSERT(vaddr < 10000);    
                while(machine->WriteMem(vaddr,4,semArray[id]->GetSemVal())==FALSE);

              }
              else if (type==SYNCH_SET)
              {
                //printf("SYNC_SET");
                // printf("vitual Address1 %d , %d , %d %d\n",vaddr,currentThread->GetPID(),id,type);
                ASSERT(vaddr < 10000);
                 while (machine->ReadMem(vaddr, 4, &memval)==FALSE);
                int value=memval;
                semArray[id]->SetSemVal(value);
                
              }
        }
        else
        {
          success=0;
        }
      if(success==1)
      {
        returnval=0;
      }
      machine->WriteRegister(2, returnval);

       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

      //printf("the return value is %d\n",returnval);


    }

 else if ((which == SyscallException) && (type ==syscall_CondGet))
    {
       int key =machine->ReadRegister(4);
       //printf("key is %d",key);
       
       int result=-1;
       for (int i=0;i<MAX_COND_COUNT;i++)
       {
        if (condValid[i] && condKey[i]==key)
        {
          result=i;
          break;
        }
       }
       if (result==-1)
       {
        int i;
        for (i=0;i<MAX_COND_COUNT;i++)
        {
          if (condValid[i]==0)
          {
            result=i;
            condKey[i]=key;
            condValid[i]=1;
            condArray[i] = new Condition("Condition");
         
            break;
          }

       }
       }
       machine->WriteRegister(2, result);

       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }
    else if ((which == SyscallException) && (type ==syscall_CondOp))
    {
      int id_cond = (unsigned)machine->ReadRegister(4);
      int type=machine->ReadRegister(5);
      int id_sem= machine->ReadRegister(6);

      //printf("Value is %d\n",value);
      //printf("%d,%d,%d\n",id_cond,type,id_sem);
      if (condValid[id_cond] && semValid[id_sem])
       {
              if (type==COND_OP_WAIT)
              {
                //printf("COND_OP_WAIT ");
                condArray[id_cond]->Wait(semArray[id_sem]);
                // semValid[id]=0;
                // delete semArray[id];
                // semArray[id]=NULL;
                // semKey[id]=-1;

              }
              else if (type==COND_OP_SIGNAL)
              {
                //printf("COND_OP_SIGNAL");
                condArray[id_cond]->Signal();
                //machine->WriteMem(vaddr,4,semArray[id]->GetSemVal());

              }
              else if (type==COND_OP_BROADCAST)
              {
              //printf("COND_OP_BROADCAST");
                condArray[id_cond]->Broadcast();

                // machine->ReadMem(vaddr,4,&memval);
                // int value=memval;
                // semArray[id]->SetSemVal(value);
                
              }
        }
        else
        {
          printf("invalid condition variable or semaphore");
        }

       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

      //printf("the return value is %d\n",returnval);



    }
    else if ((which == SyscallException) && (type ==syscall_CondRemove))
    {
      int id = (unsigned)machine->ReadRegister(4);
      int returnval=-1;
      int success=1;
      if (condValid[id])
       {
              
             //   printf("COND_REMOVE");
                condValid[id]=0;
                delete condArray[id];
                condArray[id]=NULL;
                condKey[id]=-1;

        }
        else
        {
          success=0;
        }
      if(success==1)
      {
        returnval=0;
      }
     machine->WriteRegister(2, returnval);
     machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
     machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
     machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

      //printf("the return value is %d\n",returnval);


    }

else if (which ==PageFaultException)
{
int addr=machine->ReadRegister(BadVAddrReg);
int virtualPageNumber=addr/PageSize;
OpenFile * execFile=(currentThread->space)->executableCode;
int code=currentThread->GetPID();
int pageNumber=machine->getMainMemoryPage(code,-1);
TranslationEntry * pageTable=(currentThread->space)->GetPageTable();
ASSERT(pageNumber>=0);

 if (pageTable[virtualPageNumber].readFromDisk==FALSE)
  {

  NoffHeader noffH;
  execFile->ReadAt((char *)&noffH, sizeof(noffH), 0);
  if ((noffH.noffMagic != NOFFMAGIC) && 
  (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    SwapHeader(&noffH);
  ASSERT(noffH.noffMagic == NOFFMAGIC);
  execFile->ReadAt((char *)&noffH, sizeof(noffH), 0);
  int vpn_code=0;
  int offset_code=0;
  int size_code=0;
  int vpn_data=0;
  int offset_data=0;
  int size_data=0;
  int code_start=0;
  int code_end=0;
  int data_start=0;
  int data_end=0;
if (noffH.code.size > 0) 
{
vpn_code = noffH.code.virtualAddr/PageSize;
offset_code = noffH.code.virtualAddr%PageSize;
size_code=noffH.code.size;
code_start=vpn_code*PageSize+offset_code;
code_end=code_start+size_code;
}
if (noffH.initData.size > 0) 
{
    vpn_data = noffH.initData.virtualAddr/PageSize;
    offset_data = noffH.initData.virtualAddr%PageSize;
    size_data=noffH.initData.size;
    data_start=vpn_data*PageSize+offset_data;
    data_end=data_start+size_data;
}
int s1=virtualPageNumber*PageSize;
int e1=virtualPageNumber*PageSize+PageSize;
int s2=code_start;
int e2=code_end;
int maxs=s1>s2?s1:s2;
int mins=e1<e2?e1:e2;
if (maxs<mins)
{
  execFile->ReadAt(&(machine->mainMemory[pageNumber*PageSize+maxs-s1]),
(mins-maxs), noffH.code.inFileAddr+maxs-s2);
}
s2=data_start;
e2=data_end;
maxs=s1>s2?s1:s2;
mins=e1<e2?e1:e2;
if (maxs<mins)
{
  execFile->ReadAt(&(machine->mainMemory[pageNumber*PageSize+maxs-s1]),
(mins-maxs),noffH.initData.inFileAddr+maxs-s2);
}
pageTable[virtualPageNumber].physicalPage = pageNumber;
pageTable[virtualPageNumber].valid = TRUE;
}
else
{
int l=virtualPageNumber*PageSize;
while(l<virtualPageNumber*PageSize+PageSize)
{
  machine->mainMemory[pageNumber*PageSize+l-virtualPageNumber*PageSize]=(currentThread->space)->hardDiskBuffer[l];
  l=l+1;
}
pageTable[virtualPageNumber].physicalPage = pageNumber;
pageTable[virtualPageNumber].valid = TRUE;
}
 currentThread->SortedInsertInWaitQueue(1000+stats->totalTicks);

} 
else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

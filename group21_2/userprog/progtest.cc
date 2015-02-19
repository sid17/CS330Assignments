// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}


void 
ParentExit_StartSchedule ()
{
       int exitcode = 0;
       int i;
       printf("[pid %d]: Exit called. Code: %d\n", currentThread->GetPID(), exitcode);
       // We do not wait for the children to finish.
       // The children will continue to run.
       // We will worry about this when and if we implement signals.
       exitThreadArray[currentThread->GetPID()] = true;

       // Find out if all threads have called exit
       for (i=0; i<thread_index; i++) {
          if (!exitThreadArray[i]) break;
       }
       //printf("%d==%d,%d\n",i,thread_index,exitcode);
       currentThread->Exit(i==thread_index, exitcode);
}

void
ForkStFunction (int dummy)
{
   currentThread->Startup();
   machine->Run();
}

void 
BatchSubmit (char *filename, int n)
{
                
                //printf("Filename%s-%d\n",filename,n);
                OpenFile *executable = fileSystem->Open(filename);
                AddrSpace *space;
                if (executable == NULL) 
                {
                printf("Unable to open file %s\n", filename);
                return;
                }
                Thread *child; 
                space = new AddrSpace(executable);  
                Base_Priority_Array[currentThread->GetPID()]=n+50; 
                Priority_Array[currentThread->GetPID()]=n+50;
                child = new Thread("Forked Thread");
                Base_Priority_Array[currentThread->GetPID()]=50; 
                Priority_Array[currentThread->GetPID()]=50;
                child->space = space;    
                child->SetInitRegisters(child->space->GetNumPages());
                 
                child->StackAllocate (ForkStFunction, 0);
                // int currtime=stats->totalTicks;
                // child->Thread_Start_Time=currtime;
                // child->Time_To_Enter_Ready_Queue=currtime;  
                 //printf("In progtest total ticks = %d and sytem ticks = %d user ticks = %d\n",stats->totalTicks,stats->systemTicks,stats->userTicks);
                child->Schedule ();
                // int currtime=stats->totalTicks;
               
                //child->PrintAttributes();
                //scheduler->Print();
                //printf("Child PID is:%d has priority %d\n",child->GetPID(),n);
                // if (child->GetPID()==3)
                //   child->PrintRegisters();
                delete executable;
                return;
}
void
Set_Type_Algo(int n)
{
  Type_Of_Algorithm=n;
  //printf("Type_Of_Algorithm %d\n",n);
}



void 
setTimerVal(int n)
{

//printf("Value set is %d \n",n);
t_ticks=n;

}
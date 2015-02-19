// thread.cc 
//	Routines to manage threads.  There are four main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef	// this is put at the top of the
					// execution stack, for detecting 
					// stack overflows

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------

Thread::Thread(char* threadName)
{
    int i;

    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif

    threadArray[thread_index] = this;
    pid = thread_index;
    thread_index++;
    ASSERT(thread_index < MAX_THREAD_COUNT);
    if (currentThread != NULL) {
       ppid = currentThread->GetPID();
       currentThread->RegisterNewChild (pid);
       Base_Priority_Array[pid]=Base_Priority_Array[ppid];
       Priority_Array[pid]=Base_Priority_Array[ppid];
    }
    else ppid = -1;

    childcount = 0;
    waitchild_id = -1;


    Previous_Cpu_Burst=20;
    Estimated_Cpu_Burst=20;
    Time_To_Enter_Ready_Queue=0;
    Time_To_Exit_Ready_Queue=0;
    Thread_Start_Time=0;
    Thread_Exit_Time=0;
    Number_Of_Threads+=1;
    Burst_Counter=0;
    Burst_Length=0;
    Thread_Wait_Time=0;
    Thread_Start_Execution=0;
    Start_Sleep=0;
    Total_Sleep=0;
    
    for (i=0; i<MAX_CHILD_COUNT; i++) exitedChild[i] = false;

    instructionCount = 0;
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single integer argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, int arg)
{
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
	  name, (int) func, arg);
    
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    // currentThread->Time_To_Enter_Ready_Queue
    scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts 
					// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    





void 
Thread::PrintAttributes()
{
    printf("PID=%d\nPPID=%d\nPrevious_Cpu_Burst=%d\nEstimated_Cpu_Burst=%d\n,Number_Of_Threads=%d\n,Base_Priority_Array=%d\n Priority_Array=%d\n,Parent_Priority=%d\n,TypeOfAlgorithm=%d\n",GetPID(),GetPPID(),Previous_Cpu_Burst,Estimated_Cpu_Burst,Number_Of_Threads,Base_Priority_Array[GetPID()],Priority_Array[GetPID()],Priority_Array[GetPPID()],Type_Of_Algorithm);
    
    
}   



//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT(*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we don't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we set "threadToBeDestroyed", 
//	so that Scheduler::Run() will call the destructor, once we're
//	running in the context of a different thread.
//
// 	NOTE: we disable interrupts, so that we don't get a time slice 
//	between setting threadToBeDestroyed, and going to sleep.
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\"\n", getName());
    
    threadToBeDestroyed = currentThread;
    Sleep();					// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::SetChildExitCode
//      Called by an exiting thread on parent's thread object.
//----------------------------------------------------------------------

void
Thread::SetChildExitCode (int childpid, int ecode)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   ASSERT(i < childcount);
   childexitcode[i] = ecode;
   exitedChild[i] = true;

   if (waitchild_id == (int)i) {
      waitchild_id = -1;
      // I will wake myself up
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      scheduler->ReadyToRun(this);
      (void) interrupt->SetLevel(oldLevel);
   }
}

//----------------------------------------------------------------------
// Thread::Exit
//      Called by ExceptionHandler when a thread calls Exit.
//      The argument specifies if all threads have called Exit, in which
//      case, the simulation should be terminated.
//----------------------------------------------------------------------


void
Thread::Exit (bool terminateSim, int exitcode)
{
    (void) interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%s\"\n", getName());
//printf("time in exit is %d\n",stats->totalTicks);
    
    threadToBeDestroyed = currentThread;

DEBUG('t', "Finishing thread \"%d\"\n", threadToBeDestroyed->GetPID());

    Thread *nextThread;

    status = BLOCKED;

    currentThread->Thread_Exit_Time=stats->totalTicks;
    
    currentThread->Total_Sleep=currentThread->Total_Sleep+stats->totalTicks-currentThread->Start_Sleep;

    Start_Sleep=stats->totalTicks;


    Completion_Time[threadToBeDestroyed->GetPID()]=threadToBeDestroyed->Thread_Exit_Time;






  int current_time1=stats->systemTicks+stats->userTicks;

  int current_cpu_burst1=current_time1 - currentThread->Thread_Start_Execution;
      
  


if (current_cpu_burst1>0)  
{

  Cpu_Usage[currentThread->GetPID()]=Cpu_Usage[currentThread->GetPID()]+current_cpu_burst1;
  int qw=current_cpu_burst1-currentThread->Estimated_Cpu_Burst;
  if (qw<0){qw=-qw;}
  error_cpu_burst= error_cpu_burst + qw;
  currentThread->Estimated_Cpu_Burst=(int)(0.5*(double)current_cpu_burst1+0.5*(double)currentThread->Estimated_Cpu_Burst);

   
    for (int i=0;i<Number_Of_Threads;i++)
    {
        if (!exitThreadArray[i])
        {
           Cpu_Usage[i]=Cpu_Usage[i]/2; 
           Priority_Array[i]=Base_Priority_Array[i]+Cpu_Usage[i]/2;
        }

    }

}  


    // Set exit code in parent's structure provided the parent hasn't exited
    if (ppid != -1) {
       ASSERT(threadArray[ppid] != NULL);
       if (!exitThreadArray[ppid]) {
          threadArray[ppid]->SetChildExitCode (pid, exitcode);
       }
    }

    while ((nextThread = scheduler->FindNextToRun()) == NULL) {
        if (terminateSim) {
           DEBUG('i', "Machine idle.  No interrupts to do.\n");
           printf("\nNo threads ready or runnable, and no pending interrupts.\n");
           printf("Assuming all programs completed.\n");

      int current_time=stats->userTicks + stats->systemTicks;
      int current_time_total=stats->totalTicks;
      int current_cpu_burst=current_time - currentThread->Thread_Start_Execution;
      if (current_cpu_burst>0)
        {
            currentThread->Burst_Counter++; 
            //currentThread->Estimated_Cpu_Burst=(int)(0.5*current_cpu_burst+0.5*oldThread->Estimated_Cpu_Burst);
            currentThread->Burst_Length+=current_cpu_burst;
            Total_Cpu_Burst=Total_Cpu_Burst+current_cpu_burst;
            /*FILE * f1;
            f1 = fopen("/users/btech/smanocha/cs330assignment2/nachos/code/userprog/result1.txt","a");
           
            fprintf(f1,"\n---%d_%d---",currentThread->GetPID(),current_cpu_burst);
            fclose (f1);*/

            Number_Of_Cpu_Burst=Number_Of_Cpu_Burst+1;

            if (current_cpu_burst>Max_Cpu_Burst)
            Max_Cpu_Burst=current_cpu_burst;
            if (current_cpu_burst<Min_Cpu_Burst)
            Min_Cpu_Burst=current_cpu_burst;
         }
        
//Completion_Time[currentThread->GetPID()]=currentThread->Thread_Exit_Time - currentThread->Thread_Start_Time;

   DEBUG('t', "Last thread : Thread being destroyed  PID:\"%d\" , Start Time:%d , Exit Time:%d ,Burst_Counter:%d ,Burst_Length: %d, Thread_Wait_Time: %d , sleep time %d  \n", currentThread->GetPID(),currentThread->Thread_Start_Time,currentThread->Thread_Exit_Time,currentThread->Burst_Counter,currentThread->Burst_Length,currentThread->Thread_Wait_Time,Total_Sleep);
    
    //printf("Type_Of_Algorithm %d\n",Type_Of_Algorithm );    
if (Type_Of_Algorithm<=10)
{


printf("\n \n \n----------Printing the Statistics----------\n\n");
// if ((stats->totalTicks-stats->idleTicks)==Total_Cpu_Burst)
//       printf("Verified the value of total burst\n");

printf("Total Cpu Busy Time:%d\n",stats->totalTicks-stats->idleTicks);

printf("Total Execution Time:%d\n",stats->totalTicks);

printf("CPU Usage:%lf \n",(1.0-((double)stats->idleTicks/(double)stats->totalTicks))*100);







printf("Maximum CPU Burst Length:%d\n",Max_Cpu_Burst);
printf("Minimum CPU Burst Length:%d\n",Min_Cpu_Burst);
printf("Average CPU Burst:%lf \n",(double)Total_Cpu_Burst/(double)Number_Of_Cpu_Burst);
printf("Number of  CPU Burst:%d \n",Number_Of_Cpu_Burst);
if (Type_Of_Algorithm==2)
  printf("Relative Error in CPU Burst Estimation:%lf \n",(double)error_cpu_burst/(double)Total_Cpu_Burst);
printf("Average Waiting Time:%lf \n",(double)Total_Waiting_Time/(double)Number_Of_Threads);
int max1,min1,sum1;
long long int sum_sq;
max1=Completion_Time[1];
min1=Completion_Time[1];
sum1=Completion_Time[1];
sum_sq=(long long int)((long long int)Completion_Time[1]*(long long int)Completion_Time[1]);
for (int i=2;i<Number_Of_Threads;i++)
{
  if (Completion_Time[i]>max1)
    max1=Completion_Time[i];
  if (Completion_Time[i]<min1)
    min1=Completion_Time[i];
  sum1+=Completion_Time[i];
  sum_sq+=(long long int)((long long int)Completion_Time[i]*(long long int)Completion_Time[i]);
}
//printf("%lld\n",sum_sq);
printf("Maximum Completion Time(excluding main process):%d \n",max1);
printf("Minimum Completion Time(excluding main process):%d \n",min1);
printf("Average Completion Time(excluding main process):%lf \n",(double)sum1/(Number_Of_Threads-1));
double avg2=((double)sum1/(Number_Of_Threads-1))*((double)sum1/(Number_Of_Threads-1));
double var2=(double)sum_sq/(Number_Of_Threads-1);
printf("Variance of Completion Time(excluding main process):%lf \n",var2-avg2);



printf("\n----------Statistics Printed----------\n\n\n");
// FILE * f1;
// f1 = fopen("/users/btech/smanocha/cs330assignment2/nachos/code/userprog/result_1.txt","a");
// //printf("File handler is %d\n",f1);
// ////For batch 1 2 3 4fprintf(f1,"%d***** & %lf\\% & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,(1.0-((double)stats->idleTicks/(double)stats->totalTicks))*100,(double)Total_Waiting_Time/(double)Number_Of_Threads);

// // Part 2 fprintf(f1,"%d***** & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,(double)Total_Waiting_Time/(double)Number_Of_Threads);


// //Part3 fprintf(f1,"%d***** & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,(double)error_cpu_burst/(double)Total_Cpu_Burst);

// fprintf(f1,"%d***** & %d & %d & %lf & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,max1,min1,(double)sum1/(Number_Of_Threads-1),var2-avg2);

// fclose (f1);

// FILE * f1;
// f1 = fopen("/users/btech/smanocha/cs330assignment2/nachos/code/userprog/result_1.txt","a");
// //printf("File handler is %d\n",f1);


// fprintf(f1,"%d %d\n",t_ticks,stats->totalTicks);
////For batch 1 2 3 4fprintf(f1,"%d***** & %lf\\% & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,(1.0-((double)stats->idleTicks/(double)stats->totalTicks))*100,(double)Total_Waiting_Time/(double)Number_Of_Threads);

// Part 2 fprintf(f1,"%d***** & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,(double)Total_Waiting_Time/(double)Number_Of_Threads);


//Part3 fprintf(f1,"%d***** & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,(double)error_cpu_burst/(double)Total_Cpu_Burst);

//fprintf(f1,"%d***** & %d & %d & %lf & %lf \\\\ \n \\hline \n",Type_Of_Algorithm,max1,min1,(double)sum1/(Number_Of_Threads-1),var2-avg2);

//fclose (f1);




}
else if (Type_Of_Algorithm==11)
{


  printf("\n \n \n----------Printing the Statistics----------\n\n");
// if ((stats->totalTicks-stats->idleTicks)==Total_Cpu_Burst)
//       printf("Verified the value of total burst\n");

printf("Total Cpu Busy Time:%d\n",stats->totalTicks-stats->idleTicks);

printf("Total Execution Time:%d\n",stats->totalTicks);

printf("CPU Usage:%lf \n",(1.0-((double)stats->idleTicks/(double)stats->totalTicks))*100);







printf("Maximum CPU Burst Length:%d\n",Max_Cpu_Burst);
printf("Minimum CPU Burst Length:%d\n",Min_Cpu_Burst);
printf("Average CPU Burst:%lf \n",(double)Total_Cpu_Burst/(double)Number_Of_Cpu_Burst);
printf("Number of  CPU Burst:%d \n",Number_Of_Cpu_Burst);

printf("Average Waiting Time:%lf \n ",(double)Total_Waiting_Time/(double)Number_Of_Threads);
int max1,min1,sum1;
long long int sum_sq;
max1=Completion_Time[0];
min1=Completion_Time[0];
sum1=Completion_Time[0];
sum_sq=(long long int)((long long int)Completion_Time[0]*(long long int)Completion_Time[0]);
for (int i=1;i<Number_Of_Threads;i++)
{
  if (Completion_Time[i]>max1)
    max1=Completion_Time[i];
  if (Completion_Time[i]<min1)
    min1=Completion_Time[i];
  sum1+=Completion_Time[i];
  sum_sq+=(long long int)((long long int)Completion_Time[i]*(long long int)Completion_Time[i]);
}
//printf("%lld\n",sum_sq);
printf("Maximum Completion Time(including main process(for arguement type -x)):%d \n",max1);
printf("Minimum Completion Time(including main process(for arguement type -x)):%d \n",min1);
printf("Average Completion Time(including main process(for arguement type -x)):%lf \n",(double)sum1/(Number_Of_Threads));
double avg2=((double)sum1/(Number_Of_Threads))*((double)sum1/(Number_Of_Threads));
double var2=(double)sum_sq/(Number_Of_Threads);
printf("Variance of Completion Time(including main process(for arguement type -x)):%lf \n",var2-avg2);

printf("\n----------Statistics Printed----------\n\n\n");

}



           interrupt->Halt();
        }
        else interrupt->Idle();      // no one to run, wait for an interrupt
    }

    scheduler->Run(nextThread); // returns when we've been signalled
}





// void
// Thread::Exit_StartParent_Schedule (bool terminateSim, int exitcode)
// {
//     (void) interrupt->SetLevel(IntOff);
//     ASSERT(this == currentThread);

//     DEBUG('t', "Finishing thread \"%s\"\n", getName());

//     threadToBeDestroyed = currentThread;

//     Thread *nextThread;

//     status = BLOCKED;

//    while ((nextThread = scheduler->FindNextToRun()) == NULL) {
//         if (terminateSim) {
//            DEBUG('i', "Machine idle.  No interrupts to do.\n");
//            printf("\nNo threads ready or runnable, and no pending interrupts.\n");
//            printf("Assuming all programs completed.\n");
//            interrupt->Halt();
//         }
//         else interrupt->Idle();      // no one to run, wait for an interrupt
//     }
//     scheduler->Run(nextThread); // returns when we've been signalled
// }

//----------------------------------------------------------------------
// Thread::Yield
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    
//printf("Time in yield is %d\n",stats->totalTicks);
    for (int i=0;i<Number_Of_Threads;i++)
        {
            
            if (!exitThreadArray[i])
            {
              //printf("The priority is%d and pid is %d\n",Priority_Array[i],i);
            }
        }

    DEBUG('t', "Yielding thread \"%s\"\n", getName());

int current_time1=stats->systemTicks+stats->userTicks;

int current_cpu_burst1=current_time1 - currentThread->Thread_Start_Execution;
    
if (current_cpu_burst1>0)  
{

Cpu_Usage[currentThread->GetPID()]=Cpu_Usage[currentThread->GetPID()]+current_cpu_burst1;

//printf("CPU Usage is %d\n",Cpu_Usage[currentThread->GetPID()]);

int qw=current_cpu_burst1-currentThread->Estimated_Cpu_Burst;
if (qw<0){qw=-qw;}
error_cpu_burst= error_cpu_burst + qw;
currentThread->Estimated_Cpu_Burst=(int)(0.5*(double)current_cpu_burst1+0.5*(double)currentThread->Estimated_Cpu_Burst);

    // run a for loop
    for (int i=0;i<Number_Of_Threads;i++)
    {
        if (!exitThreadArray[i])
        {
           Cpu_Usage[i]=Cpu_Usage[i]/2; 
           Priority_Array[i]=Base_Priority_Array[i]+Cpu_Usage[i]/2;
        }

    }

}  


//printf("Timer in yield is %d\n",stats->totalTicks);

scheduler->ReadyToRun(this);

nextThread = scheduler->FindNextToRun();

int current_time=stats->systemTicks+stats->userTicks;

if (nextThread == NULL)
{

      printf("never called \n");
}
else if (nextThread->GetPID()==currentThread->GetPID())
{
      //printf("Equal threads \n");
    
      int current_cpu_burst=current_time - currentThread->Thread_Start_Execution;
      DEBUG('t', "Old thread old time: \"%d\"\n",
      currentThread->Thread_Start_Execution);
        if (current_cpu_burst>0)
        {
            currentThread->Burst_Counter++; 
            currentThread->Burst_Length+=current_cpu_burst;
            Total_Cpu_Burst=Total_Cpu_Burst+current_cpu_burst;
            // FILE * f1;
            // f1 = fopen("/users/btech/smanocha/cs330assignment2/nachos/code/userprog/result1.txt","a");
           
            // fprintf(f1,"\n---%d_%d---",currentThread->GetPID(),current_cpu_burst);
            // fclose (f1);

            Number_Of_Cpu_Burst=Number_Of_Cpu_Burst+1;
            if (current_cpu_burst>Max_Cpu_Burst)
            Max_Cpu_Burst=current_cpu_burst;
            if (current_cpu_burst<Min_Cpu_Burst)
            Min_Cpu_Burst=current_cpu_burst;
        }
        currentThread->Thread_Start_Execution=current_time;
}
else
{
  scheduler->Run(nextThread);
}

(void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread is blocked
//	waiting on a synchronization variable (Semaphore, Lock, or Condition).
//	Eventually, some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
Thread::Sleep ()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    Start_Sleep=stats->totalTicks;      // Current thread has just gone to sleep

    status = BLOCKED;


int current_time1=stats->systemTicks+stats->userTicks;

int current_cpu_burst1=current_time1 - currentThread->Thread_Start_Execution;
    
if (current_cpu_burst1>0)  
{

Cpu_Usage[currentThread->GetPID()]=Cpu_Usage[currentThread->GetPID()]+current_cpu_burst1;
int qw=current_cpu_burst1-currentThread->Estimated_Cpu_Burst;
if (qw<0){qw=-qw;}

error_cpu_burst= error_cpu_burst + qw;
currentThread->Estimated_Cpu_Burst=(int)(0.5*(double)current_cpu_burst1+0.5*(double)currentThread->Estimated_Cpu_Burst);

    // run a for loop
    for (int i=0;i<Number_Of_Threads;i++)
    {
        if (!exitThreadArray[i])
        {
           Cpu_Usage[i]=Cpu_Usage[i]/2; 
           Priority_Array[i]=Base_Priority_Array[i]+Cpu_Usage[i]/2;
        }

    }

}  




    while ((nextThread = scheduler->FindNextToRun()) == NULL)
	interrupt->Idle();	// no one to run, wait for an interrupt
        
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	Dummy functions because C++ does not allow a pointer to a member
//	function.  So in order to do this, we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)_ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState] = (int) _ThreadRoot;
    machineState[StartupPCState] = (int) InterruptEnable;
    machineState[InitialPCState] = (int) func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = machine->ReadRegister(i);
}


void
Thread::PrintRegisters()
{
    for (int i = 0; i < NumTotalRegs; i++)
        printf("%d",userRegisters[i]); 
     printf("\n");
     //scheduler->Print();
    //  Thread * next = scheduler->FindNextToRun();
    //   printf("Next Pid%d\n",next->GetPID());

    //   next = scheduler->FindNextToRun();
    //   printf("Next Pid%d\n",next->GetPID());


    // next = scheduler->FindNextToRun();
    //   printf("Next Pid%d\n",next->GetPID());

}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	    machine->WriteRegister(i, userRegisters[i]);
}

void
Thread::SetInitRegisters(int num)
{
                  //printf("function is called \n");
                 for (int i = 0; i < NumTotalRegs; i++)
                    userRegisters[i]=0;
                    userRegisters[PCReg]=0;
                    userRegisters[NextPCReg]=4;
                    userRegisters[StackReg]=num * PageSize - 16;
                    
}


//----------------------------------------------------------------------
// Thread::CheckIfChild
//      Checks if the passed pid belongs to a child of mine.
//      Returns child id if all is fine; otherwise returns -1.
//----------------------------------------------------------------------

int
Thread::CheckIfChild (int childpid)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   if (i == childcount) return -1;
   return i;
}

//----------------------------------------------------------------------
// Thread::JoinWithChild
//      Called by a thread as a result of syscall_Join.
//      Returns the exit code of the child being joined with.
//----------------------------------------------------------------------

int
Thread::JoinWithChild (int whichchild)
{
   // Has the child exited?
   if (!exitedChild[whichchild]) {
      // Put myself to sleep
      waitchild_id = whichchild;
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      printf("[pid %d] Before sleep in JoinWithChild.\n", pid);
      Sleep();
      printf("[pid %d] After sleep in JoinWithChild.\n", pid);
      (void) interrupt->SetLevel(oldLevel);
   }
   return childexitcode[whichchild];
}

//----------------------------------------------------------------------
// Thread::ResetReturnValue
//      Sets the syscall return value to zero. Used to set the return
//      value of syscall_Fork in the created child.
//----------------------------------------------------------------------

void
Thread::ResetReturnValue ()
{
   userRegisters[2] = 0;
}

//----------------------------------------------------------------------
// Thread::Schedule
//      Enqueues the thread in the ready queue.
//----------------------------------------------------------------------

void
Thread::Schedule()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);        // ReadyToRun assumes that interrupts
                                        // are disabled!
    //printf("Scheduler called for PID: %d",GetPID());

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Startup
//      Part of the scheduling code needed to cleanly start a forked child.
//----------------------------------------------------------------------

void
Thread::Startup()
{
   scheduler->Tail();
}

//----------------------------------------------------------------------
// Thread::SortedInsertInWaitQueue
//      Called by syscall_Sleep before putting the caller thread to sleep
//----------------------------------------------------------------------

void
Thread::SortedInsertInWaitQueue (unsigned when)
{
   TimeSortedWaitQueue *ptr, *prev, *temp;

   if (sleepQueueHead == NULL) {
      sleepQueueHead = new TimeSortedWaitQueue (this, when);
      ASSERT(sleepQueueHead != NULL);
   }
   else {
      ptr = sleepQueueHead;
      prev = NULL;
      while ((ptr != NULL) && (ptr->GetWhen() <= when)) {
         prev = ptr;
         ptr = ptr->GetNext();
      }
      if (ptr == NULL) {  // Insert at tail
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ASSERT(prev->GetNext() == NULL);
         prev->SetNext(ptr);
      }
      else if (prev == NULL) {  // Insert at head
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ptr->SetNext(sleepQueueHead);
         sleepQueueHead = ptr;
      }
      else {
         temp = new TimeSortedWaitQueue (this, when);
         ASSERT(temp != NULL);
         temp->SetNext(ptr);
         prev->SetNext(temp);
      }
   }

   IntStatus oldLevel = interrupt->SetLevel(IntOff);
   Sleep();
   (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::IncInstructionCount
//      Called by Machine::Run to update instruction count
//----------------------------------------------------------------------

void
Thread::IncInstructionCount (void)
{
   instructionCount++;
}

//----------------------------------------------------------------------
// Thread::GetInstructionCount
//      Called by syscall_NumInstr
//----------------------------------------------------------------------

unsigned
Thread::GetInstructionCount (void)
{
   return instructionCount;
}
#endif

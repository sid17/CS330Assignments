// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List; 
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());
    //printf("In ready to run total ticks = %d and sytem ticks = %d user ticks = %d\n",stats->totalTicks,stats->systemTicks,stats->userTicks);
    int currtime=stats->totalTicks;
    if(thread ->Time_To_Enter_Ready_Queue==0 && thread->GetPID()!=0)
    {
        thread ->Thread_Start_Time=currtime;
       // printf("Call thread as pid , %d\n",thread->GetPID());    
    }
    thread ->Time_To_Enter_Ready_Queue=currtime; 
    if (thread->getStatus() == BLOCKED)
        thread->Total_Sleep=thread->Total_Sleep+stats->totalTicks-thread->Start_Sleep;

    
    thread->setStatus(READY);
    readyList->Append((void *)thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{

    if (Type_Of_Algorithm==1)
    {
       return (Thread *)readyList->Remove(); 
    }
    else if (Type_Of_Algorithm==2)
    {

        int min11=9999999;
        int mintime=0;
        int sch_thread=-1;
        for (int i=0;i<Number_Of_Threads;i++)
        {
            
            if (!exitThreadArray[i])
            {
                //printf("Algo 3 %d,%d,%d\n",threadArray[i]->Estimated_Cpu_Burst,i,threadArray[i]->Time_To_Enter_Ready_Queue);
                if (threadArray[i]->getStatus()==READY)
                {
                    if (threadArray[i]->Estimated_Cpu_Burst<min11)
                    {
                        min11=threadArray[i]->Estimated_Cpu_Burst;
                        mintime=threadArray[i]->Time_To_Enter_Ready_Queue;
                        sch_thread=i;
                    }
                    if (threadArray[i]->Estimated_Cpu_Burst==min11)
                    {
                        if (mintime>threadArray[i]->Time_To_Enter_Ready_Queue)
                        {
                            mintime=threadArray[i]->Time_To_Enter_Ready_Queue;
                            sch_thread=i;
                        }
                    }
                
                }
                
            }
        }

if (sch_thread!=-1)
{
    // printf("Ready Final1 is %d,%d,%d\n",threadArray[sch_thread]->Estimated_Cpu_Burst,sch_thread,threadArray[sch_thread]->Time_To_Enter_Ready_Queue);
    // printf("Ready Final2 is %d,%d,%d\n",min11,sch_thread,mintime); 

    for (int i=0;i<Number_Of_Threads;i++)
    {

    Thread * currthread = (Thread *)readyList->Remove();
    if (currthread->GetPID()==sch_thread)
    {
      return currthread;
      break;  
    }
    else
        readyList->Append((void *)currthread);  
    }
    

}
else 
{
    return NULL;
}

//return (Thread *)readyList->Remove(); 

    }           
    else if (Type_Of_Algorithm==3 || Type_Of_Algorithm==4 || Type_Of_Algorithm==5 || Type_Of_Algorithm==6 )
    {
        return (Thread *)readyList->Remove(); 
    }
    else if (Type_Of_Algorithm==7 || Type_Of_Algorithm==8 || Type_Of_Algorithm==9 || Type_Of_Algorithm==10 )
    {

        int min11=9999999;
        int mintime=0;
        int sch_thread=-1;
        for (int i=0;i<Number_Of_Threads;i++)
        {
            
            if (!exitThreadArray[i])
            {
                //printf("Algo Unix %d,%d,%d\n",Priority_Array[i],i,threadArray[i]->Time_To_Enter_Ready_Queue);
                if (threadArray[i]->getStatus()==READY)
                {
                    if (Priority_Array[i]<min11)
                    {
                        min11=Priority_Array[i];
                        mintime=threadArray[i]->Time_To_Enter_Ready_Queue;
                        sch_thread=i;
                    }
                    if (Priority_Array[i]==min11)
                    {
                        if (mintime>threadArray[i]->Time_To_Enter_Ready_Queue)
                        {
                            mintime=threadArray[i]->Time_To_Enter_Ready_Queue;
                            sch_thread=i;
                        }
                    }
                   
                
                }
                
            }
        }

if (sch_thread!=-1)
{
    // printf("Algo Final1 is %d,%d,%d\n",Priority_Array[sch_thread],sch_thread,threadArray[sch_thread]->Time_To_Enter_Ready_Queue);
    // printf("AlgoReady Final2 is %d,%d,%d\n",min11,sch_thread,mintime); 

    for (int i=0;i<Number_Of_Threads;i++)
    {

    Thread * currthread = (Thread *)readyList->Remove();
    if (currthread->GetPID()==sch_thread)
    {

        //printf("Find next to run:%d\n",currthread->GetPID());
        return currthread;
         break;
    }
    else
        readyList->Append((void *)currthread);  
    }
    

}
else 
{
    //printf("Find next to run:NULL Value\n");
    return NULL;
}








    }

    else
    {
        return (Thread *)readyList->Remove(); 
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    

//printf("Entered in Run %d\n",stats->totalTicks);

    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running

    //printf("Timer in ready to run  is %d\n",stats->totalTicks);


      DEBUG('t', "Switching from thread \"%d\" to thread  \"%d\"\n",
      oldThread->GetPID(), nextThread->GetPID());



      DEBUG('t', "Priorities are Priority_Array: \"%d\" Base_Priority_Array  \"%d\" and CPU Usage: %d\n",
      Priority_Array[oldThread->GetPID()],Base_Priority_Array[oldThread->GetPID()],Cpu_Usage[oldThread->GetPID()]);

    int current_cpu_burst;
    int current_time=stats->systemTicks+stats->userTicks;
    int current_time_total=stats->totalTicks;
    if (oldThread->GetPID()==0 && oldThread->Burst_Counter==0)
    {
        current_cpu_burst=current_time - 0;
        if (current_cpu_burst>0)
        {
            oldThread->Burst_Counter++;
            oldThread->Burst_Length+=current_cpu_burst;
            Total_Cpu_Burst=Total_Cpu_Burst+current_cpu_burst;
            Number_Of_Cpu_Burst=Number_Of_Cpu_Burst+1;

            // FILE * f1;
            // f1 = fopen("/users/btech/smanocha/cs330assignment2/nachos/code/userprog/result1.txt","a");
           
            // fprintf(f1,"\n---%d_%d---",oldThread->GetPID(),current_cpu_burst);
            // fclose (f1);

            Max_Cpu_Burst=current_cpu_burst;
            Min_Cpu_Burst=current_cpu_burst;
        }
        
        currentThread->Thread_Start_Execution=current_time;
        currentThread->Thread_Wait_Time=currentThread->Thread_Wait_Time+current_time_total - currentThread->Time_To_Enter_Ready_Queue;
        Total_Waiting_Time=Total_Waiting_Time+current_time_total - currentThread->Time_To_Enter_Ready_Queue;

   
DEBUG('t', "Thread Scheduled Time %d",current_time);

    }
    else
    {
        current_cpu_burst=current_time - oldThread->Thread_Start_Execution;
        DEBUG('t', "Old thread old time: \"%d\"\n",
        oldThread->Thread_Start_Execution);
        if (current_cpu_burst>0)
        {

            oldThread->Burst_Counter++; 
            oldThread->Burst_Length+=current_cpu_burst;
            Total_Cpu_Burst=Total_Cpu_Burst+current_cpu_burst;
            // FILE * f1;
            // f1 = fopen("/users/btech/smanocha/cs330assignment2/nachos/code/userprog/result1.txt","a");
           
            // fprintf(f1,"\n---%d_%d---",oldThread->GetPID(),current_cpu_burst);
            
            // fclose (f1);

            Number_Of_Cpu_Burst=Number_Of_Cpu_Burst+1;
            if (current_cpu_burst>Max_Cpu_Burst)
            Max_Cpu_Burst=current_cpu_burst;
            if (current_cpu_burst<Min_Cpu_Burst)
            Min_Cpu_Burst=current_cpu_burst;
    }
        
    currentThread->Thread_Start_Execution=current_time;
    currentThread->Thread_Wait_Time=currentThread->Thread_Wait_Time+current_time_total - currentThread->Time_To_Enter_Ready_Queue;
    Total_Waiting_Time=Total_Waiting_Time+current_time_total - currentThread->Time_To_Enter_Ready_Queue;
    DEBUG('t', "new time:  \"%d\"\n",
      current_time);

    }
    
 


    _SWITCH(oldThread, nextThread);
    

// if (threadToBeDestroyed->GetPID()==0)
//     printf("!11!!!!\n");
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    
if (threadToBeDestroyed != NULL) {

// printf("threadToBeDestroyed %d\n",threadToBeDestroyed -> GetPID());
DEBUG('t', "Thread being destroyed  PID:\"%d\" , Start Time:%d , Exit Time:%d ,Burst_Counter:%d ,Burst_Length: %d, Thread_Wait_Time: %d , Sleep Time \n", threadToBeDestroyed->GetPID(),threadToBeDestroyed->Thread_Start_Time,threadToBeDestroyed->Thread_Exit_Time,threadToBeDestroyed->Burst_Counter,threadToBeDestroyed->Burst_Length,threadToBeDestroyed->Thread_Wait_Time,threadToBeDestroyed->Total_Sleep);
delete threadToBeDestroyed;
threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Tail
//      This is the portion of Scheduler::Run after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
Scheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

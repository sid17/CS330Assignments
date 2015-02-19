// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"


// External functions used by this file

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out),BatchSubmit (char *filename, int n),Set_Type_Algo(int n),setTimerVal(int n);
extern void ParentExit_StartSchedule ();
extern void MailTest(int networkID);

//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int argCount;			// the number of arguments 
					// for a particular command

    DEBUG('t', "Entering main");

//printf("No of passed arguements %d \n ",argc);

int algo_type=0;
int flag_f=0;
for (int i=0;i<argc;i++)
{
	//printf("%s\n",*(argv+i));
	if (!strcmp(*(argv+i), "-F")) 
	{
		//filename1=;
		//printf("%s\n",*(argv+i+1));

		char *filename=*(argv+i+1);

		flag_f=1;

FILE *fp;
fp = fopen(filename, "r");
char ch;


//printf("Filename is hello %s\n",filename);

while(fscanf(fp,"%c",&ch) == 1)
{
	if (ch==' ')
	{
		
		continue;
		
	}
	else if (ch=='\n')
	{
		break;
	}
	else
	{
		algo_type=algo_type*10+ch-48;
	}

}


	break;
	}
}
printf("Type of algorithm is %d,\n",algo_type);


if (flag_f==1)
{


	if (algo_type==1 || algo_type==2)
	{
		setTimerVal(100);
	}
	else if (algo_type==3 || algo_type==7)
	{
		setTimerVal(32);
	}
	else if (algo_type==4 || algo_type==8)
	{
		setTimerVal(65);
	}
	else if (algo_type==5 || algo_type==9)
	{
		setTimerVal(97);
	}
	else if (algo_type==6 || algo_type==10)
	{
		setTimerVal(20);
	}

}

else
{
	Set_Type_Algo(11);
	setTimerVal(100);
}
// printf("the passed value is %d \n ",atoi(*(argv+argc-1)));
// setTimerVal(atoi(*(argv+argc-1)));
    
(void) Initialize(argc, argv);

    
#ifdef THREADS
    ThreadTest();
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
        if (!strcmp(*argv, "-z"))               // print copyright
            printf (copyright);
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-x")) {        	// run a user program
	    ASSERT(argc > 1);
	    // printf("%s",*(argv+1));
	    // return 0;
         StartProcess(*(argv + 1));
            argCount = 2;
        } 


else if (!strcmp(*argv, "-F")) {        	// run a user program
	    ASSERT(argc > 1);

char *filename=*(argv+1);



FILE *fp;
fp = fopen(filename, "r");
char ch;
char buffer[1000];
int count=0;
int num=0;
int flag=0;
int space_flag=0;
int type_algo=0;
int algo_flag=0;
while(fscanf(fp,"%c",&ch) == 1)
{
	if (ch==' ')
	{
		
		continue;
		
	}
	else if (ch=='\n')
	{
		break;
	}
	else
	{
		type_algo=type_algo*10+ch-48;
	}

}
if (type_algo>=1&&type_algo<=10)
	algo_flag=1;
else
	printf("Scheduling type not defined");
Set_Type_Algo(type_algo);
while(fscanf(fp,"%c",&ch) == 1 && algo_flag)
{
	if (ch==' ')
	{
		space_flag=1;
		continue;
		
	}
		
	if (ch=='\n')
	{
		if (count==0)
			continue;
		buffer[count]='\0';
		if (num==0)
			num=100;
		//printf("%s---%d\n",buffer,num);
		if (num>100)
			{flag=1;break;}
		BatchSubmit(buffer,num);
		int k;
		for (k=0;k<count;k++)
		{
			buffer[k]='\0';
		}
		count=0;
		num=0;
		space_flag=0;


	}
	else if (ch>=48 && ch<=57 && space_flag)
	{
		num=num*10+ch-48;
	}

	else
	{
		buffer[count++]=ch;
	}
}
if (count!=0 && !flag && algo_flag)
{
	buffer[count]='\0';
	if (num==0)
		num=100;
// printf("%s---%d\n",buffer,num);
	BatchSubmit(buffer,num);
}
if (!flag && algo_flag)
	ParentExit_StartSchedule();
else
	printf("Priority value cannot be greater that 100\n");
fclose(fp);
   
return(0);





        } 




        else if (!strcmp(*argv, "-c")) {      // test the console
	    if (argc == 1)
	        ConsoleTest(NULL, NULL);
	    else {
		ASSERT(argc > 2);
	        ConsoleTest(*(argv + 1), *(argv + 2));
	        argCount = 3;
	    }
	    interrupt->Halt();		// once we start the console, then 
					// Nachos will loop forever waiting 
					// for console input
	}
#endif // USER_PROGRAM
#ifdef FILESYS
	if (!strcmp(*argv, "-cp")) { 		// copy from UNIX to Nachos
	    ASSERT(argc > 2);
	    Copy(*(argv + 1), *(argv + 2));
	    argCount = 3;
	} else if (!strcmp(*argv, "-p")) {	// print a Nachos file
	    ASSERT(argc > 1);
	    Print(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-r")) {	// remove Nachos file
	    ASSERT(argc > 1);
	    fileSystem->Remove(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-l")) {	// list Nachos directory
            fileSystem->List();
	} else if (!strcmp(*argv, "-D")) {	// print entire filesystem
            fileSystem->Print();
	} else if (!strcmp(*argv, "-t")) {	// performance test
            PerformanceTest();
	}
#endif // FILESYS
#ifdef NETWORK
        if (!strcmp(*argv, "-o")) {
	    ASSERT(argc > 1);
            Delay(2); 				// delay for 2 seconds
						// to give the user time to 
						// start up another nachos
            MailTest(atoi(*(argv + 1)));
            argCount = 2;
        }
#endif // NETWORK
    }

    currentThread->Finish();	// NOTE: if the procedure "main" 
				// returns, then the program "nachos"
				// will exit (as any other normal program
				// would).  But there may be other
				// threads on the ready list.  We switch
				// to those threads by saying that the
				// "main" thread is finished, preventing
				// it from returning.
    return(0);			// Not reached...
}

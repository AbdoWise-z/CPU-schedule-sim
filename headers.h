#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>


//enable if you want to see the output written to a file
//shouldn't use this in INTERACTIVE mode btw ...
//#define OUT_TO_FILE
#define SYNC_IO setbuf(stdout , NULL);
//#define PHASE_1 

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================

#define CLK_MS 100
#define CLK_START_DELAY_MS 100

//if defined , the code will work as phase 1
//#define PHASE_1_CODE

//enable Interactive mode if this is defined
#define INTERACTIVE

//enable if you want to see log output
//#define ENABLE_LOG

//the console size , vs-code is 143 (143 - 7 = 136 ~= 130)
#define CONSOLE_WIDTH 130

#ifndef ENABLE_LOG
#define Logger(x...) 
#else
#define Logger(x...) printf(x)
#endif

int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        Logger("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}


#include "data_structure.h"

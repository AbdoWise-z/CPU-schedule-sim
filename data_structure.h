#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define STATE_NOT_READY   0
#define STATE_READY       1

typedef struct ProcessInfo{
    int id , arrival , runtime , priority , memSize; //input values
    int pid , state , finish_time , start_time , remainning; //runtime values
    int mem_start , mem_end; //mem values
} ProcessInfo;

typedef struct SchedulerMessage{
    long type;
    ProcessInfo p;
} SchedulerMessage;

typedef struct ProcessTransferBlock{
    int have_data;
    int ready;
    int done_recieving;
    ProcessInfo data;
} ProcessTransferBlock;

typedef struct ControlBlock{
    int active_pid; //which processes should be running right now ?
    int remainning; //a value that the process returns to the scheduler
    int lock1;      //a lock before storing the value
                    //think of it as if the scheduler is telling the process that it is ready to receive a value
    int ready;      //a flag to determine if the process is ready to store a value
} ControlBlock;

typedef struct ProcessesMessage{
    long type;
    int remainning;
} ProcessesMessage;

#define PQ_MAX_SIZE 2000
#define PQ_t ProcessInfo
#define PQ_Q_t PriorityQueue

#include "PriorityQueue.h"

#define CQ_Q_t CircularQueue
#define CQ_t ProcessInfo
#define CQ_MAX_SIZE 2000

#include "CircularQueue.h"

typedef struct MemorySlot{
    int start;
    int end;
    int id;
} MemorySlot;

#define LL_t MemorySlot
#define LL_L_t LinkedList

#include "LinkedList.h"


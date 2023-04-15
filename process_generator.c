#include "headers.h"
#include <string.h>
#include <ctype.h>
#include <time.h>

//TODO: fix the high clk speed issue between PG and SC , (queues are bad ...)

void clearResources(int);

PriorityQueue* pq = NULL;
int sc_m_q = -1;
FILE* cfg = NULL;

int main(int argc, char * argv[])
{
    
#ifdef OUT_TO_FILE
    freopen("results.txt" , "a+" , stdout);
    SYNC_IO;
#endif

    signal(SIGINT, clearResources);
    
    printf("[PG] pid: %d , gpid: %d \n" , getpid() , getpgrp());

    // 1. Read the input files.
    cfg = fopen("processes.txt" , "r");
    int i , j = 0;
    char buff[1024];
    ProcessInfo info;
    pq = createPriorityQueue();

    while (fgets(buff , 1024 , cfg) != NULL){
        i = 0;
        while (isspace(buff[i])) {i++;}
        if (buff[i] == '#') continue;

        sscanf(buff , "%d %d %d %d" , &info.id , &info.arrival , &info.runtime , &info.priority);
        insert(pq , INT_MAX - info.arrival , info);
        //printf("p: %d , %d , %d , %d\n" , info.id , info.arrival , info.runtime , info.priority);
    }

    fclose(cfg);
    cfg = NULL;

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Select a type of scheduling:\n");
    printf("    1. Non-preemptive Highest Priority First (HPF).\n");
    printf("    2. Shortest Remaining time Next (SRTN).\n");
    printf("    3. Round Robin (RR).\n");
    printf("    4. Preemptive Highest Priority First (HPF).\n");
    int sq_t = 3;
    int quanta_size = 0;
    while (sq_t > 4 || sq_t < 1){
        scanf("%d" , &sq_t);
        if (sq_t <= 4 || sq_t >= 1)
            break;
        printf("invalid input , select a number between { 1 , 3 }");
    }

    if (sq_t == 3){
        quanta_size = 5;
        while (quanta_size <= 0){
            printf("Enter a valid quanta size: ");
            scanf("%d" , &quanta_size);
        }
    }
    // 3. Initiate and create the scheduler and clock processes.
    pid_t clk = fork();
    if (clk == 0){
        execl("./out/clk.out" , "" , NULL);
        printf("[PG] Error in clk\n");
        exit(EXIT_FAILURE);
    }

    usleep(50 * 1000);

    pid_t scheduler = fork();
    char sq_tb[100];
    sprintf(sq_tb , "%d" , sq_t);
    char quanta_b[100];
    sprintf(quanta_b , "%d" , quanta_size);

    if (scheduler == 0){
        execl("./out/scheduler.out" , sq_tb , quanta_b , NULL);
        printf("[PG] Error in scheduler\n");
        exit(EXIT_FAILURE);
    }

    
    printf("[PG] Scadular running at : %d\n" , scheduler);
    printf("[PG] clk running at      : %d\n" , scheduler);


    initClk();
    // To get time use this
    int x = getClk();
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    sc_m_q = msgget(ftok("PG_SC" , 15) , 0666 | IPC_CREAT);

    SchedulerMessage msg;
    printf("[PG] Preparing %d processes\n" , pq->size);
    while (pq->size){
        ProcessInfo process = extractMax(pq);
        while (getClk() < process.arrival){} //wait until it arrives
        if (getClk() != process.arrival)
            printf("[PG] Warnning , Process '%d' was unable to arrive on time (offset: %d)\n" , process.id , getClk() - process.arrival);
        msg.p = process;
        msg.type = (pq->size > 0) ? 2 : 1;
        printf("[PG] Sending process: %d , at time %d\n" , process.id , getClk());
        msgsnd(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , 0);
    }

    
    //destroyClk(false); //this is up to the scheduler now , cuz it will run more than the process_generator
    if (pq){
        free(pq);
        pq = NULL;
    }

    printf("[PG] finished\n");
    msgrcv(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , 3 , !IPC_NOWAIT); //wait for exit message from Scheduler
    printf("[PG] Terminating\n");
}



void clearResources(int signum)
{
    if (pq)
        free(pq);

    if (cfg)
        fclose(cfg);

    exit(0);
}

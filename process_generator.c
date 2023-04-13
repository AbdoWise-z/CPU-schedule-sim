#include "headers.h"
#include <string.h>
#include <ctype.h>
#include <time.h>

void clearResources(int);

int sc_m_q = -1;
PriorityQueue* pq = NULL;


int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    FILE* cfg = fopen("processes.txt" , "r");
    int i , j = 0;
    char buff[1024];
    ProcessInfo info;
    PriorityQueue* pq = createPriorityQueue();

    while (fgets(buff , 1024 , cfg) != NULL){
        i = 0;
        while (isspace(buff[i])) {i++;}
        if (buff[i] == '#') continue;

        sscanf(buff , "%d %d %d %d" , &info.id , &info.arrival , &info.runtime , &info.priority);
        insert(pq , INT_MAX - info.arrival , info);
        //printf("p: %d , %d , %d , %d\n" , info.id , info.arrival , info.runtime , info.priority);
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Select a type of scheduling:\n");
    printf("    1. Non-preemptive Highest Priority First (HPF).\n");
    printf("    2. Shortest Remaining time Next (SRTN).\n");
    printf("    3. Round Robin (RR).\n");
    int sq_t = -1;
    while (sq_t > 3 || sq_t < 1){
        scanf("%d" , &sq_t);
        if (sq_t <= 3 || sq_t >= 1)
            break;
        printf("invalid input , select a number between { 1 , 3 }");
    }
    // 3. Initiate and create the scheduler and clock processes.
    pid_t clk = fork();
    if (clk == 0){
        execl("./out/clk.out" , "" , NULL);
        printf("Error in clk\n");
        exit(EXIT_FAILURE);
    }

    usleep(50 * 1000);

    pid_t scheduler = fork();
    char sq_tp[10];
    sprintf(sq_tp , "%d" , sq_t);
    if (scheduler == 0){
        execl("./out/scheduler.out" , sq_tp , NULL);
        printf("Error in scheduler\n");
        exit(EXIT_FAILURE);
    }

    
    printf("Scadular running at : %d\n" , scheduler);
    printf("clk running at      : %d\n" , scheduler);


    initClk();
    // To get time use this
    int x = getClk();
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    int sc_m_q = msgget(scheduler , 666 | IPC_CREAT);
    

    destroyClk(true);
}



void clearResources(int signum)
{
    if (sc_m_q >= 0){
        msgctl(sc_m_q , IPC_RMID , (struct msqid_ds*) 0);
    }

    if (pq){
        free(pq);
    }
}

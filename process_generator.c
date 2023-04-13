#include "headers.h"
#include <string.h>
#include <ctype.h>

void clearResources(int);



int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    FILE* cfg = fopen("processes.txt" , "r");
    int i , j = 0;
    char buff[1024];
    ProcessInfo info;
    CircularQueue* cq = createCircularQueue();

    while (fgets(buff , 1024 , cfg) != NULL){
        i = 0;
        while (isspace(buff[i])) {i++;}
        if (buff[i] == '#') continue;

        sscanf(buff , "%d %d %d %d" , &info.id , &info.arrival , &info.runtime , &info.priority);
        enqueue(cq , info);
        //printf("p: %d , %d , %d , %d\n" , info.id , info.arrival , info.runtime , info.priority);
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Select a type of scheduling:\n");
    printf("    1. Non-preemptive Highest Priority First (HPF).\n");
    printf("    2. Shortest Remaining time Next (SRTN).\n");
    printf("    3. Round Robin (RR).\n");
    int sq_t;
    scanf("%d" , &sq_t);

    // 3. Initiate and create the scheduler and clock processes.
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}



void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}

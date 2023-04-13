#include "headers.h"
#include <string.h>
#include <ctype.h>

void clearResources(int);



int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock

    FILE* cfg = fopen("processes.txt" , "r");
    int i , j = 0;
    char buff[1024];
    ProcessInfo info;
    
    while (fgets(buff , 1024 , cfg) != NULL){
        i = 0;
        while (isspace(buff[i])) {i++;}
        if (buff[i] == '#') continue;

        sscanf(buff , "%d %d %d %d" , &info.id , &info.arrival , &info.runtime , &info.priority);
        //printf("p: %d , %d , %d , %d\n" , info.id , info.arrival , info.runtime , info.priority);
    }

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

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

    bool ui = true;
    int sq_type = -1;
    int quanta_size = -1;
    int mem_type = -1;

    char* input_file = NULL;

    if (argc > 1){
        ui = false;
        input_file = argv[1];
        for (int i = 2;i < argc;i++){
            if (strcmp(argv[i] , "-sch") == 0){
                if (i + 1 >= argc) {
                    ui = true;
                    printf("Invalid run command [No SCH type], requesting info from user.");
                    break;
                }

                sq_type = atoi(argv[i + 1]);
                if (sq_type < 1 || sq_type > 5){
                    printf("Invalid run command [ Invalid SCH type ], requesting info from user.");
                    ui = true;
                    break;
                }
            }

            if (strcmp(argv[i] , "-q") == 0){
                if (i + 1 >= argc) {
                    ui = true;
                    printf("Invalid run command [ No quanta size ], requesting info from user.");
                    break;
                }

                quanta_size = atoi(argv[i + 1]);
                if (quanta_size < 1){
                    printf("Invalid run command [ quanta_size < 1 ], requesting info from user.");
                    ui = true;
                    break;
                }
            }

            if (strcmp(argv[i] , "-mem") == 0){
                if (i + 1 >= argc) {
                    ui = true;
                    printf("Invalid run command [No mem type], requesting info from user.");
                    break;
                }

                mem_type = atoi(argv[i + 1]);
                if (mem_type < 1 || mem_type > 3){
                    printf("Invalid run command [ Invalid mem type ], requesting info from user.");
                    ui = true;
                    break;
                }
            }
        }
    }

    if (mem_type > 3 || mem_type < 1 || sq_type > 5 || sq_type < 1 || quanta_size < 1){
        ui = true;
    }

    if (ui){
        // 1. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
        if (input_file == NULL) input_file = "processes.txt";
        printf("Select a type of scheduling:\n");
        printf("    1. Non-preemptive Highest Priority First (HPF).\n");
        printf("    2. Shortest Remaining time Next (SRTN).\n");
        printf("    3. Round Robin (RR).\n");
        printf("    4. Preemptive Highest Priority First (HPF).\n");
        printf("    5. First Come First Serve.\n");
        
        while (sq_type > 5 || sq_type < 1){
            scanf("%d" , &sq_type);
            if (sq_type <= 5 || sq_type >= 1)
                break;
            printf("invalid input , select a number between [ 1 , 4 ]");
        }

        if (sq_type == 3){
            while (quanta_size <= 0){
                printf("Enter a valid quanta size: ");
                scanf("%d" , &quanta_size);
            }
        }

        printf("Select memory mapping type: \n");
        printf("    1. First fit\n");
        printf("    2. Next fit\n");
        printf("    3. Buddy\n");
        
        while (mem_type > 3 || mem_type < 1){
            scanf("%d" , &mem_type);
            if (mem_type <= 4 || mem_type >= 1)
                break;
            printf("invalid input , select a number between [ 1 , 3 ]");
        }
    }

    // 2. Read the input files.
    cfg = fopen(input_file , "r");
    int i , j = 0;
    char buff[1024];
    ProcessInfo info;
    createPriorityQueue(&pq);

    while (fgets(buff , 1024 , cfg) != NULL){
        i = 0;
        while (isspace(buff[i])) {i++;}
        if (buff[i] == '#') continue;

        sscanf(buff , "%d %d %d %d %d" , &info.id , &info.arrival , &info.runtime , &info.priority , &info.memSize);
        insert(pq , INT_MAX - info.arrival , info);
        //printf("p: %d , %d , %d , %d\n" , info.id , info.arrival , info.runtime , info.priority);
    }

    fclose(cfg);
    cfg = NULL;
    
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
    sprintf(sq_tb , "%d" , sq_type);
    char quanta_b[100];
    sprintf(quanta_b , "%d" , quanta_size);
    char mem_b[100];
    sprintf(mem_b , "%d" , mem_type);


    if (scheduler == 0){
        execl("./out/scheduler.out" , sq_tb , quanta_b , mem_b , NULL);
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
        ProcessInfo process = extract(pq);
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

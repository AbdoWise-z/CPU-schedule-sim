#include "headers.h"

#define CLK_INIT clk = getClk()
#define CLK_WAIT(x) while (clk + x > getClk()) {}

int sc_m_q; //PG --> SC message queue
//int p_m_q;  //SC <-> process message queu

ControlBlock* ProcessControl;
int controlBlockId;

int clk;
int mem_type;
bool received_all = false;


//Scheduling types
void hpf(bool);
void srtn();
void rr(int);
void fcfs();

//Memory mapping types
bool mm_firstFitAlloc(ProcessInfo* info);
bool mm_nextFitAlloc(ProcessInfo* info);
bool mm_buddyAlloc(ProcessInfo* info);

void mm_clearMemory(ProcessInfo* info);

// bool isTaken(int);
// void markTaken(int start , int end);
// void freeMem(int start , int end);

void get_process(ProcessInfo* p);

ProcessInfo current_process;
void run_for(ProcessInfo* p , int qouta);
bool switch_to(ProcessInfo* p);

PriorityQueue* pq;
PriorityQueue* finish_queue;
CircularQueue* waiting_queue;
LinkedList* memMap;

SchedulerMessage msg;
//ProcessesMessage pMsg;

void clearResources(int);


int main(int argc, char* argv[])
{
    
#ifdef OUT_TO_FILE
    SYNC_IO;
#endif

    signal(SIGINT, clearResources);

    initClk();
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.

    //init all the variables
    int t , q;

    t = atoi(argv[0]);
    q = atoi(argv[1]);
    mem_type = atoi(argv[2]);


    printf("[Scheduler] cfg , t : %d  , q : %d , mem_type: %d\n" , t , q , mem_type);

    sc_m_q = msgget(ftok("PG_SC" , 15) , 0666 | IPC_CREAT);
    //p_m_q  = msgget(ftok("SC_P"  , 15) , 0666 | IPC_CREAT);

    controlBlockId = shmget(ftok("SC_P"  , 15), sizeof(ControlBlock), IPC_CREAT | 0666);
    ProcessControl = (ControlBlock *) shmat(controlBlockId, (void *)0, 0);
    ProcessControl->active_pid = -1;
    ProcessControl->lock1 = -1;
    
    current_process.state = STATE_NOT_READY;
    current_process.id    = -1;

    createPriorityQueue(&pq);
    createPriorityQueue(&finish_queue);
    createCircularQueue(&waiting_queue);
    createLL(&memMap);

    switch (t)
    {
    case 1:
        hpf(false);
        break;
    case 2:
        srtn();
        break;
    case 3:
        rr(q);
        break;
    case 4:
        hpf(true);
        break;
    case 5:
        fcfs();
        break;
    
    default:
        printf("[Scheduler] failed to select a type.\n");
        break;
    }

    printf("[Scheduler] Finished \n");
    
    msg.type = 3; //tell pg it can exit
    msgsnd(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , !IPC_NOWAIT);

    printf("                           [results]\n");
    printf("#             id     arrival  start_time     runtime finish_time    priority\n");
    PQ_t proc;
    int i = 0;
    while (finish_queue->size){
        proc = extract(finish_queue);
        printf("%04d%12d%12d%12d%12d%12d%12d\n" , i++ , proc.id , proc.arrival , proc.start_time , proc.runtime , proc.finish_time , proc.priority);
    }

    destroyClk(true);
}

bool switch_to(ProcessInfo* p){
    if (current_process.id != p->id){
        if (current_process.id != -1){
            kill(current_process.pid , SIGSTOP);
            printf("[Scheduler] Pausing: %d \n" , current_process.pid);
        }

        if (p->state == STATE_NOT_READY){
            p->pid = fork();
            p->start_time = getClk();
            
            if (p->pid == 0){
                char buff[100];
                sprintf(buff , "%d" , p->runtime);
                execl("./out/process.out" , buff , NULL);
                exit(-1);
            }
            printf("[Scheduler] Running a child process for the first time: %d \n" , p->pid);
            p->state = STATE_READY;
        }else if (p->state == STATE_READY){
            printf("[Scheduler] Resuming: %d\n" , p->pid);
            kill(p->pid , SIGCONT);
        }
        return true;
    }
    return false;
}

void run_for(ProcessInfo* p , int qouta){
    if (p->remainning <= 0){
        printf("[Scheduler] Error , trying to run a process that should have finished: %d\n" , p->pid);
        return;
    }

    switch_to(p);

    //rintf("in: run_for %d\n" , qouta);

    while (qouta){
        qouta--;
        
        ProcessControl->lock1 = 1;           //tell the process that the Scheduler is busy now and can't receive values now
        ProcessControl->active_pid = p->pid; //tell the process it can start its jop
        ProcessControl->ready = 0;

        CLK_INIT;
        CLK_WAIT(1);
        
        //int k = msgrcv(p_m_q , &pMsg , sizeof(ProcessesMessage) - sizeof(long) , 0 , !IPC_NOWAIT);
        //if (k < 0){
        //    printf("[Scheduler] Error in run_for , process didn't return any result\n");
        //    return;
        //}
        
        while (ProcessControl->ready == 0){} //wait for the process to be ready

        ProcessControl->active_pid = -1;     //prevent the process from taking extra quota
        ProcessControl->lock1 = 0;           //tell the process that the scheduler is ready to receive the value
        
        while (ProcessControl->lock1 == 0){} //wait for the process to write

        //printf("[Scheduler] Processes %d ran for 1 quota \n" , p->pid);

        p->remainning = ProcessControl->remainning;
        if (p->remainning == 0){
            p->finish_time = getClk();
            //printf("[Scheduler] process finished , pid: %d , id: %d\n" , p->pid , p->id);
            break;//no need to continue
        }
    }

    //printf("out: run_for %d\n" , qouta);

}

void get_process(ProcessInfo* p){
    
    //printf("get_p in\n");
    usleep(CLK_MS / 4);

    while (msgrcv(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , 0 , IPC_NOWAIT) > 0){
        enqueue(waiting_queue , msg.p);

        //received_all = false;

        if (msg.type == 1){
            printf("[Scheduler] No more processes to receive.\n");
            received_all = true; //done receiving ...
        }
        
    }

    p->id = -1;

    int s = waiting_queue->size;
    if (s > 0){
        //printf("get_p size -> %d\n" , waiting_queue->size);
        
        ProcessInfo temp;
        for (int i = 0;i < s;i++){
            temp = dequeue(waiting_queue);
            //printf("get_p dequeue\n");
            if (mem_type == 1){
                //printf("get_p memtype1\n");
                if (mm_firstFitAlloc(&temp)){
                    *p = temp;

                    p->pid = -1; //default value
                    p->state = STATE_NOT_READY;
                    p->remainning = p->runtime;

                    //printf("get_p out\n");
        
                    return;
                }
            }else if (mem_type == 2){
                if (mm_nextFitAlloc(&temp)){
                    *p = temp;

                    p->pid = -1; //default value
                    p->state = STATE_NOT_READY;
                    p->remainning = p->runtime;
        
                    return;
                }
            }else if (mem_type == 3){
                if (mm_buddyAlloc(&temp)){
                    *p = temp;

                    p->pid = -1; //default value
                    p->state = STATE_NOT_READY;
                    p->remainning = p->runtime;
        
                    return;
                }
            }
            enqueue(waiting_queue , temp);
        }
    }

    //printf("get_p out empty\n");
    

    // p->id = -1; //didnt receive anything

    // int s = msgrcv(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , 0 , IPC_NOWAIT);
    // if (s > 0){
    //     *p = msg.p;
    //     p->pid = -1; //default value
    //     p->state = STATE_NOT_READY;
    //     p->remainning = p->runtime;
        
    //     if (msg.type == 1){
    //         printf("[Scheduler] No more processes to receive.\n");
    //         return true; //done reading
    //     }
    //     return false;
    // }

    // p->id = -1; //didnt receive anything

    // return false;
}

void hpf(bool preemptive){
    printf("[Scheduler] Running HPF\n");
    ProcessInfo recv;
    
    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        recv.id = INT_MAX;
        while (recv.id > 0 && !received_all){
            get_process(&recv);

            if (recv.id > 0){
                printf("[Scheduler] Adding a processes to the queue: %d , Priority: %d\n" , recv.id , recv.priority);
                if (recv.remainning == 0){
                    printf("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                    recv.start_time = getClk();
                    recv.finish_time = recv.start_time;
                    insert(finish_queue , -getClk() , recv);
                }else{
                    insert(pq , -recv.priority , recv);
                }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //printf("[Scheduler] selecting next , id: %d , pid: %d , pri: %d\n" , next.id , next.pid , next.priority);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0)
                insert(pq , preemptive ? -next.priority : INT_MAX , next);
            else{
                //printf("[Scheduler] process finished\n");
                insert(finish_queue , -getClk() , next);
                mm_clearMemory(&next);
                current_process.id = -1; //no current , no need to stop the next one
                
            }
        } else {
            CLK_INIT;
            CLK_WAIT(1);
            printf("[Scheduler] waiting...\n");
        }
    }
    printf("[Scheduler] Finished HPF\n");
}

void srtn(){
    printf("[Scheduler] Running SRTN\n");
    ProcessInfo recv;

    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        recv.id = INT_MAX;
        while (recv.id > 0 && !received_all){
            get_process(&recv);

            if (recv.id > 0){
                printf("[Scheduler] Adding a processes to the queue: %d , RT: %d\n" , recv.id , recv.remainning);
                if (recv.remainning == 0){
                    printf("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                    recv.start_time = getClk();
                    recv.finish_time = recv.start_time;
                    insert(finish_queue , -getClk() , recv);
                }else{
                    insert(pq , -recv.remainning , recv);
                }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //printf("[Scheduler] selecting next , id: %d , pid: %d , RT: %d\n" , next.id , next.pid , next.remainning);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0)
                insert(pq , -next.remainning , next);
            else{
                //printf("[Scheduler] process finished\n");
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }
        } else {
            CLK_INIT;
            CLK_WAIT(1);
            printf("[Scheduler] waiting...\n");
        }
    }
    printf("[Scheduler] Finished SRTN\n");
}

void rr(int q){
    printf("[Scheduler] Running RR\n");
    ProcessInfo recv;
    
    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        recv.id = INT_MAX;
        while (recv.id > 0 && !received_all){
            get_process(&recv);
            if (recv.id > 0){
                printf("[Scheduler] Adding a processes to the queue: %d , RT: %d\n" , recv.id , recv.remainning);
                if (recv.remainning == 0){
                    printf("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                    recv.start_time = getClk();
                    recv.finish_time = recv.start_time;
                    insert(finish_queue , -getClk() , recv);
                }else{
                    insert(pq , -getClk() , recv);
                }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //printf("[Scheduler] selecting next , id: %d , pid: %d , RT: %d\n" , next.id , next.pid , next.remainning);
            run_for(&next , q);
            current_process = next;
            if (next.remainning > 0){
                //printf("[Scheduler] re-inserting\n");
                insert(pq , -getClk() , next);
            }else{
                //printf("[Scheduler] process finished\n");
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }

            //printf("[Scheduler] jop done for %d\n" , next.id);

        } else {
            CLK_INIT;
            CLK_WAIT(1);
            printf("[Scheduler] waiting...\n");
        }
    }
    printf("[Scheduler] Finished RR\n");
}

void fcfs(){
    printf("[Scheduler] Running FCFS\n");
    ProcessInfo recv;

    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        recv.id = INT_MAX;
        while (recv.id > 0 && !received_all){
            get_process(&recv);

            if (recv.id > 0){
                printf("[Scheduler] Adding a processes to the queue: %d , RT: %d\n" , recv.id , recv.remainning);
                if (recv.remainning == 0){
                    printf("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                    recv.start_time = getClk();
                    recv.finish_time = recv.start_time;
                    insert(finish_queue , -getClk() , recv);
                }else{
                    insert(pq , -recv.arrival , recv);
                }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //printf("[Scheduler] selecting next , id: %d , pid: %d , RT: %d\n" , next.id , next.pid , next.remainning);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0)
                insert(pq , 1 , next);
            else{
                //printf("[Scheduler] process finished\n");
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }
        } else {
            CLK_INIT;
            CLK_WAIT(1);
            printf("[Scheduler] waiting...\n");
        }
    }
    printf("[Scheduler] Finished FCFS\n");
}

bool mm_firstFitAlloc(ProcessInfo* info){
    //TODO: impelement this
    return true;
}

bool mm_nextFitAlloc(ProcessInfo* info){
    //TODO: implement this
    return true;
}

bool mm_buddyAlloc(ProcessInfo* info){
    //TODO: implement this
    return true;
}

void mm_clearMemory(ProcessInfo* info){
    //TODO: implement this
}

void clearResources(int i){

    
    if (sc_m_q >= 0){
        msgctl(sc_m_q , IPC_RMID , 0);
    }

    //if (p_m_q >= 0){
    //    msgctl(p_m_q , IPC_RMID , 0);
    //}

    if (controlBlockId >= 0){
        shmdt(ProcessControl);
        shmctl(controlBlockId, IPC_RMID, NULL);
    }

    if (pq)
        free(pq);

    if (finish_queue)
        free(finish_queue);

        
    exit(0);
}




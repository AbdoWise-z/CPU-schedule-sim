#include "headers.h"
#include <ctype.h>
#include <time.h>

#define CLK_INIT clk = getClk()
#define CLK_WAIT(x) while (clk + x > getClk()) {}

FILE* memPtr;
FILE* schPtr;
int sc_m_q; //PG --> SC message queue
//int p_m_q;  //SC <-> process message queu

ControlBlock* ProcessControl;
int controlBlockId;

int clk;
int mem_type;
bool received_all = false;

int sch_start_time = 0;
int sch_finish_time = 0;

//Scheduling types
void hpf(bool);
void srtn();
void rr(int);
void fcfs();

//Memory mapping types
bool mm_firstFitAlloc(ProcessInfo* info);
bool mm_nextFitAlloc(ProcessInfo* info);
bool mm_buddyAlloc(ProcessInfo* info);
bool mm_inf(ProcessInfo* info);

void mm_clearMemory(ProcessInfo* info);

// bool isTaken(int);
// void markTaken(int start , int end);
// void freeMem(int start , int end);

void get_process(ProcessInfo* p);

ProcessInfo current_process;
void run_for(ProcessInfo* p , int qouta);
bool switch_to(ProcessInfo* p);
void drawInteractive();

PriorityQueue* pq;
PriorityQueue* finish_queue;
CircularQueue* waiting_queue;
LinkedList* memMap;
LinkedList* executionGraph;

SchedulerMessage msg;
//ProcessesMessage pMsg;

void clearResources(int);
float calculateSD(float data[] , int);


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

    schPtr = fopen("scheduler.log" , "w");
    if(schPtr == NULL)
        Logger("couldn't find scheduler file\n");
    setbuf(schPtr , NULL);

    memPtr = fopen("memory.log" , "w");
    if(memPtr == NULL)
        Logger("couldn't find memory file\n");
    setbuf(memPtr , NULL);
    

    int t , q;

    t = atoi(argv[0]);
    q = atoi(argv[1]);
    mem_type = atoi(argv[2]);


    Logger("[Scheduler] cfg , t : %d  , q : %d , mem_type: %d\n" , t , q , mem_type);

    sc_m_q = msgget(ftok("PG_SC" , 15) , 0666 | IPC_CREAT);
    //p_m_q  = msgget(ftok("SC_P"  , 15) , 0666 | IPC_CREAT);

    controlBlockId = shmget(ftok("SC_P"  , 15), sizeof(ControlBlock), IPC_CREAT | 0666);
    ProcessControl = (ControlBlock *) shmat(controlBlockId, (void *)0, 0);
    ProcessControl->active_pid = -1;
    ProcessControl->lock1 = 1;
    ProcessControl->ready = 0;
    
    current_process.state = STATE_NOT_READY;
    current_process.id    = -1;

    createPriorityQueue(&pq);
    createPriorityQueue(&finish_queue);
    createCircularQueue(&waiting_queue);
    createLL(&memMap);
    createLL(&executionGraph);

#ifdef INTERACTIVE
    drawInteractive();
#endif

    sch_start_time = getClk();


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
        Logger("[Scheduler] failed to select a type.\n");
        break;
    }

    sch_finish_time = getClk();

    Logger("[Scheduler] Finished \n");
    
    msg.type = 3; //tell pg it can exit
    msgsnd(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , !IPC_NOWAIT);

    Logger("                           [results]\n");
    Logger("#             id     arrival  start_time     runtime finish_time    priority\n");
    PQ_t proc;
    int i = 0;

    int run_time_total = 0;
    float* arr = (float*) malloc(sizeof(float) * finish_queue->size);
    int size = finish_queue->size;
    float tWTA = 0;
    float tW = 0;

    while (finish_queue->size){
        proc = extract(finish_queue);
        
        run_time_total += proc.runtime;
        int wait = proc.start_time - proc.arrival;
        int TA = proc.finish_time - proc.arrival;
        double WTA = TA / (double) proc.runtime;
        tW += wait;
        tWTA += WTA;
        arr[i] = WTA;


        Logger("%04d%12d%12d%12d%12d%12d%12d\n" , i++ , proc.id , proc.arrival , proc.start_time , proc.runtime , proc.finish_time , proc.priority);
    }

    FILE* final = fopen("scheduler.perf" , "w");
    if(final == NULL)
        Logger("couldn't find final file\n");
    //setbuf(final , NULL);

    fprintf(final , "CPU Utilization: %0.2f%%\n" , ((float) run_time_total * 100.0f / (sch_finish_time - sch_start_time)));
    fprintf(final , "Avg Waiting: %0.2f\n" , ((float)tW / size));
    fprintf(final , "Avg WTA: %0.2f\n" , (tWTA / size));
    fprintf(final , "Std WTA: %0.2f\n" , calculateSD(arr , size));

    fclose(final);
    
    destroyClk(true);

#ifdef INTERACTIVE
    printf("Simulation done , took %d clk\n" , sch_finish_time - sch_start_time);
#endif 
}


float calculateSD(float data[] , int s) {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < s; ++i) {
        sum += data[i];
    }
    mean = sum / s;
    for (i = 0; i < s; ++i) {
        SD += pow(data[i] - mean, 2);
    }
    return sqrt(SD / s);
}

bool switch_to(ProcessInfo* p){
    //Logger("switch_to: in\n");
    
    if (current_process.id != p->id){
        if (current_process.id != -1){
            kill(current_process.pid , SIGSTOP);
            Logger("[Scheduler] Pausing: %d \n" , current_process.pid);
            int wait = current_process.start_time - current_process.arrival;
            fprintf(schPtr,"At time %d process %d stopped arr %d total %d remain %d wait %d\n",getClk(), current_process.id, current_process.arrival , current_process.runtime , current_process.remainning , wait);

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

            Logger("[Scheduler] Running a child process for the first time: %d \n" , p->pid);
            int wait = p->start_time - p->arrival;
            fprintf(schPtr,"At time %d process %d started arr %d total %d remain %d wait %d\n",getClk(), p->id, p->arrival , p->runtime , p->remainning , wait);
            p->state = STATE_READY;
        }else if (p->state == STATE_READY){
            Logger("[Scheduler] Resuming: %d\n" , p->pid);
            int wait = p->start_time - p->arrival;
            fprintf(schPtr,"At time %d process %d resumed arr %d total %d remain %d wait %d\n",getClk(), p->id, p->arrival , p->runtime , p->remainning , wait);
            kill(p->pid , SIGCONT);
        }

        //Logger("switch_to: out\n");
        return true;
    }
    //Logger("switch_to: out\n");
    
    return false;
}

void run_for(ProcessInfo* p , int qouta){
    if (p->remainning < 0){ //should be "<=" but the document said no ..
        Logger("[Scheduler] Error , trying to run a process that should have finished: %d\n" , p->pid);
        return;
    }

    switch_to(p);

    //Logger("in: run_for %d\n" , qouta);

    int start = getClk();

    while (qouta){
        qouta--;
        
        ProcessControl->lock1 = 1;           //tell the process that the Scheduler is busy now and can't receive values now
        ProcessControl->active_pid = p->pid; //tell the process it can start its jop
        
        if (p->runtime){
            CLK_WAIT(1);
        
        //int k = msgrcv(p_m_q , &pMsg , sizeof(ProcessesMessage) - sizeof(long) , 0 , !IPC_NOWAIT);
        //if (k < 0){
        //    Logger("[Scheduler] Error in run_for , process didn't return any result\n");
        //    return;
        //}
        
            while (ProcessControl->ready == 0){} //wait for the process to be ready
            ProcessControl->ready = 0;

            ProcessControl->active_pid = -1;     //prevent the process from taking extra quota
            ProcessControl->lock1 = 0;           //tell the process that the scheduler is ready to receive the value
        
            while (ProcessControl->lock1 == 0){} //wait for the process to write
        
        //Logger("[Scheduler] Processes %d ran for 1 quota \n" , p->pid);
            p->remainning = ProcessControl->remainning;
        }

        if (p->remainning == 0){
            p->finish_time = getClk();
            //Logger("[Scheduler] process finished , pid: %d , id: %d\n" , p->pid , p->id);
            int wait = p->start_time - p->arrival;
            int TA = p->finish_time - p->arrival;
            double WTA = TA / (double) p->runtime; 
            fprintf(schPtr,"At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",getClk(), p->id, p->arrival , p->runtime , p->remainning , wait, TA , WTA);
            break;//no need to continue
        }
    }

    int end = getClk();
#ifdef INTERACTIVE
//, (p->start_time == start ? 2 : 0) | (p->finish_time == end ? 1 : 0)

    MemorySlot slot = {start , end , p->id }; //MemorySlot ~= GraphSlot in the the concept of the data it holds
    insertLL(executionGraph , slot);
    drawInteractive();
#endif
    //Logger("out: run_for %d\n" , qouta);

}

//C doesn't have it for some reason xD
int max(int a , int b){
    return a > b ? a : b;
}

void drawInteractive(){
    //heavy function ..
    //Logger("drawInteractive: in\n");
    setbuf(stdout , NULL);
    
    int processes = 0;
    LL_Node* start = executionGraph->start;
    while (start){
        processes = max(processes , start->value.id);
        start = start->next;
    }

    LinkedList** lists = malloc(sizeof(LinkedList*) * processes);
    for (int i = 0;i < processes;i++){
        createLL(&lists[i]);
    }

    start = executionGraph->start;
    while (start){
        insertLL(lists[start->value.id - 1] , start->value);
        start = start->next;
    }

    // LL_Node* test = lists[0]->start;
    // while (test){
    //     printf("%d : %d -> %d\n" , test->value.id , test->value.start , test->value.end);
    //     test = test->next;
    // }
    // printf("------------\n");

    // //for (int i = 0;i < processes)

    system("clear");

    int clk_end = getClk();
    if (clk_end < CONSOLE_WIDTH / 5){
        clk_end = CONSOLE_WIDTH / 5;
    }
    int clk_start = clk_end - CONSOLE_WIDTH / 5;

    printf(" CLK  |");
    for (int i = clk_start;i <= clk_end;i++){
        printf("%04d " , i);
    }
    printf("\n");
    for (int i = 0;i < CONSOLE_WIDTH + 7;i++){
        if ((i > 7 ) && (i - 7) % 5 == 0){
            printf("|");
            continue;
        }
        printf("-");
    }
    printf("\n");

    for (int i = 0;i < processes;i++){
        printf("P%04d |" , i+1);
        LL_Node* node = lists[i]->start;

        for (int j = clk_start;j <= clk_end;j++){
            
            while (node && node->value.end < j){
                node = node->next;
            }
            
            if (node == NULL) break;

            bool quota = false;
            bool enter = false;
            bool exit  = false;

            if (node->value.start < j && node->value.end >= j){
                quota = true;
            }

            if (quota){
                printf("\u2597");
                printf("\u2584");
                printf("\u2584");
                printf("\u2584");
                printf("\u2596");
            }else{
                printf("     ");
            }
        }
        printf("\n");
    }

    for (int i = 0;i < CONSOLE_WIDTH + 7;i++){
        printf("-");
    }

    printf("\n");

    for (int i = 0;i < processes;i++){
        clearLL(&lists[i]);
    }

    free(lists);

    //Logger("drawInteractive: out\n");

}

void get_process(ProcessInfo* p){

    //Logger("get_process: in\n");
    
    
    usleep(CLK_MS); //sleep for 1/1000 of the clk

    while (msgrcv(sc_m_q , &msg , sizeof(SchedulerMessage) - sizeof(long) , 0 , IPC_NOWAIT) > 0){
        enqueue(waiting_queue , msg.p);
        if (msg.type == 1){
            Logger("[Scheduler] No more processes to receive.\n");
            received_all = true; //done receiving ...
        }

        usleep(CLK_MS / 100);
    }

    //Logger("get_process: mem-alloc\n");
    

    p->id = -1;

    int s = waiting_queue->size;
    if (s > 0){
        ProcessInfo temp;
        for (int i = 0;i < s;i++){
            temp = dequeue(waiting_queue);
            if (mem_type == 1){
                if (mm_firstFitAlloc(&temp)){
                    *p = temp;

                    p->pid = -1; //default value
                    p->state = STATE_NOT_READY;
                    p->remainning = p->runtime;
                    //Logger("get_process: out\n");
                    return;
                }
            }else if (mem_type == 2){
                if (mm_nextFitAlloc(&temp)){
                    *p = temp;

                    p->pid = -1; //default value
                    p->state = STATE_NOT_READY;
                    p->remainning = p->runtime;
                    //Logger("get_process: out\n");
                    return;
                }
            }else if (mem_type == 3){
                //Logger("trying to alloc %d for %d t3\n" , temp.memSize , temp.id);
                if (mm_buddyAlloc(&temp)){
                    *p = temp;

                    p->pid = -1; //default value
                    p->state = STATE_NOT_READY;
                    p->remainning = p->runtime;
                    //Logger("get_process: out\n");
                    return;
                }
            } else {
                mm_inf(&temp);
                *p = temp;
                p->pid = -1; //default value
                p->state = STATE_NOT_READY;
                p->remainning = p->runtime;
                //Logger("get_process: out\n");
                return;
            }
            enqueue(waiting_queue , temp);
        }
    }

    //Logger("get_process: out\n");
    
}

void hpf(bool preemptive){
    Logger("[Scheduler] Running HPF\n");
    ProcessInfo recv;
    
    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        CLK_INIT;
        
        recv.id = INT_MAX;
        while (recv.id > 0){
            get_process(&recv);

            if (recv.id > 0){
                Logger("[Scheduler] Adding a processes to the queue: %d , Priority: %d\n" , recv.id , recv.priority);
                insert(pq , -recv.priority , recv);
                // if (recv.remainning == 0){
                //     Logger("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                //     recv.start_time = getClk();
                //     recv.finish_time = recv.start_time;
                //     insert(finish_queue , -getClk() , recv);
                // }else{
                //     insert(pq , -recv.priority , recv);
                // }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //Logger("[Scheduler] selecting next , id: %d , pid: %d , pri: %d\n" , next.id , next.pid , next.priority);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0)
                insert(pq , preemptive ? -next.priority : INT_MAX , next);
            else{
                //Logger("[Scheduler] process finished\n");
                mm_clearMemory(&next);
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }
        } else {
            CLK_WAIT(1);
            Logger("[Scheduler] waiting... %d %d %d\n" , received_all , waiting_queue->size , pq->size);
        }
    }
    Logger("[Scheduler] Finished HPF\n");
}

void srtn(){
    Logger("[Scheduler] Running SRTN\n");
    ProcessInfo recv;

    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        CLK_INIT;

        recv.id = INT_MAX;
        while (recv.id > 0){
            get_process(&recv);

            if (recv.id > 0){
                Logger("[Scheduler] Adding a processes to the queue: %d , RT: %d\n" , recv.id , recv.remainning);
                insert(pq , -recv.remainning , recv);
                // if (recv.remainning == 0){
                //     Logger("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                //     recv.start_time = getClk();
                //     recv.finish_time = recv.start_time;
                //     insert(finish_queue , -getClk() , recv);
                // }else{
                //     insert(pq , -recv.remainning , recv);
                // }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //Logger("[Scheduler] selecting next , id: %d , pid: %d , RT: %d\n" , next.id , next.pid , next.remainning);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0)
                insert(pq , -next.remainning , next);
            else{
                //Logger("[Scheduler] process finished\n");
                mm_clearMemory(&next);
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }
        } else {
            CLK_WAIT(1);
            Logger("[Scheduler] waiting... %d %d %d\n" , received_all , waiting_queue->size , pq->size);
        }
    }
    Logger("[Scheduler] Finished SRTN\n");
}

int rq;
void rr(int q){
    Logger("[Scheduler] Running RR\n");
    ProcessInfo recv;
    rq = 0;
    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        CLK_INIT;
        //Logger("rr-loop-start\n");

        recv.id = INT_MAX;
        while (recv.id > 0){
            get_process(&recv);

            if (recv.id > 0){
                Logger("[Scheduler] Adding a processes to the queue: %d , RT: %d\n" , recv.id , recv.remainning);
                insert(pq , -getClk() , recv);
                
                // if (recv.remainning == 0){
                //     Logger("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                //     recv.start_time = getClk();
                //     recv.finish_time = recv.start_time;
                //     insert(finish_queue , -getClk() , recv);
                // }else{
                //     insert(pq , -getClk() , recv);
                // }
            }
        }

        if (pq->size > 0){ //we have something to run
            //Logger("rr-pq-get\n");
            if (rq == 0)
                rq = q;
            
            ProcessInfo next = extract(pq);
            //Logger("[Scheduler] selecting next , id: %d , pid: %d , RT: %d\n" , next.id , next.pid , next.remainning);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0){
                //Logger("[Scheduler] re-inserting\n");
                insert(pq , (--rq) > 0 ? rq : -getClk() , next);
            }else{
                //Logger("[Scheduler] process finished\n");
                rq = 0;
                mm_clearMemory(&next);
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }

            //Logger("[Scheduler] jop done for %d\n" , next.id);

        } else {
            CLK_WAIT(1);
            Logger("[Scheduler] waiting... %d %d %d\n" , received_all , waiting_queue->size , pq->size);
        }
    }
    Logger("[Scheduler] Finished RR\n");
}

void fcfs(){
    Logger("[Scheduler] Running FCFS\n");
    ProcessInfo recv;

    while (!received_all || waiting_queue->size > 0 || pq->size > 0){
        CLK_INIT;

        recv.id = INT_MAX;
        while (recv.id > 0){
            get_process(&recv);

            if (recv.id > 0){
                Logger("[Scheduler] Adding a processes to the queue: %d , RT: %d\n" , recv.id , recv.remainning);
                insert(pq , -recv.arrival , recv);
                
                // if (recv.remainning == 0){
                //     Logger("[Scheduler] Input Error , process %d has no runtime , ignoring\n" , recv.id);
                //     recv.start_time = getClk();
                //     recv.finish_time = recv.start_time;
                //     insert(finish_queue , -getClk() , recv);
                // }else{
                //     insert(pq , -recv.arrival , recv);
                // }
            }
        }

        if (pq->size > 0){ //we have something to run
            ProcessInfo next = extract(pq);
            //Logger("[Scheduler] selecting next , id: %d , pid: %d , RT: %d\n" , next.id , next.pid , next.remainning);
            run_for(&next , 1);
            current_process = next;
            if (next.remainning > 0)
                insert(pq , 1 , next);
            else{
                //Logger("[Scheduler] process finished\n");
                mm_clearMemory(&next);
                insert(finish_queue , -getClk() , next);
                current_process.id = -1; //no current , no need to stop the next one
            }
        } else {
            CLK_WAIT(1);
            Logger("[Scheduler] waiting... %d %d %d\n" , received_all , waiting_queue->size , pq->size);
        }
    }
    Logger("[Scheduler] Finished FCFS\n");
}


bool FF_insert(LL_L_t* ll, ProcessInfo *new_process)
{
    MemorySlot temp ;
    if(ll->start == NULL)
    {
        //i'm here so head is null insert at position 1
        // allocate memory slot :
        temp.id=new_process->id; // get id of process
        temp.start=0;
        temp.end=new_process->memSize-1;
        // insert node: 
        insertLL(ll,temp);
        new_process->mem_start=0;
        new_process->mem_end=new_process->memSize-1; //             set the ranges in the process info : needed for output in the future
        return 1; //                                                successful insertion
    }
    else
    {
        // there was a start node -> check for holes or reach the end
         if(ll->start->value.start != 0)//                         is the first position in memory vacant ?
    {
        // I'm here so -> first position in memory is vacant
        // check if this place is valid 
        if(ll->start->value.start >= (new_process->memSize) )
        {
        // this place is valid
        temp.id=new_process->id;
        temp.start=0;
        temp.end=new_process->memSize-1;
        insertAfterLL(ll,NULL,temp);
        Logger("I'm here");
        new_process->mem_start=0;
        new_process->mem_end=temp.end;
        return 1; //                                                successeful insertion
        }
    }
    // then let's iterate on linked list to find suitable place "it may be a hole"
    LL_Node* iterator= ll->start;
    while(iterator->next!= NULL)
    // 1 2 3 4 5 - > null
    // 1       
    {
        // check for hole
        if(iterator->value.end+1 != ((LL_Node*)iterator->next)->value.start )
        {
            // i'm a hole get my size and check whether it's available to be done
            // 20 ->40 || 60 ->80
            // hole = 60 -40 = 20 
            // 41 to 59 = 19 
            if(((LL_Node*)iterator->next)->value.start - iterator->value.end >= new_process->memSize)
            {
                // valid position
              temp.id=new_process->id;
              temp.start= iterator->value.end+1; // the process introduced in the hole starts after the end +1 
              temp.end=new_process->memSize+temp.start-1;
               insertAfterLL(ll,iterator,temp);
                new_process->mem_start=temp.start; 
                new_process->mem_end=temp.end; //                                 set process paramaters helps in the output
                return 1; //                                                             successfull insertion
            }
        }
             // no hole yet gi'm at the end
             iterator=iterator->next; 
    }
    //i'm here , i wasn't successfull in insertion yet -> check if the tail is valid to insert after
    if(1023 - iterator->value.end >=  new_process->memSize-1) //1024 -980 =  44  = 981 to 1024
    {
        // valid to enter 
        Logger("BYE\n");
        temp.start=iterator->value.end+1;
        temp.end=temp.start+new_process->memSize-1;
        temp.id= new_process->id;
        insertAfterLL(ll,iterator,temp);
        new_process->mem_start=temp.start;
        new_process->mem_end= temp.end;
        return 1;
    } 
    return 0; // i wasn't successfull after all let the scheduler block me
    }
}

bool mm_firstFitAlloc(ProcessInfo* info){
    if(FF_insert(memMap,info))
    {
        fprintf(memPtr,"At time %d allocated %d bytes for process %d from %d to %d\n", getClk(),
        info->mem_end - info->mem_start  , info->id , info->mem_start , info->mem_end);
        Logger("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(),
        info->mem_end - info->mem_start  , info->id , info->mem_start , info->mem_end);
        return 1;
    }
    return 0;
}

bool mm_nextFitAlloc(ProcessInfo* info){
    LL_Node* end = memMap->end;
    int pos = -1;
    if (memMap->end){
        if (1023 - memMap->end->value.end >= info->memSize){
            pos = memMap->end->value.end + 1;
        }else{
            end = memMap->start;
            if (end->value.start >= info->memSize){
                pos = 0;
                end = NULL;
            } else {
                while (end && end->next){
                    if (((LL_Node*) end->next)->value.start - end->value.end - 1 >= info->memSize){
                        pos = end->value.end + 1;
                        break;
                    }
                    end = end->next;
                }
            }
        }
    }else{
        pos = 0;
    }

    if (pos == -1)
        return false;

    //Logger("pos = %d\n"  , pos);

    MemorySlot slot = {pos , pos + info->memSize - 1 , info->id};
    info->mem_start = pos;
    info->mem_end = pos + info->memSize - 1;
    insertAfterLL(memMap , end , slot);

    if (end != NULL){
        LL_Node* self = end->next;
        if (self->next){
            LL_Node* ll_end = memMap->end;
            LL_Node* ll_start = memMap->start;
            
            //link end with start
            ll_end->next = ll_start;
            ll_start->prev = ll_end;

            //set the new start
            memMap->start = self->next;

            //unlink with next
            ((LL_Node*) self->next)->prev = NULL;
            self->next = NULL;

            //set self as the new end of the LL
            memMap->end = self;
        }
    }

    fprintf(memPtr, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), info->memSize , info->id , info->mem_start , info->mem_end);
    Logger("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), info->memSize , info->id , info->mem_start , info->mem_end);
        
    return true;
}

bool mm_buddyAlloc(ProcessInfo* info){
    int level = (int)ceil(log2(info->memSize));
    int blockSize = (int)pow(2 , level);
    if (blockSize == 0) blockSize = 1; //idk how would this happen , but well ..

    MemorySlot m;
   
    for (int i = 0; i < 1024; i += blockSize){
        
        LL_Node* temp = memMap->start;
        bool free = true;
        int blockStart = i;
        int blockEnd = i + blockSize - 1;

        while(temp != NULL){
            if ((temp->value.start >= blockStart && temp->value.end <= blockEnd ) || ( blockStart >= temp->value.start && blockEnd <= temp->value.end)){
                free = false;
                break;
            }
            temp = temp->next;
        }

        if(!free)
            continue;

        m.start = blockStart;
        m.end = blockEnd;
        m.id = info->id;
        insertLL(memMap , m);
        
        info->mem_start = i;
        info->mem_end = i + blockSize - 1;
        fprintf(memPtr, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), blockSize , info->id , info->mem_start , info->mem_end);
        Logger("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), blockSize , info->id , info->mem_start , info->mem_end);
        return true;
    }

    return false;
}

int inf_mem_pos = 0;
bool mm_inf(ProcessInfo* info){
    info->mem_start = inf_mem_pos;
    inf_mem_pos += info->memSize;
    info->mem_end = inf_mem_pos - 1;
    Logger("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), info->memSize , info->id , info->mem_start , info->mem_end);
    return true;
}

void mm_clearMemory(ProcessInfo* info){
    //Logger("mm_clearMemory: in\n");
    LL_Node* temp = memMap->start;

    while(temp){
        if(info->id == temp->value.id)
            break;
        temp = temp->next;
    }

    if (temp)
        removeLL(memMap , temp);

    int blockSize = info->mem_end - info->mem_start + 1;
    fprintf(memPtr,"At time %d freed %d bytes for process %d from %d to %d\n", getClk(), blockSize , info->id , info->mem_start, info->mem_end);
    Logger("At time %d freed %d bytes for process %d from %d to %d\n", getClk(), blockSize , info->id , info->mem_start, info->mem_end);
    //Logger("mm_clearMemory: out\n");
    //info->mem_start = -1;
    //info->mem_end = -1;
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

    if (schPtr)
        fclose(schPtr);
    
    if (memPtr)
        fclose(memPtr);
        
    exit(0);
}




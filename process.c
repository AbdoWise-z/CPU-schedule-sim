#include "headers.h"

#define CLK_INIT clk = getClk()
#define CLK_WAIT(x) while (clk + x > getClk()) {}

int remainingtime;
//int p_m_q;
bool runninning = true;
int clk;

ControlBlock* ProcessControl;
int controlBlockId;

void stop(int i){
    signal(SIGSTOP , stop);
}

void resume(int i){
    CLK_INIT;
    signal(SIGCONT , resume);
}

void finish(int i){
    exit(-1);
}

int main(int agrc, char * argv[])
{
    
#ifdef OUT_TO_FILE
    SYNC_IO;
#endif

    signal(SIGSTOP , stop);
    signal(SIGCONT , resume);
    signal(SIGINT , finish);
    
    initClk();
    
    //p_m_q  = msgget(ftok("SC_P"  , 15) , 0666 | IPC_CREAT);
    controlBlockId = shmget(ftok("SC_P"  , 15), sizeof(ControlBlock), IPC_CREAT | 0666);
    ProcessControl = (ControlBlock *) shmat(controlBlockId, (void *)0, 0);
    
    remainingtime = atoi(argv[0]);

    printf("[Process] started , runtime: %d \n" , remainingtime);
    
    while (remainingtime > 0){
        
        while (ProcessControl->active_pid != getpid()){}

        CLK_INIT;
        //printf("[Process] clk=%d" , clk);
        CLK_WAIT(1);
        remainingtime--;
        printf("[Process] running | pid: %d , remainingtime: %d \n" , getpid() , remainingtime);

        //ProcessesMessage pmsg;
        //pmsg.type = 1;
        //pmsg.remainning = remainingtime;
        //msgsnd(p_m_q , &pmsg , sizeof(ProcessesMessage) - sizeof(long) , !IPC_NOWAIT);

        ProcessControl->ready = 1; //tell the scheduler that the process is ready
        
        while (ProcessControl->lock1){} //wait for the scheduler to be ready

        ProcessControl->remainning = remainingtime; //store the value and notify the scheduler
        ProcessControl->lock1 = 1;

    }

    printf("[Process] finished  , pid: %d \n" , getpid());
    

    return 0;
}

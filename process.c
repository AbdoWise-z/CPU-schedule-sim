#include "headers.h"

#define CLK_INIT clk = getClk()
#define CLK_WAIT(x) while (clk + x > getClk()) {}

int remainingtime;
int p_m_q;
bool runninning = true;
int clk;

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
    signal(SIGSTOP , stop);
    signal(SIGCONT , resume);
    signal(SIGINT , finish);
    
    initClk();
    
    p_m_q  = msgget(ftok("SC_P"  , 15) , 0666 | IPC_CREAT);
    remainingtime = atoi(argv[0]);

    printf("[Process] started , runtime: %d \n" , remainingtime);
    
    while (remainingtime > 0){
        CLK_INIT;
        //printf("[Process] clk=%d" , clk);
        CLK_WAIT(1);
        remainingtime--;
        printf("[Process] running | pid: %d , remainingtime: %d \n" , getpid() , remainingtime);

        ProcessesMessage pmsg;
        pmsg.type = 1;
        pmsg.remainning = remainingtime;
        msgsnd(p_m_q , &pmsg , sizeof(ProcessesMessage) - sizeof(long) , !IPC_NOWAIT);
    }

    printf("[Process] finished  , pid: %d \n" , getpid());
    

    return 0;
}

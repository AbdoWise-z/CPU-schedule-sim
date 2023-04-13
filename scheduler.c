#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.

    int sc_m_q = msgget(getpid() , 666 | IPC_CREAT);


    while(1){}
    
    destroyClk(true);
}

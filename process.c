#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        //maybe we should do it in a more fancy way tho ...
        int c = getClk();
        while (getClk() + 1 != c)
            remainingtime--;
        
    }
    
    destroyClk(false);
    
    return 0;
}

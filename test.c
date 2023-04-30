#include "headers.h"
#include <string.h>
#include <ctype.h>
#include <time.h>

int main(){
    printf("test started\n");
    MemoryMap* map;
    createLL(&map);

    MemroySlot slot = {1 ,4};
    insertLL(map , slot);
    
    slot.start = 2;
    slot.end = 2;
    insertLL(map , slot);
    
    slot.start = 3;
    slot.end = 4;
    insertLL(map , slot);
    
    slot.start = 4;
    slot.end = 2;
    insertLL(map , slot);
    
    slot.start = 5;
    slot.end = 4;
    insertLL(map , slot);
    
    LL_Node* s = map->start;
    while (s){
        printf("%d %d" , s->value.start , s->value.end);
        if (s->value.end == 4){
            LL_Node* n = s->next;
            removeLL(map , s);
            printf(" removed\n");
            s = n;
            continue;
        }
        printf("\n");
        s = s->next;
    }

    printf("------\n");

    s = map->start;

    while (s){
        printf("%d %d\n" , s->value.start , s->value.end);
        s = s->next;
    }



    printf("test ended\n");
    return 0;
}
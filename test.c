#include "headers.h"
#include <string.h>
#include <ctype.h>
#include <time.h>

int main(){
    printf("test started\n");
    LinkedList* map;
    createLL(&map);

    MemorySlot slot = {1 ,4};
    for (int i = 0;i < 10;i++){
        slot.start = i;
        slot.end = 5 * i;
        insertLL(map , slot);
    }

    LL_Node* s = map->start;
    slot.start = -1;
    slot.end = -1;

    while (s){
        printf("%d %d" , s->value.start , s->value.end);
        printf("\n");
        insertAfterLL(map , s , slot);
        s = ((LL_Node*)s->next)->next;
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
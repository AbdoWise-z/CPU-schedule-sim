#ifdef LL_L_t
#ifdef LL_t


typedef struct LL_Node{
    LL_t value;
    void* next;
    void* prev;
} LL_Node;

typedef struct {
    int size;
    LL_Node* start;
    LL_Node* end;
} LL_L_t;

void createLL(LL_L_t** ll){
    *ll = (LL_L_t*) malloc( sizeof(LL_L_t) );
    (*ll)->size = 0;
}

void insertLL(LL_L_t* ll , LL_t val){
    if (ll->size == 0){
        LL_Node* node = (LL_Node*) malloc( sizeof(LL_Node) );
        node->next = NULL;
        node->value = val;
        ll->start = node;
        ll->end = node;
        ll->size++;
        return;
    }

    //setup end and the next node values
    ll->end->next = malloc( sizeof(LL_Node) );
    ((LL_Node*) ll->end->next)->next  = NULL;
    ((LL_Node*) ll->end->next)->value = val;
    ((LL_Node*) ll->end->next)->prev = ll->end;

    ll->end = ll->end->next;
    ll->size++;
}


void insertAfterLL(LL_L_t* ll , LL_Node* t , LL_t val){
    if (t == NULL){ //if we want to insert at the start
        LL_Node* insert = (LL_Node*) malloc( sizeof(LL_Node) );
        insert->value = val;

        if (ll->start){
            ll->start->prev = insert;
            // if (ll->size == 1)
            //     ll->end = ll->start;
        } else {
            ll->end = insert; //this is the first item to insert
        }

        insert->next = ll->start;
        ll->start = insert;

    }else{
        LL_Node* insert = (LL_Node*) malloc( sizeof(LL_Node) );
        insert->value = val;

        insert->next = t->next;
        insert->prev = t;
        if (t->next)
            ((LL_Node*) t->next)->prev = insert;
        t->next = insert;

        if (t == ll->end){
            ll->end = insert;
        }
    }

    ll->size++;
}

void removeLL(LL_L_t* ll , LL_Node* n){
    if (ll->size < 1){
        printf("LL is empty !!\n");
        return;
    }

    if (ll->size == 1){
        ll->start = NULL;
        ll->end = NULL;
    }else{
        if (n->prev && n->next) {
            ((LL_Node*) n->prev)->next = n->next;
            ((LL_Node*) n->next)->prev = n->prev;
        } else if (n == ll->start){
            ll->start = n->next;
            ll->start->prev = NULL;
        } else if (n == ll->end){
            ll->end = n->prev;
            ll->end->next = NULL;
        }
    }

    ll->size--;

    if (ll->size <= 1){
        ll->end = ll->start;
    }
    
    free(n);
}


#endif
#endif
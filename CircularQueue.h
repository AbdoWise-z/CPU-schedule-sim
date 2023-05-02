//Implementation of a CircularQueue using array
//


#ifdef CQ_t
#ifdef CQ_Q_t
#ifdef CQ_MAX_SIZE

typedef struct CQ_Q_t{
    CQ_t *arr;
    int front;
    int rear;
    int size;
} CQ_Q_t;


//creates a queue at the given pointer
void createCircularQueue(CQ_Q_t** q){
    CQ_Q_t* c = (CQ_Q_t*) malloc(sizeof(CQ_Q_t));
    c->arr = (CQ_t*) malloc(sizeof(CQ_t) * CQ_MAX_SIZE);
    c->front = -1;
    c->rear = -1;
    c->size = 0;

    *q = c;
}


//returns true if queue is full
int isCQFull(CQ_Q_t *queue) {
    return ((queue->front == 0 && queue->rear == CQ_MAX_SIZE - 1) || 
            (queue->rear == (queue->front - 1) % (CQ_MAX_SIZE - 1)));
}

//returns true if queue is empty
int isCQEmpty(CQ_Q_t *queue) {
    return (queue->front == -1);
}


//inserts item to the queue
void enqueue(CQ_Q_t *queue, CQ_t data) {
    if (isCQFull(queue)) {
        Logger("CQueue is Full!\n");
        return;
    }
    
    if (queue->front == -1) {
        queue->front = 0;
    }

    queue->rear = (queue->rear + 1) % CQ_MAX_SIZE;
    queue->arr[queue->rear] = data;
    queue->size++;
}

//returns the first item in the queue
//and removes it from the queue
CQ_t dequeue(CQ_Q_t *queue) {
    if (isCQEmpty(queue)) {
        Logger("Queue is Empty!\n");
        return queue->arr[0];
    }
    CQ_t data = queue->arr[queue->front];
    if (queue->front == queue->rear) {
        queue->front = -1;
        queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % CQ_MAX_SIZE;
    }
    queue->size--;
    return data;
}

#endif
#endif
#endif
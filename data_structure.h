#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_SIZE 100

#define STATE_NOT_READY   0
#define STATE_READY       1

typedef struct {
    int id , arrival , runtime , priority; //input values
    int pid , state , finish_time , start_time , remainning; //runtime values
} ProcessInfo;

typedef struct {
    long type;
    ProcessInfo p;
} SchedulerMessage;


typedef struct {
    long type;
    int remainning;
} ProcessesMessage;

#define PQ_t ProcessInfo
#define CQ_t ProcessInfo

typedef struct {
    int priority[MAX_SIZE];
    PQ_t data[MAX_SIZE];

    int size;
} PriorityQueue;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void swap_pqt(PQ_t *a, PQ_t *b) {
    PQ_t temp = *a;
    *a = *b;
    *b = temp;
}

void maxHeapify(PriorityQueue *pq, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < pq->size && pq->priority[left] > pq->priority[largest]) {
        largest = left;
    }

    if (right < pq->size && pq->priority[right] > pq->priority[largest]) {
        largest = right;
    }

    if (largest != i) {
        swap(&pq->priority[i], &pq->priority[largest]);
        swap_pqt(&pq->data[i], &pq->data[largest]);
        maxHeapify(pq, largest);
    }
}

PriorityQueue *createPriorityQueue() {
    PriorityQueue *pq = (PriorityQueue *) malloc(sizeof(PriorityQueue));
    pq->size = 0;
    return pq;
}

void insert(PriorityQueue *pq, int priority , PQ_t item) {
    if (pq->size >= MAX_SIZE) {
        printf("Priority Queue is full\n");
        return;
    }

    pq->priority[pq->size] = priority;
    pq->data[pq->size++]   = item;

    int i = pq->size - 1;
    while (i > 0 && pq->priority[(i - 1) / 2] < pq->priority[i]) {
        swap_pqt(&pq->data[(i - 1) / 2], &pq->data[i]);
        swap(&pq->priority[(i - 1) / 2], &pq->priority[i]);
        i = (i - 1) / 2;
    }
}

PQ_t extractMax(PriorityQueue *pq) {
    if (pq->size <= 0) {
        printf("Priority Queue is empty\n");
        return pq->data[0];
    }

    int max = pq->priority[0];
    PQ_t m_data = pq->data[0];
    pq->data[0] = pq->data[--pq->size];
    pq->priority[0] = pq->priority[  pq->size];
    maxHeapify(pq, 0);

    return m_data;
}

typedef struct {
    CQ_t *arr;
    int front;
    int rear;
    int size;
} CircularQueue;

CircularQueue* createCircularQueue(){
    CircularQueue* c = (CircularQueue*) malloc(sizeof(CircularQueue));
    c->arr = (CQ_t*) malloc(sizeof(CQ_t) * MAX_SIZE);
    c->front = -1;
    c->rear = -1;
    c->size = 0;
    return c;
}

int isCQFull(CircularQueue *queue) {
    return ((queue->front == 0 && queue->rear == MAX_SIZE - 1) || 
            (queue->rear == (queue->front - 1) % (MAX_SIZE - 1)));
}

int isCQEmpty(CircularQueue *queue) {
    return (queue->front == -1);
}

void enqueue(CircularQueue *queue, CQ_t data) {
    if (isCQFull(queue)) {
        printf("CQueue is Full!\n");
        return;
    }
    if (queue->front == -1) {
        queue->front = 0;
    }
    queue->rear = (queue->rear + 1) % MAX_SIZE;
    queue->arr[queue->rear] = data;
    queue->size++;
}

CQ_t dequeue(CircularQueue *queue) {
    if (isCQEmpty(queue)) {
        printf("Queue is Empty!\n");
        return queue->arr[0];
    }
    CQ_t data = queue->arr[queue->front];
    if (queue->front == queue->rear) {
        queue->front = -1;
        queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % MAX_SIZE;
    }
    queue->size--;
    return data;
}

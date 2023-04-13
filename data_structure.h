#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_SIZE 100

typedef struct ProcessInfo{
    int id , arrival , runtime , priority;
} ProcessInfo;

#define PQ_t ProcessInfo

typedef struct PriorityQueue {
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

int main() {
    PriorityQueue *pq = createPriorityQueue();
    ProcessInfo info = {1,2,3,4};
    insert(pq, 3 , info);
    info.id = 2;
    insert(pq, 18 , info);
    info.id = 3;
    insert(pq, 45 , info);
    info.id = 4;
    insert(pq, 11 , info);

    // insert(pq, 2 ,);
    // insert(pq, 15);
    // insert(pq, 5);
    // insert(pq, 4);
    // insert(pq, 45);

    printf("Max value: %d\n", extractMax(pq).id);
    printf("Max value: %d\n", extractMax(pq).id);
    printf("Max value: %d\n", extractMax(pq).id);

    return 0;
}

#ifdef PQ_MAX_SIZE
#ifdef PQ_Q_t
#ifdef PQ_t

typedef struct PQ_Q_t{
    int priority[PQ_MAX_SIZE];
    PQ_t data[PQ_MAX_SIZE];

    int size;
} PQ_Q_t;

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

void maxHeapify(PQ_Q_t *pq, int i) {
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

void createPriorityQueue(PQ_Q_t** pq) {
    *pq = (PQ_Q_t *) malloc(sizeof(PQ_Q_t));
    (*pq)->size = 0;
}

void insert(PQ_Q_t *pq, int priority , PQ_t item) {
    if (pq->size >= PQ_MAX_SIZE) {
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

PQ_t extract(PQ_Q_t *pq) {
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

#endif
#endif
#endif
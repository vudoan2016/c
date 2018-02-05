#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_MAX 10

typedef struct queue_s {
  int head;
  int tail;
  int q[QUEUE_MAX];
} queue_t;

static inline void queue_init(queue_t *q)
{
  q->head = 0;
  q->tail = 0;
  memset(q->q, 0, sizeof(int)*QUEUE_MAX);

}
static inline bool queue_is_full(queue_t *q)
{
  return ((q->tail + 1) % QUEUE_MAX == q->head);
}

static inline bool queue_is_empty(queue_t *q)
{
  return (q->head == q->tail);
}

static inline bool enqueue(queue_t *q, int elem)
{
  bool status = false;
  
  if (!queue_is_full(q)) {
    q->q[q->tail] = elem;
    q->tail = (q->tail + 1) % QUEUE_MAX;
    status = true;
  }
  return status;
}

static int dequeue(queue_t *q)
{
  q->head = (q->head + 1) % QUEUE_MAX;
  return q->q[q->head];
}

#endif

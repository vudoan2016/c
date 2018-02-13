#ifndef __STACK_H__
#define __STACK_H__

#define STACK_SIZE_MAX 10

typedef struct stack_s {
  int head;
  int data[STACK_SIZE_MAX];
} stack_t;

static inline void stack_init(stack_t *st)
{
  st->head = 0;
  memset(st->data, 0, sizeof(int)*STACK_SIZE_MAX);
}

static inline bool stack_is_full(stack_t *st)
{
  return (st->head == STACK_SIZE_MAX-1);
}

static inline bool stack_is_empty(stack_t *st)
{
  return (st->head == 0);
}

static inline void stack_push(stack_t *st, int data)
{
  st->data[++st->head] = data;
}

static inline int stack_pop(stack_t *st)
{
  int data = st->data[st->head];
  st->head--;
  return data;
}

#endif

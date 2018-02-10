#ifndef __HEAP_H
#define __HEAP_H

#define HEAP_MAX 100

typedef struct heap_data_s {
  int key;
  int element;
} heap_data_t;

typedef struct heap_s {
  int last;
  heap_data_t *data;
} heap_t;

void heap_init(heap_t *h);
bool heap_is_empty(heap_t *h);
bool heap_insert(heap_t *h, heap_data_t *elem);
heap_data_t *heap_delete(heap_t *h);
void heap_destroy(heap_t *h);

#endif

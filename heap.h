#ifndef __HEAP_H
#define __HEAP_H

#define HEAP_MAX 100

typedef struct heap_node_s {
  int key;
  int data;
} heap_node_t;

typedef struct heap_s {
  int last;
  heap_node_t *node;
} heap_t;

void heap_init(heap_t *h);
bool heap_is_empty(heap_t *h);
bool heap_insert(heap_t *h, heap_node_t *elem);
heap_node_t *heap_delete(heap_t *h);
void heap_destroy(heap_t *h);

#endif

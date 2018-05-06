#ifndef __LIST_H__
#define __LIST_H__

typedef struct node_s {
  int data;
  struct node_s *next;
} node_t;

typedef struct list_s {
  node_t *head;
  node_t *tail;
} list_t;

#endif

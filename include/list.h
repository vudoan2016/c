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

void list_init(list_t **l);
void list_insert(list_t *l, int data);
void reverse(node_t **head);
void print_list(node_t *head);
void remove_duplicate(node_t *head);
bool merged(node_t *head1, node_t *head2);
void list_merge(node_t *head1, node_t *head2, int position);
node_t* list_remove(node_t *head, int x);
void print_reverse(node_t *head);
void *remove_nth(node_t **h, int nth);
void remove_nth_from_tail(node_t **h, int nth);

#endif

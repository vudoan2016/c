#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "include/list.h"
#include "include/debug.h"

#define DEBUG 1

void list_init(list_t **l)
{
  *l = (list_t *)calloc(1, sizeof(list_t));
}

/* append to list */
void list_insert(list_t *l, int data)
{
  node_t *p = (node_t *)calloc(1, sizeof(int));

  if (p) {
    p->data = data;
    p->next = NULL;
    if (l->head == NULL) 
      l->head = p;
    if (l->tail == NULL)
      l->tail = p;
    else {
      l->tail->next = p;
      l->tail = p;
    }
  }
}

void reverse(node_t **head)
{
  node_t *cur = *head, *prev = NULL, *tmp;

  while (cur) {
    tmp = cur->next;
    cur->next = prev;
    prev = cur;
    cur = tmp;
  }
  *head = prev;
}

void print_list(node_t *head)
{
  node_t *cur = head;
  
  while (cur) {
    printf("%d", cur->data);
    cur = cur->next;
    if (cur)
      printf(", ");
  }

  printf("\n");
}

void remove_duplicate(node_t *head)
{
  unsigned int *ba = (unsigned int *)calloc(1, 0x7ffffff * sizeof(unsigned int));
  
  node_t *cur = head, *prev = NULL, *tmp;
  while (cur) {
    if (ba[(unsigned int)(cur->data)>>5] & (1 << (unsigned int)(cur->data)%32)) {
      tmp = cur;
      cur = cur->next;
      if (prev) {
	prev->next = cur;
      }
      /* delete the node that head is pointing to so update head */
      if (head == tmp) {
	head = cur;
      }
      free(tmp);
    } else {
      ba[(unsigned int)(cur->data)>>5] |= 1 << (unsigned int)(cur->data)%32;
      prev = cur;
      cur = cur->next;
    }
  }
}

bool merged(node_t *head1, node_t *head2)
{
  node_t *cur1 = head1, *cur2 = head2;

  return (!(head1 == NULL || head2 == NULL));
  
  while (cur1 && cur1->next) {
    cur1 = cur1->next;
  }

  while (cur2 && cur2 != cur1) {
    cur2 = cur2->next;
  }
  return (cur2 != NULL);
}

/* Merge 2 singly linked lists at position 'position' from the head. */
void list_merge(node_t *head1, node_t *head2, int position)
{
  node_t *cur1 = head1, *cur2 = head2;
  int count = 0;
  
  while (cur1 && count < position) {
    cur1 = cur1->next;
    count++;
  }

  while (cur2 && cur2->next) {
    cur2 = cur2->next;
  }

  if (cur2) {
    cur2->next = cur1;
  }
}

node_t* list_remove(node_t *head, int x)
{
  node_t *prev = NULL, *cur = head, *tmp;
  while (cur) {
    if (cur->data == x) {
      tmp = cur;
      cur = cur->next;
      if (prev) {
	prev->next = cur;
      }
      /* delete the node that head is pointing to so update head */
      if (head == tmp)
	head = cur;
      free(tmp);
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  return head;
}

void print_reverse(node_t *head)
{
  if (head == NULL) {
    return;
  }

  print_reverse(head->next);
  printf("%d, ", head->data);
}

/* remove the nth node from head */
void *remove_nth(node_t **h, int nth)
{
  node_t *n = *h, *p = NULL;
  int i = 1;

  while (n && i < nth) {
    p = n;
    n = n->next;
    i++;
  }

  if (n && i == nth) {
    printf("removing node with data %d\n", n->data);
    if (p) {
      p->next = n->next;
    } else {
      *h = n->next;
    }
    free(n);
  }
}

void remove_nth_from_tail(node_t **h, int nth)
{
  node_t *c = *h, *n = *h, *p = NULL;
  int i = 0;
  if (!c)
    return;
  while (c) {
    if (i == nth-1) {
      DBG("remove node %d from tail", nth);
      if (c->next == NULL) {
	if (p) {
	  p->next = n->next;
	} else {
	  *h = n->next;
	}
	free(n);
	break;
      } else {
	p = n;
	n = n->next;
      }
    } else {
      i++;
    }
    c = c->next;
  }
}


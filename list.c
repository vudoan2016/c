#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"

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

void list_test()
{
  char data_file[] = "list_data.txt";
  FILE *fp;
  char input[512], *p, *token;
  int data, position = 5, x = 200;
  list_t *l = NULL, *l1 = NULL;
  
  list_init(&l);
  list_init(&l1);
  
  if ((fp = fopen(data_file, "r")) == NULL) {
    return;
  }

  if (fgets(input, 512, fp) != NULL) {
    p = input;
    while (token = strtok_r(p, " ", &p)) {
      list_insert(l, atoi(token));
    }
  }

  if (fgets(input, 512, fp) != NULL) {
    p = input;
    while (token = strtok_r(p, " ", &p)) {
      list_insert(l1, atoi(token));
    }
  }

  printf("1. Remove duplicates\n");
  print_list(l->head);
  remove_duplicate(l->head);
  print_list(l->head);
  printf("\n\n");
  
  printf("2. Print in reversed order\n");
  print_reverse(l->head);
  printf("\n\n");
  
  printf("3. List is reversed\n");
  reverse(&l->head);
  print_list(l->head);
  printf("\n\n");
  
  printf("4. Remove %d from list\n", x);
  print_list(l1->head);
  l1->head = list_remove(l1->head, 200);
  print_list(l1->head);
  printf("\n\n");

  printf("5. Merge 2 lists\n");
  list_merge(l->head, l1->head, position);
  print_list(l->head);
  print_list(l1->head);
  printf("\n\n");

  printf("6. Are lists merged?\n");
  printf("%s", merged(l->head, l1->head) == true ? "yes" : "no");
  printf("\n\n");

}

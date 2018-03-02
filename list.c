#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"

void list_insert(node_t **head, int data)
{
  node_t *new = calloc(1, sizeof(int));

  if (new) {
    new->data = data;
    new->next = *head;
    *head = new;
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
  node_t *cur = head, *next = NULL;
  while (cur && cur->next) {
    next = cur->next;
    if (cur->data == next->data) {
      cur->next = next->next;
      free(next);
      next = cur->next;
    } else {
      cur = next;
      next = cur->next;
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
  char *data_file = "list_data.txt";
  FILE *fp;
  char input[512], *p, *token;
  int data, position = 5;
  node_t *head1 = NULL, *head2 = NULL;
  
  if ((fp = fopen(data_file, "r")) == NULL) {
    return;
  }

  if (fgets(input, 512, fp) != NULL) {
    p = input;
    while (token = strtok_r(p, " ", &p)) {
      list_insert(&head1, atoi(token));
    }
  }

  if (fgets(input, 512, fp) != NULL) {
    p = input;
    while (token = strtok_r(p, " ", &p)) {
      list_insert(&head2, atoi(token));
    }
  }

  print_list(head1);

  printf("After removing duplicate: ");
  remove_duplicate(head1);
  print_list(head1);
  
  printf("list is printed in reversed order:\n");
  print_reverse(head1);
  printf("\n");
  
  reverse(&head1);
  printf("list is reversed: ");
  print_list(head1);
  
  list_merge(head1, head2, position);

  printf("list1 and list2 are %s\n",
	 merged(head1, head2) == true ? "merged" : "not merged");
}

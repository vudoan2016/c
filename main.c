#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers.h"
#include "include/list.h"

static void list_ut();

int main()
{

  list_ut();
#if 0
  string_test();
  misc();
  graph_test();
  permutation_test();
  heap_test();
  tree_test();
  sort_test();
#endif
  
  return 0;
}

static void list_ut()
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

  print_list(l1->head);
  position = 2;
  remove_nth_from_tail(&l1->head, position);
  print_list(l1->head);

  print_list(l->head);
  position = 25;
  remove_nth(&l->head, position);
  print_list(l->head);


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

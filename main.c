#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "headers.h"
#include "include/list.h"

static void list_ut();

int main()
{
  string_test();

#if 0
  list_test();
  misc();
  graph_test();
  permutation_test();
  heap_test();
  tree_test();
  sort_test();
#endif
  
  return 0;
}

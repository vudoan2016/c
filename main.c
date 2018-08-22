#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "headers.h"
#include "include/list.h"

static void list_ut();

int main()
{
  list_test();
#if 0
  misc();
  tree_test();
  string_test();
  graph_test();
  permutation_test();
  heap_test();
  sort_test();
#endif
  
  return 0;
}

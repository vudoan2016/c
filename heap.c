/* 
 * https://www.cs.cmu.edu/~adamchik/15-121/lectures/Binary%20Heaps/heaps.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "heap.h"

static void swap(heap_node_t *x, heap_node_t *y)
{
  heap_node_t tmp;
  memcpy(&tmp, x, sizeof(heap_node_t));
  memcpy(x, y, sizeof(heap_node_t));
  memcpy(y, &tmp, sizeof(heap_node_t));
}

void heap_init(heap_t *h)
{
  h->last = 1; /* first element will be stored at index 1 */
  h->node = calloc(1, HEAP_MAX*sizeof(heap_node_t));
}

bool heap_is_empty(heap_t *h)
{
  return (h->last == 1);
}

void heap_destroy(heap_t *h)
{
  free(h->node);
}

static inline int heap_key_compare(heap_node_t *x, heap_node_t *y)
{
  return (x->key - y->key);
}

bool heap_insert(heap_t *h, heap_node_t *elem)
{
  bool result = false;
  int parent, cur;
  
  if (h->last < HEAP_MAX) {
    memcpy(&h->node[h->last], elem, sizeof(heap_node_t));
    cur = h->last;
    parent = h->last/2;
    while (cur > 1 &&
	   (heap_key_compare(&h->node[cur], &h->node[parent]) < 0)) {
      swap(&h->node[cur], &h->node[parent]);
      cur = parent;
      parent = cur/2;
    }

    h->last++;
    result = true;
  }
  return result;
}

/* 
 * Swap the last element with the first (index 1) then percolate 
 * the first element down.
 */
heap_node_t *heap_delete(heap_t *h)
{
  int cur, left_child, right_child;
  heap_node_t *min;
  
  min = calloc(1, sizeof(heap_node_t));
  memcpy(min, &h->node[1], sizeof(heap_node_t));

  memcpy(&h->node[1], &h->node[--h->last], sizeof(heap_node_t));

  cur = 1;
  left_child = 2*cur;
  right_child = 2*cur+1;

  while (left_child < h->last || right_child < h->last) {
    if (left_child < h->last) {
      cur = left_child;
    }
    if (right_child < h->last &&
	(heap_key_compare(&h->node[cur], &h->node[right_child]) > 0)) {
      cur = right_child;
    }

    if (heap_key_compare(&h->node[cur], &h->node[cur/2]) > 1) {
      break;
    }
    swap(&h->node[cur], &h->node[cur/2]);
    left_child = 2*cur;
    right_child = 2*cur+1;
  }

  return min;
}

void heap_print(heap_t *h)
{
  int i;
  
  printf("Last:%d. ", h->last);
  for (i = 1; i < h->last; i++) {
    printf("%d, ", h->node[i].key);
  }
  printf("\n");
}
 
 void heap_test()
 {
   char *node_file = "heap_node.txt";
   FILE *fp = fopen(node_file, "r");
   heap_t *h = NULL;
   heap_node_t elem, *elem_p;
   
   h = calloc(1, sizeof(heap_t));
   if (h == NULL) {
     printf("Unable to allocate memory for a heap\n");
     return;
   }
   
   heap_init(h);
   
   if (fp) {
     while (!feof(fp)) {
       if (fscanf(fp, "%d", &elem.key) == 1) {
	 heap_insert(h, &elem);
	 heap_print(h);
       }
     }

     while (h->last > 1) {
       elem_p = heap_delete(h);
       printf("min %d\n", elem_p->key);
       free(elem_p);
       heap_print(h);
     }       
   }
   heap_destroy(h);
   free(h);
   fclose(fp);
 }

/* 
 * https://www.cs.cmu.edu/~adamchik/15-121/lectures/Binary%20Heaps/heaps.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "heap.h"

static void swap(int *x, int *y)
{
  int tmp = *x;
  *x = *y;
  *y = tmp;
}

void heap_init(heap_t *h)
{
  h->last = 1; /* first element will be stored at index 1 */
  h->data = calloc(1, HEAP_MAX*sizeof(heap_data_t));
}

bool heap_is_empty(heap_t *h)
{
  return (h->last > 1);
}

void heap_destroy(heap_t *h)
{
  free(h->data);
}

bool heap_compare(heap_data_t *x, heap_data_t *y)
{
  return (x->key < y->key);
}

bool heap_insert(heap_t *h, heap_data_t *elem)
{
  bool result = false;
  int parent, cur;
  
  if (h->last < HEAP_MAX) {
    memcpy(&h->data[h->last], elem, sizeof(heap_data_t));
    h->data[h->last] = elem;
    cur = h->last;
    parent = h->last/2;
    while (cur > 1 && h->data[cur] < h->data[parent]) {
      swap(&h->data[cur], &h->data[parent]);
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
int heap_delete(heap_t *h)
{
  int cur, left_child, right_child;
  int min = h->data[1];

  h->data[1] = h->data[--h->last];

  cur = 1;
  left_child = 2*cur;
  right_child = 2*cur+1;

  while (left_child < h->last || right_child < h->last) {
    if (left_child < h->last) {
      cur = left_child;
    }
    if (right_child < h->last && h->data[cur] > h->data[right_child]) {
      cur = right_child;
    }

    if (h->data[cur] > h->data[cur/2]) {
      break;
    }
    swap(&h->data[cur], &h->data[cur/2]);
    left_child = 2*cur;
    right_child = 2*cur+1;
  }

  return min;
}

void heap_print(heap_t *h)
{
  int i;
  
  printf("Last:%d, ", h->last);
  for (i = 1; i < h->last; i++) {
    printf("%d, ", h->data[i]);
  }
  printf("\n");
}
 
 void heap_test()
 {
   char *data_file = "heap_data.txt";
   FILE *fp = fopen(data_file, "r");
   heap_t *h = NULL;
   int elem;
   
   h = calloc(1, sizeof(heap_t));
   if (h == NULL) {
     printf("Unable to allocate memory for a heap\n");
     return;
   }
   
   heap_init(h);
   
   if (fp) {
     while (!feof(fp)) {
       if (fscanf(fp, "%d", &elem) == 1) {
	 heap_insert(h, elem);
	 heap_print(h);
       }
     }

     while (h->last > 1) {
       elem = heap_delete(h);
       printf("min %d\n", elem);
       heap_print(h);
     }       
   }
   heap_destroy(h);
   free(h);
   fclose(fp);
 }

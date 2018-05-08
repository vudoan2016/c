#include <stdio.h>
#include <string.h>
#include "include/debug.h"

#define DEBUG 0

void merge_sort(int *a, int size)
{
  int tmp[size], i = 0, j = size/2, k = 0;
  
  if (size > 1) {
    merge_sort(a, size/2);
    merge_sort(a+size/2, size - size/2);

    /* merge the 2 halves */
    while (k < size) {
      if (i < size/2 && j < size) {
	if (a[i] < a[j]) {
	  DBG("size: %d, k: %d, i: %d, a[%d] = %d", size, k, i, i, a[i]);
	  tmp[k++] = a[i++];
	} else {
	  DBG("size: %d, k: %d, j: %d, a[%d] = %d", size, k, j, j, a[j]);
	  tmp[k++] = a[j++];
	}
      } else if (i < size/2) {
	DBG("size: %d, k: %d, i: %d, a[%d] = %d", size, k, i, i, a[i]);
	tmp[k++] = a[i++];
      } else {
	DBG("size: %d, k: %d, j: %d, a[%d] = %d", size, k, j, j, a[j]);
	tmp[k++] = a[j++];
      }
    }
    memcpy(a, tmp, size*sizeof(int));
    for (i = 0; i < size; i++) {
      printf("%d ", a[i]);
    }
    printf("\n");
  }
}

void sort_test()
{
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  int i, size = 11;
  
  merge_sort(a, size);
  printf("Array after merged sort: ");
  for (i = 0; i < size; i++) {
    printf("%d ", a[i]);
  }
  printf("\n");
}

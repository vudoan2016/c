#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "include/debug.h"
#include "include/utils.h"

#define ROW_MAX 6
#define COL_MAX 6

#define DEBUG 1

/* Given a 2D array which has a missing number in each row. The numbers in
 * each row are sorted and incremental by 1. Find the number using binary search.
 */
static void find_missing(int a[ROW_MAX][COL_MAX])
{
  int start, middle, end, i, j;
  bool found = false;
  
  for (i = 0; i < ROW_MAX; i++) {
    start = 0;
    end = COL_MAX-1;
    middle = (start + end)/2;
    
    /* binary search for the missing number in each row */
    while (start <= middle && !found) {
      if (a[i][middle] != a[i][middle-1]+1) {
	/* middle is the missing number */
	found = true;
      } else if (a[i][middle] > a[i][0] + middle) {
	/* The missing number is on the left of middle */
	end = middle-1;
      } else {
	/* The missing number is on the right of middle */
	start = middle+1;
      }
      middle = (start + end)/2;
    }

    /* The below printing (O(n)) defeats the purpose of the binary search (O(logn)).
     * The above logic should be in seperate function where it can be called
     * multiple times. 
     */
    printf("[");
    for (j = 0; j < COL_MAX; j++) {
      printf("%d", a[i][j]);
      if (j < COL_MAX-1) {
	printf(", ");
      }
    }
    printf("]: ");
    if (found) {
      printf("missing number %d\n", a[i][0] + middle);
    } else {
      printf("no missing number\n");
    }
    found = false;
  }
}

static void endianess()
{
  int x = 0x12345678;
  char *c = (char *)&x;

  if (*c == 0x12) {
    printf("big endian machine\n");
  } else {
    printf("little endian machine\n");
  }
}

static void find_duplicates(unsigned int a[], int size)
{
  int i;
  unsigned int *ba;

  ba = (unsigned int*)calloc(1, sizeof(unsigned int)*BA_MAX);
  
  printf("Duplicates in: ");
  for (i = 0; i < size; i++) {
    printf("%d", a[i]);
    if (i < size-1) {
      printf(", ");
    }
  }
  printf(": ");

  for (i = 0; i < size; i++) {
    if (ba_search(ba, a[i])) {
      printf("%d, ", a[i]);
    } else {
      ba_insert(ba, a[i]);
    }
  }
  printf("\n");
  free(ba);
}

/*
 * https://www.hackerrank.com/challenges/coin-change/problem
 */
int combo(int sum, int size, int *a)
{
  int i, ii, j, k, count = 0, remain;
  int seq[size] = {0};

  for (i = 0; i < size; i++) {
    remain = sum - a[i];
    DBG("i: %d, remain = %d", i, remain);
    seq[i] = a[i];
    if (remain == 0) {
      count++;
      printf("count = %d, {", count);
      for (k = 0; k <= i; k++) {
	printf("%d ", seq[k]);
      }
      printf("}\n");
    }
    for (ii = i+1; ii < size; ii++) {
      remain -= a[ii];
      seq[ii] = a[ii];
      DBG("ii: %d, remain = %d", ii, remain);
      if (remain == 0) {
	count++;
	printf("count = %d, {", count);
	for (k = 0; k <= ii; k++) {
	  printf("%d ", seq[k]);
	}
	printf("}\n");
      }
      for (j = ii+1; j < size && remain > a[j]; j++) {
	seq[j] = a[j];
	if (j < size-1) {
	  remain -= a[j];
	} else {
	  remain -= remain % a[j];
	}
	DBG("j: %d, remain = %d", j, remain);
	if (remain == 0) {
	  count++;
	  printf("count = %d, {", count);
	  for (k = 0; k <= j; k++) {
	    DBG("%d ", seq[k]);
	  }
	  printf("}\n");
	}
      }
      remain = sum - a[i];
    }
  }

  return count;
}

void misc()
{
  int a[ROW_MAX][COL_MAX] = {{4, 5, 6, 7, 8, 10},
			     {4, 5, 7, 8, 9, 10},
			     {4, 5, 6, 8, 9, 10},
			     {4, 5, 6, 7, 9, 10},
			     {101, 102, 103, 104, 105, 107},
			     {11, 12, 13, 14, 15, 16}};
  unsigned int b[] = {10001, 10001, 5, 6, 9, 6, 1, 20001, 9, 20001};
  int sum = 4, size = 3, count, i;
  int c[] = {1, 2, 3};
  
  /* find missing numbers in a 2D array */
  find_missing(a);

  /* find duplicated unsigned int using bit array */
  find_duplicates(b, 10);
  
  /* test system's endianess */
  endianess();

  combo(sum, size, c);
}


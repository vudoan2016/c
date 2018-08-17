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
    printf("big\n");
  } else {
    printf("little\n");
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
    if (ba_lookup(ba, a[i])) {
      printf("%d, ", a[i]);
    } else {
      ba_insert(ba, a[i]);
    }
  }
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

/* Sum of "month" contiguous squares = "day" */
int birthday_cake(unsigned int a[], int size, int month, int day)
{
  int i, matches = 0, sum = 0;

  for (i = 0; i < month; i++) {
    sum += a[i];
    matches++;
  }
  for (i = 1; i < size; i++) {
    sum -= a[i-1];
    sum += a[i+month-1];
    if (sum == day) {
      matches++;
    }
  }
  return matches;
}

/* TBD */
static int fibonacci(int n)
{
  int i = 0, prev1 = 0, prev2 = 0, num = 0;

  printf("%d numbers of the Fibonacci sequence: ", n);
  while (i++ < n) {
    num = prev1 + prev2;
    if (prev1 == 0)
      prev1 = 1;
    else 
      prev1 = prev2;
    prev2 = num;
    printf("%d ", num);
  }
  printf("\n");
  return num;
}

/* Reverse an integer. Return 0 if the result is overflowed. */
int reverse_int(int x)
{
  int y = 0;
  long int z = 0;
  
  while (x) {
    z = (long)y*10 + x%10;
    if ((z > 0x7fffffff) || (z < (int)0x80000000)) {
      return 0;
    }
    y = z;
    x = x/10;
  }
  return y;
}

void misc()
{
  char file_name[] = "misc_ut.txt", *str;
  FILE *fp;
  char line[512];
  int a[ROW_MAX][COL_MAX] = {{4, 5, 6, 7, 8, 10},
                             {4, 5, 7, 8, 9, 10},
                             {4, 5, 6, 8, 9, 10},
                             {4, 5, 6, 7, 9, 10},
                             {101, 102, 103, 104, 105, 107},
                             {11, 12, 13, 14, 15, 16}};
  unsigned int b[] = {10001, 10001, 5, 6, 9, 6, 1, 20001, 9, 20001};
  int sum = 4, size = 3, count, i = 0, x;
  int c[] = {1, 2, 3};
  long month, day;
  int z[] = {1534236469, -2147483412, -10, 123};
  
  for (i = 0; i < sizeof(z)/sizeof(int); i++) {
    printf("%d is reversed of %d\n", reverse_int(z[i]), z[i]);
  }
    
  fp = fopen(file_name, "r");
  if (fp == NULL) {
    printf("Unable to open file\n");
    return;
  }

  printf("1. Find missing number\n");
  find_missing(a);
  printf("\n\n");
  
  printf("2. Find duplicates\n");
  find_duplicates(b, 10);
  printf("\n\n");
  
  printf("x. Endianess\n");
  endianess();
  printf("\n\n");

  printf("3. Combination\n");
  combo(sum, size, c);
  printf("\n\n");

  printf("4. Birthday cake\n");
  fgets(line, 512, fp);
  str = strtok(line, " ");
  size = 0;
  while (str) {
    b[size++] = atoi(str);
    str = strtok(line, " ");
  }
  DBG("%s", "b[]: ");
  for (i = 0; i < size; i++) {
    DBG("%d ", b[i]);
  }

  fgets(line, 512, fp);
  month = strtol(strtok(line, " "), NULL, 10);
  day = strtol(strtok(line, " "), NULL, 10);
  count = birthday_cake(b, size, month, day);
  printf("%d of %ld contiguous squares with sum %ld\n",
         count, month, day);
  fclose(fp);

}


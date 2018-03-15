#include <stdio.h>
#include "include/debug.h"

#define ROW_MAX 5
#define COL_MAX 6

#define DEBUG 1

/* Given a 2D array which has a missing number in each row. The numbers in
 * each row are sorted and incremental by 1. Find the number using binary search.
 */
void find_missing(int a[][COL_MAX])
{
  int start, middle, end, i;
  
  for (i = 0; i < ROW_MAX; i++) {
    start = 0;
    end = ROW_MAX;
    while (start <= end) {
      middle = (start + end)/2;

      /* Check if middle is the missing number */
      if (middle > start && a[i][middle] != a[i][middle-1]+1) {
	break;
      } else if (a[i][middle] > a[i][0] + middle) {
	/* The missing number is on the left of middle */
	end = middle-1;
      } else {
	/* The missing number is on the right of middle */
	start = middle+1;
      }
    }
    printf("%d: missing number %d\n", i, a[i][0] + middle);
  }
}

/*
 * Implement Rabin-Karp rolling hash algorithm.
 */
void string_search(char *str, char *pattern)
{

}

/*
 * Replace duplicate characters by a number. For example, "abbbcccc" is 
 * represented as "ab3c4."
 */
void string_compress(char *str)
{
  int dup = 0;
  char *c;
  char *base = str;

  c = base+1;
  while (c && *c != '\0') {
    DBG("c: %c, base: %c\n", *c, *base);
    if (*base == *c) {
      dup++;
      DBG("count: %d\n", dup);
    } else {
      if (dup) {
	*++base = (dup + 48) + 1; // add 1 is for the base
      }
      base++;
      if (base != c) {
	// start a new sequence
	*base = *c;
      }
      dup = 0;
    }
    c++;
  }
  if (dup) {
    *++base = (dup+48) + 1; // add 1 is for the base
    *++base = '\0';
  }
  DBG("str: %s\n", str);
}

void misc()
{
  int a[ROW_MAX][COL_MAX] = {{4, 5, 6, 7, 8, 10},
			     {4, 5, 7, 8, 9, 10},
			     {4, 5, 6, 8, 9, 10},
			     {4, 5, 6, 7, 9, 10},
			     {101, 102, 103, 104, 105, 107}};
  char str[] = "abbbbccccccc";

  find_missing(a);
  printf("String %s ", str);
  string_compress(str);
  printf("in compressed form %s.\n", str);
}

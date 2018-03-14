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
 * Implement Rabin-Karp algorithm
 */
void string_search(char *str, char *pattern)
{

}

void string_compress(char *str)
{
  int count = 0;
  char *x, *y = str;

  x = y+1;
  while (x && *x != '\0') {
    DBG("x: %c, y: %c\n", *x, *y);
    if (*y == *x) {
      count++;
      DBG("count: %d\n", count);
    } else {
      if (count) {
	y++;
	*y = count + 48;
      }
      y++;
      if (y != x) {
	*y = *x;
      }
      count = 0;
    }
    x++;
  }
  if (count) {
    y++;
    *y = count+48;
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
  string_compress(str);
}

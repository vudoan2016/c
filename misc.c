#include <stdio.h>

#define ROW_MAX 5
#define COL_MAX 6

/* Given a 2D array which has a missing number in each row. The numbers in
 * each row are sorted and incremental by 1. Find the number using binary search.
 */
int find_missing(int a[][COL_MAX])
{
  int start, middle, end, i, j;
  
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

void misc()
{
  int a[ROW_MAX][COL_MAX] = {{4, 5, 6, 7, 8, 10},
			     {4, 5, 7, 8, 9, 10},
			     {4, 5, 6, 8, 9, 10},
			     {4, 5, 6, 7, 9, 10},
			     {101, 102, 103, 104, 105, 107}};
  find_missing(a);
}

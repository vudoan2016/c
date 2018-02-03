#include <stdio.h>
#include <string.h>

static void swap(char *a, char *b)
{
  char tmp = *a;
  *a = *b;
  *b = tmp;
}

static int factorial(int n)
{
  int result = n;
  while (n > 1) {
    result *= (n-1);
    n--;
  }
  return result;
}

/*
 * Heap's algorithm using recrusion.
 * If n is odd swap the first and last character. Otherwise swap the ith element
 * and the last one.
 */
static void permute(char *str, int n, int *index)
{
  int i;

  if (n == 1) {
    printf("%d: %s\n", *index, str);
    (*index)++;
    return;
  }    

  for (i = 0; i < n; i++) {
    permute(str, n-1, index);
    if (n & 1) {
      swap(str, str+n-1);
    } else {
      swap(str+i, str+n-1);
    } 
  }
}

/*
 * Permute a string iteratively.
 * 1. copy the original string (str) to a temporary string (str1)
 * 2. swap the first leter with other letters
 * 3. repeat the bellow op (string-length-1)! times:
 *      swap the second letter with the next letter until the second letter reaches the end.
 *    For example, the string is abcd. The permutations with the first letter 'a' are 'acbd',
 *    'acdb', 'adcb', 'adbc', 'abdc', 'abcd'
 */
static void iter_permute(char *str)
{
  int i, j, k, fact, len = strlen(str);
  char str1[len+1];

  for (i = 0; i < len; i++) {
    strncpy(str1, str, len);
    swap(str1, str1+i);
    fact = factorial(len-1);
    for (j = 0; j < fact; j++) {
      for (k = 1; k < len-1; k++) {
	swap(str1+k, str1+k+1);
	printf("i:%d, j:%d, k:%d: %s\n", i, j, k, str1);
      }
    }
  }
}

void string_test()
{
  char str[] = "abc";
  int index = 0;

  permute(str, strlen(str), &index);

  iter_permute(str);
}

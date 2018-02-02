#include <stdio.h>
#include <string.h>

/*
 * Heap's algorithm using recrusion.
 * If n is odd swap the first and last character. Otherwise swap the ith element
 * and the last one.
 */
static void permute(char *str, int n, int *index)
{
  int i;
  char tmp;

  if (n == 1) {
    printf("%d: %s\n", *index, str);
    (*index)++;
    return;
  }    

  for (i = 0; i < n; i++) {
    permute(str, n-1, index);
    if (n & 1) {
      tmp = str[0];
      str[0] = str[n-1];
      str[n-1] = tmp;
    } else {
      tmp = str[i];
      str[i] = str[n-1];
      str[n-1] = tmp;
    } 
  }
}

void string_test()
{
  char str[] = "abcdefgh";
  int index = 0;

  permute(str, strlen(str), &index);
}

#include <stdio.h>

/*
 * Heap's algorithm using recrusion.
 * If n is odd swap the first and last character. Otherwise swap the ith element
 * and the last one.
 */
void permute(char *str, int n)
{
  int i;
  char tmp;

  if (n == 1) {
    printf("n:%d, i:%d: %s\n", n, i, str);
  }    
  for (i = 0; n > 1 && i < n; i++) {
    permute(str, n-1);
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


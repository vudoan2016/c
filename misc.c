#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
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

/*
 * Implement Rabin-Karp rolling hash algorithm. H = c1*a^k-1 + c2*a^k-2 + 
 * c3*a^k-3 ... where c is a character in the input str (c1c2c3...cn) and a 
 * is a constant and k is the length of the pattern. Essentially it means
 * that each character occupies 1 byte (if the base is 256). 
 * There is a problem with this algorithm. If the base is 256 then the pattern 
 * can only have 4 characters. Longer patterns will result in hash collision.
 * Text character starts with the 'space' (32) and ends with '~' (126).
 * https://brilliant.org/wiki/rabin-karp-algorithm/
 *
 * Output: the number of matches
 */
int grep(char *pattern, char str[])
{
  unsigned int i, k = strlen(pattern);
  int base = 64, matches = 0;
  unsigned int h = 0, pattern_h = 0;
  char *c;
  
  if (k > strlen(str)) {
    return 0;
  }
  
  for (i = 0; i < k; i++) {
    pattern_h += (pattern[i]-31)*pow(base, k-i-1);
    DBG("i: %d, c: %c (%d), pattern_h: %d", i, pattern[i], pattern[i], pattern_h);
  }
      
  c = str;
  for (i = 0; i < k; i++) {
    h += (*c-31) * pow(base, k-i-1);
    DBG("i: %d, c: %c (%d), h: %d", i, *c, *c, h);
    c++;
  }

  i = 0;
  if (h == pattern_h) {
    matches++;
    DBG("found a match. key %d", h);
  }
  
  while (c && *c != '\0') {
    h -= (str[i]-31) * pow(base, k-1);
    h *= base;
    h += *c-31;
    DBG("c: %c (%d), h: %d", *c, *c, h);
    if (h == pattern_h) {
      matches++;
      DBG("found a match. key %d", h);
    }
      
    c++;
    i++;
  }
  return matches;
}

/*
 * Replace duplicate characters by a number. For example, "abbbcccc" is 
 * represented as "ab3c4."
 */
static void string_compress(char *str)
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

void misc()
{
  int a[ROW_MAX][COL_MAX] = {{4, 5, 6, 7, 8, 10},
			     {4, 5, 7, 8, 9, 10},
			     {4, 5, 6, 8, 9, 10},
			     {4, 5, 6, 7, 9, 10},
			     {101, 102, 103, 104, 105, 107},
			     {11, 12, 13, 14, 15, 16}};
  unsigned int b[] = {10001, 10001, 5, 6, 9, 6, 1, 20001, 9, 20001};
  char str[] = "abbbbccccbbc", pattern[] = "void";
  char file[] = "misc.c";
  FILE *fp = NULL;
  char buf[4*1024] = "";
  int matches = 0;
  
  /* find missing numbers in a 2D array */
  find_missing(a);

  /* find duplicated unsigned int using bit array */
  find_duplicates(b, 10);
  
  /* compress a string */
  DBG("String %s ", str);
  string_compress(str);
  printf("in compressed form %s.\n", str);

  if ((fp = fopen(file, "r")) == NULL) {
    printf("Unable to open file %s\n", file);
    return;
  }
  
  while (!feof(fp)) {
    fgets(buf, 1024, fp);
    DBG("%s", buf);
    matches += grep(pattern, buf);
  }
  fclose(fp);
  
  printf("found %d match(es) of \'%s\' in file %s\n", matches, pattern, file);

  /* test system's endianess */
  endianess();
}

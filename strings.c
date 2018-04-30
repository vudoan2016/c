#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "include/debug.h"

#define DEBUG 0
#define LINE_MAX 1024

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

/* Multiply 2 numbers given as strings */
char *multiplication(char *x, char *y)
{
  int i, j, k, z, carry, carry1, sum;
  char *prod = NULL, c, *p;

  prod = (char *)calloc(1, 256);
  for (i = strlen(y)-1; i >= 0; i--) {
    carry = 0;
    k = strlen(y)-i-1;
    for (j = strlen(x)-1; j >= 0; j--) {
      z = (y[i]-48)*(x[j]-48) + carry;
      carry = z/10;
      DBG("%d * %d = %d, carry = %d", x[j]-48, y[i]-48, z%10, carry);
      /* add to prod */
      sum = carry1 + (prod[k] > 0 ? prod[k] - 48 + z%10 : z%10);
      prod[k] = sum%10 + 48;
      carry1 = sum/10;
      k++;
      p = prod;
    }
    /* Take care of the carry-overs */
    carry += carry1;
    while (carry) {
      sum = carry + carry1 + (prod[k] > 0 ? prod[k] - 48 : 0);
      prod[k] = sum % 10 + 48;
      carry = sum/10;
      k++;
    }
  }
  for (i = 0; i < k/2; i++) {
    c = prod[i];
    prod[i] = prod[k-1-i];
    prod[k-1-i] = c;
  }
  prod[k] = '\0';
  return prod;
}

void string_test()
{
  char x[LINE_MAX], y[LINE_MAX];
  char *prod = NULL;
  char file[] = "misc.c";
  FILE *fp = NULL;
  char buf[4*1024] = "";
  char uncompressed[LINE_MAX];
  char pattern[] = "void";
  int matches = 0;

  /* Test1: multiply 2 strings */
  fgets(x, LINE_MAX, stdin);
  x[strlen(x)-1] = '\0';
  fgets(y, LINE_MAX, stdin);
  y[strlen(y)-1] = '\0';
  prod = multiplication(x, y);
  printf("%s * %s = %s\n", x, y, prod);
  free(prod);

  /* Test2: compress a string */
  printf("Enter a string to compress: ");
  fgets(x, LINE_MAX, stdin);
  x[strlen(x)-1] = '\0';
  DBG("String %s ", x);
  string_compress(x);
  printf("in compressed form %s.\n", x);

  /* Test3: implement grep */
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
}

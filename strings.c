#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
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
* Replace duplicate letters by a number. For example, "abbbcccc" is 
* represented as "a1b3c4."
*/
static void string_compress(char *str)
{
  int dup = 0;
  char *c = str;
  char tmp[strlen(str)+1] = "";
  int i = 0;

  while (*c != '\0') {
    if (tmp[i] == *c) {
      dup++;
    } else {
      if (dup > 0) {
        i += sprintf(&tmp[++i], "%d", dup);
        tmp[i] = *c;
        dup = 1;
      } else {
        tmp[i] = *c;
        dup = 1;
      }
    }
    c++;
    if (*c == '\0') {
      i += sprintf(&tmp[++i], "%d", dup);
      tmp[i] = '\0';
    }
  }
  strncpy(str, tmp, i+1);
}

void compress_duplicates(char *str) 
{
    char *c = str, *p = str, *tmp;
    int count = 0;
    char buf[10];
    
    if (!str)
        return;
    while (*c != '\0') {
        count = 1;
        tmp = c+1;
        while (*tmp != '\0' && *tmp == *c) {
            count++;
            tmp++;
        }
        if (count > 1) {
            *p = *c;
            itoa(count, buf, 10);
            strncpy(++p, buf, strlen(buf));
            p += strlen(buf);
            c = tmp;
        } else {
            *p++ = *c++;
        }
    }
    *p = '\0';
}

/* String multiplication. Need to support negative multiplicands */
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

/* Return index of substr in str. -1 if not found. */
int index_of(char *str, char *substr)
{
  char *c = str;
  int i;

  if (!str || !substr || !strlen(substr))
    return -1;

  while (*c != '\0') {
    if (*c == substr[0] && strlen(c) >= strlen(substr)) {
      DBG("@c %p, @str %p", c, str);
      for (i = 0; i < strlen(substr); i++) {
        if (c[i] != substr[i]) {
          break;
        }
      }
      if (i == strlen(substr)) {
        return (c-str);
      }
    }
    c++;
  }
  return -1;
}

/* Find the first non-repeating character in a string in O(n) */
char first_non_repeating(char *str)
{
  char htb[256] = {0};
  char *c = str;

  while (c && *c != '\0') {
    htb[*c] += 1;
    c++;
  }
  c = str;
  while (c && *c != '\0') {
    if (htb[*c] == 1) {
      return *c;
    }
    c++;
  }
  return *c;
}

/* 
 * Print out the indices of all the anagrams of pattern 'p' in a string 's.'
 * Eg, s=cbaebabacdef, p=abc prints out [0, 6] and s=abab, p=ab prints out [0, 1, 2]. 
 * Todo: return an array of the indices and the size of the array as well. 
 * The caller needs to free the array.
 * int *anagram_indices(char *str, char *p, int *size)
 */
void anagram_indices(char *str, char *p) 
{
  unsigned char htbl[256] = {0};
  char *c = p;
  int i, j;
  bool is_anagram = true;

  for (i = 0; i <= strlen(str) - strlen(p); i++) {
    c = p;
    while (c && *c != '\0') {
      htbl[*c]++;
      c++;
    }
    is_anagram = true;
    for (j = i; j < i+strlen(p); j++) {
      if (htbl[str[j]] > 0) {
        htbl[str[j]]--;
      } else {
        is_anagram = false;
        DBG("Not an anagram %c", str[j]);
        break;
      }
    }
    if (is_anagram) {
      c = p;
      while (c && *c != '\0') {
        if (htbl[*c] != 0) {
          is_anagram = false;
          DBG("Not an anagram %c", str[j]);
          break;
        }      
        c++;
      }
      /* found an anagram */
      if (is_anagram) {
        printf("%d\n", i);
      } else {
        c = p;
        while (c && *c != '\0') {
          htbl[*c] = 0;
          c++;
        }
      }
    } else {
      c = p;
      while (c && *c != '\0') {
        htbl[*c] = 0;
        c++;
      }
    }
  }
}

int longest_non_repeating_substring(char *str)
{
  unsigned int bit_array[256/32] = {0};
  char *c = str, *p;
  int max = 0, count = 0;

  while (c && *c != '\0') {
    p = c;
    count = 0;
    while (*p != '\0') {
      if (bit_array[*p/32] & (1 << *p % 32)) {
        if (count > max) {
          max = count;
          break; 
        }
      } else {
        bit_array[*p/32] |= 1 << (*p % 32);
        count++;
      }
      p++;
    }
    memset(bit_array, 0, sizeof(bit_array));
    c++;
  }
  return max;
}

void reverse_chars_in_word(char *str) 
{
    char *c = str, *p = str, *tmp = NULL;
    char x;
    
    if (str == NULL)
        return;
    while (c) {
        if ((*c == ' ' || *c == '\0') && tmp != NULL && *tmp != ' ') {
            while (p < tmp) {
                x = *p;
                *p = *tmp;
                *tmp = x;
                tmp--;
                p++;
            }
            if (*c == '\0') {
                break;
            }
        } else if (*c != ' ' && tmp != NULL && *tmp == ' ') {
            p = c;
        }
        tmp = c;
        c++;
    }
}

void remove_extra_space(char *str)
{
    char *c = str, *p = str;
    int count = 0;
    
    if (!str)
        return;
    while (*c != '\0') {
        if (*c == ' ') {
            count++;
            if (count == 1) {
                *p = ' ';
                p++;
            }
            c++;
        } else {
            count = 0;
            if (c != p) {
                *p = *c;
            }
            c++;
            p++;
        }
    }
    *p = '\0';
}

void string_test()
{
  char x[LINE_MAX] = "hello", y[LINE_MAX];
  char *prod = NULL;
  char file[] = "string_ut.txt";
  FILE *fp = NULL;
  char buf[4*1024] = "";
  char substr[][100] = {"lol", "of", "hello world", "ll", "hell"};
  char pattern[] = "ll";
  int matches = 0, i, count = 5, align = 12;

  if ((fp = fopen(file, "r")) == NULL) {
    printf("Unable to open file %s\n", file);
    return;
  }

  printf("1. String multiplication\n");
  for (i = 0; i < count; i++) {
    fgets(x, LINE_MAX, fp);
    x[strlen(x)-1] = '\0';
    fgets(y, LINE_MAX, fp);
    y[strlen(y)-1] = '\0';
    prod = multiplication(x, y);
    printf("%s * %s = %s\n", x, y, prod);
    free(prod);
  }
  printf("\n\n");

  printf("2. Compress a string\n");
  fgets(x, LINE_MAX, fp);
  /* get rid of EOL stored by fgets */
  x[strlen(x)-1] = '\0';
  printf("%s\n", x);
  string_compress(x);
  printf("%s\n", x);
  printf("\n\n");

  printf("3: grep '%s' %s\n", pattern, file);
  while (!feof(fp)) {
    fgets(buf, 1024, fp);
    matches += grep(pattern, buf);
  }
  printf("found %d match(es) of '%s' in file %s\n", matches, pattern, file);
  printf("\n\n");

  printf("4: index of substring\n");
  for (i = 0; i < sizeof(substr)/100; i++) {
    printf("index of '%s' in '%s' = %d\n", substr[i], x, index_of(x, substr[i]));
  }

  printf("\n\n");
  strcpy(x, "wwhhaatt\'s uup");
  printf("5: first non-repeating character in %s: %c\n", x, first_non_repeating(x));

  printf("\n\n");
  strcpy(x, "cbaebabacd");
  strcpy(y, "abc");
  printf("6: print indices of anagrams of \'%s\' in \'%s\'\n", y, x);
  anagram_indices(x, y);

  printf("\n\n");
  printf("7: longest non-repeating substring\n");
  printf("%*s: %d\n", align, x, longest_non_repeating_substring(x));
  strcpy(x, "pwwkew");
  printf("%*s: %d\n", align, x, longest_non_repeating_substring(x));
  strcpy(x, "bbbb");
  printf("%*s: %d\n", align, x, longest_non_repeating_substring(x));
  fclose(fp);
}

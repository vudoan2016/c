#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void reverse(char *str) 
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

int main()
{
    char str1[] = "This is a        test only                  !", 
         str2[] = "       This is another test ....", 
         str3[] = "",
         str4[] = "Hellooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo .... anybody here?";
         
    printf("Before: length=%d, %s\n", strlen(str4), str4);
    compress_duplicates(str4);
    printf("After: length=%d, %s\n", strlen(str4), str4);
    return 0;
}
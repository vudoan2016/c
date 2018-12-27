#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct node_s {
    int val;
    struct node_s *next;
} node_t;

bool is_palindrome(node_t *h)
{
    int i=0, size=0;
    node_t *c = h, *m = NULL, *t = NULL, *n = NULL, *tmp;
    while (c) {
        size++;
        c = c->next;
    }
    if (size < 2)
        return true;
    m = h;
    while (i < size/2) {
        i++;
        m = m->next;
    }
    if (size%2)
        m = m->next;
    printf("size = %d, m = %d\n", size, m->val);
    /* reverse the 2nd half */
    t = m;
    n = t->next;
    t->next = NULL;
    while (n) {
        tmp = n->next;
        n->next = t;
        t = n;
        n = tmp;
    }
    c = h;
    while (c) {
        printf("%d ", c->val);
        c = c->next;
    }
    printf("\n");
    n = t;
    while (n) {
        printf("%d ", n->val);
        n = n->next;
    }
    printf("\n");
    
    /* check for palindrome */
    c = h;
    n = t;
    while (n) {
        if (n->val != c->val)
            return false;
        n = n->next;
        c = c->next;
        if (c == m)
            break;
    }
    
    n = t->next;
    t->next = NULL;
    while (n) {
        tmp = n->next;
        n->next = t;
        t = n;
        n = tmp;
        if (n == m) 
            break;
    }
    return true;
}

int main() 
{
    node_t *c = (node_t*)calloc(1, sizeof(node_t)), *h, *tmp;
    c->val = 1;
    c->next = NULL;
    h = c;

    c = (node_t*)calloc(1, sizeof(node_t));
    if (c) {
        c->val = 2;
        c->next = h;
        h = c;
    }
    c = (node_t*)calloc(1, sizeof(node_t));
    if (c) {
        c->val = 2;
        c->next = h;
        h = c;
    }
    c = (node_t*)calloc(1, sizeof(node_t));
    if (c) {
        c->val = 1;
        c->next = h;
        h = c;
    }

    c = h;
    while (c) {
        printf("%d ", c->val);
        c = c->next;
    }
    printf("\n");

    printf("list is %s\n", is_palindrome(h) ? "palindrome" : "not palindrome");
    c = h;
    while (c) {
        printf("%d ", c->val);
        c = c->next;
    }
    printf("\n");

    c = h;
    while (c) {
        tmp = c;
        c = c->next;
        free(tmp);
    }
    return 0;
}


#ifndef __TREE_H__
#define __TREE_H__

typedef struct node_s {
  int data;
  struct node_s *left;
  struct node_s *right;
} node_t;

#endif

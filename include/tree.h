#ifndef __TREE_H__
#define __TREE_H__

typedef struct node_s {
  int key;
  int height;
  struct node_s *left;
  struct node_s *right;
} node_t;

void tree_print(node_t *root);
node_t *avl_insert(node_t *root, int key);

#endif

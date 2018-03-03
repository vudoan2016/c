#ifndef __AVL_H__
#defne __AVL_H__

typedef struct node_s {
  int height;
  struct node_s *left;
  struct node_s *right;
} node_t;

#endif

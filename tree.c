#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tree.h"

static void tree_init(node_t *root)
{
  root->left = NULL;
  root->right = NULL;
}

/*
 * left < root < right & no duplicate
 */
static void tree_insert(node_t **root, int key)
{
  node_t *node_p, *parent_p = NULL, *new_p = calloc(1, sizeof(node_t));
  bool duplicate = false;
  
  new_p->key = key;
  new_p->left = NULL;
  new_p->right = NULL;

  if (*root == NULL) {
    *root = new_p;
    return;
  }

  node_p = *root;
  while (node_p && !duplicate) {
    parent_p = node_p;
    if (key == node_p->key) {
      duplicate = true;
    }
    node_p = (key < node_p->key ? node_p->left : node_p->right);
  }

  if (!duplicate) {
    if (key < parent_p->key) {
      parent_p->left = new_p;
    } else {
      parent_p->right = new_p;
    }
  }
}

static void tree_delete(node_t **root, int key)
{
  node_t *x = *root, *parent = NULL, *y;

  while (x) {
    if (x->key == key) {
      break;
    }
    parent = x;
    x = x->key < key ? x->right : x->left;
  }

  if (x) {
    if (x->left == NULL ||  x->right == NULL) {
      if (parent == NULL) {
	*root = NULL;
	printf("key %d is the only node and it's at the root\n", key);
      } else if (x == parent->left) {
	parent->left = x->left ? x->left : x->right;
	if (parent->left) {
	  printf("key %d is at a node which has no children\n", key);
	} else {
	  printf("key %d is at a leaf node\n", key);
	}
      } else {
	parent->right = x->left ? x->left : x->right;
	if (parent->right) {
	  printf("key %d is at a node which has no children\n", key);
	} else {
	  printf("key %d is at a leaf node\n", key);
	}
      }
      free(x);
    } else {
      // find the eldest child to replace x
      parent = x;
      y = x->left;
      while (y->right) {
	parent = y;
	y = y->right;
      }
      printf("the eldest child is %d\n", y->key);
      parent->right = y->left;
      x->key = y->key;
      free(y);
    }
  } else {
    printf("key %d is not found in tree\n", key);
  }
}

/*
 * Inorder traversal (left then self then right).
 */
void tree_print(node_t *root)
{
  if (root == NULL) {
    return;
  }
  tree_print(root->left);
  printf("%d ", root->key);
  tree_print(root->right);
}

bool tree_search(node_t *root, int key)
{
  node_t *n = root;

  while (n && n->key != key) {
    n = key < n->key ? n->left : n->right;
  }
  return (n != NULL);
}

void tree_test()
{
  char *tree_data = "tree_data.txt";
  FILE *fp;
  node_t *root = NULL;
  int key;
  
  if ((fp = fopen(tree_data, "r")) == NULL) {
    printf("Unable to open file %s to read\n", tree_data);
    return;
  }

  while (!feof(fp)) {
    if (fscanf(fp, "%d", &key) == 1) {
      tree_insert(&root, key);
    }
  }

  tree_print(root);
  printf("\n");

  /* delete */
  key = -1;
  tree_delete(&root, key);
  tree_print(root);
  
  /* Search */
  key = 20;
  printf("%d is %s\n", key,
	 tree_search(root, key) == true ? "found" : "not found");
  fclose(fp);
}


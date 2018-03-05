#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "include/tree.h"
#include "include/debug.h"

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
	*root = x->left ? x->left : x->right;
	printf("key %d is at the root node\n", key);
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
      if (y == parent->right) {
	parent->right = y->left;
      } else {
	parent->left = y->left;
      }
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
  printf("(%d,%d) ", root->key, root->height);
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

void tree_longest_path(node_t *root)
{

}

void tree_print_by_level(node_t *root)
{

}

void tree_test()
{
  char *tree_data = "tree_data.txt";
  FILE *fp;
  node_t *root = NULL, *x;
  int key;
  
  if ((fp = fopen(tree_data, "r")) == NULL) {
    printf("Unable to open file %s to read\n", tree_data);
    return;
  }

  while (!feof(fp)) {
    if (fscanf(fp, "%d", &key) == 1) {
      /*tree_insert(&root, key);*/
      root = avl_insert(root, key);
    }
  }

  printf("Tree: ");
  tree_print(root);
  printf("\n");

  /* delete */
#if 0
  while (x = root) {
    printf("Deleting key %d:\n", x->key);
    tree_delete(&root, x->key);
  }
  printf("Tree: ");
  tree_print(root);
  printf("\n");
#endif
  
  /* search */
  key = 20;
  printf("Searching: %d is %s\n", key,
	 tree_search(root, key) == true ? "found" : "not found");
  fclose(fp);
}


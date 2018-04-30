#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "include/tree.h"
#include "include/debug.h"

#define DEBUG 1

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
  node_t *node_p, *parent_p = NULL, *new_p = (node_t *)calloc(1, sizeof(node_t));
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
	DBG("key %d is at the root node", key);
      } else if (x == parent->left) {
	parent->left = x->left ? x->left : x->right;
	if (parent->left) {
	  DBG("key %d is at a node which has no children", key);
	} else {
	  DBG("key %d is at a leaf node", key);
	}
      } else {
	parent->right = x->left ? x->left : x->right;
	if (parent->right) {
	  DBG("key %d is at a node which has no children", key);
	} else {
	  DBG("key %d is at a leaf node", key);
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
      DBG("the eldest child is %d", y->key);
      if (y == parent->right) {
	parent->right = y->left;
      } else {
	parent->left = y->left;
      }
      x->key = y->key;
      free(y);
    }
  } else {
    DBG("key %d is not found in tree", key);
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

/* 
 * Use BFS 
 */
void tree_in_level_traverse(node_t *root)
{
}

/*
 * Test if a tree is a BST 
 */
bool tree_is_bst(node_t *root)
{
  node_t *node = root, *parent = NULL;
  
  if (root == NULL) {
    return true;
  } else if ((root->left && root->key < root->left->key) ||
	     (root->right && root->key > root->right->key)) {
    DBG("key %d, left %d, right %d", root->key, root->left ? root->left->key : -2,
	root->right ? root->right->key : -1);
    return false;
  } else {
    return (tree_is_bst(root->left) && tree_is_bst(root->right));
  }
}

void tree_test()
{
  char tree_data[] = "tree_data.txt";
  FILE *fp;
  node_t *root = NULL;
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
#if 1
  printf("Tree: ");
  tree_print(root);
  printf("\n");
#endif
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

  /* test if tree is bst */
  printf("Tree is %s\n", tree_is_bst(root) ? "bst" : "not bst");
 }


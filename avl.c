#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "include/tree.h"
#include "include/debug.h"

#define DEBUG 0

static int avl_max_height(node_t *left, node_t *right)
{
  if (left == NULL && right == NULL) {
    return 0;
  } else if (left == NULL || right == NULL) {
      return left ? left->height : right->height;
  } else {
    return left->height > right->height ? left->height : right->height;
  }
}

static int avl_get_height(node_t *root)
{
  return root ? root->height : 0;
}

/*
 * https://www.geeksforgeeks.org/avl-tree-set-1-insertion/
 */
node_t *avl_insert(node_t *root, int key)
{
  node_t *new_p, *x, *y;
  if (root == NULL) {
    new_p = (node_t *)calloc(1, sizeof(node_t));
    new_p->key = key;
    new_p->height = 1;
    return new_p;
  }
  
  if (key < root->key) {
    DBG("insert key %d to the left of %d", key, root->key);
    root->left = avl_insert(root->left, key);
  } else {
    DBG("insert key %d to the right of %d", key, root->key);
    root->right = avl_insert(root->right, key);
  }
  DBG("insert key %d, root %d, height %d", key, root->key, root->height);

  if ((avl_get_height(root->left) - avl_get_height(root->right)) > 1) {
    y = root->left;
    if (key > root->left->key) {
      /* rotate left */
      x = y->right;
      DBG("Rotate left: root %d, y %d, x %d", root->key, y->key, x->key);
      y->right = x->left;
      x->left = y;
      root->left = x;
      y->height = avl_max_height(y->left, y->right) + 1;
      x->height = avl_max_height(x->left, x->right) + 1;
      DBG("x=%d, h=%d, left=%d, right=%d", x->key, x->height, x->left ? x->left->key : -1,
	  x->right ? x->right->key : -1);
      DBG("y=%d, h=%d, left=%d, right=%d", y->key, y->height, y->left ? y->left->key : -1,
	  y->right ? y->right->key : -1);
      
      /* rotate right */
      DBG("... %s", "rotate right");
      root->left = x->right;
      x->right = root;
      
      root->height = avl_max_height(root->left, root->right) + 1;
      y->height = avl_max_height(y->left, y->right) + 1;
      x->height = avl_max_height(x->left, x->right) + 1;
      DBG("x=%d, h=%d, left=%d, right=%d", x->key, x->height, x->left ? x->left->key : -1,
	  x->right ? x->right->key : -1);
      DBG("y=%d, h=%d, left=%d, right=%d", y->key, y->height, y->left ? y->left->key : -1,
	  y->right ? y->right->key : -1);
      DBG("root=%d, h=%d, left=%d, right=%d", root->key, root->height, root->left ? root->left->key : -1,
	  root->right ? root->right->key : -1);
      return x;
    } else {
      /* rotate right */
      DBG("Rotate right: root %d, y %d", root->key, y->key);
      root->left = y->right;
      y->right = root;
      root->height = avl_max_height(root->left, root->right) + 1;
      y->height = avl_max_height(y->left, y->right) + 1;
      DBG("y=%d,h=%d", y->key, y->height);
      return y;
    }
  } else if ((avl_get_height(root->right) - avl_get_height(root->left)) > 1) {
    y = root->right;
    if (key < root->right->key) {
      /* rotate right */
      x = y->left;
      DBG("Rotate right: root %d, left %d, right %d, y %d, x %d",
	  root->key, root->left ? root->left->key : -1,
	  root->right ? root->right->key : -1, y->key, x->key);
      y->left = x->right;
      x->right = y;
      root->right = x;
      
      /* rotate left */
      DBG("... %s", "rotate left"); 
      root->right = x->left;
      x->left = root;
      root->height = avl_max_height(root->left, root->right) + 1;
      y->height = avl_max_height(y->left, y->right) + 1;
      x->height = avl_max_height(x->left, x->right) + 1;
      DBG("x=%d,h=%d, y=%d,h=%d", x->key, x->height, y->key, y->height);
      return x;
    } else {
      /* rotate left */
      DBG("Rotate left: root %d, x %d", root->key, y->key);
      root->right = y->left;
      y->left = root;
      root->height = avl_max_height(root->left, root->right) + 1;
      y->height = avl_max_height(y->left, y->right) + 1;
      DBG("y=%d,h=%d", y->key, y->height);
      return y;
    }
  }
  root->height = avl_max_height(root->left, root->right) + 1;
  return root;
}

node_t *avl_delete()
{
  return NULL;
}

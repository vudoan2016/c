#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "include/tree.h"
#include "include/debug.h"

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
  node_t *new, *x, *y;
  if (root == NULL) {
    new = calloc(1, sizeof(node_t));
    new->key = key;
    new->height = 1;
    return new;
  }
  
  if (key < root->key) {
    DBG("insert key %d to the left of %d\n", key, root->key);
    root->left = avl_insert(root->left, key);
  } else {
    DBG("insert key %d to the right of %d, right %d\n", key, root->key,
	root->right ? root->right->key : -1);
    root->right = avl_insert(root->right, key);
  }
  root->height = avl_max_height(root->left, root->right) + 1;
  DBG("insert key %d, root %d, height %d\n", key, root->key, root->height);

  if (root == root->right || root == root->left) {
    assert(0);
  }
  
  
  if ((avl_get_height(root->left) - avl_get_height(root->right)) > 1) {
    y = root->left;
    if (key > root->left->key) {
      /* rotate left */
      x = y->right;
      DBG("Rotate left: root %d, y %d, x %d\n", root->key, y->key, x->key);
      y->right = x->left;
      x->left = y;
      root->left = x;
      
      /* rotate right */
      root->right = x->right;
      x->right = root;
      if (root == root->right || root == root->left ||
	  (y && (root == y->left || root == y->right)) ||
	  (x && (root == x->left || root == x->right))) {
	assert(0);
      }
      return x;
    } else {
      /* rotate right */
      DBG("Rotate right: root %d, y %d\n", root->key, y->key);
      root->left = y->right;
      y->right = root;
      if (root == root->right || root == root->left ||
	  (y && (root == y->left || root == y->right)) ||
	  (x && (root == x->left || root == x->right))) {
	assert(0);
      }
      return y;
    }
  } else if ((avl_get_height(root->right) - avl_get_height(root->left)) > 1) {
    y = root->right;
    if (key < root->right->key) {
      /* rotate right */
      x = y->left;
      DBG("Rotate right: root %d, left %d, right %d, y %d, x %d\n",
	  root->key, root->left ? root->left->key : -1,
	  root->right ? root->right->key : -1, y->key, x->key);
      y->left = x->right;
      x->right = y;
      root->right = x;
      if (root == root->right || root == root->left ||
	  (y && (root == y->left || root == y->right)) ||
	  (x && (root == x->left || root == x->right))) {
	assert(0);
      }
      
      /* rotate left */
      root->right = x->left;
      x->left = root;
      if (root == root->right || root == root->left ||
	  (y && (root == y->left || root == y->right)) ||
	  (x && (root == x->left || root == x->right))) {
	assert(0);
      }

      return x;
    } else {
      /* rotate left */
      DBG("Rotate left: root %d, x %d\n", root->key, y->key);
      root->right = y->left;
      y->left = root;
      if (root == root->right || root == root->left ||
	  (y && (root == y->left || root == y->right)) ||
	  (x && (root == x->left || root == x->right))) {
	assert(0);
      }
      return y;
    }
  }
  if (root == root->right || root == root->left) {
    assert(0);
  }
  
  return root;
}

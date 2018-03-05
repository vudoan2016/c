#include <stdlib.h>
#include <stdio.h>
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

node_t *avl_insert(node_t *root, int key)
{
  node_t *new, *x, *y;
  if (root == NULL) {
    new = calloc(1, sizeof(node_t));
    new->key = key;
    new->height = 1;
    new->left = NULL;
    new->right = NULL;
    return new;
  }
  
  if (key < root->key) {
    root->left = avl_insert(root->left, key);
  } else {
    root->right = avl_insert(root->right, key);
  }
  root->height = avl_max_height(root->left, root->right) + 1;
  DBG("insert key %d, height %d\n", key, root->height);

  if (root->height > 1) {
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
	return x;
      } else {
	/* rotate right */
	DBG("Rotate right: root %d, y %d\n", root->key, y->key);
	root->left = y->right;
	y->right = root;
	return y;
      }
    } else if ((avl_get_height(root->right) - avl_get_height(root->left)) > 1) {
      y = root->right;
      if (key < root->right->key) {
	/* rotate right */
	x = y->left;
	DBG("Rotate right: root %d, y %d, x %d\n", root->key, y->key, x->key);
	y->left = x->right;
	x->right = y;
	root->right = x;

	/* rotate left */
	root->right = x->left;
	x->left = root;
	return x;
      } else {
	/* rotate left */
	DBG("Rotate left: root %d, x %d\n", root->key, y->key);
	root->right = y->left;
	y->left = root;
	return y;
      }
    }
  }
  return root;
}

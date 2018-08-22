#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "include/tree.h"
#include "include/debug.h"

#define DEBUG 0

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
    new_p->height = parent_p->height + 1;
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
  if (root->height == 0) {
    printf("In-order traversal: ");  
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

/* 
 * Todo: find the longest path to a leaf node in a tree assuming that 
 * each node doesn't keep track of the height.
 */
int longest_path(node_t *root)
{
  int left, right;

  if (root == NULL || (root->left == NULL && root->right == NULL)) {
    return 0;
  }
  left = longest_path(root->left);
  right = longest_path(root->right);

  if (left > right)
    return left + 1;
  return right + 1;
}

/* 
 * Traverse the tree by visiting all nodes in each level.
 * BFS the tree and mark the first node of the next level using the index
 * first_node.
 */
void in_level_traverse(node_t *root)
{
  int h = longest_path(root);
  node_t **q;
  int i = 0, j, first_node = 1, level = 0;

  q = (node_t **)calloc(1, sizeof(node_t *) * h * pow(2, h));
  q[i] = root;
  while (level <= h) {
    printf("level %d: ", level);
    j = first_node;
    DBG("i = %d, first_node = %d", i, tail);
    for (; i < first_node; i++) {
      printf("%d, ", q[i]->key);
      if (q[i]->left) {
        q[j] = q[i]->left;
        j++;
      }
      if (q[i]->right) {
        q[j] = q[i]->right;
        j++;
      }
    }
    first_node = j;
    printf("\n");
    level++;
  }
  free(q);
}

/* 
 * Todo: Implement b-tree.
 */
 
void tree_test()
{
  char tree_data[] = "tree_data.txt";
  FILE *fp;
  node_t *root = NULL, *x;
  int key, align = 10;
  
  if ((fp = fopen(tree_data, "r")) == NULL) {
    printf("Unable to open file %s to read\n", tree_data);
    return;
  }
  
  while (!feof(fp)) {
    if (fscanf(fp, "%d", &key) == 1) {
      tree_insert(&root, key);
      /*root = avl_insert(root, key);*/
    }
  }

  tree_print(root);
  printf("\n");

  key = 20;
  printf("1. Searching\n");
  printf("%*d %-*s\n", align, key, align, 
        tree_search(root, key) == true ? "found" : "not found");
  key = 54;
  printf("%*d %-*s\n", align, key, align, 
        tree_search(root, key) == true ? "found" : "not found");
  key = 6;
  printf("%*d %-*s\n", align, key, align, 
        tree_search(root, key) == true ? "found" : "not found");
  printf("\n");

  printf("2. Tree is %s\n", tree_is_bst(root) ? "bst" : "not bst");
  printf("\n");

  printf("3. Max height = %d\n", longest_path(root));
  printf("\n");

  printf("4. Level BFS\n");
  in_level_traverse(root);
  printf("\n");

  printf("4. Deletion\n");
  while (x = root) {
    DBG("Deleting key %d:", x->key);
    tree_delete(&root, x->key);
  }
  tree_print(root);
  
  fclose(fp);
}


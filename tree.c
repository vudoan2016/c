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
 * left < root < right
 */
static void tree_insert(node_t **root, int data)
{
  node_t *node_p, *parent_p = NULL, *new_p = calloc(1, sizeof(node_t));

  new_p->data = data;
  new_p->left = NULL;
  new_p->right = NULL;

  if (*root == NULL) {
    *root = new_p;
    return;
  }

  node_p = *root;
  while (node_p) {
    parent_p = node_p;
    node_p = (data < node_p->data ? node_p->left : node_p->right);
  }

  if (data < parent_p->data) {
    parent_p->left = new_p;
  } else {
    parent_p->right = new_p;
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
  printf("%d ", root->data);
  tree_print(root->right);
}

bool binary_search(node_t *root, int key)
{
  node_t *n = root;

  while (n && n->data != key) {
    n = key < n->data ? n->left : n->right;
  }
  return (n != NULL);
}

void tree_test()
{
  char *tree_data = "tree_data.txt";
  FILE *fp;
  node_t *root = NULL;
  int data;
  
  if ((fp = fopen(tree_data, "r")) == NULL) {
    printf("Unable to open file %s to read\n", tree_data);
    return;
  }

  while (!feof(fp)) {
    if (fscanf(fp, "%d", &data) == 1) {
      tree_insert(&root, data);
    }
  }

  tree_print(root);
  printf("\n");
  data = 20;
  printf("%d is %s\n", data,
	 binary_search(root, data) == true ? "found" : "not found");
  fclose(fp);
}


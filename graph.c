#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "graph.h"
#include "queue.h"

static void add_vertex(vertex_t adj[][MAX_VERTICES], int x, int y)
{
  printf("%s: adding vertex %d, %d\n", __FUNCTION__, x, y);
  adj[x][y].weight = 1;
  adj[x][0].val = x;
  adj[x][0].visited = false;
}

static void remove_vertex(vertex_t adj[][MAX_VERTICES], int x, int y)
{
  adj[x][y].weight = 0;
  adj[x][y].weight = 0;
  adj[x][0].val = 0;
}

static void bfs(vertex_t adj[][MAX_VERTICES], vertex_t *v)
{
  int i, val;
  queue_t *q;

  enqueue(q, v->val);
  while (!queue_is_empty(q)) {
    val = dequeue(q);
    adj[val][0].visited = true;
    for (i = 0; i < MAX_VERTICES; i++) {
      if (adj[val][i].weight && !adj[i][0].visited) {
	printf("Enqueueing neighbor %d\n", i);
	enqueue(q, i);
      }
    }
    printf("%d\n", val);
  }

  for (i = 0; i < MAX_VERTICES; i++) {
    adj[i][0].visited = false;
  }
}

void graph_test()
{
  vertex_t adj[MAX_VERTICES][MAX_VERTICES];
  char *data_file = "graph_data.txt";
  FILE *fp;
  int x, y, i, j;
  
  memset(adj[0], 0, sizeof(vertex_t)*MAX_VERTICES*MAX_VERTICES);

  if ((fp = fopen(data_file, "r")) == NULL) {
    printf("Unable to open file %s\n", data_file);
  }

  while (!feof(fp)) {
    if (fscanf(fp, ("%d, %d"), &x, &y) == 2) {
      add_vertex(adj, x, y);
    }
  }

  for (i = 0; i < MAX_VERTICES; i++) {
    for (j = 0; j < MAX_VERTICES; j++) {
      if (adj[i][j].weight) {
	printf("vertex %d, neighbor %d\n", adj[i][0].val, j);
      }
    }
  }

  bfs(adj, &adj[2][0]);

  fclose(fp);
}

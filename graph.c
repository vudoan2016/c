#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "queue.h"

static void add_vertex(vertex_t adj[][MAX_VERTICES], int x, int y)
{
  adj[x][y].weight = 1;
  adj[x][0].id = x;
  adj[x][0].visited = false;
}

static void remove_vertex(vertex_t adj[][MAX_VERTICES], int x, int y)
{
  adj[x][y].weight = 0;
  adj[x][0].id = 0;
  adj[x][0].visited = false;
}

static void bfs(vertex_t adj[][MAX_VERTICES], int vertex)
{
  int i, v;
  queue_t *q = calloc(1, sizeof(queue_t));

  printf("BFS from source %d\n", vertex);
  queue_init(q);
  enqueue(q, vertex);
  adj[vertex][0].visited = true;
  while (!queue_is_empty(q)) {
    v = dequeue(q);
    for (i = 0; i < MAX_VERTICES; i++) {
      if (adj[v][i].weight && !adj[i][0].visited) {
	enqueue(q, i);
	adj[i][0].visited = true;
      }
    }
    printf("%d\n", v);
  }

  for (i = 0; i < MAX_VERTICES; i++) {
    adj[i][0].visited = false;
  }

  free(q);
}

void graph_test()
{
  vertex_t adj[MAX_VERTICES][MAX_VERTICES];
  char *data_file = "graph_data.txt";
  FILE *fp;
  int x, y;

  memset(adj[0], 0, sizeof(vertex_t)*MAX_VERTICES*MAX_VERTICES);

  if ((fp = fopen(data_file, "r")) == NULL) {
    printf("Unable to open file %s\n", data_file);
  }

  while (!feof(fp)) {
    if (fscanf(fp, ("%d, %d"), &x, &y) == 2) {
      add_vertex(adj, x, y);
    }
  }

  bfs(adj, 2);
  bfs(adj, 1);
  bfs(adj, 0);
  bfs(adj, 3);
  
  fclose(fp);
}

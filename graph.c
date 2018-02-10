#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "queue.h"
#include "heap.h"

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

void find_shortest_path(adj_list_vertex_t *adj[MAX_VERTICES], int source)
{
  int i, new_dist;
  heap_t *h;
  heap_data_t data, *data_p;
  adj_list_vertex_t *neighbor;
  
  for (i = 0; i < MAX_VERTICES; i++) {
    if (adj[i]) {
      adj[i]->visited = false;
      neighbor = adj[i]->neighbor;
      while (neighbor) {
	neighbor->dist = INFINITY;
	neighbor = neighbor->neighbor;
      }
    }
  }

  h = calloc(1, sizeof(heap_t));
  heap_init(h);
  adj[source]->dist = 0;
  data.key = adj[source]->dist;
  data.element = source;
  heap_insert(h, &data);

  while (heap_is_empty(h) == false) {
    data_p = heap_delete(h);
    printf("min dist %d, vertex %d\n", data_p->key, data_p->element);
    neighbor = adj[data_p->element]->neighbor;
    while (neighbor) {
      new_dist = neighbor->dist == INFINITY ? INFINITY : neighbor->dist + neighbor->weight;
      if (new_dist < neighbor->dist) {
	neighbor->dist = new_dist;
	adj[neighbor->vertex]->predecessor = data_p->element;
      }
      if (neighbor->visited == false) {
	data.key = neighbor->dist;
	data.element = neighbor->vertex;
	heap_insert(h, &data);
      }
      neighbor = neighbor->neighbor;
    }
    adj[data_p->element]->visited = true;
  }
  heap_destroy(h);
  free(h);
}

void print_shortest_path(adj_list_vertex_t *adj[MAX_VERTICES], int dest)
{
  int i = dest;
  
  printf("predecessor from destination %d: ", dest);

  while (adj[i]->predecessor != adj[i]->vertex) {
    printf("%d, ", adj[i]->predecessor);
    i = adj[i]->predecessor;
  }
  printf("\n");
}

void adj_list_add_vertex(adj_list_vertex_t *adj[MAX_VERTICES], int vertex,
			int neighbor, int weight)
{
  adj_list_vertex_t *neighbor_p;

  if (adj[vertex] == NULL) {
    adj[vertex] = calloc(1, sizeof(adj_list_vertex_t));
    adj[vertex]->vertex = vertex;
  }

  neighbor_p = calloc(1, sizeof(adj_list_vertex_t));
  neighbor_p->neighbor = adj[vertex]->neighbor;
  neighbor_p->vertex = neighbor;
  neighbor_p->weight = weight;
  neighbor_p->predecessor = vertex;
  adj[vertex]->neighbor = neighbor_p;
}

static void adj_list_print(adj_list_vertex_t *adj[MAX_VERTICES])
{
  int i;
  adj_list_vertex_t *neighbor;
  
  for (i = 0; i < MAX_VERTICES; i++) {
    if (adj[i] != NULL) {
      printf("Vertex: %d, neighbors at ", adj[i]->vertex);
      neighbor = adj[i]->neighbor;
      while (neighbor) {
	printf("vertex %d, weight %d\n", neighbor->vertex, neighbor->weight);
	neighbor = neighbor->neighbor;
      }
    }
    printf("\n");
  }
}

static void
adj_list_remove_vertex(adj_list_vertex_t *adj[MAX_VERTICES], int vertex)
{
}

void graph_test()
{
  vertex_t adj[MAX_VERTICES][MAX_VERTICES];
  char *data_file = "graph_data.txt";
  char *data_file_1 = "graph_data1.txt";
  FILE *fp;
  int x, y, weight;
  adj_list_vertex_t *adj_list[MAX_VERTICES] = {NULL};
  
  memset(adj[0], 0, sizeof(vertex_t)*MAX_VERTICES*MAX_VERTICES);

  if ((fp = fopen(data_file, "r")) == NULL) {
    printf("Unable to open file %s\n", data_file);
    return;
  }

  while (!feof(fp)) {
    if (fscanf(fp, ("%d, %d"), &x, &y) == 2) {
      add_vertex(adj, x, y);
    }
  }

#if 0
  bfs(adj, 2);
  bfs(adj, 1);
  bfs(adj, 0);
  bfs(adj, 3);
#endif
  fclose(fp);

  if ((fp = fopen(data_file_1, "r")) == NULL) {
    printf("Unable to open file %s\n", data_file_1);
    return;
  }

  while (!feof(fp)) {
    if (fscanf(fp, "%d, %d, %d", &x, &y, &weight) == 3) {
      adj_list_add_vertex(adj_list, x, y, weight);
    }
  }
  adj_list_print(adj_list);

  find_shortest_path(adj_list, 1);
  print_shortest_path(adj_list, 5);
  
  fclose(fp);
}

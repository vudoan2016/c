#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "graph.h"
#include "queue.h"
#include "heap.h"
#include "stack.h"

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

/*
 * Find shortest paths from source to other vertices in the graph using 
 * Dijkstra's algorithm.
 */
void find_shortest_path(adj_list_t *adj[MAX_VERTICES], int source)
{
  int i, new_dist;
  heap_t *h;
  heap_node_t node, *node_p;
  neighbor_t *neighbor;
  
  for (i = 0; i < MAX_VERTICES; i++) {
    if (adj[i]) {
      adj[i]->visited = false;
      adj[i]->dist = INFINITY;
    }
  }

  h = calloc(1, sizeof(heap_t));
  heap_init(h);
  adj[source]->dist = 0;
  node.key = adj[source]->dist;
  node.data = source;
  heap_insert(h, &node);

  while (heap_is_empty(h) == false) {
    node_p = heap_delete(h);
    if (adj[node_p->data]->visited == false) {
      neighbor = adj[node_p->data]->neighbor;
      adj[node_p->data]->visited = true;
      while (neighbor) {
	new_dist = adj[node_p->data]->dist + neighbor->weight;
	if (new_dist < adj[neighbor->vertex]->dist) {
	  adj[neighbor->vertex]->dist = new_dist;
	  adj[neighbor->vertex]->previous = node_p->data;
	}
	if (adj[neighbor->vertex]->visited == false) {
	  node.key = adj[neighbor->vertex]->dist;
	  node.data = neighbor->vertex;
	  heap_insert(h, &node);
	}
	neighbor = neighbor->next;
      }
    }
  }
  heap_destroy(h);
  free(h);
}

void print_shortest_path(adj_list_t *adj[MAX_VERTICES])
{
  int i, dst;

  for (dst = 0; dst < MAX_VERTICES; dst++) {
    if (adj[dst] && adj[dst]->dist) {
      printf("path from destination %d: ", dst);
      
      i = adj[dst]->previous;
      while (adj[i]->dist != 0) {
	printf("%d, ", i);
	i = adj[i]->previous;
      }
      printf("%d\n", i);
    }
  }
}

void adj_list_add_vertex(adj_list_t *adj[MAX_VERTICES], int vertex,
			int neighbor, int weight)
{
  neighbor_t *neighbor_p;

  if (adj[vertex] == NULL) {
    adj[vertex] = calloc(1, sizeof(adj_list_t));
    adj[vertex]->vertex = vertex;
  }

  neighbor_p = calloc(1, sizeof(neighbor_t));
  neighbor_p->next = adj[vertex]->neighbor;
  neighbor_p->vertex = neighbor;
  neighbor_p->weight = weight;
  adj[vertex]->neighbor = neighbor_p;
}

static void adj_list_print(adj_list_t *adj[MAX_VERTICES])
{
  int i;
  neighbor_t *neighbor;
  
  for (i = 0; i < MAX_VERTICES; i++) {
    if (adj[i] != NULL) {
      printf("Vertex: %d, neighbors at ", adj[i]->vertex);
      neighbor = adj[i]->neighbor;
      while (neighbor) {
	printf("(vertex %d, weight %d), ", neighbor->vertex, neighbor->weight);
	neighbor = neighbor->next;
      }
      printf("\n");
    }
  }
}

static void
adj_list_remove_vertex(adj_list_t *adj[MAX_VERTICES], int vertex)
{
  neighbor_t *tmp;
  neighbor_t *neighbor_p = adj[vertex]->neighbor;

  while (neighbor_p) {
    tmp = neighbor_p;
    neighbor_p = neighbor_p->next;
    free(tmp);
  }
  free(adj[vertex]);
  adj[vertex] = NULL;
}

static void
adj_list_destroy(adj_list_t *adj[MAX_VERTICES])
{
  int i;
  for (i = 0; i < MAX_VERTICES; i++) {
    if (adj[i]) {
      adj_list_remove_vertex(adj, i);
    }
  }
}

static void
adj_list_dfs(adj_list_t *adj[MAX_VERTICES], int source, int dst)
{
  stack_t *st;
  neighbor_t *neighbor_p;
  int v;
  st = calloc(1, sizeof(stack_t));

  stack_init(st);
  for (v = 0; v < MAX_VERTICES; v++) {
    if (adj[v]) {
      adj[v]->visited = false;
    }
  }
  
  v = source;
  while (v != dst) {
    neighbor_p = adj[v]->neighbor;
    while (neighbor_p && adj[neighbor_p->vertex]->visited == true) {
      neighbor_p = neighbor_p->next;
    }
    if (neighbor_p && adj[neighbor_p->vertex]->visited == false) {
      DBG("push %d\n", neighbor_p->vertex);
      v = neighbor_p->vertex;
      adj[v]->visited = true;
      stack_push(st, neighbor_p->vertex);
    } else if (stack_is_empty(st) == false) {
      v = stack_pop(st);
      DBG("pop %d\n", v);
    } else {
      break;
    }
  }

  if (v == dst) {
    while (stack_is_empty(st) == false) {
      v = stack_pop(st);
      printf("%d, ", v);
    }
    printf("\n");
  }
  free(st);
}

void graph_test()
{
  vertex_t adj[MAX_VERTICES][MAX_VERTICES];
  char *data_file = "graph_data.txt";
  char *data_file_1 = "graph_data1.txt";
  FILE *fp;
  int x, y, weight, source, dst;
  adj_list_t *adj_list[MAX_VERTICES] = {NULL};
  
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

  for (source = 1; source < MAX_VERTICES; source++) {
    if (adj_list[source]) {
      find_shortest_path(adj_list, source);
      print_shortest_path(adj_list);
    }
  }

  source = 1;
  dst = 4;
  adj_list_dfs(adj_list, source, dst);
  
  adj_list_destroy(adj_list);  
  fclose(fp);
}

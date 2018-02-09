#ifndef __GRAPH_H
#define __GRAPH_H

#define MAX_VERTICES 10
#define INFINITY     0x7FFFFFFF

typedef struct vertex_s {
  int weight;
  bool visited;
  int id;
} vertex_t;

typedef struct adj_list_vertex_s {
  int vertex;
  int weight;
  int dist;
  int predecessor;
  bool visited;
  struct adj_list_vertex_s *neighbor;
} adj_list_vertex_t;

#endif

#ifndef __GRAPH_H
#define __GRAPH_H

#define MAX_VERTICES 10
#define INFINITY     0x7FFFFFFF

/*
 * 2D array representation of a graph.
 */
typedef struct vertex_s {
  int vertex;
  int weight;
  bool visited;
  int id;
} vertex_t;
  
/* 
 * Adjacency list representation of a graph.
 */
typedef struct neighbor_s {
  int weight;
  int vertex;
  struct neighbor_s *next;
} neighbor_t;

typedef struct adj_list_s {
  int vertex;
  int dist;
  bool visited;
  int previous;
  neighbor_t *neighbor;
} adj_list_t;

#endif

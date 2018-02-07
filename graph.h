#ifndef __GRAPH_H
#define __GRAPH_H

#define MAX_VERTICES 10

typedef struct vertex_s {
  int weight;
  bool visited;
  int id;
} vertex_t;

#endif

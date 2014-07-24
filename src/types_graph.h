/*
 * types_graph.h
 *
 *  Created on: Feb 23, 2009
 *      Author: LIU Lu
 */

#ifndef TYPES_GRAPH_H_
#define TYPES_GRAPH_H_

#define NNULL (Vertex*)NULL

typedef struct edge_st
{
	char id[128]; /* unique id */
	int cost; /* = length * weight */
	struct vertex_st *start; /* from vertex */
	struct vertex_st *end; /* to vertex */
} Edge;

typedef struct vertex_st
{
	char id[32]; /* unique id */
	int distance; /* tentative distance value */
	struct vertex_st *parent; /* successor vertex in the SPT */
	int outdegree;
	int first; /* array index of the first outgoing edge */
	int status;
	char attr_amenity[64]; /* attribute of amenity */
	/* heap memory */
	struct vertex_st *heap_parent; /* heap parent pointer */
	struct vertex_st *son; /* heap successor */
	struct vertex_st *next; /* next brother */
	struct vertex_st *prev; /* previous brother */
	long deg; /* number of children */
} Vertex;

typedef struct path_recorder_st
{
	char id[32];
	int distance;
	char parent_id[32];
} PathRecorder;

typedef struct graph_st
{
	char id[32]; /* mode id of graph */
	Vertex **vertices; /* vertex set */
	int vertexCount; /* number of vertices */
	Edge **edges; /* edge set */
	int edgeCount; /* number of edges */
	struct graph_st *next; /* next graph in the linked list */
} Graph;

#endif /* TYPES_GRAPH_H_ */

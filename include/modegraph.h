/*
 * =====================================================================================
 *
 *       Filename:  modegraph.h
 *
 *    Description:  ADT declarations of multimodal graph and its components
 *
 *        Version:  1.0
 *        Created:  2009/02/23 10时44分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#ifndef MODEGRAPH_H_
#define MODEGRAPH_H_
#include <stdint.h>

#define NNULL (Vertex*)NULL
#define VERY_FAR 1073741823

typedef struct Edge Edge;
typedef struct Vertex Vertex;
typedef struct ModeGraph ModeGraph;
typedef struct SwitchPoint SwitchPoint;

struct Edge
{
	int     mode_id;
	double  length;
	double  length_factor;
	double  speed_factor;
	int64_t from_vertex_id;
	int64_t to_vertex_id;
	Edge    *adjNext;
};

struct Vertex
{
	int64_t id; /* unique id */
	double  temp_cost; /* tentative distance value */
	double  distance;
	double  elapsed_time;
	double  walking_distance;
	double  walking_time;
	Vertex  *parent; /* successor vertex in the SPT */
	int     outdegree;
	Edge    *outgoing; /* first outgoing edge in adjacency edge list */
	int     status;
	Vertex  *next; /* next vertex in queue */
};

struct ModeGraph
{
	int    id; /* mode id of graph */
	Vertex **vertices; /* vertex set */
	int    vertex_count; /* number of vertices */
	int    edge_count; /* number of edges */
};

struct SwitchPoint
{
	int64_t from_vertex_id;
	int64_t to_vertex_id;
	double  length;
	double  length_factor;
	double  speed_factor;
};

#endif /* MODEGRAPH_H_ */

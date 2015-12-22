/*
 * =====================================================================================
 *
 *       Filename:  modegraph.h
 *
 *    Description:  ADT declarations of multimodal graph and its components
 *
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
#include <stdlib.h>
#include <string.h>

#define NNULL (Vertex*)NULL
#define VERY_FAR 1073741823
/* TOTAL_MODES is 7 instead of 9 because so far the TAXI mode is not considered, 
 * and the public transport is actually a abstract mode consisting of UNDERGROUND, 
 * SUBURBAN, TRAM and BUS */
#define TOTAL_MODES 7

typedef struct Edge Edge;
typedef struct Vertex Vertex;
typedef struct ModeGraph ModeGraph;
typedef struct SwitchPoint SwitchPoint;
typedef enum MODES MODES;

/* Must be consistent everywhere in the whole multimodal routing application,
 * as defined in the external db:
 *
 *        mode_name       | mode_id 
 * -----------------------+---------
 * private_car           |      11
 * foot                  |      12
 * underground           |      13
 * suburban              |      14
 * tram                  |      15
 * bus                   |      16
 * public_transportation |      19
 * bicycle               |      17
 * taxi                  |      18
 *
 */
enum MODES {
    PRIVATE_CAR           = 11,
    FOOT                  = 12,
    UNDERGROUND           = 13,
    SUBURBAN              = 14,
    TRAM                  = 15,
    BUS                   = 16,
    BICYCLE               = 17,
    TAXI                  = 18,
    PUBLIC_TRANSPORTATION = 19
};

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
	/* TODO: how about other types of cost along the path, e.g. gas, energy,
	 * battery, fatigue, etc.? Here the ideal way is to make such metrics
	 * configurable. */
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

Vertex *SearchVertexById(Vertex **vertexArray, int len, int64_t id);
Vertex *BinarySearchVertexById(Vertex **vertexArray, int low, int high, int64_t id);

#endif /* MODEGRAPH_H_ */

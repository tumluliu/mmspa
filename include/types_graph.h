/*
 * types_graph.h
 *
 *  Created on: Feb 23, 2009
 *      Author: LIU Lu
 */

#ifndef TYPES_GRAPH_H_
#define TYPES_GRAPH_H_

#define NNULL (Vertex*)NULL
#define VERY_FAR 1073741823

typedef struct edge_st
{
//	long long id; /* unique id */
	int mode_id;
	double length;
	double length_factor;
	double speed_factor;
//	struct vertex_st *from_vertex; /* from vertex */
//	struct vertex_st *to_vertex; /* to vertex */
	long long from_vertex_id;
	long long to_vertex_id;
	struct edge_st *adjNext;
} Edge;

typedef struct vertex_st
{
	long long id; /* unique id */
	double temp_cost; /* tentative distance value */
	double distance;
	double elapsed_time;
	double walking_distance;
	double walking_time;
	struct vertex_st *parent; /* successor vertex in the SPT */
	int outdegree;
//	int first; /* array index of the first outgoing edge */
	struct edge_st *outgoing; /* first outgoing edge in adjacency edge list */
	int status;
/*	char attr_amenity[64];  attribute of amenity */
	/* heap memory */
//	struct vertex_st *heap_parent; /* heap parent pointer */
//	struct vertex_st *son; /* heap successor */
	struct vertex_st *next; /* next vertex in queue */
//	struct vertex_st *prev; /* previous brother */
//	long deg; /* number of children */
} Vertex;

typedef struct path_recorder_st
{
	long long vertex_id;
//	double cost;
	long long parent_vertex_id;
} PathRecorder;

typedef struct path_st
{
	long long* vertex_list;
	int vertex_list_length;
} Path;	

typedef struct graph_st
{
	int id; /* mode id of graph */
	Vertex** vertices; /* vertex set */
	int vertex_count; /* number of vertices */
//	Edge** edges; /* edge set */
	int edge_count; /* number of edges */
	struct graph_st *next; /* next graph in the linked list */
} Graph;

typedef struct switchpoint_st
{
//	long long switch_point_id;
	long long from_vertex_id;
	long long to_vertex_id;
	double length;
	double length_factor;
	double speed_factor;
} SwitchPoint;

// This is a callback function should be defined by the client
typedef int (*VertexValidationChecker)(Vertex* vertexToCheck); 

typedef struct multimodal_routing_plan_st
{
	int* mode_id_list;
	int mode_count;
	int* public_transit_mode_id_set;
	int public_transit_mode_count;
	const char** switch_condition_list;
	VertexValidationChecker* switch_constraint_list;
	VertexValidationChecker target_constraint;
	const char* cost_factor;
} MultimodalRoutingPlan;

#endif /* TYPES_GRAPH_H_ */

/*
 * =====================================================================================
 *
 *       Filename:  routingplan.h
 *
 *    Description:  Multimodal routing plan data structures
 *
 *        Version:  1.0
 *        Created:  2015/12/18 11时58分31秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#ifndef  ROUTINGPLAN_INC
#define  ROUTINGPLAN_INC
#include "modegraph.h"

typedef struct RoutingPlan RoutingPlan;
/* This is a callback function should be defined by the client */
typedef int (*VertexValidationChecker)(Vertex* vertexToCheck); 

struct RoutingPlan
{
	int                     *mode_id_list;
	int                     mode_count;
	int                     *public_transit_mode_id_set;
	int                     public_transit_mode_count;
	const char              **switch_condition_list;
	VertexValidationChecker *switch_constraint_list;
	VertexValidationChecker target_constraint;
	const char              *cost_factor;
};

#endif   /* ----- #ifndef ROUTINGPLAN_INC  ----- */

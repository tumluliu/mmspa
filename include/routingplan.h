/*
 * =====================================================================================
 *
 *       Filename:  routingplan.h
 *
 *    Description:  Multimodal routing plan data structures
 *
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

#include <stdlib.h>
#include "modegraph.h"

typedef struct RoutingPlan RoutingPlan;
/* This is a callback function for checking the validity of a vertex should be 
 * defined by the client */
typedef int (*VertexValidationChecker)(Vertex vertexToCheck); 

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

/* v2.x API */
void MSPcreateRoutingPlan(int modeCount, int publicModeCount);
void MSPsetMode(int index, int modeId);
void MSPsetPublicTransit(int index, int modeId);
void MSPsetSwitchCondition(int index, const char *spCondition);
void MSPsetSwitchConstraint(int index, VertexValidationChecker callback);
void MSPsetTargetConstraint(VertexValidationChecker callback);
void MSPsetCostFactor(const char *costFactor);
void MSPclearRoutingPlan();

/* v1.x API */
void CreateRoutingPlan(int modeCount, int publicModeCount);
void SetModeListItem(int index, int modeId);
void SetPublicTransitModeSetItem(int index, int modeId);
void SetSwitchConditionListItem(int index, const char *spCondition);
void SetSwitchingConstraint(int index, VertexValidationChecker callback);
void SetTargetConstraint(VertexValidationChecker callback);
void SetCostFactor(const char *costFactor);

#endif   /* ----- #ifndef ROUTINGPLAN_INC  ----- */

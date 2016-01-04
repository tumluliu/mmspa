/*
 * =====================================================================================
 *
 *       Filename:  routingplan.c
 *
 *    Description:  Implementations of multimodal routing plan processing
 *
 *        Created:  2015/12/21 17时24分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#include "../include/routingplan.h"

RoutingPlan *plan = NULL;

void MSPcreateRoutingPlan(int modeCount, int publicModeCount) {
	plan = (RoutingPlan *) malloc(sizeof(RoutingPlan));
	plan->mode_count = modeCount;
	plan->public_transit_mode_count = publicModeCount;
	plan->mode_id_list = (int *) calloc(modeCount, sizeof(int));
	plan->target_constraint = (VertexValidationChecker)NULL;
	if (modeCount > 1) {
		plan->switch_condition_list = (char **) calloc(modeCount - 1, 
		        sizeof(char *));
		plan->switch_constraint_list = (VertexValidationChecker *) calloc(
		        modeCount - 1, sizeof(VertexValidationChecker));
	}
	if (publicModeCount > 0)
		plan->public_transit_mode_id_set = (int *) calloc(publicModeCount, 
		        sizeof(int));
}

void MSPsetMode(int index, int modeId) {
	plan->mode_id_list[index] = modeId;
}

void MSPsetPublicTransit(int index, int modeId) {
	plan->public_transit_mode_id_set[index] = modeId;
}

void MSPsetSwitchCondition(int index, const char *spCondition) {
    plan->switch_condition_list[index] = (char *) calloc(strlen(spCondition)+1, 
            sizeof(char));
	strcpy(plan->switch_condition_list[index], spCondition);
}

void MSPsetSwitchConstraint(int index, VertexValidationChecker callback) {
	plan->switch_constraint_list[index] = callback;
}

void MSPsetTargetConstraint(VertexValidationChecker callback) {
	plan->target_constraint = callback;
}

void MSPsetCostFactor(const char *costFactor) {
    plan->cost_factor = (char *) calloc(strlen(costFactor)+1, sizeof(char));
	strcpy(plan->cost_factor, costFactor);
}

/* v1.x API, for compatibility */
void CreateRoutingPlan(int modeCount, int publicModeCount) {
    MSPcreateRoutingPlan(modeCount, publicModeCount);
}

void SetModeListItem(int index, int modeId) {
	MSPsetMode(index, modeId);
}

void SetPublicTransitModeSetItem(int index, int modeId) {
	MSPsetPublicTransit(index, modeId);
}

void SetSwitchConditionListItem(int index, const char *spCondition) {
	MSPsetSwitchCondition(index, spCondition);
}

void SetSwitchingConstraint(int index, VertexValidationChecker callback) {
	MSPsetSwitchConstraint(index, callback);
}

void SetTargetConstraint(VertexValidationChecker callback) {
	MSPsetTargetConstraint(callback);
}

void SetCostFactor(const char *costFactor) {
	MSPsetCostFactor(costFactor);
}

void MSPclearRoutingPlan() {
	if (plan->mode_count > 1) {
	    for (int i = 0; i < plan->mode_count - 1; i++)
	        free(plan->switch_condition_list[i]);
		free(plan->switch_condition_list);
		free(plan->switch_constraint_list);
	}
	if (plan->public_transit_mode_count > 1) {
	    free(plan->public_transit_mode_id_set);
	    plan->public_transit_mode_id_set = NULL;
    }
    free(plan->cost_factor);
    plan->cost_factor = NULL;
	plan->switch_condition_list = NULL;
	plan->switch_constraint_list = NULL;
	plan->target_constraint = NULL;
	free(plan->mode_id_list);
	plan->mode_id_list = NULL;
	free(plan);
	plan = NULL;
}

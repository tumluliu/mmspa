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
	plan->modes = (int *) calloc(modeCount, sizeof(int));
	plan->target_constraint = NULL;
	if (modeCount > 1) {
		plan->switch_conditions = (char **) calloc(modeCount - 1, 
		        sizeof(char *));
		plan->switch_constraints = (VertexValidationChecker *) calloc(
		        modeCount - 1, sizeof(VertexValidationChecker));
	}
	if (publicModeCount > 0)
		plan->public_transit_modes = (int *) calloc(publicModeCount, 
		        sizeof(int));
}

void MSPsetMode(int index, int modeId) {
	plan->modes[index] = modeId;
}

void MSPsetPublicTransit(int index, int modeId) {
	plan->public_transit_modes[index] = modeId;
}

void MSPsetSwitchCondition(int index, const char *spCondition) {
    plan->switch_conditions[index] = (char *) calloc(strlen(spCondition)+1, 
            sizeof(char));
	strcpy(plan->switch_conditions[index], spCondition);
}

void MSPsetSwitchConstraint(int index, VertexValidationChecker callback) {
	plan->switch_constraints[index] = callback;
}

void MSPsetTargetConstraint(VertexValidationChecker callback) {
	plan->target_constraint = callback;
}

void MSPsetCostFactor(const char *costFactor) {
    plan->cost_factor = (char *) calloc(strlen(costFactor)+1, sizeof(char));
	strcpy(plan->cost_factor, costFactor);
}

void MSPclearRoutingPlan() {
	if (plan->mode_count > 1) {
	    for (int i = 0; i < plan->mode_count - 1; i++)
	        free(plan->switch_conditions[i]);
		free(plan->switch_conditions);
		free(plan->switch_constraints);
	}
	if (plan->public_transit_mode_count > 1) {
	    free(plan->public_transit_modes);
	    plan->public_transit_modes = NULL;
    }
    free(plan->cost_factor);
    plan->cost_factor = NULL;
	plan->switch_conditions = NULL;
	plan->switch_constraints = NULL;
	plan->target_constraint = NULL;
	free(plan->modes);
	plan->modes = NULL;
	free(plan);
	plan = NULL;
}

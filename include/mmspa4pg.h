#ifndef MMSPA4PG_H_
#define MMSPA4PG_H_

#include <string.h>
#include <stdlib.h>
#include "parser.h"

void CreateRoutingPlan(int modeCount, int publicModeCount);

void SetModeListItem(int index, int modeId);

void SetPublicTransitModeSetItem(int index, int modeId);

void SetSwitchConditionListItem(int index, const char* spCondition);

void SetSwitchingConstraint(int index, VertexValidationChecker callback);

void SetTargetConstraint(VertexValidationChecker callback);

void SetCostFactor(const char* costFactor);

void MultimodalTwoQ(long long source);

Path** GetFinalPath(long long source, long long target);

double GetFinalCost(long long target, const char* costField);

void DisposePaths(Path** paths);

void Dispose();

#endif /*MMSPA4PG_H_*/

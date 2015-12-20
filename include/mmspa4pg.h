/*
 * =====================================================================================
 *
 *       Filename:  mmspa4pg.h
 *
 *    Description:  Exported API of multimodal shortest path algorithms library
 *
 *        Version:  1.0
 *        Created:  2015/12/16 10时35分19秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */
#ifndef MMSPA4PG_H_
#define MMSPA4PG_H_

#include <string.h>
#include <stdlib.h>
#include "graphassembler.h"
#include "pathinfo.h"
#include "routingplan.h"

void CreateRoutingPlan(int modeCount, int publicModeCount);

void SetModeListItem(int index, int modeId);

void SetPublicTransitModeSetItem(int index, int modeId);

void SetSwitchConditionListItem(int index, const char *spCondition);

void SetSwitchingConstraint(int index, VertexValidationChecker callback);

void SetTargetConstraint(VertexValidationChecker callback);

void SetCostFactor(const char *costFactor);

void MultimodalTwoQ(int64_t source);

Path **GetFinalPath(int64_t source, int64_t target);

double GetFinalCost(int64_t target, const char *costField);

void DisposePaths(Path **paths);

void Dispose();

#endif /*MMSPA4PG_H_*/

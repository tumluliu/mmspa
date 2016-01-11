/*
 * =====================================================================================
 *
 *       Filename:  mmspa4pg.h
 *
 *    Description:  Exported API of multimodal shortest path algorithms library
 *
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

#include "routingplan.h"
#include "routingresult.h"

/*
 * v2.x API
 */

/* Function of initializing the library, preparing and caching mode graph data */
extern int MSPinit(const char *pgConnStr);
/* Functions of creating multimodal routing plan */
extern void MSPcreateRoutingPlan(int modeCount, int publicModeCount);
extern void MSPsetMode(int index, int modeId);
extern void MSPsetPublicTransit(int index, int modeId);
extern void MSPsetSwitchCondition(int index, const char *spCondition);
extern void MSPsetSwitchConstraint(int index, VertexValidationChecker callback);
extern void MSPsetTargetConstraint(VertexValidationChecker callback);
extern void MSPsetCostFactor(const char *costFactor);
/* Function of assembling multimodal graph set for each routing plan */
extern int MSPassembleGraphs();
/* Functions of finding multimodal shortest paths */
extern Path **MSPfindPath(int64_t source, int64_t target);
extern void MSPtwoq(int64_t source);
/* Functions of fetching and releasing the path planning results */
extern Path **MSPgetFinalPath(int64_t source, int64_t target);
extern double MSPgetFinalCost(int64_t target, const char *costField);
extern void MSPclearPaths(Path **paths);
/* Function of disposing the library memory */
extern void MSPclearGraphs();
extern void MSPclearRoutingPlan();
extern void MSPfinalize();

#endif /*MMSPA4PG_H_*/

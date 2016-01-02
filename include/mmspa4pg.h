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
extern void MSPclearActiveGraphs();
extern void MSPclearRoutingPlan();
extern void MSPfinalize();

/*
 * v1.x API
 */

/* Functions of creating multimodal routing plan */
void CreateRoutingPlan(int modeCount, int publicModeCount);
void SetModeListItem(int index, int modeId);
void SetPublicTransitModeSetItem(int index, int modeId);
void SetSwitchConditionListItem(int index, const char *spCondition);
void SetSwitchingConstraint(int index, VertexValidationChecker callback);
void SetTargetConstraint(VertexValidationChecker callback);
void SetCostFactor(const char *costFactor);
int Parse(); 
int ConnectDB(const char *pgConnStr);
void DisconnectDB();
/* Functions of multimodal shortest path algorithms */
void MultimodalTwoQ(int64_t source);
/* Functions of fetching and releasing the path planning results */
Path **GetFinalPath(int64_t source, int64_t target);
double GetFinalCost(int64_t target, const char *costField);
void DisposePaths(Path **paths);
/* Function of releasing the memory resource used by active mode graphs for
 * each concrete routing plan */
void Dispose();

#endif /*MMSPA4PG_H_*/

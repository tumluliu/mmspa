/*
 * =====================================================================================
 *
 *       Filename:  routingresult.c
 *
 *    Description:  Results of multimodal path planning including final paths,
 *    costs, etc.
 *
 *        Created:  2015/12/21 18时25分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#include "../include/routingresult.h"

PathRecorder **pathRecordTable = NULL;
int *pathRecordCountArray = NULL;
int inputModeCount = 0;

static SwitchPoint searchSwitchPointByToId(int64_t id, SwitchPoint *spList, 
        int spListLength);
static PathRecorder searchRecordByVertexId(int64_t id, PathRecorder *recordList, 
        int recordListLength);

Path **GetFinalPath(int64_t source, int64_t target) {
	extern SwitchPoint **switchpointsArr;
	extern int *switchpointCounts;
	int i = 0, pathVertexCount = 1, j = 0;
	PathRecorder pr = NULL;
	Path **paths = (Path **) calloc(inputModeCount, sizeof(Path*));
	pr = searchRecordByVertexId(target, pathRecordTable[inputModeCount - 1],
			pathRecordCountArray[inputModeCount - 1]);
	while (pr->parent_vertex_id != pr->vertex_id) {
		pr = searchRecordByVertexId(pr->parent_vertex_id,
				pathRecordTable[inputModeCount - 1],
				pathRecordCountArray[inputModeCount - 1]);
		if (pr == NULL) {
			// no path found
			free(paths);
			paths = NULL;
			return NULL;
		}
		pathVertexCount++;
	}
	Path *modePath = (Path*) malloc(sizeof(Path));
	modePath->vertex_list_length = pathVertexCount;
	modePath->vertex_list = (int64_t*) calloc(pathVertexCount, sizeof(int64_t));
	paths[inputModeCount - 1] = modePath;
	pr = searchRecordByVertexId(target, pathRecordTable[inputModeCount - 1],
			pathRecordCountArray[inputModeCount - 1]);
	j = 1;
	while (pr->parent_vertex_id != pr->vertex_id) {
		paths[inputModeCount - 1]->vertex_list[pathVertexCount - j] = pr->vertex_id;
		j++;
		pr = searchRecordByVertexId(pr->parent_vertex_id,
				pathRecordTable[inputModeCount - 1],
				pathRecordCountArray[inputModeCount - 1]);
	}
	paths[inputModeCount - 1]->vertex_list[0] = pr->vertex_id;
	for (i = inputModeCount - 2; i >= 0; i--) {
		pathVertexCount = 1;
		int64_t switchFromId = searchSwitchPointByToId(pr->vertex_id, 
		        switchpointsArr[i], switchpointCounts[i])->from_vertex_id;
		pr = searchRecordByVertexId(switchFromId, pathRecordTable[i],
				pathRecordCountArray[i]);
		int64_t switchpointId = pr->vertex_id;
		while (pr->parent_vertex_id != pr->vertex_id) {
			pr = searchRecordByVertexId(pr->parent_vertex_id, pathRecordTable[i], 
			        pathRecordCountArray[i]);
			if (pr == NULL) {
				// no path found
				DisposePaths(paths);
				return NULL;
			}
			pathVertexCount++;
		}
		Path* modePath = (Path*) malloc(sizeof(Path));
		modePath->vertex_list_length = pathVertexCount;
		modePath->vertex_list = (int64_t*) calloc(pathVertexCount, sizeof(int64_t));
		paths[i] = modePath;
		pr = searchRecordByVertexId(switchpointId, pathRecordTable[i], 
		        pathRecordCountArray[i]);
		j = 1;
		while (pr->parent_vertex_id != pr->vertex_id) {
			paths[i]->vertex_list[pathVertexCount - j] = pr->vertex_id;
			j++;
			pr = searchRecordByVertexId(pr->parent_vertex_id, pathRecordTable[i], 
			        pathRecordCountArray[i]);
		}
		paths[i]->vertex_list[0] = pr->vertex_id;
	}
	return paths;
}

void DisposeResultPathTable() {
	int i = 0;
	for (i = 0; i < inputModeCount; i++) {
		int j = 0;
		for (j = 0; j < pathRecordCountArray[i]; j++) {
			free(pathRecordTable[i][j]);
			pathRecordTable[i][j] = NULL;
		}
		free(pathRecordTable[i]);
		pathRecordTable[i] = NULL;
	}
	free(pathRecordTable);
	pathRecordTable = NULL;
	free(pathRecordCountArray);
	pathRecordCountArray = NULL;
}

double GetFinalCost(int64_t target, const char *costField) {
	extern ModeGraph *activeGraphs;
	extern int graphCount;
	Vertex targetVertex = BinarySearchVertexById(
	        activeGraphs[graphCount - 1]->vertices, 0, 
	        activeGraphs[graphCount - 1]->vertex_count - 1, target);
	if (targetVertex == VNULL)
		return -1;
	else {
		if (strcmp(costField, "distance") == 0)
			return targetVertex->distance;
		else if (strcmp(costField, "duration") == 0)
			return targetVertex->duration;
		else if (strcmp(costField, "walking_distance") == 0)
			return targetVertex->walking_distance;
		else if (strcmp(costField, "walking_time") == 0)
			return targetVertex->walking_time;
		else
			return -1;
	}
}

void DisposePaths(Path **paths) {
	int i = 0;
	for (i = 0; i < inputModeCount; i++) {
		free(paths[i]->vertex_list);
		paths[i]->vertex_list = NULL;
		free(paths[i]);
		paths[i] = NULL;
	}
	free(paths);
	paths = NULL;
}

static PathRecorder searchRecordByVertexId(int64_t id, PathRecorder *recordList, 
        int recordListLength) {
	int i = 0;
	for (i = 0; i < recordListLength; i++) {
		if (recordList[i]->vertex_id == id)
			return recordList[i];
	}
	return NULL;
}

static SwitchPoint searchSwitchPointByToId(int64_t id, SwitchPoint *spList, 
        int spListLength) {
	// FIXME: There will be a potential problem here: 
	// how to deal with the m:1 relationship with in a switch point?
	// That means there will be several searching results in this function.
	// Which one should be chosen?
	int i = 0;
	for (i = 0; i < spListLength; i++) {
		if (spList[i]->to_vertex_id == id)
			return spList[i];
	}
	return NULL;
}

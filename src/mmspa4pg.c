/*
 * mmspa4pg.c
 *
 *  Created on: Oct 5, 2009
 *      Author: LIU Lu
 * 
 * Including the implementation of MultimodalTwoQ algorithm and  
 * routines for loading data from PostgreSQL database
 */
  
#include "../include/mmspa4pg.h"
 
PathRecorder*** pathRecordTable = NULL;
int *pathRecordCountArray = NULL;
int inputModeCount = 0;
MultimodalRoutingPlan* plan = NULL;

void DisposeResultPathTable();

void DisposeGraphs();

void DisposeSwitchPoints();

void DisposeRoutingPlan();

PathRecorder* SearchRecordByVertexId(long long id,
		PathRecorder** recordList, int recordListLength);
		
SwitchPoint* SearchSwitchPointByToId(long long id, 
		SwitchPoint** spList, int spListLength);
		
void CreateRoutingPlan(int modeCount, int publicModeCount)
{
	plan = (MultimodalRoutingPlan*) malloc(sizeof(MultimodalRoutingPlan));
	plan->mode_count = modeCount;
	plan->public_transit_mode_count = publicModeCount;
	plan->mode_id_list = (int*) calloc(modeCount, sizeof(int));
	plan->target_constraint = NULL;
	if (modeCount > 1)
	{
		plan->switch_condition_list = (const char**) calloc(modeCount - 1, sizeof(const char*));
		plan->switch_constraint_list = (VertexValidationChecker*) calloc(modeCount - 1, sizeof(VertexValidationChecker));
		int i = 0;
		for (i = 0; i < modeCount; i++)
		{
			plan->switch_condition_list[i] = NULL;
			plan->switch_constraint_list[i] = NULL;
		}
	}
	if (publicModeCount > 0)
		plan->public_transit_mode_id_set = (int*) calloc(publicModeCount, sizeof(int));
}

void SetModeListItem(int index, int modeId)
{
	plan->mode_id_list[index] = modeId;
}

void SetPublicTransitModeSetItem(int index, int modeId)
{
	plan->public_transit_mode_id_set[index] = modeId;
}

void SetSwitchConditionListItem(int index, const char* spCondition)
{
	plan->switch_condition_list[index] = spCondition;
}

void SetSwitchingConstraint(int index, VertexValidationChecker callback)
{
	plan->switch_constraint_list[index] = callback;
}

void SetTargetConstraint(VertexValidationChecker callback)
{
	plan->target_constraint = callback;
}

void SetCostFactor(const char* costFactor)
{
	plan->cost_factor = costFactor;
}

void Dispose()
{
	DisposeGraphs();
	DisposeSwitchPoints();
	DisposeResultPathTable();
	DisposeRoutingPlan();
}

void DisposeGraphs()
{
	extern Graph **graphs;
	extern int graphCount;
	int i = 0;
	for (i = 0; i < graphCount; i++)
	{
		int j = 0;
		for (j = 0; j < graphs[i]->vertex_count; j++)
		{
			Edge* current = graphs[i]->vertices[j]->outgoing;
			while (current != NULL)
			{
				Edge* temp = current->adjNext;
				free(current);
				current = NULL;
				current = temp;
			}
			graphs[i]->vertices[j]->outgoing = NULL;
			free(graphs[i]->vertices[j]);
			graphs[i]->vertices[j] = NULL;
		}
		free(graphs[i]);
		graphs[i] = NULL;
	}
	free(graphs);
	graphs = NULL;
}

void DisposeSwitchPoints()
{
	extern SwitchPoint*** switchpointsArr;
	extern int* switchpointCounts;
	extern int graphCount;
	if (graphCount > 1)
	{
		int i = 0, j = 0;
		for (i = 0; i < graphCount - 1; i++)
		{
			for (j = 0; j < switchpointCounts[i]; j++)
			{
				free(switchpointsArr[i][j]);
				switchpointsArr[i][j] = NULL;
			}
			free(switchpointsArr[i]);
			switchpointsArr[i] = NULL;
		}
		free(switchpointCounts);
		free(switchpointsArr);
		switchpointsArr = NULL;
		switchpointCounts = NULL;
	}
}

void DisposeResultPathTable()
{
	int i = 0;
	for (i = 0; i < inputModeCount; i++)
	{
		int j = 0;
		for (j = 0; j < pathRecordCountArray[i]; j++)
		{
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

void DisposeRoutingPlan()
{
	if (plan->mode_count > 1)
	{
		free(plan->switch_condition_list);
		free(plan->switch_constraint_list);
	}
	plan->switch_condition_list = NULL;
	plan->switch_constraint_list = NULL;
	plan->target_constraint = NULL;
	free(plan->mode_id_list);
	plan->mode_id_list = NULL;
	free(plan);
	plan = NULL;
}

void DisposePaths(Path** paths)
{
	int i = 0;
	for (i = 0; i < inputModeCount; i++)
	{
		free(paths[i]->vertex_list);
		paths[i]->vertex_list = NULL;
		free(paths[i]);
		paths[i] = NULL;
	}
	free(paths);
	paths = NULL;
}

Path** GetFinalPath(long long source, long long target)
{
	extern SwitchPoint*** switchpointsArr;
	extern int* switchpointCounts;
	int i = 0, pathVertexCount = 1, j = 0;
	PathRecorder* pr;
	Path** paths = (Path **) calloc(inputModeCount, sizeof(Path*));
	//printf("Mode %d\n", inputModeCount - 1);
	pr = SearchRecordByVertexId(target, pathRecordTable[inputModeCount - 1],
			pathRecordCountArray[inputModeCount - 1]);
	//printf("%s\n", pr->id);
	while (pr->parent_vertex_id != pr->vertex_id)
	{
		pr = SearchRecordByVertexId(pr->parent_vertex_id,
				pathRecordTable[inputModeCount - 1],
				pathRecordCountArray[inputModeCount - 1]);
		if (pr == NULL)
		{
			// no path found
			free(paths);
			paths = NULL;
			return NULL;
		}
		pathVertexCount++;
	}
	Path* modePath = (Path*) malloc(sizeof(Path));
	modePath->vertex_list_length = pathVertexCount;
	modePath->vertex_list = (long long*) calloc(pathVertexCount, sizeof(long long));
	paths[inputModeCount - 1] = modePath;
	pr = SearchRecordByVertexId(target, pathRecordTable[inputModeCount - 1],
			pathRecordCountArray[inputModeCount - 1]);
	j = 1;
	while (pr->parent_vertex_id != pr->vertex_id)
	{
		paths[inputModeCount - 1]->vertex_list[pathVertexCount - j] = pr->vertex_id;
		j++;
		pr = SearchRecordByVertexId(pr->parent_vertex_id,
				pathRecordTable[inputModeCount - 1],
				pathRecordCountArray[inputModeCount - 1]);
	}
	paths[inputModeCount - 1]->vertex_list[0] = pr->vertex_id;
	
	for (i = inputModeCount - 2; i >= 0; i--)
	{
		pathVertexCount = 1;
		//printf("Mode %d\n", i);
		long long switchFromId = SearchSwitchPointByToId(pr->vertex_id, switchpointsArr[i], switchpointCounts[i])->from_vertex_id;
		pr = SearchRecordByVertexId(switchFromId, pathRecordTable[i],
				pathRecordCountArray[i]);
		long long switchpointId = pr->vertex_id;
		//printf("%s\n", pr->id);
		while (pr->parent_vertex_id != pr->vertex_id)
		{
			pr = SearchRecordByVertexId(pr->parent_vertex_id, pathRecordTable[i], pathRecordCountArray[i]);
			if (pr == NULL)
			{
				// no path found
				DisposePaths(paths);
				return NULL;
			}
			//printf("%s\n", pr->id);
			pathVertexCount++;
		}
		Path* modePath = (Path*) malloc(sizeof(Path));
		modePath->vertex_list_length = pathVertexCount;
		modePath->vertex_list = (long long*) calloc(pathVertexCount, sizeof(long long));
		paths[i] = modePath;
		pr = SearchRecordByVertexId(switchpointId, pathRecordTable[i], pathRecordCountArray[i]);
		j = 1;
		while (pr->parent_vertex_id != pr->vertex_id)
		{
			paths[i]->vertex_list[pathVertexCount - j] = pr->vertex_id;
			j++;
			pr = SearchRecordByVertexId(pr->parent_vertex_id, pathRecordTable[i], pathRecordCountArray[i]);
		}
		paths[i]->vertex_list[0] = pr->vertex_id;
	}
	return paths;
}

double GetFinalCost(long long target, const char* costField)
{
	extern Graph **graphs;
	extern int graphCount;
	Vertex* targetVertex = BinarySearchVertexById(graphs[graphCount - 1]->vertices, 0, graphs[graphCount - 1]->vertex_count - 1, target);
	if (targetVertex == NNULL)
		return -1;
	else
	{
		if (strcmp(costField, "distance") == 0)
			return targetVertex->distance;
		else if (strcmp(costField, "elapsed_time") == 0)
			return targetVertex->elapsed_time;
		else if (strcmp(costField, "walking_distance") == 0)
			return targetVertex->walking_distance;
		else if (strcmp(costField, "walking_time") == 0)
			return targetVertex->walking_time;
		else
			return -1;
	}
}

PathRecorder* SearchRecordByVertexId(long long id,
		PathRecorder** recordList, int recordListLength)
{
	int i = 0;
	for (i = 0; i < recordListLength; i++)
	{
		if (recordList[i]->vertex_id == id)
			return recordList[i];
	}
	return NULL;
}

SwitchPoint* SearchSwitchPointByToId(long long id, SwitchPoint** spList, int spListLength)
{
	// There will be a potential problem here: 
	// how to deal with the m:1 relationship with in a switch point?
	// That means there will be several searching results in this function.
	// Which one should be chosen?
	int i = 0;
	for (i = 0; i < spListLength; i++)
	{
		if (spList[i]->to_vertex_id == id)
			return spList[i];
	}
	return NULL;
}

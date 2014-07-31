/*
 * multimodal-twoq.c
 *
 *  Created on: May 4, 2009
 *      Author: LIU Lu
 */
#include "mmspa4pg.h"

/* status of vertex regarding to queue */
#define UNREACHED   -1
#define IN_QUEUE     0
#define WAS_IN_QUEUE 1

#define INIT_QUEUE(source)\
{\
	begin = end = source;\
	entry = NNULL;\
	source->next = NNULL;\
	source->status = IN_QUEUE;\
}\

#define NONEMPTY_QUEUE           ( begin != NNULL )

#define NODE_IN_QUEUE(vertex)      ( vertex->status == IN_QUEUE )

#define NODE_WAS_IN_QUEUE(vertex)  ( vertex->status == WAS_IN_QUEUE )

#define EXTRACT_FIRST(vertex)\
{\
	vertex = begin;\
	vertex->status = WAS_IN_QUEUE;\
	if (begin == entry)\
		entry = NNULL;\
	begin = begin->next;\
}\

#define INSERT_TO_ENTRY(vertex)\
{\
	if (entry != NNULL)\
	{\
		vertex->next = entry->next;\
		entry->next = vertex;\
		if (entry == end)\
			end = vertex;\
	}\
	else\
	{\
		if (begin == NNULL)\
			end = vertex;\
		vertex->next = begin;\
		begin = vertex;\
	}\
	vertex->status = IN_QUEUE;\
	entry = vertex;\
}\

#define INSERT_TO_BACK(vertex)\
{\
	if (begin == NNULL)\
		begin = vertex;\
	else\
		end->next = vertex;\
\
	end = vertex;\
	end->next = NNULL;\
	vertex->status = IN_QUEUE;\
}\

extern char* GetSwitchPoint(const char* mode1, const char* mode2);
extern void DisposeResultPathTable();

void TwoQSearch(Graph* g, Vertex** begin, Vertex** end, Vertex** entry,	
				PathRecorder*** prev, const char* costFactor, 
				VertexValidationChecker checkConstraint);
void MultimodalTwoQInit(Graph* g, Graph* last_g, SwitchPoint** spList, int spListLength, 
						VertexValidationChecker checkConstraint, 
						long long source, Vertex** begin, Vertex** end, 
						Vertex** entry, PathRecorder*** prev, const char* costFactor);

void MultimodalTwoQ(long long source)
{
	extern Graph** graphs;
	extern SwitchPoint*** switchpointsArr;
	extern int* switchpointCounts;
	extern PathRecorder*** pathRecordTable;
	extern int* pathRecordCountArray;
	extern int inputModeCount;
	extern MultimodalRoutingPlan* plan;
	int i = 0;
	Vertex *begin = NNULL, *end = NNULL, *entry = NNULL;
	if (pathRecordTable != NULL)
		DisposeResultPathTable();
	pathRecordTable = (PathRecorder***) calloc(plan->mode_count,
			sizeof(PathRecorder**));
	pathRecordCountArray = (int*) calloc(plan->mode_count, sizeof(int));
	inputModeCount = plan->mode_count;
	for (i = 0; i < plan->mode_count; i++)
	{
		pathRecordCountArray[i] = graphs[i]->vertex_count;
		PathRecorder** pathRecordArray = (PathRecorder**) calloc(
				pathRecordCountArray[i], sizeof(PathRecorder*));
		if (i == 0)
			MultimodalTwoQInit(graphs[i], NULL, NULL, 0, NULL, source, &begin, &end,
					&entry, &pathRecordArray, plan->cost_factor);
		else
			MultimodalTwoQInit(graphs[i], graphs[i-1], switchpointsArr[i-1], switchpointCounts[i-1], plan->switch_constraint_list[i-1],
					source, &begin, &end, &entry, &pathRecordArray, plan->cost_factor);
		TwoQSearch(graphs[i], &begin, &end, &entry, &pathRecordArray, plan->cost_factor, plan->target_constraint);
		pathRecordTable[i] = pathRecordArray;
	}
}

void MultimodalTwoQInit(Graph* g, Graph* last_g, SwitchPoint** spList, int spListLength, 
						VertexValidationChecker checkConstraint,  
						long long source, Vertex** begin, Vertex** end, 
						Vertex** entry, PathRecorder*** prev, const char* costFactor)
{
	int i = 0;
	int v_number = g->vertex_count;
	Vertex *src;
	
	for (i = 0; i < v_number; i++)
	{
		g->vertices[i]->temp_cost = VERY_FAR;
		g->vertices[i]->distance = VERY_FAR;
		g->vertices[i]->elapsed_time = VERY_FAR;
		g->vertices[i]->walking_distance = VERY_FAR;
		g->vertices[i]->walking_time = VERY_FAR;
		g->vertices[i]->parent = NNULL;
		g->vertices[i]->status = UNREACHED;
		g->vertices[i]->next = NNULL;
		PathRecorder *tmpRecorder = (PathRecorder*) malloc(
				sizeof(PathRecorder));
		tmpRecorder->vertex_id = g->vertices[i]->id;
//		tmpRecorder->cost = VERY_FAR;
		tmpRecorder->parent_vertex_id = -1 * VERY_FAR;
		(*prev)[i] = tmpRecorder;
	}
	
	(*entry) = NNULL;
	if (spList == NULL)
	{
		src = BinarySearchVertexById(g->vertices, 0, g->vertex_count - 1, source);
		// INIT_QUEUE(src)
		src->temp_cost = 0;
		src->distance = 0;
		src->elapsed_time = 0;
		src->walking_distance = 0;
		src->walking_time = 0;
		src->parent = src;
		(*begin) = (*end) = src;
		src->next = NNULL;
		src->status = IN_QUEUE;
	}
	else
	{
		/* relax every switch point pairs (i.e. switch edges) */
		(*begin) = (*end) = NNULL;
		double costNew;
		for (i = 0; i < spListLength; i++)
		{			
			Vertex* switchFrom = BinarySearchVertexById(last_g->vertices,
						0, last_g->vertex_count - 1, spList[i]->from_vertex_id);
			Vertex* switchTo = BinarySearchVertexById(g->vertices,
						0, g->vertex_count - 1, spList[i]->to_vertex_id);
			if (switchFrom != NNULL && switchTo != NNULL)
			{
				//printf("processing switch point pairs: %lld, %lld \n", switchFrom->id, switchTo->id);
				if (checkConstraint != NULL)
				{
					//printf("checking from switch point %lld with temp distance = %f ... ", switchFrom->id, switchFrom->distance);
					if (checkConstraint(switchFrom) != 0)
					{
						//printf("invalid \n");
						continue;
					}
					//printf("valid \n ");
				}
				if (strcmp(costFactor, "speed") == 0)
					costNew = switchFrom->temp_cost + (spList[i]->length * spList[i]->speed_factor);
				else if (strcmp(costFactor, "length") == 0)
					costNew = switchFrom->temp_cost + (spList[i]->length * spList[i]->length_factor);
				else /*default case: use speed factor*/
					costNew = switchFrom->temp_cost + (spList[i]->length * spList[i]->speed_factor);
				if (costNew < switchTo->temp_cost)
				{
					switchTo->temp_cost = costNew;
					switchTo->parent = switchTo;
					switchTo->distance = switchFrom->distance + (spList[i]->length * spList[i]->length_factor);
					switchTo->elapsed_time = switchFrom->elapsed_time + (spList[i]->length * spList[i]->speed_factor);
					switchTo->walking_distance = switchFrom->walking_distance + (spList[i]->length * spList[i]->length_factor);
					switchTo->walking_time = switchFrom->walking_time + (spList[i]->length * spList[i]->speed_factor);
					/* enqueue switch points */
					// INSERT_TO_BACK(switchTo)
					if (switchTo->status != IN_QUEUE)
					{
						if ((*begin) == NNULL)
							*begin = switchTo;
						else
							(*end)->next = switchTo;
						(*end) = switchTo;
						(*end)->next = NNULL;
						switchTo->status = IN_QUEUE;
					}
				}
			}
		}
	}
}

void TwoQSearch(Graph* g, Vertex** begin, Vertex** end, Vertex** entry,
		PathRecorder*** prev, const char* costFactor, VertexValidationChecker checkConstraint)
{
	double costNew;
	Vertex *vertexFrom, *vertexTo;
	Edge *edge_ij;
	int edgeCount = 0, i = 0, vertexCount = 0;

	while ((*begin) != NNULL)
	{
		// EXTRACT_FIRST(vertexFrom)
		vertexFrom = *begin;
		//printf("extract first vertex %lld in the queue \n", vertexFrom->id);
		vertexFrom->status = WAS_IN_QUEUE;
		if ((*begin) == (*entry))
			*entry = NNULL;
		*begin = (*begin)->next;
		if (checkConstraint != NULL)
		{
			//printf("checking constraint on %lld with temp distance = %f ... ", vertexFrom->id, vertexFrom->distance);
			if (checkConstraint(vertexFrom) != 0)
			{
				//printf("invalid \n");
				continue;
			}
			//printf("valid \n");
		}
		edgeCount = vertexFrom->outdegree;
		edge_ij = vertexFrom->outgoing;
		while (edge_ij != NULL)
		{ 
			/* scanning edges outgoing from vertexFrom*/
			vertexTo = BinarySearchVertexById(g->vertices, 0, g->vertex_count - 1, edge_ij->to_vertex_id);
			if (strcmp(costFactor, "speed") == 0)
				costNew = vertexFrom->temp_cost + (edge_ij->length * edge_ij->speed_factor);
			else if (strcmp(costFactor, "length") == 0)
				costNew = vertexFrom->temp_cost + (edge_ij->length * edge_ij->length_factor);
			else /*default case: use speed factor*/
				costNew = vertexFrom->temp_cost + (edge_ij->length * edge_ij->speed_factor);
			if (costNew < vertexTo->temp_cost)
			{
				vertexTo->temp_cost = costNew;
				vertexTo->parent = vertexFrom;
				vertexTo->distance = vertexFrom->distance + (edge_ij->length * edge_ij->length_factor);
				vertexTo->elapsed_time = vertexFrom->elapsed_time + (edge_ij->length * edge_ij->speed_factor);
				if (edge_ij->mode_id == 1002)
				{
					vertexTo->walking_distance = vertexFrom->walking_distance + (edge_ij->length * edge_ij->length_factor);
					vertexTo->walking_time = vertexFrom->walking_time + (edge_ij->length * edge_ij->speed_factor);
				}
				else
				{
					vertexTo->walking_distance = vertexFrom->walking_distance;
					vertexTo->walking_time = vertexFrom->walking_time;
				}
				if (!vertexTo->status == IN_QUEUE)
				{
					if (vertexTo->status == WAS_IN_QUEUE)
					{
						// INSERT_TO_ENTRY(vertexTo)
						if ((*entry) != NNULL)
						{
							vertexTo->next = (*entry)->next;
							(*entry)->next = vertexTo;
							if ((*entry) == (*end))
								(*end) = vertexTo;
						}
						else
						{
							if ((*begin) == NNULL)
								(*end) = vertexTo;
							vertexTo->next = (*begin);
							(*begin) = vertexTo;
						}
						vertexTo->status = IN_QUEUE;
						(*entry) = vertexTo;
					}
					else
					{
						// INSERT_TO_BACK(vertexTo)
						if ((*begin) == NNULL)
							(*begin) = vertexTo;
						else
							(*end)->next = vertexTo;

						(*end) = vertexTo;
						(*end)->next = NNULL;
						vertexTo->status = IN_QUEUE;
					}
				}
			}
			edge_ij = edge_ij->adjNext;
		} /* end of scanning vertexFrom */
	} /* end of the main loop */

	vertexCount = g->vertex_count;
	for (i = 0; i < vertexCount; i++)
	{
		(*prev)[i]->vertex_id = g->vertices[i]->id;
		if (g->vertices[i]->parent == NNULL)
			(*prev)[i]->parent_vertex_id = -1 * VERY_FAR;
		else
			(*prev)[i]->parent_vertex_id = g->vertices[i]->parent->id;
//		(*prev)[i]->cost = g->vertices[i]->distance;
	}
}


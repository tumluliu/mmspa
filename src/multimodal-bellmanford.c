/*
 * multimodal-bellmanford.c
 *
 *  Created on: Feb 23, 2009
 *      Author: LIU Lu
 */
#include <stdlib.h>
#include "mmspa.h"

#define IN_QUEUE     0
#define OUT_OF_QUEUE 1

extern char* GetSwitchPoint(const char* mode1, const char* mode2);
extern void DisposeResultPathTable();

void BellmanFordSearch(Graph* g, Vertex** begin, Vertex** end,
		PathRecorder*** prev);
void MultimodalBellmanFordInit(Graph* g, Graph* last_g, const char* spValue,
		const char* source, Vertex** begin, Vertex** end, PathRecorder*** prev);

void MultimodalBellmanFord(const char** modeList, int modeListLength,
		const char* source)
{
	extern Graph** graphs;
	extern int graphCount;
	extern PathRecorder*** pathRecordTable;
	extern int* pathRecordCountArray;
	extern int inputModeCount;
	int i = 0, j = 0;
	Graph *workGraph = NULL, *lastGraph = NULL;
	Vertex *begin = NNULL, *end = NNULL;
	if (pathRecordTable != NULL)
		DisposeResultPathTable();
	pathRecordTable = (PathRecorder***) calloc(modeListLength,
			sizeof(PathRecorder**));
	pathRecordCountArray = (int*) calloc(modeListLength, sizeof(int));
	inputModeCount = modeListLength;
	for (i = 0; i < modeListLength; i++)
	{
		for (j = 0; j < graphCount; j++)
		{
			if (strcmp(modeList[i], graphs[j]->id) == 0)
			{
				workGraph = graphs[j];
				break;
			}
		}
		pathRecordCountArray[i] = workGraph->vertexCount;
		PathRecorder** pathRecordArray = (PathRecorder**) calloc(
				pathRecordCountArray[i], sizeof(PathRecorder*));
		if (i == 0)
			MultimodalBellmanFordInit(workGraph, lastGraph, "", source, &begin,
					&end, &pathRecordArray);
		else
			MultimodalBellmanFordInit(workGraph, lastGraph, GetSwitchPoint(
					modeList[i - 1], modeList[i]), source, &begin, &end,
					&pathRecordArray);
		BellmanFordSearch(workGraph, &begin, &end, &pathRecordArray);
		lastGraph = workGraph;
		pathRecordTable[i] = pathRecordArray;
	}
}

void MultimodalBellmanFordInit(Graph* g, Graph* last_g, const char* spValue,
		const char* source, Vertex** begin, Vertex** end, PathRecorder*** prev)
{
	int i = 0;
	int v_number = g->vertexCount;
	Vertex *src;
	if (strcmp(spValue, "") == 0)
	{
		for (i = 0; i < v_number; i++)
		{
			g->vertices[i]->distance = VERY_FAR;
			g->vertices[i]->parent = NNULL;
			g->vertices[i]->status = OUT_OF_QUEUE;
			PathRecorder *tmpRecorder = (PathRecorder*) malloc(
					sizeof(PathRecorder));
			strcpy(tmpRecorder->id, g->vertices[i]->id);
			tmpRecorder->distance = VERY_FAR;
			strcpy(tmpRecorder->parent_id, "NNULL");
			(*prev)[i] = tmpRecorder;
		}
		src = SearchVertexById(g->vertices, g->vertexCount, source);
		src->distance = 0.0;
		src->parent = src;
		*begin = *end = src;
		src->next = NNULL;
		src->status = IN_QUEUE;
	}
	else
	{
		for (i = 0; i < v_number; i++)
		{
			PathRecorder *tmpRecorder = (PathRecorder*) malloc(
					sizeof(PathRecorder));
			strcpy(tmpRecorder->id, g->vertices[i]->id);
			tmpRecorder->distance = VERY_FAR;
			strcpy(tmpRecorder->parent_id, "NNULL");
			(*prev)[i] = tmpRecorder;
			if (strcmp(g->vertices[i]->attr_amenity, spValue) != 0)
			{
				g->vertices[i]->distance = VERY_FAR;
				g->vertices[i]->parent = NNULL;
				g->vertices[i]->status = OUT_OF_QUEUE;
			}
			else
			{
				Vertex *switch_point = SearchVertexById(last_g->vertices,
						last_g->vertexCount, g->vertices[i]->id);
				if (switch_point != NNULL)
				{
					g->vertices[i]->distance = switch_point->distance;
					g->vertices[i]->parent = g->vertices[i];
					/* enqueue switch points */
					if ((*begin) == NNULL)
						*begin = g->vertices[i];
					else
						(*end)->next = g->vertices[i];
					*end = g->vertices[i];
					(*end)->next = NNULL;
					g->vertices[i]->status = IN_QUEUE;
				}
				else
				{
					g->vertices[i]->distance = VERY_FAR;
					g->vertices[i]->parent = NNULL;
					g->vertices[i]->status = OUT_OF_QUEUE;
				}
			}
		}
	}
}

void BellmanFordSearch(Graph* g, Vertex** begin, Vertex** end,
		PathRecorder*** prev)
{
	long distanceNew, distanceFrom;
	Vertex *vertexFrom, *vertexTo;
	Edge *edge_ij;
	int edgeCount = 0, edgeIndex = 0, i = 0, num_scans = 0, vertexCount = 0;

	while (*begin != NNULL)
	{
		vertexFrom = *begin;
		vertexFrom->status = OUT_OF_QUEUE;
		*begin = (*begin)->next;
		if (((vertexFrom->parent)->status) == OUT_OF_QUEUE)
		{
			num_scans++;
			edgeCount = vertexFrom->outdegree;
			distanceFrom = vertexFrom->distance;
			edgeIndex = vertexFrom->first;

			for (i = 0; i < edgeCount; i++)
			{ /*scanning arcs outgoing from node_from*/
				edge_ij = g->edges[edgeIndex];
				edgeIndex++;
				vertexTo = edge_ij->end;
				distanceNew = distanceFrom + (edge_ij->cost);
				if (distanceNew < vertexTo->distance)
				{
					vertexTo->distance = distanceNew;
					vertexTo->parent = vertexFrom;
					if (vertexTo->status != IN_QUEUE)
					{
						if (*begin == NNULL)
							*begin = vertexTo;
						else
							(*end)->next = vertexTo;
						(*end) = vertexTo;
						(*end)->next = NNULL;
						vertexTo->status = IN_QUEUE;
					}
				}
			}
		}
	}
	vertexCount = g->vertexCount;
	for (i = 0; i < vertexCount; i++)
	{
		strcpy((*prev)[i]->id, g->vertices[i]->id);
		if (g->vertices[i]->parent == NNULL)
			strcpy((*prev)[i]->parent_id, "NNULL");
		else
			strcpy((*prev)[i]->parent_id, g->vertices[i]->parent->id);
		(*prev)[i]->distance = g->vertices[i]->distance;
	}
}

/*
 * multimodal-dijkstra.c
 *
 *  Created on: Feb 23, 2009
 *      Author: LIU Lu
 */
#include "mmspa.h"
#include "f_heap.h"

#define NODE_IN_FHEAP(node) (node->status > OUT_OF_HEAP)

extern char* GetSwitchPoint(const char* mode1, const char* mode2);
extern void DisposeResultPathTable();

Vertex* GetMinVertex(Vertex** vertices, int count);
void DijkstraSearch(Graph* g, PathRecorder*** prev);
void MultimodalDijkstraInit(Graph* g, Graph* last_g, const char* spValue,
		const char* source, Vertex** begin, Vertex** end, PathRecorder*** prev);

void MultimodalDijkstra(const char** modeList, int modeListLength,
		const char *source)
{
	extern Graph** graphs;
	extern int graphCount;
	extern PathRecorder*** pathRecordTable;
	extern int* pathRecordCountArray;
	extern int inputModeCount;
	int i = 0, j = 0;
	Graph *workGraph = NULL, *lastGraph = NULL;
	Vertex *begin, *end;
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
			MultimodalDijkstraInit(workGraph, lastGraph, "", source, &begin,
					&end, &pathRecordArray);
		else
			MultimodalDijkstraInit(workGraph, lastGraph, GetSwitchPoint(
					modeList[i - 1], modeList[i]), source, &begin, &end,
					&pathRecordArray);
		DijkstraSearch(workGraph, &pathRecordArray);
		lastGraph = workGraph;
		pathRecordTable[i] = pathRecordArray;
	}
}

void MultimodalDijkstraInit(Graph* g, Graph* last_g, const char* spValue,
		const char* source, Vertex** begin, Vertex** end, PathRecorder*** prev)
{
	int i = 0;
	int v_count = g->vertexCount;
	Vertex *src;
	Init_fheap(v_count);
	if (strcmp(spValue, "") == 0)
	{
		for (i = 0; i < v_count; i++)
		{
			g->vertices[i]->distance = VERY_FAR;
			g->vertices[i]->parent = NNULL;
			g->vertices[i]->status = OUT_OF_HEAP;
			PathRecorder *tmpRecorder = (PathRecorder*) malloc(
					sizeof(PathRecorder));
			strcpy(tmpRecorder->id, g->vertices[i]->id);
			tmpRecorder->distance = VERY_FAR;
			strcpy(tmpRecorder->parent_id, "NNULL");
			(*prev)[i] = tmpRecorder;
		}
		src = SearchVertexById(g->vertices, g->vertexCount, source);
		src->distance = 0;
		src->parent = src;
		Insert_to_fheap(src);
	}
	else
	{
		for (i = 0; i < v_count; i++)
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
				g->vertices[i]->status = OUT_OF_HEAP;
			}
			else
			{
				Vertex *switch_point = SearchVertexById(last_g->vertices,
						last_g->vertexCount, g->vertices[i]->id);
				if (switch_point != NNULL)
				{
					g->vertices[i]->distance = switch_point->distance;
					g->vertices[i]->parent = g->vertices[i];
					/* enheap switch points */
					Insert_to_fheap(g->vertices[i]);
				}
				else
				{
					//printf("can not find switch point\n");
					g->vertices[i]->distance = VERY_FAR;
					g->vertices[i]->parent = NNULL;
					g->vertices[i]->status = OUT_OF_HEAP;
				}
			}
		}
	}
}

void DijkstraSearch(Graph *g, PathRecorder*** prev)
{
	int vertexCount;
	int i = 0, edge_index = 0, out_degree = 0;
	Edge *edge_ij;
	Vertex *vertex_to, *min_vertex;
	int dist_from, dist_new;

	/* DIKF implementation */
	while (1)
	{
		min_vertex = Extract_min();
		if (min_vertex == NNULL)
			break;

		out_degree = min_vertex->outdegree;
		dist_from = min_vertex->distance;
		edge_index = min_vertex->first;

		for (i = 0; i < out_degree; i++)
		{
			/*scanning arcs outgoing from node_from*/
			edge_ij = (g->edges)[edge_index];
			edge_index++;
			vertex_to = edge_ij->end;

			dist_new = dist_from + (edge_ij->cost);
			if (dist_new < vertex_to->distance)
			{
				vertex_to->distance = dist_new;
				vertex_to->parent = min_vertex;

				if (NODE_IN_FHEAP(vertex_to))
					Fheap_decrease_key(vertex_to);
				else
					Insert_to_fheap(vertex_to);
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
	DisposeFibHeap();
}

Vertex* GetMinVertex(Vertex** vertices, int count)
{
	int i = 0, min_index = -1;
	double min_dist = VERY_FAR;
	for (i = 0; i < count; i++)
	{
		if (vertices[i]->distance < min_dist)
			min_index = i;
	}
	return vertices[min_index];
}


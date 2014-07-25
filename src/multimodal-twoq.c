/*
 * multimodal-twoq.c
 *
 *  Created on: May 4, 2009
 *      Author: LIU Lu
 */
#include "../include/mmspa.h"

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
		PathRecorder*** prev);
void MultimodalTwoQInit(Graph* g, Graph* last_g, const char* spValue,
		const char* source, Vertex** begin, Vertex** end, Vertex** entry,
		PathRecorder*** prev);

void MultimodalTwoQ(const char** modeList, int modeListLength,
		const char* source)
{
	extern Graph** graphs;
	extern int graphCount;
	extern PathRecorder*** pathRecordTable;
	extern int* pathRecordCountArray;
	extern int inputModeCount;
	int i = 0, j = 0;
	Graph *workGraph = NULL, *lastGraph = NULL;
	Vertex *begin = NNULL, *end = NNULL, *entry = NNULL;
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
			MultimodalTwoQInit(workGraph, lastGraph, "", source, &begin, &end,
					&entry, &pathRecordArray);
		else
			MultimodalTwoQInit(workGraph, lastGraph, GetSwitchPoint(modeList[i
					- 1], modeList[i]), source, &begin, &end, &entry,
					&pathRecordArray);
		TwoQSearch(workGraph, &begin, &end, &entry, &pathRecordArray);
		lastGraph = workGraph;
		pathRecordTable[i] = pathRecordArray;
	}
}

void MultimodalTwoQInit(Graph* g, Graph* last_g, const char* spValue,
		const char* source, Vertex** begin, Vertex** end, Vertex** entry,
		PathRecorder*** prev)
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
			g->vertices[i]->status = UNREACHED;
			PathRecorder *tmpRecorder = (PathRecorder*) malloc(
					sizeof(PathRecorder));
			strcpy(tmpRecorder->id, g->vertices[i]->id);
			tmpRecorder->distance = VERY_FAR;
			strcpy(tmpRecorder->parent_id, "NNULL");
			(*prev)[i] = tmpRecorder;
		}
		src = SearchVertexById(g->vertices, g->vertexCount, source);
		// INIT_QUEUE(src)
		src->distance = 0;
		src->parent = src;
		*begin = *end = src;
		*entry = NNULL;
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
				g->vertices[i]->status = UNREACHED;
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
					// INSERT_TO_BACK(g->vertices[i])
					if ((*begin) == NNULL)
						*begin = g->vertices[i];
					else
						(*end)->next = g->vertices[i];
					(*end) = g->vertices[i];
					(*end)->next = NNULL;
					g->vertices[i]->status = IN_QUEUE;
				}
				else
				{
					g->vertices[i]->distance = VERY_FAR;
					g->vertices[i]->parent = NNULL;
					g->vertices[i]->status = UNREACHED;
				}
			}
		}
	}
}

void TwoQSearch(Graph* g, Vertex** begin, Vertex** end, Vertex** entry,
		PathRecorder*** prev)
{
	long distanceNew, distanceFrom;
	Vertex *vertexFrom, *vertexTo;
	Edge *edge_ij;
	int edgeCount = 0, edgeIndex = 0, i = 0, vertexCount = 0;

	while ((*begin) != NNULL)
	{
		// EXTRACT_FIRST(vertexFrom)
		vertexFrom = *begin;
		vertexFrom->status = WAS_IN_QUEUE;
		if ((*begin) == (*entry))
			*entry = NNULL;
		*begin = (*begin)->next;

		edgeCount = vertexFrom->outdegree;
		distanceFrom = vertexFrom->distance;
		edgeIndex = vertexFrom->first;

		for (i = 0; i < edgeCount; i++)
		{ /*scanning edges outgoing from vertexFrom*/
			edge_ij = g->edges[edgeIndex];
			edgeIndex++;
			vertexTo = edge_ij->end;
			distanceNew = distanceFrom + (edge_ij->cost);
			if (distanceNew < vertexTo->distance)
			{
				vertexTo->distance = distanceNew;
				vertexTo->parent = vertexFrom;
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
			} /* end of scanning  vertex_from */
		} /* end of the main loop */
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


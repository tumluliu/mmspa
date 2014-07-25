/*
 * mmspa.c
 *
 *  Created on: Mar 25, 2009
 *      Author: LIU Lu
 */
#include "../include/mmspa.h"

PathRecorder*** pathRecordTable = NULL;
int *pathRecordCountArray = NULL;
int inputModeCount = 0;

void DisposeResultPathTable();
void DisposeGraphs();
char* GetSwitchPoint(const char* mode1, const char* mode2);
PathRecorder* SearchRecordByVertexId(const char* id,
		PathRecorder** record_list, int record_list_len);

char* GetSwitchPoint(const char *mode1, const char *mode2)
{
	if (strcmp(mode1, "car") == 0)
	{
		if (strcmp(mode2, "foot") == 0)
			return "parking";
		if (strcmp(mode2, "underground") == 0)
			return "P+R";
	}
	else if (strcmp(mode1, "foot") == 0)
	{
		if (strcmp(mode2, "underground") == 0)
			return "underground_station";
	}
	else if (strcmp(mode1, "underground") == 0)
	{
		if (strcmp(mode2, "foot") == 0)
			return "underground_station";
	}
	return NULL;
}

void Dispose()
{
	DisposeGraphs();
	DisposeResultPathTable();
}

void DisposeGraphs()
{
	extern Graph **graphs;
	extern int graphCount;
	int i = 0;
	for (i = 0; i < graphCount; i++)
	{
		int j = 0;
		for (j = 0; j < graphs[i]->vertexCount; j++)
			free(graphs[i]->vertices[j]);
		for (j = 0; j < graphs[i]->edgeCount; j++)
			free(graphs[i]->edges[j]);
		free(graphs[i]);
	}
	free(graphs);
	graphs = NULL;
}

void DisposeResultPathTable()
{
	int i = 0;
	for (i = 0; i < inputModeCount; i++)
	{
		int j = 0;
		for (j = 0; j < pathRecordCountArray[i]; j++)
			free(pathRecordTable[i][j]);
		free(pathRecordTable[i]);
	}
	free(pathRecordTable);
	pathRecordTable = NULL;
	free(pathRecordCountArray);
	pathRecordCountArray = NULL;
}

void GetFinalPath(const char* source, const char* target)
{
	int i = 0;
	PathRecorder* pr;
	printf("Mode %d\n", inputModeCount - 1);
	pr = SearchRecordByVertexId(target, pathRecordTable[inputModeCount - 1],
			pathRecordCountArray[inputModeCount - 1]);
	printf("%s\n", pr->id);
	while (strcmp(pr->parent_id, pr->id) != 0)
	{
		pr = SearchRecordByVertexId(pr->parent_id,
				pathRecordTable[inputModeCount - 1],
				pathRecordCountArray[inputModeCount - 1]);
		printf("%s\n", pr->id);
	}
	for (i = inputModeCount - 2; i >= 0; i--)
	{
		printf("Mode %d\n", i);
		pr = SearchRecordByVertexId(pr->id, pathRecordTable[i],
				pathRecordCountArray[i]);
		while (strcmp(pr->parent_id, pr->id) != 0)
		{
			pr = SearchRecordByVertexId(pr->parent_id, pathRecordTable[i],
					pathRecordCountArray[i]);
			printf("%s\n", pr->id);
		}
	}
}

PathRecorder* SearchRecordByVertexId(const char* id,
		PathRecorder** record_list, int record_list_len)
{
	int i = 0;
	for (i = 0; i < record_list_len; i++)
	{
		if (strcmp(record_list[i]->id, id) == 0)
			return record_list[i];
	}
	return NULL;
}

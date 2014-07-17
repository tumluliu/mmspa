/*
 * parser.c
 *
 *  Created on: Mar 18, 2009
 *      Author: LIU Lu
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include "parser.h"

Graph** graphs = NULL;
int graphCount = 0;

void ReadVertices(char* buffer, FILE* pFile, Vertex*** vertexArrayAddr,
		int* vertexCount);

void ReadEdges(char* buffer, FILE* pFile, Edge*** edgeArrayAddr,
		int* edgeCount, Vertex** vertexArray, int vertexCount);

void ReadAttributes(const char* path);

int InitializeGraphs(char* graphsPath);

int Parse(const char* path)
{
	DIR *dirptr = NULL;
	struct dirent *entry;
	char graphspath[128];
	strcpy(graphspath, path);
	strcat(graphspath, "graphs\\");

	if (InitializeGraphs(graphspath) == EXIT_FAILURE)
	{
		printf("initialization of graphs failed\n");
		return EXIT_FAILURE;
	}

	if ((dirptr = opendir(graphspath)) == NULL)
	{
		printf("can not open directory!\n");
		return EXIT_FAILURE;
	}
	else
	{
		int modeNum = -1;
		while ((entry = readdir(dirptr)))
		{
			FILE* pFile = NULL;
			char* mode;
			char buffer[256];
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..")
					!= 0 && strcmp(entry->d_name, ".svn") != 0)
			{
				char filepath[128];
				Graph* tmpGraph;
				strcpy(filepath, graphspath);
				strcat(filepath, entry->d_name);
				mode = strtok(entry->d_name, ".");
				modeNum++;
				tmpGraph = (Graph*) malloc(sizeof(Graph));
				strcpy(tmpGraph->id, mode);
				pFile = fopen(filepath, "r");
				if (pFile != NULL)
				{
					int numOfVertices = 0, numOfEdges = 0;
					Vertex** vertices;
					Edge** edges;
					ReadVertices(buffer, pFile, &vertices, &numOfVertices);
					ReadEdges(buffer, pFile, &edges, &numOfEdges, vertices,
							numOfVertices);
					tmpGraph->edges = edges;
					tmpGraph->edgeCount = numOfEdges;
					tmpGraph->vertices = vertices;
					tmpGraph->vertexCount = numOfVertices;
					graphs[modeNum] = tmpGraph;
				}
				fclose(pFile);
			}
		}
		if (closedir(dirptr))
		{
			printf("can not close directory\n");
			return EXIT_FAILURE;
		}
		ReadAttributes(path);
	}
	return EXIT_SUCCESS;
}

int InitializeGraphs(char* graphsPath)
{
	DIR *dirptr = NULL;
	struct dirent *entry;
	graphCount = 0;
	graphs = NULL;
	if ((dirptr = opendir(graphsPath)) == NULL)
	{
		printf("can not open directory!\n");
		return EXIT_FAILURE;
	}
	else
	{
		/* get the number of modes */
		while ((entry = readdir(dirptr)))
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..")
					!= 0 && strcmp(entry->d_name, ".svn") != 0)
				graphCount++;
		graphs = (Graph **) calloc(graphCount, sizeof(Graph*));
		if (closedir(dirptr))
		{
			printf("can not close directory\n");
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

void ReadVertices(char* buffer, FILE* pFile, Vertex*** vertexArrayAddr,
		int* vertexCount)
{
	/* read the vertices information from the first segment */
	int numOfVertices, i = 0;
	fgets(buffer, 256, pFile);
	numOfVertices = atoi(buffer);
	*vertexCount = numOfVertices;
	*vertexArrayAddr = (Vertex**) calloc(numOfVertices, sizeof(Vertex*));
	for (i = 0; i < numOfVertices; i++)
	{
		Vertex* tmpVertex;
		char* pch;
		fgets(buffer, 256, pFile);
		tmpVertex = (Vertex *) malloc(sizeof(Vertex));
		/* vertex id */
		pch = strtok(buffer, ";");
		strcpy(tmpVertex->id, pch);
		/* X coordinate */
		pch = strtok(NULL, ";");
		/*tmpVertex->x = atof(pch);*/
		/* Y coordinate */
		pch = strtok(NULL, ";");
		/*tmpVertex->y = atof(pch);*/
		/* out degree */
		pch = strtok(NULL, ";");
		tmpVertex->outdegree = atoi(pch);
		tmpVertex->first = -1;
		strcpy(tmpVertex->attr_amenity, "");
		(*vertexArrayAddr)[i] = tmpVertex;
	}
}

void ReadEdges(char* buffer, FILE* pFile, Edge*** edgeArrayAddr, int* edgeCount,
		Vertex** vertexArray, int vertexCount)
{
	/* read the edges information from the second segment */
	int numOfEdges, i = 0;
	char lastStartVertexId[32];
	fgets(buffer, 256, pFile);
	numOfEdges = atoi(buffer);
	*edgeCount = numOfEdges;
	*edgeArrayAddr = (Edge **) calloc(numOfEdges, sizeof(Edge *));
	strcpy(lastStartVertexId, "");
	for (i = 0; i < numOfEdges; i++)
	{
		Edge* tmpEdge;
		char* pch;
		double length, weight;
		fgets(buffer, 256, pFile);
		/* there are 5 fields in one line record */
		/* edge id */
		tmpEdge = (Edge*) malloc(sizeof(Edge));
		pch = strtok(buffer, ";");
		strcpy(tmpEdge->id, pch);
		/* from vertex id */
		pch = strtok(NULL, ";");
		tmpEdge->start = SearchVertexById(vertexArray, vertexCount, pch);
		if (strcmp(pch, lastStartVertexId) != 0)
			tmpEdge->start->first = i;
		strcpy(lastStartVertexId, pch);
		/* to vertex id*/
		pch = strtok(NULL, ";");
		tmpEdge->end = SearchVertexById(vertexArray, vertexCount, pch);
		/* edge length */
		pch = strtok(NULL, ";");
		length = atof(pch);
		/*tmpEdge->length = atof(pch);*/
		/* edge weight */
		pch = strtok(NULL, ";");
		weight = atof(pch);
		/*tmpEdge->weight = atof(pch);*/
		tmpEdge->cost = (int) (length * weight * 1000);
		(*edgeArrayAddr)[i] = tmpEdge;
	}
}

void ReadAttributes(const char* path)
{
	char filepath[128];
	FILE* pFile;
	strcpy(filepath, path);
	strcat(filepath, "attributes\\vertexattr.txt");
	pFile = fopen(filepath, "r");
	if (pFile != NULL)
	{
		char buffer[256];
		while (fgets(buffer, 256, pFile) != NULL)
		{
			char *pch = strtok(buffer, ";:");
			char vertexId[32];
			int i = 0;
			strcpy(vertexId, pch);
			pch = strtok(NULL, ";:");
			/* ignore all the non-amenity attributes temporally */
			if (strcmp(pch, "amenity") != 0)
				continue;
			pch = strtok(NULL, ";:");
			for (i = 0; i < graphCount; i++)
			{
				Vertex *attributedVertex = SearchVertexById(
						graphs[i]->vertices, graphs[i]->vertexCount, vertexId);
				if (attributedVertex != NULL)
					strcpy(attributedVertex->attr_amenity, pch);
			}
		}
	}
	fclose(pFile);
}

Vertex* SearchVertexById(Vertex** vertex_array, int len, const char* id)
{
	int i = 0;
	for (i = 0; i < len; i++)
	{
		if (strcmp(vertex_array[i]->id, id) == 0)
			return vertex_array[i];
	}
	return NNULL;
}

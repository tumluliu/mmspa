/*
 * parser.c
 *
 *  Created on: Mar 18, 2009
 *      Author: LIU Lu
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libpq-fe.h"
#include "mmspa4pg/parser.h"

Graph** graphs = NULL;
SwitchPoint*** switchpointsArr = NULL;
int* switchpointCounts = NULL;
int graphCount = 0;
PGconn* conn;

//void ReadVertices(PGresult* res, Vertex*** vertexArrayAddr);

void ReadGraph(PGresult* res, Vertex*** vertexArrayAddr, int vertexCount, const char* costFactor);
		
void ReadSwitchPoints(PGresult* res, SwitchPoint*** switchpointArrayAddr);

void CombineGraphs(Vertex** vertexArray, int vertexCount, SwitchPoint** switchpointArray, int switchpointCount);

int InitializeGraphs(int graphCount);

static void exit_nicely(PGconn *conn)
{
    PQfinish(conn);
    exit(1);
}

int ConnectDB(const char* pgConnString)
{
	conn = PQconnectdb(pgConnString);
	if (PQstatus(conn) != CONNECTION_OK)
    {
    	printf("Connection to database failed: %s", PQerrorMessage(conn));
		return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void DisconnectDB()
{
	PQfinish(conn);
}

int Parse()
{
	extern MultimodalRoutingPlan* plan;	
	
	if (InitializeGraphs(plan->mode_count) == EXIT_FAILURE)
	{
		printf("initialization of graphs failed\n");
		return EXIT_FAILURE;
	}

	int i = 0;
	graphCount = plan->mode_count;
	for (i = 0; i < plan->mode_count; i++)
	{
		int modeId = plan->mode_id_list[i];
		int vertexCount = 0;
		char switchFilterCondition[1024];
		char vertexFilterCondition[512];
		char graphFilterCondition[1024];
		Vertex** vertices;
		SwitchPoint** switchpoints;
		Graph* tmpGraph;
		tmpGraph = (Graph*) malloc(sizeof(Graph));
		tmpGraph->id = modeId;
		if (modeId == 1900)
		{
			// construct query clause for foot and all the selected public transit modes;
			int j = 0;
			char publicModesClause[256];
			sprintf(publicModesClause, "vertices.mode_id = 1002 OR vertices.mode_id = %d", plan->public_transit_mode_id_set[0]);
			for (j = 1; j < plan->public_transit_mode_count; j++)
			{
				int publicModeId = plan->public_transit_mode_id_set[j];
				char strPublicModeIdSeg[32];
				sprintf(strPublicModeIdSeg, " OR vertices.mode_id = %d", publicModeId);
				strcat(publicModesClause, strPublicModeIdSeg);
			}
//			sprintf(graphFilterCondition,
//			"(SELECT vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, vertices.out_degree, edges.mode_id, edges.edge_id FROM edges \
//INNER JOIN vertices ON edges.from_id=vertices.vertex_id WHERE (%s) \
//UNION SELECT vertices.vertex_id, NULL, NULL, NULL, NULL, out_degree, mode_id FROM vertices \
//WHERE out_degree=0 AND (%s)) ORDER BY vertex_id", publicModesClause, publicModesClause);
			sprintf(graphFilterCondition, 
			"(SELECT vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, vertices.out_degree, edges.mode_id FROM edges \
INNER JOIN vertices ON edges.from_id=vertices.vertex_id WHERE (%s) \
UNION SELECT vertices.vertex_id, NULL, NULL, NULL, out_degree, mode_id FROM vertices \
WHERE out_degree=0 AND (%s)) ORDER BY vertex_id", publicModesClause, publicModesClause);
			sprintf(vertexFilterCondition, "SELECT COUNT(*) FROM vertices WHERE (%s)", publicModesClause);
		}
		else
		{
//			sprintf(graphFilterCondition,
//			"(SELECT vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, vertices.out_degree, edges.mode_id, edges.edge_id FROM edges \
//INNER JOIN vertices ON edges.from_id=vertices.vertex_id WHERE edges.mode_id=%d \
//UNION SELECT vertices.vertex_id, NULL, NULL, NULL, NULL, out_degree, mode_id FROM vertices \
//WHERE out_degree=0 AND mode_id=%d) ORDER BY vertex_id", modeId, modeId);
			sprintf(graphFilterCondition, 
			"(SELECT vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, vertices.out_degree, edges.mode_id FROM edges \
INNER JOIN vertices ON edges.from_id=vertices.vertex_id WHERE edges.mode_id=%d \
UNION SELECT vertices.vertex_id, NULL, NULL, NULL, out_degree, mode_id FROM vertices \
WHERE out_degree=0 AND mode_id=%d) ORDER BY vertex_id", modeId, modeId);
			sprintf(vertexFilterCondition, "SELECT COUNT(*) FROM vertices WHERE mode_id=%d", modeId);
		}
		
		// Retrieve the number of vertices
		PGresult* vertexResults;
		vertexResults = PQexec(conn, vertexFilterCondition);
	    if (PQresultStatus(vertexResults) != PGRES_TUPLES_OK)
	    {
	        fprintf(stderr, "query in vertices table failed: %s", PQerrorMessage(conn));
	        PQclear(vertexResults);
	        exit_nicely(conn);
	    }
		vertexCount = atoi(PQgetvalue(vertexResults, 0, 0));
		PQclear(vertexResults);

		// Retrieve and read graph
		PGresult* graphResults;
		graphResults = PQexec(conn, graphFilterCondition);
	    if (PQresultStatus(graphResults) != PGRES_TUPLES_OK)
	    {
	        fprintf(stderr, "query in edges and vertices table failed: %s", PQerrorMessage(conn));
	        PQclear(graphResults);
	        exit_nicely(conn);
	    }
		ReadGraph(graphResults, &vertices, vertexCount, plan->cost_factor);
		PQclear(graphResults);
		
		// Combine all the selected public transit modes and foot networks
		// if current mode_id is 1900
		if (modeId == 1900)
		{
			// read switch points between all the PT mode pairs 
			int j = 0, k = 0;
			char publicSwitchClause[1024];
			sprintf(publicSwitchClause, 
				" ((from_mode_id = 1002 AND to_mode_id = %d) OR (from_mode_id = %d AND to_mode_id = 1002))", 
				plan->public_transit_mode_id_set[0], plan->public_transit_mode_id_set[0]);
			for (j = 1; j < plan->public_transit_mode_count; j++)
			{
				int publicModeId = plan->public_transit_mode_id_set[j];					
				char strPublcSwitchSeg[128];
				sprintf(strPublcSwitchSeg, 
					" OR ((from_mode_id = 1002 AND to_mode_id = %d) OR (from_mode_id = %d AND to_mode_id = 1002))", 
					publicModeId, publicModeId);
				strcat(publicSwitchClause, strPublcSwitchSeg);
				// Retrieve switch point infos between (1002, j)
			}
			for (j = 0; j < plan->public_transit_mode_count - 1; j++)
				for (k = j + 1; k < plan->public_transit_mode_count; k++) 
				{
					int fromPublicModeId = plan->public_transit_mode_id_set[j];
					int toPublicModeId = plan->public_transit_mode_id_set[k];
					// Retrieve switch point infos between (j,k)
					char strPublcSwitchSeg[128];
					sprintf(strPublcSwitchSeg, 
						" OR ((from_mode_id = %d AND to_mode_id = %d) OR (from_mode_id = %d AND to_mode_id = %d))",
						fromPublicModeId, toPublicModeId, toPublicModeId, fromPublicModeId);  
					strcat(publicSwitchClause, strPublcSwitchSeg);
				}
			sprintf(switchFilterCondition, "SELECT from_vertex_id, to_vertex_id, cost FROM switch_points WHERE %s", publicSwitchClause);
			PGresult* switchpointResults;
			switchpointResults = PQexec(conn, switchFilterCondition);
		    if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK)
		    {
		        fprintf(stderr, "query in switch_points table failed: %s", PQerrorMessage(conn));
		        PQclear(switchpointResults);
		        exit_nicely(conn);
		    }
		    SwitchPoint** publicSwitchpoints;
		    int publicSwitchpointCount;
		    publicSwitchpointCount = PQntuples(switchpointResults);
			ReadSwitchPoints(switchpointResults, &publicSwitchpoints);
			PQclear(switchpointResults);
			// combine the graphs by adding switch lines in the (vertices, edges) set and get the mode 1900 graph
			CombineGraphs(vertices, vertexCount, publicSwitchpoints, publicSwitchpointCount);
		}
		
		tmpGraph->vertices = vertices;
		tmpGraph->vertex_count = vertexCount;
		graphs[i] = tmpGraph;
		if (i > 0)
		{
			int j = 0;
			if ((plan->mode_id_list[i-1] != 1900) && (plan->mode_id_list[i] != 1900))
				sprintf(switchFilterCondition, "SELECT from_vertex_id, to_vertex_id, cost FROM switch_points WHERE from_mode_id=%d AND to_mode_id=%d AND %s", 
					plan->mode_id_list[i-1], plan->mode_id_list[i], plan->switch_condition_list[i-1]);
			else if (plan->mode_id_list[i-1] == 1900)
			{
				char strFromModeClause[256];
				sprintf(strFromModeClause, "from_mode_id = 1002 ");
				for (j = 0; j < plan->public_transit_mode_count; j++)
				{
					int publicModeId = plan->public_transit_mode_id_set[j];
					char strPublicSegClause[128];
					sprintf(strPublicSegClause, "OR from_mode_id = %d ", publicModeId);
					strcat(strFromModeClause, strPublicSegClause);
				}
				sprintf(switchFilterCondition, "SELECT from_vertex_id, to_vertex_id, cost FROM switch_points WHERE (%s) AND to_mode_id=%d AND %s", 
					strFromModeClause, plan->mode_id_list[i], plan->switch_condition_list[i-1]);
			}
			else if (plan->mode_id_list[i] == 1900)
			{
				char strToModeClause[256];
				sprintf(strToModeClause, "to_mode_id = 1002 ");
				for (j = 0; j < plan->public_transit_mode_count; j++)
				{
					int publicModeId = plan->public_transit_mode_id_set[j];
					char strPublicSegClause[128];
					sprintf(strPublicSegClause, "OR to_mode_id = %d ", publicModeId);
					strcat(strToModeClause, strPublicSegClause);
				}
				sprintf(switchFilterCondition, "SELECT from_vertex_id, to_vertex_id, cost FROM switch_points WHERE from_mode_id=%d AND (%s) AND %s", 
					plan->mode_id_list[i-1], strToModeClause, plan->switch_condition_list[i-1]);
			}
			PGresult* switchpointResults;
			switchpointResults = PQexec(conn, switchFilterCondition);
		    if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK)
		    {
		        fprintf(stderr, "query in switch_points table failed: %s", PQerrorMessage(conn));
		        PQclear(switchpointResults);
		        exit_nicely(conn);
		    }
			switchpointCounts[i-1] = PQntuples(switchpointResults);
//			printf("number of switch points: %d\n", switchpointCounts[i-1]);
			ReadSwitchPoints(switchpointResults, &switchpoints);
			switchpointsArr[i-1] = switchpoints;
			PQclear(switchpointResults);
		}
	}
	
	return EXIT_SUCCESS;
}

int InitializeGraphs(int graphCount)
{
	graphs = (Graph **) calloc(graphCount, sizeof(Graph*));
	if (graphCount > 1)
	{
		switchpointsArr = (SwitchPoint ***) calloc(graphCount - 1, sizeof(SwitchPoint**));
		switchpointCounts = (int *) calloc(graphCount - 1, sizeof(int));
	}
	return EXIT_SUCCESS;
}

void ReadGraph(PGresult* res, Vertex*** vertexArrayAddr, int vertexCount, const char* costFactor)
{
	/* read the edges and vertices from the query result */
	int recordCount = 0, i = 0, outgoingCursor = 0, vertexCursor = 0;
	recordCount = PQntuples(res);	
	*vertexArrayAddr = (Vertex**) calloc(vertexCount, sizeof(Vertex*));
	Vertex* tmpVertex = NULL;
	for (i = 0; i < recordCount; i++)
	{
		/* fields in the query results:		 * 
		 * vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, vertices.out_degree, edges.mode_id, edges.edge_id
		 * 0,                  1,           2,            3,                  4,                   5,             6
		 */
		Edge* tmpEdge;
		tmpEdge = (Edge*) malloc(sizeof(Edge));
		if (outgoingCursor == 0)
		{
			// a new group of edge records with the same from_vertex starts...			
			tmpVertex = (Vertex *) malloc(sizeof(Vertex));
			/* vertex id */		
			tmpVertex->id = atoll(PQgetvalue(res, i, 0));
			/* out degree */		
			tmpVertex->outdegree = atoi(PQgetvalue(res, i, 4));
//			tmpVertex->first = -1;
			tmpVertex->outgoing = NULL;
			(*vertexArrayAddr)[vertexCursor] = tmpVertex;
			vertexCursor++;
			if (tmpVertex->outdegree == 0)
				continue;
		}
		outgoingCursor++;
		if (outgoingCursor == atoi(PQgetvalue(res, i, 4)))
			outgoingCursor = 0;
		/* edge id */
		//tmpEdge->id = atoll(PQgetvalue(res, i, 1));
		/* from vertex id */
		tmpEdge->from_vertex_id = atoll(PQgetvalue(res, i, 0));
		/* to vertex id*/
		tmpEdge->to_vertex_id = atoll(PQgetvalue(res, i, 1));
		/* edge length */
		tmpEdge->length = atof(PQgetvalue(res, i, 2));
		/* speed factor */
		tmpEdge->speed_factor = atof(PQgetvalue(res, i, 3));
		/* length factor, === 1.0 */
		tmpEdge->length_factor = 1.0;
		/* mode id of edge */
		tmpEdge->mode_id = atoi(PQgetvalue(res, i, 5));
		/* attach end to the adjacency list of start */
		Edge* outgoingEdge = tmpVertex->outgoing;
		if (tmpVertex->outgoing == NULL)
			tmpVertex->outgoing = tmpEdge;
		else
		{
			while (outgoingEdge->adjNext != NULL)
				outgoingEdge = outgoingEdge->adjNext;
			outgoingEdge->adjNext = tmpEdge;
		}
		tmpEdge->adjNext = NULL;
	}
}

void ReadSwitchPoints(PGresult* res, SwitchPoint*** switchpointArrayAddr)
{
	/* read the switch_points information from the query result */
	int switchpointCount, i = 0;
	switchpointCount = PQntuples(res);
	*switchpointArrayAddr = (SwitchPoint **) calloc(switchpointCount, sizeof(SwitchPoint *));
	for (i = 0; i < switchpointCount; i++)
	{
		/* fields in query results:
		 * from_vertex_id, to_vertex_id, cost
		 * 0,              1,            2
		 */
		SwitchPoint* tmpSwitchPoint;
		tmpSwitchPoint = (SwitchPoint*) malloc(sizeof(SwitchPoint));
		/* from vertex id */
		tmpSwitchPoint->from_vertex_id = atoll(PQgetvalue(res, i, 0));
		/* to vertex id */
		tmpSwitchPoint->to_vertex_id = atoll(PQgetvalue(res, i, 1));
		tmpSwitchPoint->speed_factor = 0.015;
		tmpSwitchPoint->length_factor = 1.0;
		tmpSwitchPoint->length = atof(PQgetvalue(res, i, 2)) * tmpSwitchPoint->speed_factor;
		(*switchpointArrayAddr)[i] = tmpSwitchPoint;
	}
}

void CombineGraphs(Vertex** vertexArray, int vertexCount, SwitchPoint** switchpointArray, int switchpointCount)
{
	// Treat all the switch point pairs as new edges and add them into the graph
	int i = 0;
	for (i = 0; i < switchpointCount; i++)
	{
		Edge* tmpEdge;
		tmpEdge = (Edge*) malloc(sizeof(Edge));
		/* from vertex */
		tmpEdge->from_vertex_id = switchpointArray[i]->from_vertex_id;
		/* to vertex */
		tmpEdge->to_vertex_id = switchpointArray[i]->to_vertex_id;
		/* attach end to the adjacency list of start */
		Vertex* vertexFrom = BinarySearchVertexById(vertexArray, 0, vertexCount - 1, switchpointArray[i]->from_vertex_id);
		Edge* outgoingEdge = vertexFrom->outgoing;
		if (vertexFrom->outgoing == NULL)
			vertexFrom->outgoing = tmpEdge;
		else
		{
			while (outgoingEdge->adjNext != NULL)
				outgoingEdge = outgoingEdge->adjNext;
			outgoingEdge->adjNext = tmpEdge;
		}
		vertexFrom->outdegree++;
		tmpEdge->mode_id = 1002;
		tmpEdge->adjNext = NULL;
		/* edge length */
		tmpEdge->length = switchpointArray[i]->length;
		/* speed factor */
		tmpEdge->speed_factor = switchpointArray[i]->speed_factor;
		/* length factor, === 1.0 */
		tmpEdge->length_factor = 1.0;
	}
}

// Linear search when the vertex array is not sorted 
Vertex* SearchVertexById(Vertex** vertexArray, int len, long long id)
{
	int i = 0;
	for (i = 0; i < len; i++)
	{
		if (vertexArray[i]->id == id)
			return vertexArray[i];
	}
	return NNULL;
}

// Binary search when the vertex array is sorted
Vertex* BinarySearchVertexById(Vertex** vertexArray, int low, int high, long long id)
{
	if (high < low)
		return NNULL; // not found
	int mid = (low + high) / 2;
	if (vertexArray[mid]->id > id)
		return BinarySearchVertexById(vertexArray, low, mid - 1, id);
	else if (vertexArray[mid]->id < id)
		return BinarySearchVertexById(vertexArray, mid + 1, high, id);
	else
		return vertexArray[mid];
}

/*
 * =====================================================================================
 *
 *       Filename:  graphassembler.c
 *
 *    Description:  Read multimodal graph data from external data source,
 *    assemble multimodal graph set on the fly for each routing plan.
 *
 *        Created:  2009/03/18 09时59分45秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "libpq-fe.h"
#include "../include/graphassembler.h"
#include "../include/routingplan.h"

/* Global variable definitions */
ModeGraph **activeGraphs = NULL;
SwitchPoint ***switchpointsArr = NULL;
int *switchpointCounts = NULL;
int graphCount = 0;

static void disposeGraphs();
static void disposeSwitchPoints();
extern void DisposeRoutingPlan();
extern void DisposeResultPathTable();

static void exitPostgreNicely(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

PGconn* conn;

static int connectPostgre(const char *pgConnString) {
    conn = PQconnectdb(pgConnString);
    if (PQstatus(conn) != CONNECTION_OK) {
        printf("Connection to database failed: %s", PQerrorMessage(conn));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static void disconnectPostgre() {
    PQfinish(conn);
}

static ModeGraph graphBase[TOTAL_MODES];

static void loadAllGraphs() {

}


static int initGraphs(int graphCount) {
    activeGraphs = (ModeGraph **) calloc(graphCount, sizeof(ModeGraph*));
    if (graphCount > 1) {
        switchpointsArr = (SwitchPoint ***) calloc(graphCount - 1, 
                sizeof(SwitchPoint**));
        switchpointCounts = (int *) calloc(graphCount - 1, sizeof(int));
    }
    return EXIT_SUCCESS;
}

static void readGraph(PGresult *res, Vertex ***vertexArrayAddr, int vertexCount, 
        const char *costFactor) {
    /* read the edges and vertices from the query result */
    int recordCount = 0, i = 0, outgoingCursor = 0, vertexCursor = 0;
    recordCount = PQntuples(res);	
    *vertexArrayAddr = (Vertex**) calloc(vertexCount, sizeof(Vertex*));
    Vertex* tmpVertex = NULL;
    for (i = 0; i < recordCount; i++) {
        /* fields in the query results:		 * 
         * vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, vertices.out_degree, edges.mode_id, edges.edge_id
         * 0,                  1,           2,            3,                  4,                   5,             6
         */
        Edge* tmpEdge;
        tmpEdge = (Edge*) malloc(sizeof(Edge));
        if (outgoingCursor == 0) {
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
        Edge *outgoingEdge = tmpVertex->outgoing;
        if (tmpVertex->outgoing == NULL)
            tmpVertex->outgoing = tmpEdge;
        else {
            while (outgoingEdge->adjNext != NULL)
                outgoingEdge = outgoingEdge->adjNext;
            outgoingEdge->adjNext = tmpEdge;
        }
        tmpEdge->adjNext = NULL;
    }
}

static void readSwitchPoints(PGresult *res, SwitchPoint ***switchpointArrayAddr) {
    /* read the switch_points information from the query result */
    int switchpointCount, i = 0;
    switchpointCount = PQntuples(res);
    *switchpointArrayAddr = (SwitchPoint **) calloc(switchpointCount, 
            sizeof(SwitchPoint *));
    for (i = 0; i < switchpointCount; i++) {
        /* fields in query results:
         * from_vertex_id, to_vertex_id, cost
         * 0,              1,            2
         */
        SwitchPoint *tmpSwitchPoint;
        tmpSwitchPoint = (SwitchPoint*) malloc(sizeof(SwitchPoint));
        /* from vertex id */
        tmpSwitchPoint->from_vertex_id = atoll(PQgetvalue(res, i, 0));
        /* to vertex id */
        tmpSwitchPoint->to_vertex_id = atoll(PQgetvalue(res, i, 1));
        /* ALL switching action are treated as walking mode */
        tmpSwitchPoint->speed_factor = 0.015;
        tmpSwitchPoint->length_factor = 1.0;
        tmpSwitchPoint->length = atof(PQgetvalue(res, i, 2)) * 
            tmpSwitchPoint->speed_factor;
        (*switchpointArrayAddr)[i] = tmpSwitchPoint;
    }
}

static void combineGraphs(Vertex **vertexArray, int vertexCount, 
        SwitchPoint **switchpointArray, int switchpointCount) {
    // Treat all the switch point pairs as new edges and add them into the graph
#ifdef DEBUG
    printf("[DEBUG] Start embedding %d switch points into multimodal graph with %d vertices... \n", switchpointCount, vertexCount);
#endif
    int i = 0;
    for (i = 0; i < switchpointCount; i++) {
#ifdef DEBUG
        printf("[DEBUG] Processing switch point %d\n", i + 1);
        printf("[DEBUG] from vertex id: %lld\n", switchpointArray[i]->from_vertex_id);
        printf("[DEBUG] to vertex id: %lld\n", switchpointArray[i]->to_vertex_id);
#endif
        Edge* tmpEdge;
        tmpEdge = (Edge*) malloc(sizeof(Edge));
        /* from vertex */
        tmpEdge->from_vertex_id = switchpointArray[i]->from_vertex_id;
        /* to vertex */
        tmpEdge->to_vertex_id = switchpointArray[i]->to_vertex_id;
        /* attach end to the adjacency list of start */
        // TODO: should check if the vertex searching result is null
        Vertex* vertexFrom = BinarySearchVertexById(vertexArray, 0, 
                vertexCount - 1, switchpointArray[i]->from_vertex_id);
        Edge* outgoingEdge = vertexFrom->outgoing;
        if (vertexFrom->outgoing == NULL)
            vertexFrom->outgoing = tmpEdge;
        else {
            while (outgoingEdge->adjNext != NULL)
                outgoingEdge = outgoingEdge->adjNext;
            outgoingEdge->adjNext = tmpEdge;
        }
        vertexFrom->outdegree++;
        tmpEdge->mode_id = FOOT;
        tmpEdge->adjNext = NULL;
        /* edge length */
        tmpEdge->length = switchpointArray[i]->length;
        /* speed factor */
        tmpEdge->speed_factor = switchpointArray[i]->speed_factor;
        /* length factor, === 1.0 */
        tmpEdge->length_factor = 1.0;
    }
#ifdef DEBUG
    printf("[DEBUG] Finish combining.\n");
#endif
}

// Linear search when the vertex array is not sorted 
Vertex* SearchVertexById(Vertex** vertexArray, int len, int64_t id) {
    int i = 0;
    for (i = 0; i < len; i++) {
        if (vertexArray[i]->id == id)
            return vertexArray[i];
    }
    return NNULL;
}

// Binary search when the vertex array is sorted
Vertex* BinarySearchVertexById(Vertex** vertexArray, int low, int high, 
        int64_t id) {
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

// Check if the constructed graph has dirty data
static int validateGraph(ModeGraph* g) {
    for (int i = 0; i < g->vertex_count; i++) {
        if (g->vertices[i] == NNULL) {
            // found a NULL vertex
            printf("FATAL: NULL vertex found in graph, seq number is %d, \
                    previous vertex id is %lld\n", i, g->vertices[i-1]->id);
            return EXIT_FAILURE;
        }
        int claimed_outdegree = g->vertices[i]->outdegree;
        int real_outdegree = 0;
        if ((claimed_outdegree == 0) && (g->vertices[i]->outgoing != NULL)) {
            // outgoing edges are not NULL while outdegree is 0
            printf("FATAL: bad vertex structure found! \
                    Outdegree is 0 while outgoing is not NULL. \
                    Problematic vertex id is %lld\n", g->vertices[i]->id);
            return EXIT_FAILURE;
        }
        if (claimed_outdegree >= 1) {
            if (g->vertices[i]->outgoing == NULL) {
                // outgoing edge is NULL while outdegree is larger than 0
                printf("FATAL: bad vertex structure found! \
                        Outdegree > 1 while outgoing is NULL. \
                        Problematic vertex id is %lld\n", g->vertices[i]->id);
                return EXIT_FAILURE;
            }
            Edge* pEdge = g->vertices[i]->outgoing;
            while (pEdge != NULL) {
                if (pEdge->from_vertex_id != g->vertices[i]->id) {
                    // found foreign edges not emitted from the current vertex
                    printf("FATAL: bad vertex structure found! \
                            Found an outgoing edge NOT belonging to this vertex. \
                            Problematic vertex id is %lld, edge's from_vertex_id \
                            is %lld\n", g->vertices[i]->id, pEdge->from_vertex_id);
                    return EXIT_FAILURE;
                }
                real_outdegree++;
                pEdge = pEdge->adjNext;
            }
            if (real_outdegree != claimed_outdegree) {
                // real outdegree calculated by counting the outgoing edges is 
                // not equal to the recorded outdegree
                printf("FATAL: bad vertex structure found! \
                        Number of outgoing edges is NOT equal to the outdegree \
                        it claims. Problematic vertex id is %lld\n", 
                        g->vertices[i]->id);
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}

// Retrieve the number of vertices
static int getVertexCount(const char *vertexFilterCond) {
    PGresult *vertexResults;
    vertexResults = PQexec(conn, vertexFilterCond);
    if (PQresultStatus(vertexResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in vertices table failed: %s", 
                PQerrorMessage(conn));
        PQclear(vertexResults);
        exitPostgreNicely(conn);
    }
    int vc = atoi(PQgetvalue(vertexResults, 0, 0));
#ifdef DEBUG
    printf("[DEBUG] found vertex %d\n", vc);
#endif
    PQclear(vertexResults);
    return vc;
}

// Retrieve and read graph
static void retrieveGraphData(const char *graphFilterCond, Vertex **vertices, 
        int vertexCount, RoutingPlan *p) {
    PGresult *graphResults;
    graphResults = PQexec(conn, graphFilterCond);
    if (PQresultStatus(graphResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in edges and vertices table failed: %s", 
                PQerrorMessage(conn));
        PQclear(graphResults);
        exitPostgreNicely(conn);
    }
#ifdef DEBUG
    printf("[DEBUG] Reading graphs... ");
#endif
    readGraph(graphResults, &vertices, vertexCount, p->cost_factor);
#ifdef DEBUG
    printf(" done.\n");
#endif
    PQclear(graphResults);
}

// construct query clause for foot and all the selected public 
// transit modes;
static char *constructPublicModeClause(RoutingPlan *p) {
    int j = 0;
    static char publicModeClause[256];
    sprintf(publicModeClause, 
            "vertices.mode_id = %d OR vertices.mode_id = %d", 
            FOOT, p->public_transit_mode_id_set[0]);
    for (j = 1; j < p->public_transit_mode_count; j++) {
        int publicModeId = p->public_transit_mode_id_set[j];
        char strPublicModeIdSeg[32];
        sprintf(strPublicModeIdSeg, " OR vertices.mode_id = %d", 
                publicModeId);
        strcat(publicModeClause, strPublicModeIdSeg);
    }
    return publicModeClause;
}

static void constructFilterConditions(RoutingPlan *p, int modeId, 
        char *vertexFilterCond, char *graphFilterCond) {
    if (modeId == PUBLIC_TRANSPORTATION) {
        char *pmodeClause = constructPublicModeClause(p);
        sprintf(graphFilterCond, 
                "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                WHERE (%s) UNION SELECT vertices.vertex_id, NULL, NULL, NULL, \
                out_degree, mode_id FROM vertices WHERE out_degree=0 AND (%s)) \
            ORDER BY vertex_id",
                pmodeClause, pmodeClause);
        sprintf(vertexFilterCond, 
                "SELECT COUNT(*) FROM vertices WHERE (%s)", pmodeClause);
    }
    else {
        sprintf(graphFilterCond, 
                "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                WHERE edges.mode_id=%d UNION SELECT vertices.vertex_id, NULL, \
                NULL, NULL, out_degree, mode_id FROM vertices WHERE \
                out_degree=0 AND mode_id=%d) ORDER BY vertex_id", 
                modeId, modeId);
        sprintf(vertexFilterCond, 
                "SELECT COUNT(*) FROM vertices WHERE mode_id=%d", modeId);
    }
}

static void addPublicSwitchClauseToFilter(RoutingPlan *p, char *switchFilterCond) {
    int j = 0, k = 0;
    char psClause[1024];
    sprintf(psClause, " ((from_mode_id = %d AND to_mode_id = %d) OR \
        (from_mode_id = %d AND to_mode_id = %d))", 
            FOOT, p->public_transit_mode_id_set[0], 
            p->public_transit_mode_id_set[0], FOOT);
    for (j = 1; j < p->public_transit_mode_count; j++) {
        int publicModeId = p->public_transit_mode_id_set[j];	
        char psSegment[128];
        sprintf(psSegment, " OR ((from_mode_id = %d AND to_mode_id = %d) OR \
            (from_mode_id = %d AND to_mode_id = %d))", 
                FOOT, publicModeId, publicModeId, FOOT);
        strcat(psClause, psSegment);
        // Retrieve switch point infos between (FOOT, j)
    }
    for (j = 0; j < p->public_transit_mode_count - 1; j++)
        for (k = j + 1; k < p->public_transit_mode_count; k++) {
            int fromPublicModeId = p->public_transit_mode_id_set[j];
            int toPublicModeId = p->public_transit_mode_id_set[k];
            // Retrieve switch point infos between (j,k)
            char psSegment[128];
            sprintf(psSegment, " OR ((from_mode_id = %d AND to_mode_id = %d) OR \
                (from_mode_id = %d AND to_mode_id = %d))",
                    fromPublicModeId, toPublicModeId, 
                    toPublicModeId, fromPublicModeId);  
            strcat(psClause, psSegment);
        }
    sprintf(switchFilterCond, "SELECT from_vertex_id, to_vertex_id, cost FROM \
            switch_points WHERE %s", psClause);
#ifdef DEBUG
    printf("[DEBUG] SQL statement for filtering switch points: \n");
    printf("%s\n", switchFilterCond);
#endif
}

static void retrieveSwitchPointsFromDb(const char *switchFilterCond, int *spCount, 
        SwitchPoint **publicSPs) {
    PGresult *switchpointResults;
    switchpointResults = PQexec(conn, switchFilterCond);
    if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in switch_points table failed: %s", 
                PQerrorMessage(conn));
        PQclear(switchpointResults);
        exitPostgreNicely(conn);
    }
    *spCount = PQntuples(switchpointResults);
#ifdef DEBUG
    printf("[DEBUG] Found switch points: %d\n", *spCount);
    printf("[DEBUG] Reading and parsing switch points...");
#endif
    readSwitchPoints(switchpointResults, &publicSPs);
#ifdef DEBUG
    printf(" done.\n");
#endif
    PQclear(switchpointResults);
}

// Combine all the selected public transit modes and foot networks
// if current mode_id is PUBLIC_TRANSPORTATION
static void constructPublicModeGraph(RoutingPlan *p, char *switchFilterCond, 
        Vertex **vertices, int vertexCount) {
    // read switch points between all the PT mode pairs 
    SwitchPoint **publicSwitchPoints = NULL;
    int publicSwitchPointCount = 0;
    addPublicSwitchClauseToFilter(p, switchFilterCond);
    retrieveSwitchPointsFromDb(switchFilterCond, &publicSwitchPointCount, 
            publicSwitchPoints);
    // combine the graphs by adding switch lines in the 
    // (vertices, edges) set and get the mode 
    // PUBLIC_TRANSPORTATION graph
#ifdef DEBUG
    printf("[DEBUG] Combining multimodal graphs for public transit...\n");
#endif
    combineGraphs(vertices, vertexCount, publicSwitchPoints, 
            publicSwitchPointCount);
#ifdef DEBUG
    printf(" done.\n");
#endif
}

static void constructSwitchFilterCondition(RoutingPlan *p, char *switchFilterCond, 
        int i) {
    int j = 0;
    if ((p->mode_id_list[i-1] != PUBLIC_TRANSPORTATION) && 
            (p->mode_id_list[i] != PUBLIC_TRANSPORTATION))
        sprintf(switchFilterCond, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE from_mode_id=%d AND \
                to_mode_id=%d AND %s", 
                p->mode_id_list[i-1], p->mode_id_list[i], 
                p->switch_condition_list[i-1]);
    else if (p->mode_id_list[i-1] == PUBLIC_TRANSPORTATION) {
        char fromModeClause[256];
        sprintf(fromModeClause, "from_mode_id = %d ", FOOT);
        for (j = 0; j < p->public_transit_mode_count; j++) {
            int publicModeId = p->public_transit_mode_id_set[j];
            char publicSegClause[128];
            sprintf(publicSegClause, "OR from_mode_id = %d ", 
                    publicModeId);
            strcat(fromModeClause, publicSegClause);
        }
        sprintf(switchFilterCond, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE (%s) AND to_mode_id=%d AND %s", 
                fromModeClause, p->mode_id_list[i], 
                p->switch_condition_list[i-1]);
    }
    else if (p->mode_id_list[i] == PUBLIC_TRANSPORTATION) {
        char toModeClause[256];
        sprintf(toModeClause, "to_mode_id = %d ", FOOT);
        for (j = 0; j < p->public_transit_mode_count; j++) {
            int publicModeId = p->public_transit_mode_id_set[j];
            char publicSegClause[128];
            sprintf(publicSegClause, "OR to_mode_id = %d ", publicModeId);
            strcat(toModeClause, publicSegClause);
        }
        sprintf(switchFilterCond, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE from_mode_id=%d AND (%s) AND %s", 
                p->mode_id_list[i-1], toModeClause, 
                p->switch_condition_list[i-1]);
    }
}

int LoadGraphFromDb(const char *pgConnStr) {
    /* Step 1: connect to database 
     * Step 2: read the series of graph data via SQL 
     * Step 3: read the switch points via SQL ?? */
    /* return 0 if everything succeeds, otherwise an error code */
    assert(connectPostgre(pgConnStr));
    loadAllGraphs();
    disconnectPostgre();
    return 0;
}

int ConnectDB(const char *pgConnStr) {
    return connectPostgre(pgConnStr);
}

void DisconnectDB() {
    disconnectPostgre();
}

int Parse() {
    return AssembleGraphs();
}

int AssembleGraphs() {
    extern RoutingPlan *plan;	
#ifdef DEBUG
    printf("[DEBUG] init multimodal graphs\n");
#endif
    if (initGraphs(plan->mode_count) == EXIT_FAILURE) {
        printf("initialization of graphs failed\n");
        return EXIT_FAILURE;
    }
    int i = 0;
    graphCount = plan->mode_count;
    for (i = 0; i < plan->mode_count; i++) {
        int modeId = plan->mode_id_list[i];
        int vertexCount = 0;
        char switchFilterCondition[1024];
        char vertexFilterCondition[512];
        char graphFilterCondition[1024];
        Vertex **vertices = NULL;
        ModeGraph *tmpGraph = (ModeGraph*) malloc(sizeof(ModeGraph));
        tmpGraph->id = modeId;
        constructFilterConditions(plan, modeId, vertexFilterCondition, 
                graphFilterCondition);
        vertexCount = getVertexCount(vertexFilterCondition);
        retrieveGraphData(graphFilterCondition, vertices, vertexCount, plan);
        if (modeId == PUBLIC_TRANSPORTATION) 
            constructPublicModeGraph(plan, switchFilterCondition, vertices, 
                    vertexCount);
        tmpGraph->vertices = vertices;
        tmpGraph->vertex_count = vertexCount;
        if (validateGraph(tmpGraph) == EXIT_FAILURE)
            return EXIT_FAILURE;
        activeGraphs[i] = tmpGraph;
        if (i > 0) {
            constructSwitchFilterCondition(plan, switchFilterCondition, i);
            retrieveSwitchPointsFromDb(switchFilterCondition, 
                    &switchpointCounts[i-1], switchpointsArr[i-1]);
        }
    }

    return EXIT_SUCCESS;
}

void Dispose() {
	disposeGraphs();
	disposeSwitchPoints();
	// FIXME: wierd... why not disposing result path table in DisposePaths()?
	DisposeResultPathTable();
	DisposeRoutingPlan();
}

static void disposeGraphs() {
	int i = 0;
	for (i = 0; i < graphCount; i++) {
		int j = 0;
		for (j = 0; j < activeGraphs[i]->vertex_count; j++) {
			Edge* current = activeGraphs[i]->vertices[j]->outgoing;
			while (current != NULL) {
				Edge* temp = current->adjNext;
				free(current);
				current = NULL;
				current = temp;
			}
			activeGraphs[i]->vertices[j]->outgoing = NULL;
			free(activeGraphs[i]->vertices[j]);
			activeGraphs[i]->vertices[j] = NULL;
		}
		free(activeGraphs[i]);
		activeGraphs[i] = NULL;
	}
	free(activeGraphs);
	activeGraphs = NULL;
}

static void disposeSwitchPoints() {
	if (graphCount > 1) {
		int i = 0, j = 0;
		for (i = 0; i < graphCount - 1; i++) {
			for (j = 0; j < switchpointCounts[i]; j++) {
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

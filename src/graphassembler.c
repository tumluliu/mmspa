/*
 * parser.c
 *
 *  Created on: Mar 18, 2009
 *      Author: LIU Lu
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "libpq-fe.h"
#include "../include/graphassembler.h"
#include "../include/routingplan.h"

ModeGraph **graphs = NULL;
SwitchPoint ***switchpointsArr = NULL;
int *switchpointCounts = NULL;
int graphCount = 0;

static void ReadSwitchPoints(PGresult *res, SwitchPoint ***switchpointArrayAddr);

static void CombineGraphs(Vertex **vertexArray, int vertexCount, 
        SwitchPoint **switchpointArray, int switchpointCount);

static int InitializeGraphs(int graphCount);

static int ValidateGraph(ModeGraph *g);

static void exit_nicely(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

PGconn* conn;

static int connect_db(const char *pgConnString) {
    conn = PQconnectdb(pgConnString);
    if (PQstatus(conn) != CONNECTION_OK) {
        printf("Connection to database failed: %s", PQerrorMessage(conn));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static void disconnect_db() {
    PQfinish(conn);
}


static ModeGraph **graphbase;
static SwitchPoint ** switchpointbase;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  load_graph_from_db
 *  Description:  Load ALL graph data and switch points from database 
 * =====================================================================================
 */
int load_graph_from_db(const char* pg_conn_str) 
{
    /* Step 1: connect to database 
     * Step 2: read the series of graph data via SQL 
     * Step 3: read the switch points via SQL */
    /* return 0 if everything succeeds, otherwise an error code */
    int ret = connect_db(pg_conn_str);
    assert(ret);
    load_modegraphs();
    load_switchpoints();
    return 0;
}

static void load_modegraphs() {
    return gs;
}

static void load_switchpoints() {
    return sps;
}

int assemble_graphs() {
    return parse();
}

int parse() {
    extern RoutingPlan *plan;	

#ifdef DEBUG
    printf("[DEBUG] init multimodal graphs\n");
#endif
    if (InitializeGraphs(plan->mode_count) == EXIT_FAILURE) {
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
        Vertex **vertices;
        SwitchPoint **switchpoints;
        ModeGraph *tmpGraph;
        tmpGraph = (ModeGraph*) malloc(sizeof(ModeGraph));
        tmpGraph->id = modeId;
        if (modeId == PUBLIC_TRANSPORT_MODE_ID) {
            // construct query clause for foot and all the selected public 
            // transit modes;
            int j = 0;
            char publicModesClause[256];
            sprintf(publicModesClause, 
                    "vertices.mode_id = %d OR vertices.mode_id = %d", 
                    WALKING_MODE_ID, plan->public_transit_mode_id_set[0]);
            for (j = 1; j < plan->public_transit_mode_count; j++) {
                int publicModeId = plan->public_transit_mode_id_set[j];
                char strPublicModeIdSeg[32];
                sprintf(strPublicModeIdSeg, " OR vertices.mode_id = %d", 
                        publicModeId);
                strcat(publicModesClause, strPublicModeIdSeg);
            }
            sprintf(graphFilterCondition, 
                    "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                    edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                    edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                    WHERE (%s) UNION SELECT vertices.vertex_id, NULL, NULL, NULL, \
                    out_degree, mode_id FROM vertices WHERE out_degree=0 AND (%s)) \
                ORDER BY vertex_id",
                    publicModesClause, publicModesClause);
            sprintf(vertexFilterCondition, 
                    "SELECT COUNT(*) FROM vertices WHERE (%s)", publicModesClause);
        }
        else {
            sprintf(graphFilterCondition, 
                    "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                    edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                    edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                    WHERE edges.mode_id=%d UNION SELECT vertices.vertex_id, NULL, \
                    NULL, NULL, out_degree, mode_id FROM vertices WHERE \
                    out_degree=0 AND mode_id=%d) ORDER BY vertex_id", 
                    modeId, modeId);
            sprintf(vertexFilterCondition, 
                    "SELECT COUNT(*) FROM vertices WHERE mode_id=%d", modeId);
        }

        // Retrieve the number of vertices
        PGresult *vertexResults;
        vertexResults = PQexec(conn, vertexFilterCondition);
        if (PQresultStatus(vertexResults) != PGRES_TUPLES_OK) {
            fprintf(stderr, "query in vertices table failed: %s", 
                    PQerrorMessage(conn));
            PQclear(vertexResults);
            exit_nicely(conn);
        }
        vertexCount = atoi(PQgetvalue(vertexResults, 0, 0));
#ifdef DEBUG
        printf("[DEBUG] found vertex %d\n", vertexCount);
#endif
        PQclear(vertexResults);

        // Retrieve and read graph
        PGresult *graphResults;
        graphResults = PQexec(conn, graphFilterCondition);
        if (PQresultStatus(graphResults) != PGRES_TUPLES_OK) {
            fprintf(stderr, "query in edges and vertices table failed: %s", 
                    PQerrorMessage(conn));
            PQclear(graphResults);
            exit_nicely(conn);
        }
#ifdef DEBUG
        printf("[DEBUG] Reading graphs... ");
#endif
        read_graph(graphResults, &vertices, vertexCount, plan->cost_factor);
#ifdef DEBUG
        printf(" done.\n");
#endif
        PQclear(graphResults);

        // Combine all the selected public transit modes and foot networks
        // if current mode_id is PUBLIC_TRANSPORT_MODE_ID
        if (modeId == PUBLIC_TRANSPORT_MODE_ID) {
            // read switch points between all the PT mode pairs 
            int j = 0, k = 0;
            char publicSwitchClause[1024];
            sprintf(publicSwitchClause, 
                    " ((from_mode_id = %d AND to_mode_id = %d) OR \
                    (from_mode_id = %d AND to_mode_id = %d))", 
                    WALKING_MODE_ID, plan->public_transit_mode_id_set[0], 
                    plan->public_transit_mode_id_set[0], WALKING_MODE_ID);
            for (j = 1; j < plan->public_transit_mode_count; j++) {
                int publicModeId = plan->public_transit_mode_id_set[j];	
                char strPublcSwitchSeg[128];
                sprintf(strPublcSwitchSeg, 
                        " OR ((from_mode_id = %d AND to_mode_id = %d) OR \
                        (from_mode_id = %d AND to_mode_id = %d))", 
                        WALKING_MODE_ID, publicModeId, publicModeId, WALKING_MODE_ID);
                strcat(publicSwitchClause, strPublcSwitchSeg);
                // Retrieve switch point infos between (WALKING_MODE_ID, j)
            }
            for (j = 0; j < plan->public_transit_mode_count - 1; j++)
                for (k = j + 1; k < plan->public_transit_mode_count; k++) {
                    int fromPublicModeId = plan->public_transit_mode_id_set[j];
                    int toPublicModeId = plan->public_transit_mode_id_set[k];
                    // Retrieve switch point infos between (j,k)
                    char strPublcSwitchSeg[128];
                    sprintf(strPublcSwitchSeg, 
                            " OR ((from_mode_id = %d AND to_mode_id = %d) OR \
                            (from_mode_id = %d AND to_mode_id = %d))",
                            fromPublicModeId, toPublicModeId, 
                            toPublicModeId, fromPublicModeId);  
                    strcat(publicSwitchClause, strPublcSwitchSeg);
                }
            sprintf(switchFilterCondition, 
                    "SELECT from_vertex_id, to_vertex_id, cost FROM \
                    switch_points WHERE %s", publicSwitchClause);
#ifdef DEBUG
            printf("[DEBUG] SQL statement for filtering switch points: \n");
            printf("%s\n", switchFilterCondition);
#endif
            PGresult *switchpointResults;
            switchpointResults = PQexec(conn, switchFilterCondition);
            if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK) {
                fprintf(stderr, "query in switch_points table failed: %s", 
                        PQerrorMessage(conn));
                PQclear(switchpointResults);
                exit_nicely(conn);
            }
            SwitchPoint **publicSwitchpoints;
            int publicSwitchpointCount;
            publicSwitchpointCount = PQntuples(switchpointResults);
#ifdef DEBUG
            printf("[DEBUG] Found switch points: %d\n", publicSwitchpointCount);
            printf("[DEBUG] Reading and parsing switch points...");
#endif
            ReadSwitchPoints(switchpointResults, &publicSwitchpoints);
#ifdef DEBUG
            printf(" done.\n");
#endif
            PQclear(switchpointResults);
            // combine the graphs by adding switch lines in the 
            // (vertices, edges) set and get the mode 
            // PUBLIC_TRANSPORT_MODE_ID graph
#ifdef DEBUG
            printf("[DEBUG] Combining multimodal graphs for public transit...\n");
#endif
            CombineGraphs(vertices, vertexCount, publicSwitchpoints, 
                    publicSwitchpointCount);
#ifdef DEBUG
            printf(" done.\n");
#endif
        }

        tmpGraph->vertices = vertices;
        tmpGraph->vertex_count = vertexCount;
        if (ValidateGraph(tmpGraph) == EXIT_FAILURE)
            return EXIT_FAILURE;
        graphs[i] = tmpGraph;
        if (i > 0) {
            int j = 0;
            if ((plan->mode_id_list[i-1] != PUBLIC_TRANSPORT_MODE_ID) && 
                    (plan->mode_id_list[i] != PUBLIC_TRANSPORT_MODE_ID))
                sprintf(switchFilterCondition, 
                        "SELECT from_vertex_id, to_vertex_id, cost FROM \
                        switch_points WHERE from_mode_id=%d AND \
                        to_mode_id=%d AND %s", 
                        plan->mode_id_list[i-1], plan->mode_id_list[i], 
                        plan->switch_condition_list[i-1]);
            else if (plan->mode_id_list[i-1] == PUBLIC_TRANSPORT_MODE_ID) {
                char strFromModeClause[256];
                sprintf(strFromModeClause, "from_mode_id = %d ", WALKING_MODE_ID);
                for (j = 0; j < plan->public_transit_mode_count; j++) {
                    int publicModeId = plan->public_transit_mode_id_set[j];
                    char strPublicSegClause[128];
                    sprintf(strPublicSegClause, "OR from_mode_id = %d ", 
                            publicModeId);
                    strcat(strFromModeClause, strPublicSegClause);
                }
                sprintf(switchFilterCondition, 
                        "SELECT from_vertex_id, to_vertex_id, cost FROM \
                        switch_points WHERE (%s) AND to_mode_id=%d AND %s", 
                        strFromModeClause, plan->mode_id_list[i], 
                        plan->switch_condition_list[i-1]);
            }
            else if (plan->mode_id_list[i] == PUBLIC_TRANSPORT_MODE_ID) {
                char strToModeClause[256];
                sprintf(strToModeClause, "to_mode_id = %d ", WALKING_MODE_ID);
                for (j = 0; j < plan->public_transit_mode_count; j++) {
                    int publicModeId = plan->public_transit_mode_id_set[j];
                    char strPublicSegClause[128];
                    sprintf(strPublicSegClause, "OR to_mode_id = %d ", publicModeId);
                    strcat(strToModeClause, strPublicSegClause);
                }
                sprintf(switchFilterCondition, 
                        "SELECT from_vertex_id, to_vertex_id, cost FROM \
                        switch_points WHERE from_mode_id=%d AND (%s) AND %s", 
                        plan->mode_id_list[i-1], strToModeClause, 
                        plan->switch_condition_list[i-1]);
            }
            PGresult *switchpointResults;
            switchpointResults = PQexec(conn, switchFilterCondition);
            if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK) {
                fprintf(stderr, "query in switch_points table failed: %s", 
                        PQerrorMessage(conn));
                PQclear(switchpointResults);
                exit_nicely(conn);
            }
            switchpointCounts[i-1] = PQntuples(switchpointResults);
            ReadSwitchPoints(switchpointResults, &switchpoints);
            switchpointsArr[i-1] = switchpoints;
            PQclear(switchpointResults);
        }
    }

    return EXIT_SUCCESS;
}

static int InitializeGraphs(int graphCount) {
    graphs = (ModeGraph **) calloc(graphCount, sizeof(ModeGraph*));
    if (graphCount > 1) {
        switchpointsArr = (SwitchPoint ***) calloc(graphCount - 1, 
                sizeof(SwitchPoint**));
        switchpointCounts = (int *) calloc(graphCount - 1, sizeof(int));
    }
    return EXIT_SUCCESS;
}

static void read_graph(PGresult* res, Vertex*** vertexArrayAddr, 
        int vertexCount, const char* costFactor) {
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
        else {
            while (outgoingEdge->adjNext != NULL)
                outgoingEdge = outgoingEdge->adjNext;
            outgoingEdge->adjNext = tmpEdge;
        }
        tmpEdge->adjNext = NULL;
    }
}

void ReadSwitchPoints(PGresult* res, SwitchPoint*** switchpointArrayAddr) {
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
        SwitchPoint* tmpSwitchPoint;
        tmpSwitchPoint = (SwitchPoint*) malloc(sizeof(SwitchPoint));
        /* from vertex id */
        tmpSwitchPoint->from_vertex_id = atoll(PQgetvalue(res, i, 0));
        /* to vertex id */
        tmpSwitchPoint->to_vertex_id = atoll(PQgetvalue(res, i, 1));
        tmpSwitchPoint->speed_factor = 0.015;
        tmpSwitchPoint->length_factor = 1.0;
        tmpSwitchPoint->length = atof(PQgetvalue(res, i, 2)) * 
            tmpSwitchPoint->speed_factor;
        (*switchpointArrayAddr)[i] = tmpSwitchPoint;
    }
}

static void CombineGraphs(Vertex** vertexArray, int vertexCount, 
        SwitchPoint** switchpointArray, int switchpointCount) {
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
        tmpEdge->mode_id = WALKING_MODE_ID;
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
Vertex* SearchVertexById(Vertex** vertexArray, int len, long long id) {
    int i = 0;
    for (i = 0; i < len; i++)
    {
        if (vertexArray[i]->id == id)
            return vertexArray[i];
    }
    return NNULL;
}

// Binary search when the vertex array is sorted
Vertex* BinarySearchVertexById(Vertex** vertexArray, int low, int high, 
        long long id) {
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
int ValidateGraph(Graph* g) {
    for (int i = 0; i < g->vertex_count; i++) {
        if (g->vertices[i] == NNULL) {
            // found a NULL vertex
            printf("FATAL: NULL vertex found in graph, seq number is %d, \
                    previous vertex id is %lld\n", i, g->vertices[i-1]->id);
            return EXIT_FAILURE;
        }
        /*printf("%d / %d: checking vertex %lld\n", i, g->vertex_count, */
        /*g->vertices[i]->id);*/
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
            /*printf("Outgoing edges: -\n");*/
            while (pEdge != NULL) {
                /*printf("\b |--> (%lld, %lld)\n", pEdge->from_vertex_id, pEdge->to_vertex_id);*/
                if (pEdge->from_vertex_id != g->vertices[i]->id) {
                    // found foreign edges not emitted from the current vertex
                    printf("FATAL: bad vertex structure found! \
                            Found an outgoing edge NOT belonging to this vertex. \
                            Problematic vertex id is %lld, edge's from_vertex_id is %lld\n", g->vertices[i]->id, pEdge->from_vertex_id);
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

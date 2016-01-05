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
ModeGraph *activeGraphs = NULL;
SwitchPoint **switchpointsArr = NULL;
int *switchpointCounts = NULL;
int graphCount = 0;

/* Private function declarations */
static int connectPostgre(const char *pgConnString); 
static void disconnectPostgre();
static void exitPostgreNicely(PGconn *conn);
static void loadAllGraphs();
static int initGraphs(int graphCount);
static ModeGraph deepCopyModeGraphFromCache(int modeId);
static ModeGraph shallowCopyModeGraphFromCache(int modeId);
static ModeGraph cloneModeGraph(ModeGraph g);
static void readGraph(PGresult *res, Vertex **vertexArrayAddr, int vertexCount);
static void readSwitchPoints(PGresult *res, SwitchPoint **switchpointArrayAddr);
static void embedSwitchPoints(Vertex *vertices, int vertexCount, 
        SwitchPoint *switchpoints, int switchpointCount);
static int validateGraph(ModeGraph g);
static int getVertexCount(const char *vertexCountSQL);
static void retrieveGraphData(const char *graphSQL, Vertex **vertices, 
        int vertexCount);
static char *constructPublicModeClause(RoutingPlan *p);
static void constructGraphSQL(RoutingPlan *p, int modeId, char *vertexCountSQL, 
        char *graphSQL);
static void constructPublicSwitchSQL(RoutingPlan *p, char *switchSQL);
static void retrieveSwitchPoints(const char *switchSQL, int *spCount, 
        SwitchPoint **publicSPs);
static void constructPublicModeGraph(RoutingPlan *p, Vertex *vertices, 
        int vertexCount);
static ModeGraph buildPublicModeGraphFromCache(RoutingPlan *p);
static void disposePublicModeGraph();
static void constructSwitchPointSQL(RoutingPlan *p, char *switchSQL, int i);
static void disposeActiveGraphs();
static void disposeGraphCache();
static void disposeSwitchPoints();
static void quickSortVertices(Vertex *v, int n);

/* External function declarations */
extern void DisposeRoutingPlan();
extern void DisposeResultPathTable();

int MSPinit(const char *pgConnStr) {
    /* Step 1: connect to database 
     * Step 2: read the series of graph data via SQL 
     * Step 3: read the switch points via SQL ?? */
    /* return 0 if everything succeeds, otherwise an error code */
    assert(!connectPostgre(pgConnStr));
    loadAllGraphs();
    return 0;
}

int MSPassembleGraphs() {
    extern RoutingPlan *plan;	
    if (initGraphs(plan->mode_count) == EXIT_FAILURE) {
        printf("initialization of graphs failed\n");
        return EXIT_FAILURE;
    }
    int i = 0;
    graphCount = plan->mode_count;
    for (i = 0; i < plan->mode_count; i++) {
        int modeId = plan->modes[i];
        char switchpointSQL[1024] = "";
        ModeGraph g = GNULL;
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::MSPassembleGraphs]create active graphs from cache for mode %d\n", modeId);
#endif
        if (modeId != PUBLIC_TRANSPORTATION)
            g = shallowCopyModeGraphFromCache(modeId);
        else 
            g = buildPublicModeGraphFromCache(plan);
        if (validateGraph(g) == EXIT_FAILURE)
            return EXIT_FAILURE;
        activeGraphs[i] = g;
        if (i > 0) {
            constructSwitchPointSQL(plan, switchpointSQL, i);
            retrieveSwitchPoints(switchpointSQL, &switchpointCounts[i-1], 
                    &switchpointsArr[i-1]);
        }
    }
    return EXIT_SUCCESS;
}

void MSPclearGraphs() {
    disposePublicModeGraph();
    free(activeGraphs);
    disposeSwitchPoints();
    DisposeResultPathTable();
}

void MSPfinalize() {
    disposeGraphCache();
    disconnectPostgre();
}

/*
 * v1.x API implementations
 */

int ConnectDB(const char *pgConnStr) {
    return connectPostgre(pgConnStr);
}

void DisconnectDB() {
    disconnectPostgre();
}

int Parse() {
    extern RoutingPlan *plan;	
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::Parse] init multimodal graphs\n");
#endif
    if (initGraphs(plan->mode_count) == EXIT_FAILURE) {
        printf("initialization of graphs failed\n");
        return EXIT_FAILURE;
    }
    int i = 0;
    graphCount = plan->mode_count;
    for (i = 0; i < plan->mode_count; i++) {
        int modeId = plan->modes[i];
        int vertexCount = 0;
        char switchpointSQL[1024] = "";
        char vertexCountSQL[512] = "";
        char graphSQL[1024] = "";
        Vertex *vertices = NULL;
        ModeGraph tmpGraph = (ModeGraph) malloc(sizeof(struct ModeGraph));
        assert(tmpGraph);
        tmpGraph->id = modeId;
        constructGraphSQL(plan, modeId, vertexCountSQL, graphSQL);
        vertexCount = getVertexCount(vertexCountSQL);
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::Parse] retrieve mode graph from database \n");
#endif
        retrieveGraphData(graphSQL, &vertices, vertexCount);
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::Parse] construct mode graph for public transit \n");
#endif
        if (modeId == PUBLIC_TRANSPORTATION) 
            constructPublicModeGraph(plan, vertices, vertexCount);
        tmpGraph->vertices = vertices;
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::Parse] assign vertices to tmpGraph \n");
        printf("[DEBUG][graphassembler.c::Parse] First vertex in tmpGraph: %lld\n", tmpGraph->vertices[0]->id);
#endif
        tmpGraph->vertex_count = vertexCount;
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::Parse] Validate the constructed graph...\n");
#endif
        if (validateGraph(tmpGraph) == EXIT_FAILURE)
            return EXIT_FAILURE;
        activeGraphs[i] = tmpGraph;
        if (i > 0) {
            constructSwitchPointSQL(plan, switchpointSQL, i);
            retrieveSwitchPoints(switchpointSQL, &switchpointCounts[i-1], 
                    &switchpointsArr[i-1]);
        }
    }
    return EXIT_SUCCESS;
}

void Dispose() {
    disposeActiveGraphs();
    disposeSwitchPoints();
    // FIXME: wierd... why not disposing result path table in DisposePaths()?
    DisposeResultPathTable();
    DisposeRoutingPlan();
}

/* 
 * Private function definitions 
 */

static PGconn* conn = NULL;

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

static void exitPostgreNicely(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

static ModeGraph graphCache[TOTAL_MODES];

static void loadAllGraphs() {
    int i = 0, j = 0;
    for (i = PRIVATE_CAR; i < PRIVATE_CAR + TOTAL_MODES; i++) {
        int vertexCount = 0;
        char vertexCountSQL[512] = "";
        char graphSQL[1024] = "";
        Vertex *vertices = NULL;
        ModeGraph g = (ModeGraph) malloc(sizeof(struct ModeGraph));
        assert(g);
        g->id = i;
        constructGraphSQL(NULL, i, vertexCountSQL, graphSQL);
        vertexCount = getVertexCount(vertexCountSQL);
        retrieveGraphData(graphSQL, &vertices, vertexCount);
        g->vertices = vertices;
        g->vertex_count = vertexCount;
        graphCache[j++] = g;
    }
}

static ModeGraph deepCopyModeGraphFromCache(int modeId) {
    for (int i = 0; i < TOTAL_MODES; i++)
        if (graphCache[i]->id == modeId)
            return cloneModeGraph(graphCache[i]);
    return GNULL;
}

static ModeGraph shallowCopyModeGraphFromCache(int modeId) {
#ifdef DEBUG
    printf("[DEBUG][mmspa4pg.c::shallowCopyModeGraphFromCache]copy graph by reference\n");
#endif
    for (int i = 0; i < TOTAL_MODES; i++)
        if (graphCache[i]->id == modeId)
            return graphCache[i];
    return GNULL;
}

static ModeGraph cloneModeGraph(ModeGraph g) {
    ModeGraph newG = (ModeGraph) malloc(sizeof(struct ModeGraph));
    assert(newG);
    newG->id = g->id;
    newG->vertex_count = g->vertex_count;
    Vertex *vertices = (Vertex*) calloc(newG->vertex_count, sizeof(Vertex));
    assert(vertices);
    Vertex tmpV = VNULL;
    for (int i = 0; i < g->vertex_count; i++) {
        tmpV = (Vertex) malloc(sizeof(struct Vertex));
        assert(tmpV);
        tmpV->id = g->vertices[i]->id;
        tmpV->outdegree = g->vertices[i]->outdegree;
        if (tmpV->outdegree == 0)
            tmpV->outgoing = ENULL;
        else {
            Edge head = g->vertices[i]->outgoing;
            Edge last = ENULL, p = head;
            for (int j = 0; j < g->vertices[i]->outdegree; j++) {
                Edge e = (Edge) malloc(sizeof(struct Edge));
                assert(e);
                e->from_id = p->from_id;
                e->to_id = p->to_id;
                e->length = p->length;
                e->speed_factor = p->speed_factor;
                e->length_factor = p->length_factor;
                e->mode_id = p->mode_id;
                e->adj_next = ENULL;
                if (p == head) { tmpV->outgoing = e; }
                else { last->adj_next = e; }
                last = e;
                p = p->adj_next;
            }
        }
        vertices[i] = tmpV;
    }
    newG->vertices = vertices;
    return newG;
}

static int initGraphs(int graphCount) {
    activeGraphs = (ModeGraph *) calloc(graphCount, sizeof(ModeGraph));
    assert(activeGraphs);
    if (graphCount > 1) {
        switchpointsArr = (SwitchPoint **) calloc(graphCount - 1, 
                sizeof(SwitchPoint *));
        assert(switchpointsArr);
        switchpointCounts = (int *) calloc(graphCount - 1, sizeof(int));
        assert(switchpointCounts);
    }
    return EXIT_SUCCESS;
}

static void readGraph(PGresult *res, Vertex **vertexArrayAddr, int vertexCount) {
    /* read the edges and vertices from the query result */
    int recordCount = 0, i = 0, outgoingCursor = 0, vertexCursor = 0;
    recordCount = PQntuples(res);	
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::readGraph] record count is %d\n", recordCount);
#endif
    *vertexArrayAddr = (Vertex *) calloc(vertexCount, sizeof(Vertex));
    assert(*vertexArrayAddr);
    Vertex tmpVertex = VNULL;
    for (i = 0; i < recordCount; i++) {
        /* fields in the query results:
         * vertices.vertex_id, edges.to_id, edges.length, edges.speed_factor, 
         * 0,                  1,           2,            3,                  
         * vertices.out_degree, edges.mode_id
         * 4,                   5
         */
        if (outgoingCursor == 0) {
            // a new group of edge records with the same from_vertex starts...	
            tmpVertex = (Vertex) malloc(sizeof(struct Vertex));
            assert(tmpVertex);
            /* vertex id */		
            tmpVertex->id = atoll(PQgetvalue(res, i, 0));
            /* out degree */		
            tmpVertex->outdegree = atoi(PQgetvalue(res, i, 4));
            tmpVertex->outgoing = ENULL;
            (*vertexArrayAddr)[vertexCursor++] = tmpVertex;
            if (tmpVertex->outdegree == 0)
                continue;
        }
        outgoingCursor++;
        if (outgoingCursor == atoi(PQgetvalue(res, i, 4)))
            outgoingCursor = 0;
        Edge tmpEdge = (Edge) malloc(sizeof(struct Edge));
        assert(tmpEdge);
        /* from vertex id */
        tmpEdge->from_id = atoll(PQgetvalue(res, i, 0));
        /* to vertex id */
        tmpEdge->to_id = atoll(PQgetvalue(res, i, 1));
        /* edge length */
        tmpEdge->length = atof(PQgetvalue(res, i, 2));
        /* speed factor */
        tmpEdge->speed_factor = atof(PQgetvalue(res, i, 3));
        /* length factor, === 1.0 */
        tmpEdge->length_factor = 1.0;
        /* mode id of edge */
        tmpEdge->mode_id = atoi(PQgetvalue(res, i, 5));
        /* attach end to the adjacency list of start */
        Edge outgoingEdge = tmpVertex->outgoing;
        if (tmpVertex->outgoing == ENULL)
            tmpVertex->outgoing = tmpEdge;
        else {
            while (outgoingEdge->adj_next != ENULL)
                outgoingEdge = outgoingEdge->adj_next;
            outgoingEdge->adj_next = tmpEdge;
        }
        tmpEdge->adj_next = ENULL;
#ifdef DEBUG
        /*printf("[DEBUG] ::readGraph, processed %d record, vertex id: %lld\n", */
        /*i+1, tmpVertex->id);*/
#endif
    }
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::readGraph] End of ::readGraph\n");
#endif
}

static void readSwitchPoints(PGresult *res, SwitchPoint **switchpointArrayAddr) {
    /* read the switch_points information from the query result */
    int switchpointCount, i = 0;
    switchpointCount = PQntuples(res);
    *switchpointArrayAddr = (SwitchPoint *) calloc(switchpointCount, 
            sizeof(SwitchPoint));
    assert(*switchpointArrayAddr);
    for (i = 0; i < switchpointCount; i++) {
        /* fields in query results:
         * from_vertex_id, to_vertex_id, cost
         * 0,              1,            2
         */
        SwitchPoint tmpSwitchPoint;
        tmpSwitchPoint = (SwitchPoint) malloc(sizeof(struct SwitchPoint));
        assert(tmpSwitchPoint);
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

static void embedSwitchPoints(Vertex *vertices, int vertexCount, 
        SwitchPoint *switchpoints, int switchpointCount) {
    // Treat all the switch point pairs as new edges and add them into the graph
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::embedSwitchPoints] Start embedding %d switch points into multimodal graph with %d vertices... \n", switchpointCount, vertexCount);
#endif
#ifdef DEEP_DEBUG
    /*printf("[DEEP_DEBUG][graphassembler.c::embedSwitchPoints] Input vertices: \n");*/
    /*for (int k = 0; k < vertexCount; k++) */
        /*printf("[DEEP_DEBUG][graphassembler.c::embedSwitchPoints] vertex id: %lld\n", vertices[k]->id);*/
#endif
    for (int i = 0; i < switchpointCount; i++) {
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] Processing switch point %d\n", i + 1);
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] from vertex id: %lld\n", switchpoints[i]->from_vertex_id);
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] to vertex id: %lld\n", switchpoints[i]->to_vertex_id);
#endif
        Edge tmpEdge;
        tmpEdge = (Edge) malloc(sizeof(struct Edge));
        assert(tmpEdge);
        /* from vertex */
        tmpEdge->from_id = switchpoints[i]->from_vertex_id;
        /* to vertex */
        tmpEdge->to_id = switchpoints[i]->to_vertex_id;
        tmpEdge->mode_id = FOOT;
        tmpEdge->adj_next = ENULL;
        /* edge length */
        tmpEdge->length = switchpoints[i]->length;
        /* speed factor */
        tmpEdge->speed_factor = switchpoints[i]->speed_factor;
        /* length factor, === 1.0 */
        tmpEdge->length_factor = 1.0;
        /* attach end to the adjacency list of start */
        Vertex vertexFrom = BinarySearchVertexById(vertices, 0, 
                vertexCount - 1, switchpoints[i]->from_vertex_id);
        assert(vertexFrom);
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] found vertex: %lld\n", vertexFrom->id);
#endif
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] get outgoing of vertex %lld\n", vertexFrom->id);
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] outdegree of vertex %lld is %d\n", vertexFrom->id, vertexFrom->outdegree);
#endif
        Edge e = vertexFrom->outgoing;
        if (e == ENULL) {
#ifdef DEBUG
            printf("[DEBUG][graphassembler.c::embedSwitchPoints] outgoing of vertex %lld is NULL\n", vertexFrom->id);
#endif
            vertexFrom->outgoing = tmpEdge;
        } else {
#ifdef DEBUG
            printf("[DEBUG][graphassembler.c::embedSwitchPoints] outgoing of vertex %lld is not NULL\n", vertexFrom->id);
            printf("[DEBUG][graphassembler.c::embedSwitchPoints] head of edge linked list, a.k.a outgoing edge is (%lld, %lld) \n", e->from_id, e->to_id);
#endif
            while (e->adj_next != ENULL) {
#ifdef DEBUG
                printf("[DEBUG][graphassembler.c::embedSwitchPoints] get adj next of edge (%lld, %lld) \n", e->from_id, e->to_id);
#endif
                e = e->adj_next;
            }
            /*assert(e);*/
#ifdef DEBUG
            printf("[DEBUG][graphassembler.c::embedSwitchPoints] tail edge: (%lld, %lld)\n", e->from_id, e->to_id);
#endif
            e->adj_next = tmpEdge;
#ifdef DEBUG
            printf("[DEBUG][graphassembler.c::embedSwitchPoints] new edge: (%lld, %lld) has been appended to the linked list\n", tmpEdge->from_id, tmpEdge->to_id);
#endif
        }
        vertexFrom->outdegree++;
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] edge (%lld, %lld) born from switchpoint has been added\n", tmpEdge->from_id, tmpEdge->to_id);
#endif
    }
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::embedSwitchPoints] Finish combining.\n");
#endif
}

// Linear search when the vertex array is not sorted 
Vertex SearchVertexById(Vertex* vertices, int len, int64_t id) {
    int i = 0;
    for (i = 0; i < len; i++) {
        if (vertices[i]->id == id)
            return vertices[i];
    }
    return VNULL;
}

static void quickSortVertices(Vertex *v, int n) {
    int left, right;
    int64_t p;
    Vertex t;
    if (n < 2)
        return;
    p = v[n / 2]->id;
    for (left = 0, right = n - 1;; left++, right--) {
        while (v[left]->id < p)
            left++;
        while (p < v[right]->id)
            right--;
        if (left >= right)
            break;
        t = v[left];
        v[left] = v[right];
        v[right] = t;
    }
    quickSortVertices(v, left);
    quickSortVertices(&v[left], n - left);
}

// Binary search when the vertex array is sorted
Vertex BinarySearchVertexById(Vertex *vertices, int low, int high, int64_t id) {
    assert(vertices);
    if (high < low)
        return VNULL; // not found
    int mid = (low + high) / 2;
    if (vertices[mid]->id > id)
        return BinarySearchVertexById(vertices, low, mid - 1, id);
    else if (vertices[mid]->id < id)
        return BinarySearchVertexById(vertices, mid + 1, high, id);
    else
        return vertices[mid];
}

// Check if the constructed graph has dirty data
static int validateGraph(ModeGraph g) {
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::validateGraph] Start validating graph g with mode_id %d\n", g->id);
#endif
    if (g == GNULL) {
        printf("FATAL: NULL graph input in validateGraph");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < g->vertex_count; i++) {
#ifdef DEBUG
        /*printf("[DEBUG][graphassembler.c::validateGraph] Checking vertex %d\n", i);*/
#endif
        if (g->vertices[i] == VNULL) {
            // found a NULL vertex
            printf("FATAL: NULL vertex found in graph, seq number is %d, \
                    previous vertex id is %lld\n", i, g->vertices[i-1]->id);
            return EXIT_FAILURE;
        }
        int claimedOutdegree = g->vertices[i]->outdegree;
        int realOutdegree = 0;
        if ((claimedOutdegree == 0) && (g->vertices[i]->outgoing != ENULL)) {
            // outgoing edges are not NULL while outdegree is 0
            printf("FATAL: bad vertex structure found! \
                    Outdegree is 0 while outgoing is not NULL. \
                    Problematic vertex id is %lld\n", g->vertices[i]->id);
            return EXIT_FAILURE;
        }
        if (claimedOutdegree >= 1) {
            if (g->vertices[i]->outgoing == ENULL) {
                // outgoing edge is NULL while outdegree is larger than 0
                printf("FATAL: bad vertex structure found! \
                        Outdegree > 1 while outgoing is NULL. \
                        Problematic vertex id is %lld\n", g->vertices[i]->id);
                return EXIT_FAILURE;
            }
            Edge pe = g->vertices[i]->outgoing;
            while (pe != ENULL) {
                if (pe->from_id != g->vertices[i]->id) {
                    // found foreign edges not emitted from the current vertex
                    printf("FATAL: bad vertex structure found! \
                            Found an outgoing edge NOT belonging to this vertex. \
                            Problematic vertex id is %lld, edge's from_id \
                            is %lld\n", g->vertices[i]->id, pe->from_id);
                    return EXIT_FAILURE;
                }
                realOutdegree++;
                pe = pe->adj_next;
            }
            if (realOutdegree != claimedOutdegree) {
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
static int getVertexCount(const char *vertexCountSQL) {
    PGresult *vertexResults;
    vertexResults = PQexec(conn, vertexCountSQL);
    if (PQresultStatus(vertexResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in vertices table failed: %s", 
                PQerrorMessage(conn));
        PQclear(vertexResults);
        exitPostgreNicely(conn);
    }
    int vc = atoi(PQgetvalue(vertexResults, 0, 0));
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::getVertexCount] SQL of query vertices: %s\n", vertexCountSQL);
    printf("[DEBUG][graphassembler.c::getVertexCount] %d vertices are found \n", vc);
#endif
    PQclear(vertexResults);
    return vc;
}

// Retrieve and read graph
static void retrieveGraphData(const char *graphSQL, Vertex **vertices, 
        int vertexCount) {
    PGresult *graphResults;
    graphResults = PQexec(conn, graphSQL);
    if (PQresultStatus(graphResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in edges and vertices table failed: %s", 
                PQerrorMessage(conn));
        PQclear(graphResults);
        exitPostgreNicely(conn);
    }
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveGraphData] SQL of query graphs: %s\n", graphSQL);
    printf("[DEBUG][graphassembler.c::retrieveGraphData] Reading graphs... ");
#endif
    readGraph(graphResults, vertices, vertexCount);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveGraphData] done.\n");
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
            FOOT, p->public_transit_modes[0]);
    for (j = 1; j < p->public_transit_mode_count; j++) {
        int publicModeId = p->public_transit_modes[j];
        char strPublicModeIdSeg[32];
        sprintf(strPublicModeIdSeg, " OR vertices.mode_id = %d", 
                publicModeId);
        strcat(publicModeClause, strPublicModeIdSeg);
    }
    return publicModeClause;
}

static void constructGraphSQL(RoutingPlan *p, int modeId, char *vertexCountSQL, 
        char *graphSQL) {
    if (modeId == PUBLIC_TRANSPORTATION) {
        char *pmodeClause = constructPublicModeClause(p);
        sprintf(graphSQL, 
                "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                WHERE (%s) UNION SELECT vertices.vertex_id, NULL, NULL, NULL, \
                out_degree, mode_id FROM vertices WHERE out_degree=0 AND (%s)) \
            ORDER BY vertex_id",
                pmodeClause, pmodeClause);
        sprintf(vertexCountSQL, 
                "SELECT COUNT(*) FROM vertices WHERE (%s)", pmodeClause);
    }
    else {
        sprintf(graphSQL, 
                "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                WHERE edges.mode_id=%d UNION SELECT vertices.vertex_id, NULL, \
                NULL, NULL, out_degree, mode_id FROM vertices WHERE \
                out_degree=0 AND mode_id=%d) ORDER BY vertex_id", 
                modeId, modeId);
        sprintf(vertexCountSQL, 
                "SELECT COUNT(*) FROM vertices WHERE mode_id=%d", modeId);
    }
}

static void constructPublicSwitchSQL(RoutingPlan *p, char *switchSQL) {
    int j = 0, k = 0;
    char psClause[1024];
    sprintf(psClause, " ((from_mode_id = %d AND to_mode_id = %d) OR \
        (from_mode_id = %d AND to_mode_id = %d))", 
            FOOT, p->public_transit_modes[0], 
            p->public_transit_modes[0], FOOT);
    for (j = 1; j < p->public_transit_mode_count; j++) {
        int publicModeId = p->public_transit_modes[j];	
        char psSegment[128];
        sprintf(psSegment, " OR ((from_mode_id = %d AND to_mode_id = %d) OR \
            (from_mode_id = %d AND to_mode_id = %d))", 
                FOOT, publicModeId, publicModeId, FOOT);
        strcat(psClause, psSegment);
        // Retrieve switch point infos between (FOOT, j)
    }
    for (j = 0; j < p->public_transit_mode_count - 1; j++)
        for (k = j + 1; k < p->public_transit_mode_count; k++) {
            int fromPublicModeId = p->public_transit_modes[j];
            int toPublicModeId = p->public_transit_modes[k];
            // Retrieve switch point infos between (j,k)
            char psSegment[128];
            sprintf(psSegment, " OR ((from_mode_id = %d AND to_mode_id = %d) OR \
                (from_mode_id = %d AND to_mode_id = %d))",
                    fromPublicModeId, toPublicModeId, 
                    toPublicModeId, fromPublicModeId);  
            strcat(psClause, psSegment);
        }
    sprintf(switchSQL, "SELECT from_vertex_id, to_vertex_id, cost FROM \
            switch_points WHERE %s", psClause);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructPublicSwitchSQL] SQL statement for filtering switch points: \n");
    printf("%s\n", switchSQL);
#endif
}

static void retrieveSwitchPoints(const char *switchSQL, int *spCount, 
        SwitchPoint **publicSPs) {
    PGresult *switchpointResults;
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] SQL for fetching switch points: %s\n", switchSQL);
#endif
    switchpointResults = PQexec(conn, switchSQL);
    if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in switch_points table failed: %s", 
                PQerrorMessage(conn));
        PQclear(switchpointResults);
        exitPostgreNicely(conn);
    }
    *spCount = PQntuples(switchpointResults);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] Found switch points: %d\n", *spCount);
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] Reading and parsing switch points...\n");
#endif
    readSwitchPoints(switchpointResults, publicSPs);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] done.\n");
#endif
    PQclear(switchpointResults);
}

static ModeGraph pGraph = GNULL;

static ModeGraph buildPublicModeGraphFromCache(RoutingPlan *p) {
    /*a new graph is necessary for public transit network*/
#ifdef DEBUG
    printf("[graphassembler.c::buildPublicModeGraphFromCache] Start building public mode graph by deep copying from graph cache.\n");
#endif
    pGraph = (ModeGraph) malloc(sizeof(struct ModeGraph));
    assert(pGraph);
    int vCount = 0, i = 0;
    ModeGraph fg = deepCopyModeGraphFromCache(FOOT);
    vCount += fg->vertex_count;
    ModeGraph pg[p->public_transit_mode_count];
    for (i = 0; i < p->public_transit_mode_count; i++) {
        pg[i] = deepCopyModeGraphFromCache(p->public_transit_modes[i]);
        vCount += pg[i]->vertex_count;
    }
    Vertex *vertices = (Vertex *) calloc(vCount, sizeof(Vertex));
    assert(vertices);
    int vCur = 0, j = 0;
    for (j = 0; j < fg->vertex_count; j++)
        vertices[vCur++] = fg->vertices[j];
    for (i = 0; i < p->public_transit_mode_count; i++)
        for (j = 0; j < pg[i]->vertex_count; j++)
            vertices[vCur++] = pg[i]->vertices[j];
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::buildPublicModeGraphFromCache] Do QuickSort on vertices array...\n");
#endif
    quickSortVertices(vertices, vCount);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::buildPublicModeGraphFromCache] QuickSort done!\n");
#endif
#ifdef DEEP_DEBUG
    printf("[DEEP_DEBUG][graphassembler.c::buildPublicModeGraphFromCache] vertices count: %d\n", vCount);
    printf("[DEEP_DEBUG][graphassembler.c::buildPublicModeGraphFromCache] final vertex cursor: %d\n", vCur);
    for (j = 0; j < vCount; j++)
        printf("[DEEP_DEBUG][graphassembler.c::buildPublicModeGraphFromCache] vertex id: %lld\n", vertices[j]->id);
#endif
    constructPublicModeGraph(p, vertices, vCount);
    pGraph->id = PUBLIC_TRANSPORTATION;
    pGraph->vertices = vertices;
    pGraph->vertex_count = vCount;
#ifdef DEBUG
    printf("[graphassembler.c::buildPublicModeGraphFromCache] Finish building public mode graph where there are %d vertices.\n", pGraph->vertex_count);
    printf("[graphassembler.c::buildPublicModeGraphFromCache] Its first vertex id: %lld.\n", pGraph->vertices[0]->id);
#endif
    return pGraph;
}

static void disposePublicModeGraph() {
    if (pGraph == GNULL)
        return;
    for (int i = 0; i < pGraph->vertex_count; i++) {
        Edge p, q;
        for (p = pGraph->vertices[i]->outgoing; p != ENULL; p = q) {
            q = p->adj_next;
            free(p);
            p = ENULL;
        }
        free(pGraph->vertices[i]);
        pGraph->vertices[i] = VNULL;
    }
    free(pGraph->vertices);
    pGraph->vertices = NULL;
    free(pGraph);
    pGraph = GNULL;
} 

// Combine all the selected public transit modes and foot networks
// if current mode_id is PUBLIC_TRANSPORTATION
static void constructPublicModeGraph(RoutingPlan *p, Vertex *vertices, 
        int vertexCount) {
    // read switch points between all the PT mode pairs 
    SwitchPoint *publicSPs = NULL;
    int publicSPcount = 0;
    char switchSQL[1024] = "";
    constructPublicSwitchSQL(p, switchSQL);
    retrieveSwitchPoints(switchSQL, &publicSPcount, &publicSPs);
    // combine the graphs by adding switch lines in the (vertices, edges) set 
    // and get the mode PUBLIC_TRANSPORTATION graph
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructPublicModeGraph] Combining multimodal graphs for public transit...\n");
#endif
    embedSwitchPoints(vertices, vertexCount, publicSPs, publicSPcount);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructPublicModeGraph] done.\n");
#endif
}

static void constructSwitchPointSQL(RoutingPlan *p, char *switchSQL, int i) {
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructSwitchPointSQL] parameter switchFilter Cond passed in: %s\n", switchSQL);
#endif
    int j = 0;
    if ((p->modes[i-1] != PUBLIC_TRANSPORTATION) && 
            (p->modes[i] != PUBLIC_TRANSPORTATION))
        sprintf(switchSQL, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE from_mode_id=%d AND \
                to_mode_id=%d AND %s", 
                p->modes[i-1], p->modes[i], 
                p->switch_conditions[i-1]);
    else if (p->modes[i-1] == PUBLIC_TRANSPORTATION) {
        char fromModeClause[256];
        sprintf(fromModeClause, "from_mode_id = %d ", FOOT);
        for (j = 0; j < p->public_transit_mode_count; j++) {
            int publicModeId = p->public_transit_modes[j];
            char publicSegClause[128];
            sprintf(publicSegClause, "OR from_mode_id = %d ", 
                    publicModeId);
            strcat(fromModeClause, publicSegClause);
        }
        sprintf(switchSQL, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE (%s) AND to_mode_id=%d AND %s", 
                fromModeClause, p->modes[i], 
                p->switch_conditions[i-1]);
    }
    else if (p->modes[i] == PUBLIC_TRANSPORTATION) {
        char toModeClause[256];
        sprintf(toModeClause, "to_mode_id = %d ", FOOT);
        for (j = 0; j < p->public_transit_mode_count; j++) {
            int publicModeId = p->public_transit_modes[j];
            char publicSegClause[128];
            sprintf(publicSegClause, "OR to_mode_id = %d ", publicModeId);
            strcat(toModeClause, publicSegClause);
        }
        sprintf(switchSQL, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE from_mode_id=%d AND (%s) AND %s", 
                p->modes[i-1], toModeClause, 
                p->switch_conditions[i-1]);
    }
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructSwitchPointSQL] parameter switchFilter Cond after constructing: %s\n", switchSQL);
#endif
}

static void disposeActiveGraphs() {
    for (int i = 0; i < graphCount; i++) {
        for (int j = 0; j < activeGraphs[i]->vertex_count; j++) {
            Edge p, q;
            for (p = activeGraphs[i]->vertices[j]->outgoing; p != ENULL; p = q) {
                q = p->adj_next;
                free(p);
                p = ENULL;
            }
            free(activeGraphs[i]->vertices[j]);
            activeGraphs[i]->vertices[j] = VNULL;
        }
        free(activeGraphs[i]->vertices);
        activeGraphs[i]->vertices = NULL;
        free(activeGraphs[i]);
        activeGraphs[i] = GNULL;
    }
    free(activeGraphs);
    activeGraphs = NULL;
}

static void disposeGraphCache() {
    for (int i = 0; i < TOTAL_MODES; i++) {
        for (int j = 0; j < graphCache[i]->vertex_count; j++) {
            Edge p, q;
            for (p = graphCache[i]->vertices[j]->outgoing; p != ENULL; p = q) {
                q = p->adj_next;
                free(p);
                p = ENULL;
            }
            free(graphCache[i]->vertices[j]);
            graphCache[i]->vertices[j] = VNULL;
        }
        free(graphCache[i]->vertices);
        graphCache[i]->vertices = NULL;
        free(graphCache[i]);
        graphCache[i] = GNULL;
    }
}

static void disposeSwitchPoints() {
    if (graphCount > 1) {
        for (int i = 0; i < graphCount - 1; i++) {
            for (int j = 0; j < switchpointCounts[i]; j++) {
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

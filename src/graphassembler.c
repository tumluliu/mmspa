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
SwitchPoint **pSwitchPoints = NULL;
int *switchpointCounts = NULL;
int graphCount = 0;

/* Private function declarations */
static int connectPostgre(const char *connstr); 
static void disconnectPostgre();
static void loadAllGraphs();
static int initGraphs(int gc);
static ModeGraph deepCopyModeGraphFromCache(int m);
static ModeGraph shallowCopyModeGraphFromCache(int m);
static ModeGraph cloneModeGraph(ModeGraph g);
static void readGraph(PGresult *res, Vertex **pv, int vc);
static void readSwitchPoints(PGresult *res, SwitchPoint **psp);
static void embedSwitchPoints(Vertex *v, int vc, SwitchPoint *sp, int spc);
static int validateGraph(ModeGraph g);
static int getVertexCount(const char *sql);
static void retrieveGraphData(const char *sql, Vertex **pv, int vc);
static char *constructPublicModeClause(RoutingPlan *p);
static void constructGraphSQL(RoutingPlan *p, int m, char *vsql, char *gsql);
static void constructPublicSwitchSQL(RoutingPlan *p, char *sql);
static void retrieveSwitchPoints(const char *sql, int *spc, SwitchPoint **psp);
static void constructPublicModeGraph(RoutingPlan *p, Vertex *v, int vc);
static ModeGraph buildPublicModeGraphFromCache(RoutingPlan *p);
static void disposePublicModeGraph();
static void constructSwitchPointSQL(RoutingPlan *p, char *sql, int i);
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
    int ret = connectPostgre(pgConnStr);
    assert(!ret);
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
                    &pSwitchPoints[i-1]);
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
 * Private function definitions 
 */

static PGconn *conn = NULL;

static int connectPostgre(const char *connstr) {
    conn = PQconnectdb(connstr);
    if (PQstatus(conn) != CONNECTION_OK) {
        printf("Connection to database failed: %s", PQerrorMessage(conn));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static void disconnectPostgre() {
    PQfinish(conn);
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

static ModeGraph deepCopyModeGraphFromCache(int m) {
    for (int i = 0; i < TOTAL_MODES; i++)
        if (graphCache[i]->id == m)
            return cloneModeGraph(graphCache[i]);
    return GNULL;
}

static ModeGraph shallowCopyModeGraphFromCache(int m) {
#ifdef DEBUG
    printf("[DEBUG][mmspa4pg.c::shallowCopyModeGraphFromCache]copy graph by reference\n");
#endif
    for (int i = 0; i < TOTAL_MODES; i++)
        if (graphCache[i]->id == m)
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

static int initGraphs(int gc) {
    activeGraphs = (ModeGraph *) calloc(gc, sizeof(ModeGraph));
    assert(activeGraphs);
    if (gc > 1) {
        pSwitchPoints = (SwitchPoint **) calloc(gc - 1, sizeof(SwitchPoint *));
        assert(pSwitchPoints);
        switchpointCounts = (int *) calloc(gc - 1, sizeof(int));
        assert(switchpointCounts);
    }
    return EXIT_SUCCESS;
}

static void readGraph(PGresult *res, Vertex **pv, int vc) {
    /* read the edges and vertices from the query result */
    int recordCount = 0, i = 0, outgoingCursor = 0, vertexCursor = 0;
    recordCount = PQntuples(res);	
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::readGraph] record count is %d\n", recordCount);
#endif
    *pv = (Vertex *) calloc(vc, sizeof(Vertex));
    assert(*pv);
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
            (*pv)[vertexCursor++] = tmpVertex;
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

static void readSwitchPoints(PGresult *res, SwitchPoint **psp) {
    /* read the switch_points information from the query result */
    int switchpointCount, i = 0;
    switchpointCount = PQntuples(res);
    *psp = (SwitchPoint *) calloc(switchpointCount, 
            sizeof(SwitchPoint));
    assert(*psp);
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
        (*psp)[i] = tmpSwitchPoint;
    }
}

static void embedSwitchPoints(Vertex *v, int vc, SwitchPoint *sp, int spc) {
    // Treat all the switch point pairs as new edges and add them into the graph
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::embedSwitchPoints] Start embedding %d switch points into multimodal graph with %d vertices... \n", spc, vc);
#endif
#ifdef DEEP_DEBUG
    /*printf("[DEEP_DEBUG][graphassembler.c::embedSwitchPoints] Input vertices: \n");*/
    /*for (int k = 0; k < vc; k++) */
        /*printf("[DEEP_DEBUG][graphassembler.c::embedSwitchPoints] vertex id: %lld\n", vertices[k]->id);*/
#endif
    for (int i = 0; i < spc; i++) {
#ifdef DEBUG
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] Processing switch point %d\n", i + 1);
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] from vertex id: %lld\n", sp[i]->from_vertex_id);
        printf("[DEBUG][graphassembler.c::embedSwitchPoints] to vertex id: %lld\n", sp[i]->to_vertex_id);
#endif
        Edge tmpEdge;
        tmpEdge = (Edge) malloc(sizeof(struct Edge));
        assert(tmpEdge);
        /* from vertex */
        tmpEdge->from_id = sp[i]->from_vertex_id;
        /* to vertex */
        tmpEdge->to_id = sp[i]->to_vertex_id;
        tmpEdge->mode_id = FOOT;
        tmpEdge->adj_next = ENULL;
        /* edge length */
        tmpEdge->length = sp[i]->length;
        /* speed factor */
        tmpEdge->speed_factor = sp[i]->speed_factor;
        /* length factor, === 1.0 */
        tmpEdge->length_factor = 1.0;
        /* attach end to the adjacency list of start */
        Vertex vertexFrom = BinarySearchVertexById(v, 0, vc - 1, 
                sp[i]->from_vertex_id);
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
Vertex SearchVertexById(Vertex *v, int len, int64_t id) {
    int i = 0;
    for (i = 0; i < len; i++) {
        if (v[i]->id == id)
            return v[i];
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
Vertex BinarySearchVertexById(Vertex *v, int low, int high, int64_t id) {
    assert(v);
    if (high < low)
        return VNULL; // not found
    int mid = (low + high) / 2;
    if (v[mid]->id > id)
        return BinarySearchVertexById(v, low, mid - 1, id);
    else if (v[mid]->id < id)
        return BinarySearchVertexById(v, mid + 1, high, id);
    else
        return v[mid];
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
static int getVertexCount(const char *sql) {
    PGresult *vertexResults;
    vertexResults = PQexec(conn, sql);
    if (PQresultStatus(vertexResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in vertices table failed: %s", PQerrorMessage(conn));
        PQclear(vertexResults);
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }
    int vc = atoi(PQgetvalue(vertexResults, 0, 0));
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::getVertexCount] SQL of query vertices: %s\n", sql);
    printf("[DEBUG][graphassembler.c::getVertexCount] %d vertices are found \n", vc);
#endif
    PQclear(vertexResults);
    return vc;
}

// Retrieve and read graph
static void retrieveGraphData(const char *sql, Vertex **pv, int vc) {
    PGresult *graphResults;
    graphResults = PQexec(conn, sql);
    if (PQresultStatus(graphResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in edges and vertices table failed: %s", 
                PQerrorMessage(conn));
        PQclear(graphResults);
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveGraphData] SQL of query graphs: %s\n", sql);
    printf("[DEBUG][graphassembler.c::retrieveGraphData] Reading graphs... ");
#endif
    readGraph(graphResults, pv, vc);
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

static void constructGraphSQL(RoutingPlan *p, int m, char *vsql, char *gsql) {
    if (m == PUBLIC_TRANSPORTATION) {
        char *pmodeClause = constructPublicModeClause(p);
        sprintf(gsql, 
                "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                WHERE (%s) UNION SELECT vertices.vertex_id, NULL, NULL, NULL, \
                out_degree, mode_id FROM vertices WHERE out_degree=0 AND (%s)) \
            ORDER BY vertex_id",
                pmodeClause, pmodeClause);
        sprintf(vsql, 
                "SELECT COUNT(*) FROM vertices WHERE (%s)", pmodeClause);
    }
    else {
        sprintf(gsql, 
                "(SELECT vertices.vertex_id, edges.to_id, edges.length, \
                edges.speed_factor, vertices.out_degree, edges.mode_id FROM \
                edges INNER JOIN vertices ON edges.from_id=vertices.vertex_id \
                WHERE edges.mode_id=%d UNION SELECT vertices.vertex_id, NULL, \
                NULL, NULL, out_degree, mode_id FROM vertices WHERE \
                out_degree=0 AND mode_id=%d) ORDER BY vertex_id", 
                m, m);
        sprintf(vsql, 
                "SELECT COUNT(*) FROM vertices WHERE mode_id=%d", m);
    }
}

static void constructPublicSwitchSQL(RoutingPlan *p, char *sql) {
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
    sprintf(sql, "SELECT from_vertex_id, to_vertex_id, cost FROM \
            switch_points WHERE %s", psClause);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructPublicSwitchSQL] SQL statement for filtering switch points: \n");
    printf("%s\n", sql);
#endif
}

static void retrieveSwitchPoints(const char *sql, int *spc, SwitchPoint **psp) {
    PGresult *switchpointResults;
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] SQL for fetching switch points: %s\n", sql);
#endif
    switchpointResults = PQexec(conn, sql);
    if (PQresultStatus(switchpointResults) != PGRES_TUPLES_OK) {
        fprintf(stderr, "query in switch_points table failed: %s", 
                PQerrorMessage(conn));
        PQclear(switchpointResults);
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }
    *spc = PQntuples(switchpointResults);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] Found switch points: %d\n", *spc);
    printf("[DEBUG][graphassembler.c::retrieveSwitchPoints] Reading and parsing switch points...\n");
#endif
    readSwitchPoints(switchpointResults, psp);
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
    free(fg->vertices);
    free(fg);
    for (i = 0; i < p->public_transit_mode_count; i++) {
        free(pg[i]->vertices);
        free(pg[i]);
    }
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
static void constructPublicModeGraph(RoutingPlan *p, Vertex *v, int vc) {
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
    embedSwitchPoints(v, vc, publicSPs, publicSPcount);
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructPublicModeGraph] done.\n");
#endif
    for (int i = 0; i < publicSPcount; i++)
        free(publicSPs[i]);
    free(publicSPs);
}

static void constructSwitchPointSQL(RoutingPlan *p, char *sql, int i) {
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructSwitchPointSQL] parameter switchFilter Cond passed in: %s\n", sql);
#endif
    int j = 0;
    if ((p->modes[i-1] != PUBLIC_TRANSPORTATION) && 
            (p->modes[i] != PUBLIC_TRANSPORTATION))
        sprintf(sql, 
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
        sprintf(sql, 
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
        sprintf(sql, 
                "SELECT from_vertex_id, to_vertex_id, cost FROM \
                switch_points WHERE from_mode_id=%d AND (%s) AND %s", 
                p->modes[i-1], toModeClause, 
                p->switch_conditions[i-1]);
    }
#ifdef DEBUG
    printf("[DEBUG][graphassembler.c::constructSwitchPointSQL] parameter switchFilter Cond after constructing: %s\n", sql);
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
                free(pSwitchPoints[i][j]);
                pSwitchPoints[i][j] = NULL;
            }
            free(pSwitchPoints[i]);
            pSwitchPoints[i] = NULL;
        }
        free(switchpointCounts);
        free(pSwitchPoints);
        pSwitchPoints = NULL;
        switchpointCounts = NULL;
    }
}

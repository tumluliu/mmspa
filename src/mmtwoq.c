/*
 * =====================================================================================
 *
 *       Filename:  mmtwoq.c
 *
 *    Description:  Implementation of Multimodal Two-Q path finding algorithm
 *
 *        Created:  2009/05/04 16时18分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#include <stdio.h>
#include "../include/mmtwoq.h"

/* status of vertex regarding to queue */
#define UNREACHED   -1
#define IN_QUEUE     0
#define WAS_IN_QUEUE 1

extern void DisposeResultPathTable();

static void twoQSearch(ModeGraph g, Vertex *begin, Vertex *end, Vertex *entry,	
        PathRecorder **prev, const char *costFactor, 
        VertexValidationChecker checkConstraint);
static void multimodalTwoQInit(ModeGraph g, ModeGraph last_g, SwitchPoint *spList, 
        int spListLength, VertexValidationChecker checkConstraint, int64_t source, 
        Vertex *begin, Vertex *end, Vertex *entry, PathRecorder **prev, 
        const char *costFactor);

void MultimodalTwoQ(int64_t source) {
    extern ModeGraph *activeGraphs;
    extern SwitchPoint **switchpointsArr;
    extern int *switchpointCounts;
    extern PathRecorder **pathRecordTable;
    extern int *pathRecordCountArray;
    extern int inputModeCount;
    extern RoutingPlan *plan;
    int i = 0;
#ifdef DEBUG
    printf("[DEBUG][mmtwoq.c::MultimodalTwoQ] start MultimodalTwoQ in libmmspa4pg. \n");
    printf("[DEBUG][mmtwoq.c::MultimodalTwoQ] preparing result path data structures. \n");
#endif
    Vertex begin = VNULL, end = VNULL, entry = VNULL;
    if (pathRecordTable != NULL)
        DisposeResultPathTable();
    pathRecordTable = (PathRecorder **) calloc(plan->mode_count,
            sizeof(PathRecorder*));
    pathRecordCountArray = (int *) calloc(plan->mode_count, sizeof(int));
    inputModeCount = plan->mode_count;
    for (i = 0; i < plan->mode_count; i++) {
        pathRecordCountArray[i] = activeGraphs[i]->vertex_count;
        PathRecorder *pathRecordArray = (PathRecorder*) calloc(
                pathRecordCountArray[i], sizeof(PathRecorder));
#ifdef DEBUG
        printf("[DEBUG][mmtwoq.c::MultimodalTwoQ] start doing multimodalTwoQInit... \n");
#endif
        if (i == 0)
            multimodalTwoQInit(activeGraphs[i], NULL, NULL, 0, NULL, source, &begin, 
                    &end, &entry, &pathRecordArray, plan->cost_factor);
        else
            multimodalTwoQInit(activeGraphs[i], activeGraphs[i-1], 
                    switchpointsArr[i-1], switchpointCounts[i-1], 
                    plan->switch_constraint_list[i-1], source, &begin, &end, &entry, 
                    &pathRecordArray, plan->cost_factor);
#ifdef DEBUG
        printf("[DEBUG][mmtwoq.c::MultimodalTwoQ] done! \n");
        printf("[DEBUG][mmtwoq.c::MultimodalTwoQ] start doing twoQSearch... \n");
#endif
        twoQSearch(activeGraphs[i], &begin, &end, &entry, &pathRecordArray, 
                plan->cost_factor, plan->target_constraint);
        pathRecordTable[i] = pathRecordArray;
    }
}

static void multimodalTwoQInit(ModeGraph g, ModeGraph last_g, SwitchPoint *spList, 
        int spListLength, VertexValidationChecker checkConstraint,  int64_t source, 
        Vertex *begin, Vertex *end, Vertex *entry, PathRecorder **prev, 
        const char *costFactor) {
    int i = 0;
    int v_number = g->vertex_count;
    Vertex src;

#ifdef DEBUG
    printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] multimodalTwoQInit started. \n");
    printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] traversely init vertices... \n");
    printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] number of vertices: %d \n", v_number);
#endif
    for (i = 0; i < v_number; i++) {
#ifdef DEBUG
        /*printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] init vertex: %lld, %d/%d\n", g->vertices[i]->id, i, v_number);*/
#endif
        g->vertices[i]->temp_cost = VERY_FAR;
        g->vertices[i]->distance = VERY_FAR;
        g->vertices[i]->duration = VERY_FAR;
        g->vertices[i]->walking_distance = VERY_FAR;
        g->vertices[i]->walking_time = VERY_FAR;
        g->vertices[i]->parent = VNULL;
        g->vertices[i]->status = UNREACHED;
        g->vertices[i]->next = VNULL;
        PathRecorder tmpRec = (PathRecorder)malloc(sizeof(struct PathRecorder));
        tmpRec->vertex_id = g->vertices[i]->id;
        tmpRec->parent_vertex_id = -1 * VERY_FAR;
        (*prev)[i] = tmpRec;
    }
#ifdef DEBUG
    printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] done.\n");
    printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] start processing switch point list...\n");
#endif
    (*entry) = VNULL;
    if (spList == NULL) {
#ifdef DEBUG
        printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] no switch points input.\n");
        printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] init source vertex\n");
#endif
        src = BinarySearchVertexById(g->vertices, 0, g->vertex_count - 1, source);
        src->temp_cost = 0;
        src->distance = 0;
        src->duration = 0;
        src->walking_distance = 0;
        src->walking_time = 0;
        src->parent = src;
        (*begin) = (*end) = src;
        src->next = VNULL;
        src->status = IN_QUEUE;
#ifdef DEBUG
        printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] done.\n");
#endif
    } else {
        /* relax every switch point pairs (i.e. switch edges) */
#ifdef DEBUG
        printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] number of switch points: %d.\n", spListLength);
#endif
        (*begin) = (*end) = VNULL;
        double costNew;
        for (i = 0; i < spListLength; i++) {			
            Vertex switchFrom = BinarySearchVertexById(last_g->vertices,
                    0, last_g->vertex_count - 1, spList[i]->from_vertex_id);
            Vertex switchTo = BinarySearchVertexById(g->vertices,
                    0, g->vertex_count - 1, spList[i]->to_vertex_id);
            if (switchFrom != VNULL && switchTo != VNULL) {
                if (checkConstraint != NULL) {
                    if (checkConstraint(switchFrom) != 0) {
                        continue;
                    }
                }
                if (strcmp(costFactor, "speed") == 0)
                    costNew = switchFrom->temp_cost + 
                        (spList[i]->length * spList[i]->speed_factor);
                else if (strcmp(costFactor, "length") == 0)
                    costNew = switchFrom->temp_cost + 
                        (spList[i]->length * spList[i]->length_factor);
                else /*default case: use speed factor*/
                    costNew = switchFrom->temp_cost + 
                        (spList[i]->length * spList[i]->speed_factor);
                if (costNew < switchTo->temp_cost) {
                    switchTo->temp_cost = costNew;
                    switchTo->parent = switchTo;
                    switchTo->distance = switchFrom->distance + 
                        (spList[i]->length * spList[i]->length_factor);
                    switchTo->duration = switchFrom->duration + 
                        (spList[i]->length * spList[i]->speed_factor);
                    switchTo->walking_distance = switchFrom->walking_distance + 
                        (spList[i]->length * spList[i]->length_factor);
                    switchTo->walking_time = switchFrom->walking_time + 
                        (spList[i]->length * spList[i]->speed_factor);
                    /* enqueue switch points */
                    // INSERT_TO_BACK(switchTo)
                    if (switchTo->status != IN_QUEUE) {
                        if ((*begin) == VNULL)
                            *begin = switchTo;
                        else
                            (*end)->next = switchTo;
                        (*end) = switchTo;
                        (*end)->next = VNULL;
                        switchTo->status = IN_QUEUE;
                    }
                }
            }
        }
#ifdef DEBUG
        printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] done!\n");
#endif
    }
#ifdef DEBUG
    printf("[DEBUG][mmtwoq.c::multimodalTwoQInit] multimodalTwoQInit finished. \n");
#endif
}

static void twoQSearch(ModeGraph g, Vertex *begin, Vertex *end, Vertex *entry,
        PathRecorder **prev, const char *costFactor, 
        VertexValidationChecker checkConstraint) {
    double costNew;
    Vertex vertexFrom, vertexTo;
    Edge edge_ij;
    int edgeCount = 0, i = 0, vertexCount = 0;
#ifdef DEBUG
    printf("[DEBUG][mmtwoq.c::twoQSearch] twoQSearch started. \n");
#endif
    while ((*begin) != VNULL) {
        // EXTRACT_FIRST(vertexFrom)
        vertexFrom = *begin;
        vertexFrom->status = WAS_IN_QUEUE;
        if ((*begin) == (*entry))
            *entry = VNULL;
        *begin = (*begin)->next;
        if (checkConstraint != NULL) {
            if (checkConstraint(vertexFrom) != 0) {
                continue;
            }
        }
        edgeCount = vertexFrom->outdegree;
        edge_ij = vertexFrom->outgoing;
        while (edge_ij != ENULL) { 
            /* scanning edges outgoing from vertexFrom*/
            vertexTo = BinarySearchVertexById(g->vertices, 0, g->vertex_count - 1, 
                    edge_ij->to_vertex_id);
            if (strcmp(costFactor, "speed") == 0)
                costNew = vertexFrom->temp_cost + 
                    (edge_ij->length * edge_ij->speed_factor);
            else if (strcmp(costFactor, "length") == 0)
                costNew = vertexFrom->temp_cost + 
                    (edge_ij->length * edge_ij->length_factor);
            else /*default case: use speed factor*/
                costNew = vertexFrom->temp_cost + 
                    (edge_ij->length * edge_ij->speed_factor);
            if (costNew < vertexTo->temp_cost) {
                vertexTo->temp_cost = costNew;
                vertexTo->parent = vertexFrom;
                vertexTo->distance = vertexFrom->distance + 
                    (edge_ij->length * edge_ij->length_factor);
                vertexTo->duration = vertexFrom->duration + 
                    (edge_ij->length * edge_ij->speed_factor);
                if (edge_ij->mode_id == FOOT) {
                    vertexTo->walking_distance = vertexFrom->walking_distance + 
                        (edge_ij->length * edge_ij->length_factor);
                    vertexTo->walking_time = vertexFrom->walking_time + 
                        (edge_ij->length * edge_ij->speed_factor);
                } else {
                    vertexTo->walking_distance = vertexFrom->walking_distance;
                    vertexTo->walking_time = vertexFrom->walking_time;
                }
                if (!vertexTo->status == IN_QUEUE) {
                    if (vertexTo->status == WAS_IN_QUEUE) {
                        // INSERT_TO_ENTRY(vertexTo)
                        if ((*entry) != VNULL) {
                            vertexTo->next = (*entry)->next;
                            (*entry)->next = vertexTo;
                            if ((*entry) == (*end))
                                (*end) = vertexTo;
                        } else {
                            if ((*begin) == VNULL)
                                (*end) = vertexTo;
                            vertexTo->next = (*begin);
                            (*begin) = vertexTo;
                        }
                        vertexTo->status = IN_QUEUE;
                        (*entry) = vertexTo;
                    } else {
                        // INSERT_TO_BACK(vertexTo)
                        if ((*begin) == VNULL)
                            (*begin) = vertexTo;
                        else
                            (*end)->next = vertexTo;

                        (*end) = vertexTo;
                        (*end)->next = VNULL;
                        vertexTo->status = IN_QUEUE;
                    }
                }
            }
            edge_ij = edge_ij->adjNext;
        } /* end of scanning vertexFrom */
    } /* end of the main loop */

    vertexCount = g->vertex_count;
    for (i = 0; i < vertexCount; i++) {
        (*prev)[i]->vertex_id = g->vertices[i]->id;
        if (g->vertices[i]->parent == VNULL)
            (*prev)[i]->parent_vertex_id = -1 * VERY_FAR;
        else
            (*prev)[i]->parent_vertex_id = g->vertices[i]->parent->id;
    }
#ifdef DEBUG
    printf("[DEBUG][mmtwoq.c::twoQSearch] twoQSearch finished. \n");
#endif
}

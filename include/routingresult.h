/*
 * =====================================================================================
 *
 *       Filename:  routingresult.h
 *
 *    Description:  ADT of recording the information of planning results
 *
 *        Created:  2015/12/18 14时55分21秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#ifndef  PATHINFO_INC
#define  PATHINFO_INC

#include "modegraph.h"

typedef struct PathRecorder PathRecorder;
typedef struct Path Path;

struct PathRecorder {
	int64_t vertex_id;
	int64_t parent_vertex_id;
};

struct Path {
	int64_t *vertex_list;
	int     vertex_list_length;
};	

Path **GetFinalPath(int64_t source, int64_t target);
double GetFinalCost(int64_t target, const char *costField);
void DisposePaths(Path **paths);
void DisposeResultPathTable(); 

#endif   /* ----- #ifndef PATHINFO_INC  ----- */

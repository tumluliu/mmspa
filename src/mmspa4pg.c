/*
 * =====================================================================================
 *
 *       Filename:  mmspa4pg.c
 *
 *    Description:  Implementations of the public top-level routing functions
 *
 *        Created:  2009/10/05 17时05分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */
  
#include <stdio.h>
#include <assert.h>
#include "../include/mmspa4pg.h"
#include "../include/graphassembler.h"
#include "../include/routingresult.h"
 
Path **MSPfindPath(int64_t source, int64_t target) {
#ifdef DEBUG
    printf("[DEBUG][mmspa4pg.c::MSPfindPath]assemble active graphs\n");
#endif
    assert(!MSPassembleGraphs());
#ifdef DEBUG
    printf("[DEBUG][mmspa4pg.c::MSPfindPath]find multimodal shortest paths\n");
#endif
    MultimodalTwoQ(source);
#ifdef DEBUG
    printf("[DEBUG][mmspa4pg.c::MSPfindPath]get the found path\n");
#endif
    Path **finalPath = MSPgetFinalPath(source, target);
    return finalPath;
}

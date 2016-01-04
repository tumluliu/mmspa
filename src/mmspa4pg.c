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
    assert(!MSPassembleGraphs());
    MSPtwoq(source);
    return MSPgetFinalPath(source, target);
}

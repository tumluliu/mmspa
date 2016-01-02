/*
 * =====================================================================================
 *
 *       Filename:  graphassembler.h
 *
 *    Description:  Declarations of public functions and variables for
 *                  multimodal graph assembler
 *
 *        Created:  2009/03/23 10时36分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */

#ifndef GRAPHASSEMBLER_H_
#define GRAPHASSEMBLER_H_

#include "modegraph.h"

/* Function of initializing the library, preparing and caching mode graph data */
int MSPinit(const char *pgConnStr);
/* Function of assembling multimodal graph set for each routing plan */
int MSPassembleGraphs();
/* Function of disposing the library memory */
void MSPdisposeRoutingPlanResource();
void MSPfinalize();
/* The following functions are obsoleted, but is reserved for the moment for the 
 * sake of compatability */
int Parse();
void Dispose();
int ConnectDB(const char *pgConnStr);
void DisconnectDB();

#endif /* GRAPHASSEMBLER_H_ */

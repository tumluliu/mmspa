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

int Init(const char *pgConnStr);
int LoadGraphFromDb(const char *pgConnStr);
int AssembleGraphs();
void Dispose();
/* The following functions are obsoleted, but is reserved for the moment for the 
 * sake of compatability */
int Parse();
int ConnectDB(const char *pgConnStr);
void DisconnectDB();

#endif /* GRAPHASSEMBLER_H_ */

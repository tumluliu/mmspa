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
/* This function is obsoleted, but is reserved for the moment for the sake of
 * compatability */
int Parse();
int AssembleGraphs();
void Dispose();

#endif /* GRAPHASSEMBLER_H_ */

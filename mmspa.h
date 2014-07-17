/*
 * mmspa.h
 *
 *  Created on: Mar 25, 2009
 *      Author: LIU Lu
 */

#ifndef MMSPA_H_
#define MMSPA_H_

#include <string.h>
#include <stdlib.h>
#include "parser.h"

#define VERY_FAR 1073741823

void MultimodalBellmanFord(const char** modeList, int modeListLength,
		const char* source);

void MultimodalDijkstra(const char** modeList, int modeListLength,
		const char* source);

void MultimodalTwoQ(const char** modeList, int modeListLength,
		const char* source);

void bf(int n, Vertex** nodes, Vertex* source, int m, Edge** arcs,
		int* n_scans);

int dikf(long n, Vertex** nodes, Vertex* source, Edge** arcs, int* n_scans);

void GetFinalPath(const char* source, const char* target);

void Dispose();

#endif /* MMSPA_H_ */

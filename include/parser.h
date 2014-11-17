/*
 * parser.h
 *
 *  Created on: 2009-3-23
 *      Author: liulu
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include "types_graph.h"

static const int PUBLIC_TRANSPORT_MODE_ID = 19;
static const int WALKING_MODE_ID = 12;

int ConnectDB(const char* pgConnString);

void DisconnectDB();

int Parse();

Vertex* SearchVertexById(Vertex** vertexArray, int len, long long id);

Vertex* BinarySearchVertexById(Vertex** vertexArray, int low, int high, long long id);

#endif /* PARSER_H_ */

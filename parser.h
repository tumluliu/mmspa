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

int Parse(const char* path);

Vertex* SearchVertexById(Vertex** vertexArray, int arrayLength, const char* id);

#endif /* PARSER_H_ */

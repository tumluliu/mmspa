/*
 * f_heap.h
 *
 *  Created on: Apr 16, 2009
 *      Author: LIU Lu
 */

#include "types_graph.h"
#include <math.h>
#include <stdlib.h>
#ifndef F_HEAP_H_
#define F_HEAP_H_
#define VERY_FAR 1073741823

#define BASE           1.61803
#define NOT_ENOUGH_MEM 2

#define OUT_OF_HEAP    0
#define IN_HEAP        1
#define MARKED         2

typedef struct fheap_st
{
	Vertex *min; /* the minimal node */
	long dist; /* tentative distance of min. node */
	long n; /* number of nodes in the heap */
	Vertex **deg_pointer; /* pointer to the node with given degree */
	long deg_max; /* maximal degree */
} f_heap;

void Init_fheap(long n);

void Check_min(Vertex *v);

void Insert_after_min(Vertex *v);

void Insert_to_root(Vertex *nd);

void Cut_node(Vertex *nd, Vertex *father);

void Insert_to_fheap(Vertex *nd);

void Fheap_decrease_key(Vertex *nd);

Vertex* Extract_min();

void DisposeFibHeap();

#endif /* F_HEAP_H_ */

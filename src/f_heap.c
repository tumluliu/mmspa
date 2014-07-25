/*
 * f_heap.c
 *
 *  Created on: Apr 16, 2009
 *      Author: LIU Lu
 */

#include "../include/f_heap.h"

f_heap fh;
long dg;
Vertex *after, *before, *father, *child, *first, *last, *vertex_c, *vertex_s,
		*vertex_r, *vertex_n, *vertex_l;

void Init_fheap(long n)
{
	fh.deg_max = (long) (log((double) n) / log(BASE) + 1);

	if ((fh.deg_pointer = (Vertex**) calloc(fh.deg_max, sizeof(Vertex*)))
			== (Vertex**) NULL)
		exit(NOT_ENOUGH_MEM);

	for (dg = 0; dg < fh.deg_max; dg++)
		fh.deg_pointer[dg] = NNULL;

	fh.n = 0;
	fh.min = NNULL;
}

void Check_min(Vertex *v)
{
	if (v->distance < fh.dist)
	{
		fh.dist = v->distance;
		fh.min = v;
	}
}

void Insert_after_min(Vertex *v)
{
	after = fh.min -> next;
	v -> next = after;
	after -> prev = v;
	fh.min -> next = v;
	v -> prev = fh.min;

	Check_min(v);
}

void Insert_to_root(Vertex *v)
{
	v -> heap_parent = NNULL;
	v -> status = IN_HEAP;

	Insert_after_min(v);
}

void Cut_vertex(Vertex *v, Vertex *father)
{
	after = v -> next;
	if (after != v)
	{
		before = v -> prev;
		before->next = after;
		after->prev = before;
	}

	if (father->son == v)
		father->son = after;
	(father->deg)--;
	if (father->deg == 0)
		father->son = NNULL;
}

void Insert_to_fheap(Vertex *v)
{
	v->heap_parent = NNULL;
	v->son = NNULL;
	v->status = IN_HEAP;
	v->deg = 0;

	if (fh.min == NNULL)
	{
		v->prev = v->next = v;
		fh.min = v;
		fh.dist = v->distance;
	}
	else
		Insert_after_min(v);
	fh.n++;
}

void Fheap_decrease_key(Vertex *v)
{
	if ((father = v->heap_parent) == NNULL)
		Check_min(v);
	else /* vertex isn't in the root */
	{
		if (v->distance < father->distance)
		{
			vertex_c = v;
			while (father != NNULL)
			{
				Cut_vertex(vertex_c, father);
				Insert_to_root(vertex_c);
				if (father->status == IN_HEAP)
				{
					father->status = MARKED;
					break;
				}
				vertex_c = father;
				father = father->heap_parent;
			}
		}
	}
}

Vertex* Extract_min()
{
	Vertex *v;

	v = fh.min;
	if (fh.n > 0)
	{
		fh.n--;
		fh.min->status = OUT_OF_HEAP;

		/* connecting root-list and sons-of-min-list */
		first = fh.min->prev;
		child = fh.min->son;
		if (first == fh.min)
			first = child;
		else
		{
			after = fh.min->next;
			if (child == NNULL)
			{
				first->next = after;
				after->prev = first;
			}
			else
			{
				before = child->prev;

				first->next = child;
				child->prev = first;

				before->next = after;
				after->prev = before;
			}
		}

		if (first != NNULL)
		{ /* heap is not empty */

			/* squeezing root */
			vertex_c = first;
			last = first->prev;
			while (1)
			{
				vertex_l = vertex_c;
				vertex_n = vertex_c->next;

				while (1)
				{
					dg = vertex_c->deg;
					vertex_r = fh.deg_pointer[dg];

					if (vertex_r == NNULL)
					{
						fh.deg_pointer[dg] = vertex_c;
						break;
					}
					else
					{
						if (vertex_c->distance < vertex_r->distance)
						{
							vertex_s = vertex_r;
							vertex_r = vertex_c;
						}
						else
							vertex_s = vertex_c;

						/* detach vertex_s from root */
						after = vertex_s->next;
						before = vertex_s->prev;

						after->prev = before;
						before->next = after;

						/* attach vertex_s to vertex_r */
						vertex_r->deg++;
						vertex_s->heap_parent = vertex_r;
						vertex_s->status = IN_HEAP;

						child = vertex_r->son;

						if (child == NNULL)
							vertex_r->son = vertex_s->next = vertex_s->prev
									= vertex_s;
						else
						{
							after = child->next;
							child->next = vertex_s;
							vertex_s->prev = child;
							vertex_s->next = after;
							after->prev = vertex_s;
						}

					} /* vertex_r now is father of vertex_s */
					vertex_c = vertex_r;
					fh.deg_pointer[dg] = NNULL;
				}

				if (vertex_l == last)
					break;
				vertex_c = vertex_n;
			}

			/* finding minimum */
			fh.dist = VERY_FAR;
			for (dg = 0; dg < fh.deg_max; dg++)
			{
				if (fh.deg_pointer[dg] != NNULL)
				{
					vertex_r = fh.deg_pointer[dg];
					fh.deg_pointer[dg] = NNULL;
					Check_min(vertex_r);
					vertex_r->heap_parent = NNULL;
				}
			}
		}
		else
			/* heap is empty */
			fh.min = NNULL;
	}
	return v;
}

// added by Liu Lu
// 2009-05-06
// function: Dispose the memory dynamically allocated for Fibonacci heap
void DisposeFibHeap()
{
	free(fh.deg_pointer);
}

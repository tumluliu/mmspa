#include "../include/mmspa.h"

#define VERY_FAR 1073741823
#define IN_QUEUE     0
#define OUT_OF_QUEUE 1

void bf(int n, Vertex **nodes, Vertex *source, int m, Edge **arcs, int *n_scans)
{
	Vertex *begin, *end;
	long dist_new, dist_from;
	Vertex *node_from, *node_to;
	Edge *arc_ij;
	long num_scans = 0;
	int arc_count = 0, arc_index = 0;

	/*initialization*/
	int i = 0;
	for (i = 0; i < n; i++)
	{
		nodes[i]->parent = NNULL;
		nodes[i]->distance = VERY_FAR;
		nodes[i]->status = OUT_OF_QUEUE;
	}

	source->parent = source;
	source->distance = 0;

	/*init queue*/
	begin = end = source;
	source->next = NNULL;
	source->status = IN_QUEUE;

	/*main loop*/
	while (begin != NNULL)
	{
		node_from = begin;
		node_from->status = OUT_OF_QUEUE;
		begin = begin->next;
		if (((node_from->parent)->status) == OUT_OF_QUEUE)
		{
			num_scans++;
			arc_count = node_from->outdegree;
			dist_from = node_from->distance;
			arc_index = node_from->first;

			for (i = 0; i < arc_count; i++)
			{ /*scanning arcs outgoing from node_from*/
				arc_ij = arcs[arc_index];
				arc_index++;
				node_to = arc_ij->end;
				dist_new = dist_from + (arc_ij->cost);
				if (dist_new < node_to->distance)
				{
					node_to->distance = dist_new;
					node_to->parent = node_from;
					if (node_to->status != IN_QUEUE)
					{
						if (begin == NNULL)
							begin = node_to;
						else
							end->next = node_to;
						end = node_to;
						end->next = NNULL;
						node_to->status = IN_QUEUE;
					}
				}
			}
		}

		/*end	of scanning node_from*/
	}/*end of the main loop*/

	*n_scans = num_scans;
}

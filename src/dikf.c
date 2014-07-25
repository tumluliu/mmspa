/* Dijkstra with Fibonacci heap */

#include "../include/mmspa.h"
#include "../include/f_heap.h"

#define NODE_IN_FHEAP(node) (node->status > OUT_OF_HEAP)

int dikf(long n, Vertex **nodes, Vertex *source, Edge **arcs, int *n_scans)
{
	long dist_new, dist_from;
	Vertex *node_from, *node_to;
	Edge *arc_ij;
	long num_scans = 0;
	int arc_count = 0, arc_index = 0, i = 0;;

	/* initialization */
	Init_fheap(n);

	for (i = 0; i < n; i++)
	{
		nodes[i]->parent = NNULL;
		nodes[i]->distance = VERY_FAR;
		nodes[i]->status = OUT_OF_HEAP;
	}

	source->parent = source;
	source->distance = 0;
	Insert_to_fheap(source);

	/* main loop */
	while (1)
	{
		node_from = Extract_min();
		if (node_from == NNULL)
			break;

		num_scans++;
		arc_count = node_from->outdegree;
		dist_from = node_from->distance;
		arc_index = node_from->first;

		for (i = 0; i < arc_count; i++)
		{
			/*scanning arcs outgoing from node_from*/
			arc_ij = arcs[arc_index];
			arc_index++;
			node_to = arc_ij->end;

			dist_new = dist_from + (arc_ij->cost);
			if (dist_new < node_to->distance)
			{
				node_to->distance = dist_new;
				node_to->parent = node_from;

				if (NODE_IN_FHEAP(node_to))
				{
					Fheap_decrease_key(node_to);
				}
				else
				{
					Insert_to_fheap(node_to);
				}
			}
		}
	}
	*n_scans = num_scans;
	return (0);
}

#!/usr/bin/env python

from pymmspa import PathFinder

source_list = []
mode_list = ['underground', 'foot']


def do_benchmarking(algorithm):
    # Timer clock start
    for s in source_list:
        if algorithm == 'MMTQ':
            PathFinder.multimodal_twoq(s, mode_list)
        elif algorithm == 'MMD':
            PathFinder.multimodal_dijkstra(s, mode_list)
        elif algorithm == 'MMBF':
            PathFinder.multimodal_bellmanford(s, mode_list)
    # Timer clock stop
    print "Calculation time: "

if __name__ == "__main__":
    do_benchmarking(arg1)

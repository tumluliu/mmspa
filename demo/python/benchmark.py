#!/usr/bin/env python

import sys
import time
from pymmspa import *

source_list = ["59529822", "59525660", "59524276", "59523770", "59522730",
               "59522320", "59521579", "59521043", "59520663", "59519758",
               "59519243", "59518864", "59517890", "59516900", "59509174",
               "59505072", "59503373", "568143481", "540165540", "813389942",
               "729211573", "655810285", "59533211", "59526235", "59525530",
               "59523807", "59523129", "59522463", "59521966", "59521294",
               "59520905", "59519810", "59519332", "59519142", "59518007",
               "59517610", "59509422", "59505327", "59504778", "568143762",
               "540175071", "817093120", "749091723", "674407202", "655769154",
               "59523916", "59523680", "59522594", "59522016", "59519381"]
mode_list = ['underground', 'foot']


def do_benchmarking(algorithm):
    print "Loading multimodal transportation networks in Munich..."
    t1 = time.time()
    parse("/Users/user/Research/phd-work/data/UM/")
    t2 = time.time()
    print "done! Time consumed: " + str(t2 - t1) + " seconds"
    print "Start benchmarking multimodal path calculation for algorithm " + algorithm.upper() + " ..."
    print "Test for randomly selected " + str(len(source_list)) + " sources"
    print "Input mode list are: " + str(mode_list)
    t1 = time.time()
    for s in source_list:
        if algorithm.upper() == 'MMTQ':
            mm_twoq(mode_list, s)
        elif algorithm.upper() == 'MMD':
            mm_dijkstra(mode_list, s)
        elif algorithm.upper() == 'MMBF':
            mm_bellmanford(mode_list, s)
    t2 = time.time()
    print "done! Average calculation time: " + str((t2 - t1) / len(source_list)) + " seconds"
    dispose()

if __name__ == "__main__":
    do_benchmarking(sys.argv[1])

#!/usr/bin/env python

import sys
import time
from termcolor import colored
from pymmspa4pg import *

source_list = []
target_list = []

def load_routing_options(
        sources_file_path,
        targets_file_path,
        options_file_path):
    with open(sources_file_path) as sources_file:
        source_list = [source.strip() for source in sources_file.readlines()]
    with open(targets_file_path) as targets_file:
        target_list = [target.strip() for target in targets_file.readlines()]
    options = read_routing_options(options_file_path)

def read_routing_options(options_file_path):
    return options_file_path

def do_benchmarking(sources, targets, options):
    print "Connecting to database... ",
    if connect_db("dbname = 'mmrp_munich' user = 'liulu' password = 'workhard'") != 0:
        print colored("failed!", "red")
        exit()
    print colored("done!", "green")
    print "Preparing routing plan... ",
    create_routing_plan(1, 1)
    set_mode(0, 1900)
    set_public_transit_mode(0, 1003)
    set_cost_factor("speed")
    print colored("done!", "green")
    print "Loading multimodal transportation networks in Munich... ",
    t1 = time.time()
    if parse() != 0:
        print colored("failed!", "red")
        exit()
    t2 = time.time()
    print colored("done!", "green")
    print "Time consumed: " + str(t2 - t1) + " seconds"
    print "Test for randomly selected " + str(len(sources)) + " source vertices"
    print "Routing plan: by underground trains"
    print "Start benchmarking multimodal path calculation by MultimodalTwoQ... ",
    t1 = time.time()
    for s in sources:
        multimodal_twoq(long(s))
    t2 = time.time()
    print colored("done!", "green")
    print "Average calculation time: " + str((t2 - t1) / len(sources)) + " seconds"
    print "Post processing... ",
    dispose()
    print colored("done!", "green")
    print "Disconnect database... ",
    disconnect_db()
    print colored("done!", "green")

if __name__ == "__main__":
    with open(sys.argv[1]) as f_sources:
        source_list = [source.strip() for source in f_sources.readlines()]
    with open(sys.argv[2]) as f_targets:
        target_list = [target.strip() for target in f_targets.readlines()]
    #options = read_routing_options(options_file_path)
    options = ""
    do_benchmarking(source_list, target_list, options)

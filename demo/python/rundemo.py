#!/usr/bin/env python

# author: Lu LIU
# created at: 2014-07-25
# Description: a sample of how to use pymmspa to find multimodal shortest paths

from pymmspa import PathFinder, RoutingOptions, RoutingPlan, RoutingPlanInferer, MultimodalPath

# load multimodal graph set data stored in plain text defined by myself
graphset = PathFinder.load_networks('/Users/user/Projects/mmspa/demo/data/UM')
# more data formats will be supported in the future, e.g.
# PathFinder.load_networks('/Users/user/Research/phd-work/data/UM/GEXF/mmgs.gexf', 'GEXF')
# PathFinder.load_networks('mmrp:mmrp@localhost,db=mmrp_munich', 'PGSQL')
# etc.

options = RoutingOptions.load_options('/Users/user/Projects/mmspa/demo/data/routing_options.txt')
plans = RoutingPlanInferer.generate_routing_plan(options)
paths = PathFinder.find_paths(graphset, plans, 'MMTQ')
for p in paths:
    MultimodalPath.draw_on_map(p)

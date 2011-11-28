#!/usr/bin/python

import libNeuroProofPriority as npp

status = npp.initialize_priority_scheduler("examples/graph.json", .1, .9, .5)

num_examined = 0

if status:
    while npp.get_estimated_num_remaining_edges() > 0:
        priority_info = npp.get_next_edge()
        xyzlocation = priority_info.location
        (body1, body2) = priority_info.body_pair

        # proofread body pair at location
        
        npp.set_edge_result(priority_info.body_pair, 1.0)
        num_examined += 1

    print "Num examined: " , num_examined
    npp.export_priority_scheduler("output.json")
